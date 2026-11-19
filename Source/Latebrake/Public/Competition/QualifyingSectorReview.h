#pragma once

#include "QualifyingSession.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::competition {

// Bounded, engine-neutral review of authentic gate-to-gate sector evidence.
// It never authorizes laps, changes official timing, awards, saves or physics.
inline constexpr std::size_t MaximumQualifyingSectors = 8;

enum class SectorAdmission : std::uint8_t {
    Accepted,
    NoSession,
    WrongSession,
    UnknownActor,
    StaleLap,
    InvalidLap,
    IncorrectSectorCount,
    InvalidSource,
    MisorderedGates,
    InvalidChronology,
    InconsistentTiming
};

struct SectorSettings final {
    std::uint64_t session = 0;
    std::uint64_t circuit = 0;
    int gatesPerLap = 0;
    int sectorsPerLap = 0;
    double minimumSectorSeconds = 0.01;
    double maximumSectorSeconds = 600.0;
    double maximumClockSkewSeconds = 0.05;
};

struct PhysicalSectorEvidence final {
    std::uint64_t session = 0;
    std::uint64_t circuit = 0;
    std::uint64_t actor = 0;
    std::uint64_t gateEntryFrame = 0;
    std::uint64_t gateExitFrame = 0;
    int lapOrdinal = 0;
    int fromGate = 0;
    int toGate = 0;
    double entryClockSeconds = 0.0;
    double exitClockSeconds = 0.0;
    double reportedSectorSeconds = 0.0;
    bool engineClockVerified = false;
    bool verifiedPhysicalGatePair = false;
    bool originalCircuitVerified = false;
    bool continuousPhysicalMotion = false;
    bool validTrackLimits = false;
    bool raceControlAuthorized = false;
};

struct SectorDriverReview final {
    std::uint64_t actor = 0;
    std::array<double, MaximumQualifyingSectors> bestSectorSeconds{};
    std::array<int, MaximumQualifyingSectors> bestSectorLapOrdinals{};
    std::array<double, MaximumQualifyingSectors> latestSectorSeconds{};
    double bestLapSeconds = std::numeric_limits<double>::infinity();
    double optimalSectorSumSeconds = std::numeric_limits<double>::infinity();
    double lastFinishSeconds = -1.0;
    std::uint64_t lastFinishFrame = 0;
    int lastLapOrdinal = 0;
    int reviewedLaps = 0;
    bool hasLapEvidence = false;
};

struct SectorReviewReceipt final {
    std::uint64_t actor = 0;
    int lapOrdinal = 0;
    double measuredLapSeconds = 0.0;
    double cumulativeSectorSeconds = 0.0;
    double theoreticalBestSeconds = 0.0;
    double theoreticalGainSeconds = 0.0;
    bool improvedActualLap = false;
    bool improvedTheoreticalLap = false;
    bool valid = false;
};

inline bool ValidSectorSettings(const SectorSettings& s) noexcept {
    return s.session != 0 && s.circuit != 0 &&
           s.gatesPerLap >= 2 && s.gatesPerLap <= 4096 &&
           s.sectorsPerLap >= 2 &&
           s.sectorsPerLap <= static_cast<int>(MaximumQualifyingSectors) &&
           s.sectorsPerLap <= s.gatesPerLap &&
           std::isfinite(s.minimumSectorSeconds) &&
           std::isfinite(s.maximumSectorSeconds) &&
           std::isfinite(s.maximumClockSkewSeconds) &&
           s.minimumSectorSeconds > 0.0 &&
           s.maximumSectorSeconds > s.minimumSectorSeconds &&
           s.maximumSectorSeconds <= 86400.0 &&
           s.maximumClockSkewSeconds >= 0.0 &&
           s.maximumClockSkewSeconds <= 0.25;
}

class QualifyingSectorReview final {
public:
    bool Begin(const SectorSettings& settings,
               const std::array<std::uint64_t, MaximumQualifyingCars>& roster,
               std::size_t count) noexcept {
        if (active_ || !ValidSectorSettings(settings) ||
            count == 0 || count > MaximumQualifyingCars) {
            return false;
        }
        for (std::size_t i = 0; i < count; ++i) {
            if (roster[i] == 0) return false;
            for (std::size_t j = 0; j < i; ++j) {
                if (roster[i] == roster[j]) return false;
            }
        }
        settings_ = settings;
        count_ = count;
        for (std::size_t i = 0; i < count_; ++i) {
            SectorDriverReview entry{};
            entry.actor = roster[i];
            entry.bestSectorSeconds.fill(
                std::numeric_limits<double>::infinity());
            drivers_[i] = entry;
        }
        active_ = true;
        return true;
    }

