#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::competition {

// Portable, read-only qualifying research. This never writes official results,
// launches actors, starts a race, or asserts the provenance of an Unreal sensor.
inline constexpr std::size_t MaximumQualifyingCars = 6;

enum class LapAdmission : std::uint8_t {
    Accepted,
    NoSession,
    UnknownCar,
    WrongSession,
    StaleObservation,
    InvalidSource,
    InvalidTiming,
    CapacityReached
};

struct QualifyingSettings {
    std::uint64_t session = 0;
    std::uint64_t circuit = 0;
    int gatesPerLap = 0;
    int maximumRecordedLaps = 64;
    double minimumLapSeconds = 5.0;
    double maximumLapSeconds = 2400.0;
    double maximumClockSkewSeconds = 0.05;
};

inline bool ValidQualifyingSettings(const QualifyingSettings& s) noexcept {
    return s.session != 0 && s.circuit != 0 &&
           s.gatesPerLap >= 2 && s.gatesPerLap <= 4096 &&
           s.maximumRecordedLaps >= 1 && s.maximumRecordedLaps <= 4096 &&
           std::isfinite(s.minimumLapSeconds) &&
           std::isfinite(s.maximumLapSeconds) &&
           std::isfinite(s.maximumClockSkewSeconds) &&
           s.minimumLapSeconds > 0.0 &&
           s.maximumLapSeconds >= s.minimumLapSeconds &&
           s.maximumLapSeconds <= 86400.0 &&
           s.maximumClockSkewSeconds >= 0.0 &&
           s.maximumClockSkewSeconds <= 0.25;
}

struct QualifyingLap {
    std::uint64_t session = 0;
    std::uint64_t circuit = 0;
    std::uint64_t actor = 0;
    std::uint64_t beginningFrame = 0;
    std::uint64_t endingFrame = 0;
    int lapOrdinal = 0;
    int verifiedGates = 0;
    double startSeconds = 0.0;
    double finishSeconds = 0.0;
    double reportedLapSeconds = 0.0;
    double penaltySeconds = 0.0;
    bool measuredFromEngine = false;
    bool originalCircuitVerified = false;
    bool raceControlConfirmed = false;
    bool crossedFinishPhysically = false;
    bool uninterrupted = false;
    bool legalTrackLimits = false;
};

struct QualifyingDriver {
    std::uint64_t actor = 0;
    double bestLapSeconds = std::numeric_limits<double>::infinity();
    double secondLapSeconds = std::numeric_limits<double>::infinity();
    double lastFinishSeconds = -1.0;
    std::uint64_t lastEndingFrame = 0;
    int lastLapOrdinal = 0;
    int admittedLaps = 0;
    int bestLapOrdinal = 0;
    bool hasQualified = false;
};

struct ProvisionalQualifyingGrid {
    std::array<std::uint64_t, MaximumQualifyingCars> actors{};
    std::array<double, MaximumQualifyingCars> bestLapSeconds{};
    std::array<int, MaximumQualifyingCars> bestLapOrdinals{};
    std::array<bool, MaximumQualifyingCars> qualified{};
    std::size_t count = 0;
    std::size_t qualifiedCount = 0;
    bool valid = false;
};

class QualifyingSession final {
public:
    bool Begin(const QualifyingSettings& settings,
               const std::array<std::uint64_t, MaximumQualifyingCars>& actors,
               std::size_t count) noexcept {
        if (active_ || !ValidQualifyingSettings(settings) ||
            count == 0 || count > MaximumQualifyingCars) {
            return false;
        }
        for (std::size_t i = 0; i < count; ++i) {
            if (actors[i] == 0) return false;
            for (std::size_t j = 0; j < i; ++j) {
                if (actors[i] == actors[j]) return false;
            }
        }
        settings_ = settings;
        count_ = count;
        for (std::size_t i = 0; i < count; ++i) {
            drivers_[i] = QualifyingDriver{};
            drivers_[i].actor = actors[i];
        }
        active_ = true;
        return true;
    }

