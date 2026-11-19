#pragma once

#include "QualifyingSession.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::competition {

// Source-only gate chronology. Consumes caller-supplied physical observations;
// this code cannot independently establish whether UE/Chaos produced them.
// Gate zero starts a lap, gate N ends it, and all intermediate gates must
// appear exactly once in order before a provisional lap can be produced.
enum class GateTraceAdmission : std::uint8_t {
    Started,
    Advanced,
    LapReconstructed,
    NoSession,
    InvalidRoster,
    WrongSession,
    InvalidSource,
    InvalidTiming,
    StaleOrMisordered,
    ImplausibleMotion
};

struct GateTraceSettings {
    std::uint64_t session = 0;
    std::uint64_t circuit = 0;
    int gatesPerLap = 0;
    double minimumGateIntervalSeconds = 0.01;
    double maximumGateIntervalSeconds = 60.0;
    double maximumObservedSpeedCmS = 15000.0;
    double maximumPositionNoiseCm = 50.0;
};

struct GatePositionCm {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct VerifiedGateObservation {
    std::uint64_t session = 0;
    std::uint64_t circuit = 0;
    std::uint64_t actor = 0;
    std::uint64_t engineFrame = 0;
    int lapOrdinal = 0;
    int gateOrdinal = 0;
    double clockSeconds = 0.0;
    double measuredSpeedCmS = 0.0;
    GatePositionCm position{};
    bool fromOriginalEngine = false;
    bool actualPhysicalCar = false;
    bool physicalGateCrossing = false;
    bool onAuthoredCircuit = false;
    bool uninterruptedRace = false;
    bool legalTrackLimits = false;
    bool raceControlAuthorized = false;
};

struct GateTraceCar {
    std::uint64_t actor = 0;
    GatePositionCm lastPosition{};
    std::uint64_t startFrame = 0;
    std::uint64_t lastFrame = 0;
    double startSeconds = 0.0;
    double lastSeconds = 0.0;
    int currentLapOrdinal = 0;
    int completedLapOrdinal = 0;
    int nextGate = 0;
    bool inLap = false;
};

struct GateTraceResult {
    GateTraceAdmission status = GateTraceAdmission::NoSession;
    QualifyingLap lap{};
    bool lapProduced = false;
};

inline bool ValidGateTraceSettings(const GateTraceSettings& settings) noexcept {
    return settings.session != 0 && settings.circuit != 0 &&
           settings.gatesPerLap >= 2 && settings.gatesPerLap <= 4096 &&
           std::isfinite(settings.minimumGateIntervalSeconds) &&
           std::isfinite(settings.maximumGateIntervalSeconds) &&
           std::isfinite(settings.maximumObservedSpeedCmS) &&
           std::isfinite(settings.maximumPositionNoiseCm) &&
           settings.minimumGateIntervalSeconds > 0.0 &&
           settings.maximumGateIntervalSeconds >
               settings.minimumGateIntervalSeconds &&
           settings.maximumGateIntervalSeconds <= 86400.0 &&
           settings.maximumObservedSpeedCmS > 0.0 &&
           settings.maximumObservedSpeedCmS <= 100000.0 &&
           settings.maximumPositionNoiseCm >= 0.0 &&
           settings.maximumPositionNoiseCm <= 500.0;
}

inline bool ValidGatePosition(const GatePositionCm& point) noexcept {
    return std::isfinite(point.x) && std::isfinite(point.y) &&
           std::isfinite(point.z) &&
           std::abs(point.x) <= 5.0e7 &&
           std::abs(point.y) <= 5.0e7 &&
           std::abs(point.z) <= 5.0e7;
}

inline double GatePositionDistanceCm(
    const GatePositionCm& a, const GatePositionCm& b) noexcept {
    const double x = a.x - b.x;
    const double y = a.y - b.y;
    const double z = a.z - b.z;
    return std::hypot(std::hypot(x, y), z);
}

class QualifyingGateTrace final {
public:
    bool Begin(const GateTraceSettings& settings,
               const std::array<std::uint64_t, MaximumQualifyingCars>& roster,
               std::size_t count) noexcept {
        if (active_ || !ValidGateTraceSettings(settings) ||
            count == 0 || count > MaximumQualifyingCars) {
            return false;
        }
        for (std::size_t i = 0; i < count; ++i) {
            if (roster[i] == 0) return false;
            for (std::size_t j = 0; j < i; ++j) {
                if (roster[j] == roster[i]) return false;
            }
        }
        settings_ = settings;
        count_ = count;
        for (std::size_t i = 0; i < count; ++i) {
            cars_[i] = GateTraceCar{};
            cars_[i].actor = roster[i];
        }
        active_ = true;
        return true;
    }