    SectorAdmission ReviewLap(
        const QualifyingLap& lap,
        const std::array<PhysicalSectorEvidence, MaximumQualifyingSectors>&
            evidence,
        std::size_t sectors,
        SectorReviewReceipt* receipt = nullptr) noexcept {
        if (receipt != nullptr) *receipt = SectorReviewReceipt{};
        if (!active_) return SectorAdmission::NoSession;
        if (lap.session != settings_.session ||
            lap.circuit != settings_.circuit) return SectorAdmission::WrongSession;
        std::size_t index = count_;
        for (std::size_t i = 0; i < count_; ++i) {
            if (drivers_[i].actor == lap.actor) {
                index = i;
                break;
            }
        }
        if (index == count_) return SectorAdmission::UnknownActor;
        const SectorDriverReview& previous = drivers_[index];
        if (lap.lapOrdinal <= previous.lastLapOrdinal ||
            lap.beginningFrame < previous.lastFinishFrame ||
            lap.endingFrame <= previous.lastFinishFrame ||
            lap.startSeconds < previous.lastFinishSeconds) {
            return SectorAdmission::StaleLap;
        }
        if (!lap.measuredFromEngine ||
            !lap.originalCircuitVerified ||
            !lap.raceControlConfirmed ||
            !lap.crossedFinishPhysically ||
            !lap.uninterrupted ||
            !lap.legalTrackLimits ||
            lap.actor == 0 ||
            lap.lapOrdinal <= 0 ||
            lap.verifiedGates != settings_.gatesPerLap ||
            lap.beginningFrame == 0 ||
            lap.endingFrame <= lap.beginningFrame ||
            !std::isfinite(lap.startSeconds) ||
            !std::isfinite(lap.finishSeconds) ||
            !std::isfinite(lap.reportedLapSeconds) ||
            !std::isfinite(lap.penaltySeconds) ||
            lap.penaltySeconds != 0.0 ||
            lap.startSeconds < 0.0 ||
            lap.finishSeconds <= lap.startSeconds ||
            lap.reportedLapSeconds <= 0.0) {
            return SectorAdmission::InvalidLap;
        }
        const double lapElapsed = lap.finishSeconds - lap.startSeconds;
        if (!std::isfinite(lapElapsed) ||
            std::abs(lapElapsed - lap.reportedLapSeconds) >
                settings_.maximumClockSkewSeconds) {
            return SectorAdmission::InvalidLap;
        }
        if (sectors != static_cast<std::size_t>(settings_.sectorsPerLap)) {
            return SectorAdmission::IncorrectSectorCount;
        }

        std::array<double, MaximumQualifyingSectors> elapsed{};
        std::uint64_t priorEndFrame = lap.beginningFrame;
        double priorEndSeconds = lap.startSeconds;
        int priorGate = 0;
        double sectorSum = 0.0;
        for (std::size_t i = 0; i < sectors; ++i) {
            const auto& s = evidence[i];
            if (s.session != lap.session ||
                s.circuit != lap.circuit ||
                s.actor != lap.actor ||
                s.lapOrdinal != lap.lapOrdinal ||
                !s.engineClockVerified ||
                !s.verifiedPhysicalGatePair ||
                !s.originalCircuitVerified ||
                !s.continuousPhysicalMotion ||
                !s.validTrackLimits ||
                !s.raceControlAuthorized) {
                return SectorAdmission::InvalidSource;
            }
            if (s.fromGate != priorGate ||
                s.toGate <= s.fromGate ||
                s.toGate > settings_.gatesPerLap) {
                return SectorAdmission::MisorderedGates;
            }
            if (s.gateEntryFrame < priorEndFrame ||
                s.gateExitFrame <= s.gateEntryFrame ||
                s.gateExitFrame > lap.endingFrame ||
                (i == 0 && s.gateEntryFrame != lap.beginningFrame)) {
                return SectorAdmission::InvalidChronology;
            }
            if (!std::isfinite(s.entryClockSeconds) ||
                !std::isfinite(s.exitClockSeconds) ||
                !std::isfinite(s.reportedSectorSeconds) ||
                s.entryClockSeconds < 0.0 ||
                s.exitClockSeconds <= s.entryClockSeconds ||
                s.entryClockSeconds < priorEndSeconds ||
                s.reportedSectorSeconds < settings_.minimumSectorSeconds ||
                s.reportedSectorSeconds > settings_.maximumSectorSeconds) {
                return SectorAdmission::InvalidChronology;
            }
            if (std::abs(s.entryClockSeconds - priorEndSeconds) >
                settings_.maximumClockSkewSeconds ||
                (i == 0 &&
                 std::abs(s.entryClockSeconds - lap.startSeconds) >
                     settings_.maximumClockSkewSeconds)) {
                return SectorAdmission::InconsistentTiming;
            }
            const double measured = s.exitClockSeconds - s.entryClockSeconds;
            if (!std::isfinite(measured) ||
                measured < settings_.minimumSectorSeconds ||
                measured > settings_.maximumSectorSeconds ||
                std::abs(measured - s.reportedSectorSeconds) >
                    settings_.maximumClockSkewSeconds) {
                return SectorAdmission::InconsistentTiming;
            }
            sectorSum += measured;
            if (!std::isfinite(sectorSum)) {
                return SectorAdmission::InconsistentTiming;
            }
            elapsed[i] = measured;
            priorEndFrame = s.gateExitFrame;
            priorEndSeconds = s.exitClockSeconds;
            priorGate = s.toGate;
        }
        if (priorGate != settings_.gatesPerLap ||
            priorEndFrame != lap.endingFrame) {
            return SectorAdmission::MisorderedGates;
        }
        if (std::abs(priorEndSeconds - lap.finishSeconds) >
                settings_.maximumClockSkewSeconds ||
            std::abs(sectorSum - lapElapsed) >
                settings_.maximumClockSkewSeconds *
                    static_cast<double>(sectors + 1)) {
            return SectorAdmission::InconsistentTiming;
        }

        // No state changes before every sector, actor and lap check passes.
        SectorDriverReview candidate = previous;
        const bool improvedLap = lapElapsed < candidate.bestLapSeconds;
        if (improvedLap) candidate.bestLapSeconds = lapElapsed;
        candidate.lastLapOrdinal = lap.lapOrdinal;
        candidate.lastFinishFrame = lap.endingFrame;
        candidate.lastFinishSeconds = lap.finishSeconds;
        candidate.hasLapEvidence = true;
        ++candidate.reviewedLaps;
        double theoretical = 0.0;
        for (std::size_t i = 0; i < sectors; ++i) {
            candidate.latestSectorSeconds[i] = elapsed[i];
            if (elapsed[i] < candidate.bestSectorSeconds[i]) {
                candidate.bestSectorSeconds[i] = elapsed[i];
                candidate.bestSectorLapOrdinals[i] = lap.lapOrdinal;
            }
            theoretical += candidate.bestSectorSeconds[i];
        }
        if (!std::isfinite(theoretical)) {
            return SectorAdmission::InconsistentTiming;
        }
        const bool improvedTheoretical =
            theoretical < previous.optimalSectorSumSeconds;
        candidate.optimalSectorSumSeconds = theoretical;
        drivers_[index] = candidate;
        if (receipt != nullptr) {
            receipt->actor = lap.actor;
            receipt->lapOrdinal = lap.lapOrdinal;
            receipt->measuredLapSeconds = lapElapsed;
            receipt->cumulativeSectorSeconds = sectorSum;
            receipt->theoreticalBestSeconds = theoretical;
            receipt->theoreticalGainSeconds =
                std::max(0.0, candidate.bestLapSeconds - theoretical);
            receipt->improvedActualLap = improvedLap;
            receipt->improvedTheoreticalLap = improvedTheoretical;
            receipt->valid = true;
        }
        return SectorAdmission::Accepted;
    }

    const SectorDriverReview* Driver(std::uint64_t actor) const noexcept {
        if (!active_ || actor == 0) return nullptr;
        for (std::size_t i = 0; i < count_; ++i) {
            if (drivers_[i].actor == actor) return &drivers_[i];
        }
        return nullptr;
    }

    bool Active() const noexcept { return active_; }
    std::size_t Count() const noexcept { return count_; }

private:
    SectorSettings settings_{};
    std::array<SectorDriverReview, MaximumQualifyingCars> drivers_{};
    std::size_t count_ = 0;
    bool active_ = false;
};

} // namespace latebrake::competition
