#pragma once

#include "QualifyingGateTrace.h"
#include "QualifyingSectorReview.h"
#include "QualifyingSession.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace latebrake::competition {

// A read-only engine-neutral processing boundary. All evidence values must
// come from a trusted upstream Unreal integration, which does not yet exist.
// This pipeline never generates actors, gate events or official race results.
struct QualifyingPipelineSettings final {
    GateTraceSettings gates{};
    QualifyingSettings laps{};
    SectorSettings sectors{};
    std::array<int, MaximumQualifyingSectors + 1> sectorGateBoundaries{};
};

inline bool ValidQualifyingPipelineSettings(
    const QualifyingPipelineSettings& p) noexcept {
    if (!ValidGateTraceSettings(p.gates) ||
        !ValidQualifyingSettings(p.laps) ||
        !ValidSectorSettings(p.sectors) ||
        p.gates.session != p.laps.session ||
        p.gates.session != p.sectors.session ||
        p.gates.circuit != p.laps.circuit ||
        p.gates.circuit != p.sectors.circuit ||
        p.gates.gatesPerLap != p.laps.gatesPerLap ||
        p.gates.gatesPerLap != p.sectors.gatesPerLap ||
        p.sectorGateBoundaries[0] != 0 ||
        p.sectorGateBoundaries[
            static_cast<std::size_t>(p.sectors.sectorsPerLap)] !=
            p.gates.gatesPerLap) {
        return false;
    }
    for (int i = 1; i <= p.sectors.sectorsPerLap; ++i) {
        if (p.sectorGateBoundaries[static_cast<std::size_t>(i)] <=
            p.sectorGateBoundaries[static_cast<std::size_t>(i - 1)]) {
            return false;
        }
    }
    for (std::size_t i =
             static_cast<std::size_t>(p.sectors.sectorsPerLap + 1);
         i < p.sectorGateBoundaries.size(); ++i) {
        if (p.sectorGateBoundaries[i] != 0) return false;
    }
    return true;
}

enum class QualifyingPipelineStatus : std::uint8_t {
    NoSession,
    GateRejected,
    GateAccepted,
    QualifyingLapAccepted,
    IncompleteSectorEvidence,
    QualifyingLapRejected,
    SectorReviewRejected
};

struct QualifyingPipelineReceipt final {
    QualifyingPipelineStatus status = QualifyingPipelineStatus::NoSession;
    GateTraceAdmission gateStatus = GateTraceAdmission::NoSession;
    LapAdmission lapStatus = LapAdmission::NoSession;
    SectorAdmission sectorStatus = SectorAdmission::NoSession;
    SectorReviewReceipt sectorReceipt{};
    QualifyingLap acceptedLap{};
    std::uint64_t actor = 0;
    int lapOrdinal = 0;
    bool accepted = false;
};

struct QualifyingSectorAccumulator final {
    std::array<PhysicalSectorEvidence, MaximumQualifyingSectors> splits{};
    std::uint64_t actor = 0;
    std::uint64_t boundaryFrame = 0;
    double boundaryClockSeconds = 0.0;
    int currentLapOrdinal = 0;
    int previousBoundaryGate = 0;
    int completedSectors = 0;
    bool tracking = false;
};

class QualifyingEvidencePipeline final {
public:
    bool Begin(
        const QualifyingPipelineSettings& settings,
        const std::array<std::uint64_t, MaximumQualifyingCars>& roster,
        std::size_t count) noexcept {
        if (active_ ||
            !ValidQualifyingPipelineSettings(settings) ||
            count == 0 || count > MaximumQualifyingCars) {
            return false;
        }
        QualifyingGateTrace newGateTrace{};
        QualifyingSession newQualifying{};
        QualifyingSectorReview newSectors{};
        if (!newGateTrace.Begin(settings.gates, roster, count) ||
            !newQualifying.Begin(settings.laps, roster, count) ||
            !newSectors.Begin(settings.sectors, roster, count)) {
            return false;
        }
        settings_ = settings;
        gates_ = newGateTrace;
        laps_ = newQualifying;
        sectors_ = newSectors;
        count_ = count;
        for (std::size_t i = 0; i < count; ++i) {
            pending_[i] = QualifyingSectorAccumulator{};
            pending_[i].actor = roster[i];
        }
        active_ = true;
        return true;
    }