    GateTraceResult Observe(const VerifiedGateObservation& source) noexcept {
        GateTraceResult result{};
        if (!active_) return result;
        result.status = GateTraceAdmission::WrongSession;
        if (source.session != settings_.session ||
            source.circuit != settings_.circuit) return result;

        std::size_t index = count_;
        for (std::size_t i = 0; i < count_; ++i) {
            if (cars_[i].actor == source.actor) {
                index = i;
                break;
            }
        }
        if (index == count_) {
            result.status = GateTraceAdmission::InvalidRoster;
            return result;
        }
        GateTraceCar& car = cars_[index];
        const auto reject = [&car, &result](
            GateTraceAdmission reason) noexcept -> GateTraceResult {
            // Once a trusted lap sequence is interrupted, the same engine
            // lap ordinal must never be restarted at a second gate-zero
            // observation. Otherwise missing or contradictory checkpoints
            // could be silently discarded before granting provisional pace.
            if (car.inLap) {
                car.completedLapOrdinal = std::max(
                    car.completedLapOrdinal, car.currentLapOrdinal);
            }
            car.inLap = false;
            car.nextGate = 0;
            car.currentLapOrdinal = 0;
            result.status = reason;
            return result;
        };

        if (!source.fromOriginalEngine ||
            !source.actualPhysicalCar ||
            !source.physicalGateCrossing ||
            !source.onAuthoredCircuit ||
            !source.uninterruptedRace ||
            !source.legalTrackLimits ||
            !source.raceControlAuthorized ||
            !ValidGatePosition(source.position) ||
            !std::isfinite(source.clockSeconds) ||
            !std::isfinite(source.measuredSpeedCmS) ||
            source.measuredSpeedCmS < 0.0 ||
            source.measuredSpeedCmS > settings_.maximumObservedSpeedCmS ||
            source.engineFrame == 0 ||
            source.lapOrdinal < 1 ||
            source.gateOrdinal < 0 ||
            source.gateOrdinal > settings_.gatesPerLap ||
            source.clockSeconds < 0.0) {
            return reject(GateTraceAdmission::InvalidSource);
        }

        if (!car.inLap) {
            if (source.gateOrdinal != 0 ||
                source.lapOrdinal <= car.completedLapOrdinal ||
                source.engineFrame <= car.lastFrame ||
                (source.clockSeconds <= car.lastSeconds &&
                 car.lastFrame != 0)) {
                return reject(GateTraceAdmission::StaleOrMisordered);
            }
            car.startFrame = source.engineFrame;
            car.lastFrame = source.engineFrame;
            car.startSeconds = source.clockSeconds;
            car.lastSeconds = source.clockSeconds;
            car.lastPosition = source.position;
            car.currentLapOrdinal = source.lapOrdinal;
            car.nextGate = 1;
            car.inLap = true;
            result.status = GateTraceAdmission::Started;
            return result;
        }

        if (source.gateOrdinal != car.nextGate ||
            source.lapOrdinal != car.currentLapOrdinal ||
            source.engineFrame <= car.lastFrame ||
            source.clockSeconds <= car.lastSeconds) {
            return reject(GateTraceAdmission::StaleOrMisordered);
        }
        const double interval = source.clockSeconds - car.lastSeconds;
        if (!std::isfinite(interval) ||
            interval < settings_.minimumGateIntervalSeconds ||
            interval > settings_.maximumGateIntervalSeconds) {
            return reject(GateTraceAdmission::InvalidTiming);
        }
        const double distance =
            GatePositionDistanceCm(source.position, car.lastPosition);
        const double travelBudget =
            settings_.maximumObservedSpeedCmS * interval +
            settings_.maximumPositionNoiseCm;
        if (!std::isfinite(distance) ||
            !std::isfinite(travelBudget) ||
            distance > travelBudget) {
            return reject(GateTraceAdmission::ImplausibleMotion);
        }
        car.lastFrame = source.engineFrame;
        car.lastSeconds = source.clockSeconds;
        car.lastPosition = source.position;
        if (source.gateOrdinal < settings_.gatesPerLap) {
            ++car.nextGate;
            result.status = GateTraceAdmission::Advanced;
            return result;
        }
        const double elapsed = source.clockSeconds - car.startSeconds;
        if (!std::isfinite(elapsed) || elapsed <= 0.0) {
            return reject(GateTraceAdmission::InvalidTiming);
        }
        result.lap.session = settings_.session;
        result.lap.circuit = settings_.circuit;
        result.lap.actor = car.actor;
        result.lap.lapOrdinal = car.currentLapOrdinal;
        result.lap.beginningFrame = car.startFrame;
        result.lap.endingFrame = car.lastFrame;
        result.lap.verifiedGates = settings_.gatesPerLap;
        result.lap.startSeconds = car.startSeconds;
        result.lap.finishSeconds = source.clockSeconds;
        result.lap.reportedLapSeconds = elapsed;
        result.lap.penaltySeconds = 0.0;
        result.lap.measuredFromEngine = true;
        result.lap.originalCircuitVerified = true;
        result.lap.raceControlConfirmed = true;
        result.lap.crossedFinishPhysically = true;
        result.lap.uninterrupted = true;
        result.lap.legalTrackLimits = true;
        car.completedLapOrdinal = car.currentLapOrdinal;
        car.currentLapOrdinal = 0;
        car.nextGate = 0;
        car.inLap = false;
        result.status = GateTraceAdmission::LapReconstructed;
        result.lapProduced = true;
        return result;
    }

    const GateTraceCar* Car(std::uint64_t actor) const noexcept {
        if (!active_ || actor == 0) return nullptr;
        for (std::size_t i = 0; i < count_; ++i) {
            if (cars_[i].actor == actor) return &cars_[i];
        }
        return nullptr;
    }

    std::size_t Count() const noexcept { return count_; }
    bool Active() const noexcept { return active_; }

private:
    GateTraceSettings settings_{};
    std::array<GateTraceCar, MaximumQualifyingCars> cars_{};
    std::size_t count_ = 0;
    bool active_ = false;
};

} // namespace latebrake::competition