    LapAdmission Admit(const QualifyingLap& lap) noexcept {
        if (!active_) return LapAdmission::NoSession;
        if (lap.session != settings_.session ||
            lap.circuit != settings_.circuit) {
            return LapAdmission::WrongSession;
        }
        std::size_t index = count_;
        for (std::size_t i = 0; i < count_; ++i) {
            if (drivers_[i].actor == lap.actor) {
                index = i;
                break;
            }
        }
        if (index == count_) return LapAdmission::UnknownCar;
        const QualifyingDriver& previous = drivers_[index];
        if (lap.lapOrdinal <= previous.lastLapOrdinal ||
            lap.beginningFrame < previous.lastEndingFrame ||
            lap.endingFrame <= previous.lastEndingFrame) {
            return LapAdmission::StaleObservation;
        }
        if (previous.admittedLaps >= settings_.maximumRecordedLaps) {
            return LapAdmission::CapacityReached;
        }
        if (!lap.measuredFromEngine || !lap.originalCircuitVerified ||
            !lap.raceControlConfirmed || !lap.crossedFinishPhysically ||
            !lap.uninterrupted || !lap.legalTrackLimits ||
            lap.verifiedGates != settings_.gatesPerLap ||
            lap.lapOrdinal < 1 || lap.lapOrdinal > settings_.maximumRecordedLaps ||
            lap.beginningFrame == 0 || lap.endingFrame <= lap.beginningFrame ||
            !std::isfinite(lap.penaltySeconds) || lap.penaltySeconds != 0.0) {
            return LapAdmission::InvalidSource;
        }
        if (!std::isfinite(lap.startSeconds) ||
            !std::isfinite(lap.finishSeconds) ||
            !std::isfinite(lap.reportedLapSeconds) ||
            lap.startSeconds < 0.0 ||
            lap.finishSeconds <= lap.startSeconds ||
            lap.startSeconds < previous.lastFinishSeconds ||
            lap.reportedLapSeconds < settings_.minimumLapSeconds ||
            lap.reportedLapSeconds > settings_.maximumLapSeconds) {
            return LapAdmission::InvalidTiming;
        }
        const double elapsed = lap.finishSeconds - lap.startSeconds;
        if (!std::isfinite(elapsed) ||
            elapsed < settings_.minimumLapSeconds ||
            elapsed > settings_.maximumLapSeconds ||
            std::abs(elapsed - lap.reportedLapSeconds) >
                settings_.maximumClockSkewSeconds) {
            return LapAdmission::InvalidTiming;
        }
        QualifyingDriver accepted = previous;
        accepted.lastLapOrdinal = lap.lapOrdinal;
        accepted.lastEndingFrame = lap.endingFrame;
        accepted.lastFinishSeconds = lap.finishSeconds;
        ++accepted.admittedLaps;
        if (elapsed < accepted.bestLapSeconds) {
            accepted.secondLapSeconds = accepted.bestLapSeconds;
            accepted.bestLapSeconds = elapsed;
            accepted.bestLapOrdinal = lap.lapOrdinal;
        } else if (elapsed < accepted.secondLapSeconds) {
            accepted.secondLapSeconds = elapsed;
        }
        accepted.hasQualified = true;
        drivers_[index] = accepted;
        return LapAdmission::Accepted;
    }

    ProvisionalQualifyingGrid ProvisionalGrid() const noexcept {
        ProvisionalQualifyingGrid result{};
        if (!active_) return result;
        std::array<std::size_t, MaximumQualifyingCars> order{};
        for (std::size_t i = 0; i < count_; ++i) {
            order[i] = i;
        }
        // Stable insertion sort: exact comparisons preserve deterministic
        // tie-breaking by second fastest lap, then original grid registration.
        for (std::size_t i = 1; i < count_; ++i) {
            const auto candidate = order[i];
            std::size_t j = i;
            while (j > 0 && Precedes(drivers_[candidate],
                                      drivers_[order[j - 1]])) {
                order[j] = order[j - 1];
                --j;
            }
            order[j] = candidate;
        }
        for (std::size_t i = 0; i < count_; ++i) {
            const auto& driver = drivers_[order[i]];
            result.actors[i] = driver.actor;
            result.bestLapSeconds[i] = driver.bestLapSeconds;
            result.bestLapOrdinals[i] = driver.bestLapOrdinal;
            result.qualified[i] = driver.hasQualified;
            if (driver.hasQualified) ++result.qualifiedCount;
        }
        result.count = count_;
        result.valid = true;
        return result;
    }

    const QualifyingDriver* Driver(std::uint64_t actor) const noexcept {
        if (!active_ || actor == 0) return nullptr;
        for (std::size_t i = 0; i < count_; ++i) {
            if (drivers_[i].actor == actor) return &drivers_[i];
        }
        return nullptr;
    }

    std::size_t Count() const noexcept { return count_; }
    bool Active() const noexcept { return active_; }
    std::uint64_t Session() const noexcept { return settings_.session; }

private:
    static bool Precedes(const QualifyingDriver& a,
                         const QualifyingDriver& b) noexcept {
        if (a.hasQualified != b.hasQualified) return a.hasQualified;
        if (!a.hasQualified) return false;
        if (a.bestLapSeconds != b.bestLapSeconds) {
            return a.bestLapSeconds < b.bestLapSeconds;
        }
        return a.secondLapSeconds < b.secondLapSeconds;
    }

    QualifyingSettings settings_{};
    std::array<QualifyingDriver, MaximumQualifyingCars> drivers_{};
    std::size_t count_ = 0;
    bool active_ = false;
};

} // namespace latebrake::competition