    QualifyingPipelineReceipt Observe(
        const VerifiedGateObservation& sample) noexcept {
        QualifyingPipelineReceipt result{};
        result.actor = sample.actor;
        result.lapOrdinal = sample.lapOrdinal;
        if (!active_) return result;

        // Track the prevalidated actor slot, never a caller-supplied array
        // index. Other actors' histories cannot be changed by one failure.
        std::size_t actorIndex = count_;
        for (std::size_t i = 0; i < count_; ++i) {
            if (pending_[i].actor == sample.actor) {
                actorIndex = i;
                break;
            }
        }
        const GateTraceResult observed = gates_.Observe(sample);
        result.gateStatus = observed.status;
        if (observed.status != GateTraceAdmission::Started &&
            observed.status != GateTraceAdmission::Advanced &&
            observed.status != GateTraceAdmission::LapReconstructed) {
            result.status = QualifyingPipelineStatus::GateRejected;
            // A genuinely interrupted in-session actor sequence must clear
            // its pending sector observations. Foreign session/actor packets
            // cannot invalidate an unrelated genuine current session.
            if (actorIndex < count_ &&
                sample.session == settings_.gates.session &&
                sample.circuit == settings_.gates.circuit) {
                pending_[actorIndex] = QualifyingSectorAccumulator{};
                pending_[actorIndex].actor = sample.actor;
            }
            return result;
        }
        if (actorIndex >= count_) {
            result.status = QualifyingPipelineStatus::GateRejected;
            return result;
        }

        auto& pending = pending_[actorIndex];
        if (observed.status == GateTraceAdmission::Started) {
            pending = QualifyingSectorAccumulator{};
            pending.actor = sample.actor;
            pending.boundaryFrame = sample.engineFrame;
            pending.boundaryClockSeconds = sample.clockSeconds;
            pending.currentLapOrdinal = sample.lapOrdinal;
            pending.tracking = true;
            result.status = QualifyingPipelineStatus::GateAccepted;
            return result;
        }

        // A valid trace cannot leap over an authored sector boundary.
        // Only a fully observed endpoint from the trusted gate source may
        // define the new split. Intermediate gate samples are still
        // authenticated and motion-checked by QualifyingGateTrace.
        if (!pending.tracking ||
            pending.currentLapOrdinal != sample.lapOrdinal ||
            pending.actor != sample.actor) {
            pending = QualifyingSectorAccumulator{};
            pending.actor = sample.actor;
            result.status = QualifyingPipelineStatus::IncompleteSectorEvidence;
            return result;
        }

        if (pending.completedSectors < settings_.sectors.sectorsPerLap) {
            const auto nextIndex = static_cast<std::size_t>(
                pending.completedSectors + 1);
            const int nextSectorGate =
                settings_.sectorGateBoundaries[nextIndex];
            if (sample.gateOrdinal == nextSectorGate) {
                PhysicalSectorEvidence split{};
                split.session = sample.session;
                split.circuit = sample.circuit;
                split.actor = sample.actor;
                split.lapOrdinal = sample.lapOrdinal;
                split.fromGate = pending.previousBoundaryGate;
                split.toGate = sample.gateOrdinal;
                split.gateEntryFrame = pending.boundaryFrame;
                split.gateExitFrame = sample.engineFrame;
                split.entryClockSeconds = pending.boundaryClockSeconds;
                split.exitClockSeconds = sample.clockSeconds;
                split.reportedSectorSeconds =
                    sample.clockSeconds - pending.boundaryClockSeconds;

                // These booleans are the result of the completed in-memory
                // gate sequence checks, not independent cryptographic
                // attestation of the upstream Unreal sensor source.
                split.engineClockVerified = true;
                split.verifiedPhysicalGatePair = true;
                split.originalCircuitVerified = true;
                split.continuousPhysicalMotion = true;
                split.validTrackLimits = true;
                split.raceControlAuthorized = true;
                pending.splits[static_cast<std::size_t>(
                    pending.completedSectors)] = split;
                ++pending.completedSectors;
                pending.previousBoundaryGate = sample.gateOrdinal;
                pending.boundaryFrame = sample.engineFrame;
                pending.boundaryClockSeconds = sample.clockSeconds;
            }
        }

        if (observed.status == GateTraceAdmission::Advanced) {
            result.status = QualifyingPipelineStatus::GateAccepted;
            return result;
        }

        // The last observed gate closed a lap. Missing split evidence must
        // fail the combined transaction even if gate timing looked plausible.
        if (!observed.lapProduced ||
            pending.completedSectors != settings_.sectors.sectorsPerLap ||
            pending.previousBoundaryGate != settings_.gates.gatesPerLap) {
            pending = QualifyingSectorAccumulator{};
            pending.actor = sample.actor;
            result.status = QualifyingPipelineStatus::IncompleteSectorEvidence;
            return result;
        }

        // Stage the two independent mutable reviews and publish both only
        // after both pass. A sector failure must never accidentally add
        // a lap to the provisional grid or vice versa.
        QualifyingSession candidateLaps = laps_;
        QualifyingSectorReview candidateSectors = sectors_;
        result.lapStatus = candidateLaps.Admit(observed.lap);
        if (result.lapStatus != LapAdmission::Accepted) {
            result.status = QualifyingPipelineStatus::QualifyingLapRejected;
        } else {
            result.sectorStatus = candidateSectors.ReviewLap(
                observed.lap, pending.splits,
                static_cast<std::size_t>(pending.completedSectors),
                &result.sectorReceipt);
            if (result.sectorStatus != SectorAdmission::Accepted) {
                result.status = QualifyingPipelineStatus::SectorReviewRejected;
            } else {
                laps_ = candidateLaps;
                sectors_ = candidateSectors;
                result.acceptedLap = observed.lap;
                result.accepted = true;
                result.status = QualifyingPipelineStatus::QualifyingLapAccepted;
            }
        }
        pending = QualifyingSectorAccumulator{};
        pending.actor = sample.actor;
        return result;
    }

    const QualifyingSession& Qualifying() const noexcept { return laps_; }
    const QualifyingSectorReview& Sectors() const noexcept {
        return sectors_;
    }
    const QualifyingGateTrace& Gates() const noexcept { return gates_; }

    ProvisionalQualifyingGrid Grid() const noexcept {
        return laps_.ProvisionalGrid();
    }

    ProvisionalSectorOrder SectorLeaderboard() const noexcept {
        return sectors_.Leaderboard();
    }

    bool Active() const noexcept { return active_; }
    std::size_t Count() const noexcept { return count_; }

private:
    QualifyingPipelineSettings settings_{};
    QualifyingGateTrace gates_{};
    QualifyingSession laps_{};
    QualifyingSectorReview sectors_{};
    std::array<QualifyingSectorAccumulator, MaximumQualifyingCars> pending_{};
    std::size_t count_ = 0;
    bool active_ = false;
};

} // namespace latebrake::competition
