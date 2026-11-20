#pragma once

#include "QualifyingGridAudit.h"
#include "Track/CircuitStartGrid.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::competition {

// Source-only, non-authoritative starting-grid *proposal*. A review receipt
// and upstream-provided actor tokens cannot attest a real Chaos/UE5.8 spawn.
// The actual GameMode remains responsible for physical start-line safety.
enum class QualifyingGridPlanFault : std::uint8_t {
    None,
    InvalidSession,
    UnverifiedSource,
    InvalidEvidence,
    MissingQualifiedLap,
    WrongActorRoster,
    MissingPhysicalActor,
    UnsafeCircuitGrid
};

struct PhysicalGridRoster final {
    std::uint64_t session = 0;
    std::array<std::uint64_t, MaximumQualifyingCars> actorTokens{};
    std::array<bool, MaximumQualifyingCars> actorObservationVerified{};
    bool originalCircuitVerified = false;
    bool engineRosterVerified = false;
    bool physicsEpochCurrent = false;
};

struct PlannedQualifyingStart final {
    std::uint64_t actor = 0;
    int plannedSlot = -1;
    latebrake::CircuitPose originalPose{};
    double bestQualifyingSeconds = std::numeric_limits<double>::infinity();
    bool qualified = false;
    bool rosterObserved = false;
};

struct QualifyingStartGridPlan final {
    std::uint64_t session = 0;
    std::array<PlannedQualifyingStart, MaximumQualifyingCars> slots{};
    std::size_t count = 0;
    std::size_t qualifiedCount = 0;
    QualifyingGridPlanFault fault = QualifyingGridPlanFault::InvalidEvidence;
    int faultSlot = -1;
    bool valid = false;
};

inline QualifyingStartGridPlan PrepareProvisionalQualifyingStartGrid(
    const GridAuditReceipt& audit,
    const PhysicalGridRoster& roster) noexcept {
    QualifyingStartGridPlan output{};
    output.session = roster.session;
    if (roster.session == 0) {
        output.fault = QualifyingGridPlanFault::InvalidSession;
        return output;
    }
    if (!roster.originalCircuitVerified ||
        !roster.engineRosterVerified ||
        !roster.physicsEpochCurrent) {
        output.fault = QualifyingGridPlanFault::UnverifiedSource;
        return output;
    }
    if (!audit.fullyAgrees || audit.fault != GridAuditFault::None ||
        audit.count != MaximumQualifyingCars ||
        audit.qualifiedCount > audit.count ||
        audit.sectorQualifiedCount != audit.qualifiedCount) {
        output.fault = QualifyingGridPlanFault::InvalidEvidence;
        return output;
    }
    // A source-only plan cannot invent the entire qualifying order if
    // no verified lap was submitted. Unqualified drivers may still appear
    // at the back in stable registration order when there is actual timing.
    if (audit.qualifiedCount == 0) {
        output.fault = QualifyingGridPlanFault::MissingQualifiedLap;
        return output;
    }
    if (!latebrake::IsPlannedStartGridSafe()) {
        output.fault = QualifyingGridPlanFault::UnsafeCircuitGrid;
        return output;
    }

    std::array<bool, MaximumQualifyingCars> consumed{};
    std::size_t measuredQualified = 0;
    for (std::size_t slot = 0; slot < MaximumQualifyingCars; ++slot) {
        output.faultSlot = static_cast<int>(slot);
        const auto& source = audit.byGridSlot[slot];
        if (source.actor == 0 ||
            source.gridPosition != static_cast<int>(slot + 1) ||
            !source.lapAgrees) {
            output.fault = QualifyingGridPlanFault::InvalidEvidence;
            return output;
        }
        std::size_t matched = MaximumQualifyingCars;
        for (std::size_t candidate = 0;
             candidate < MaximumQualifyingCars; ++candidate) {
            if (roster.actorTokens[candidate] == source.actor) {
                if (matched != MaximumQualifyingCars) {
                    output.fault = QualifyingGridPlanFault::WrongActorRoster;
                    return output;
                }
                matched = candidate;
            }
        }
        if (matched == MaximumQualifyingCars || consumed[matched]) {
            output.fault = QualifyingGridPlanFault::WrongActorRoster;
            return output;
        }
        if (!roster.actorObservationVerified[matched]) {
            output.fault = QualifyingGridPlanFault::MissingPhysicalActor;
            return output;
        }
        consumed[matched] = true;

        latebrake::CircuitPose pose{};
        if (!latebrake::TryGetStartGridPose(
                static_cast<int>(slot), pose) ||
            !latebrake::IsValidStartGridPose(pose) ||
            !latebrake::StartGridFootprintOnPavement(pose) ||
            latebrake::StartGridFootprintTouchesBarrier(pose)) {
            output.fault = QualifyingGridPlanFault::UnsafeCircuitGrid;
            return output;
        }
        PlannedQualifyingStart& target = output.slots[slot];
        target.actor = source.actor;
        target.plannedSlot = static_cast<int>(slot);
        target.originalPose = pose;
        target.qualified = source.hasValidLap;
        target.rosterObserved = true;
        if (source.hasValidLap) {
            if (!std::isfinite(source.measuredBestLapSeconds) ||
                source.measuredBestLapSeconds <= 0.0) {
                output.fault = QualifyingGridPlanFault::InvalidEvidence;
                return output;
            }
            target.bestQualifyingSeconds =
                source.measuredBestLapSeconds;
            ++measuredQualified;
        } else if (source.measuredBestLapSeconds !=
                   std::numeric_limits<double>::infinity()) {
            output.fault = QualifyingGridPlanFault::InvalidEvidence;
            return output;
        }
    }
    if (measuredQualified != audit.qualifiedCount) {
        output.fault = QualifyingGridPlanFault::InvalidEvidence;
        return output;
    }
    for (bool observed : consumed) {
        if (!observed) {
            output.fault = QualifyingGridPlanFault::WrongActorRoster;
            return output;
        }
    }
    output.count = MaximumQualifyingCars;
    output.qualifiedCount = measuredQualified;
    output.fault = QualifyingGridPlanFault::None;
    output.faultSlot = -1;
    output.valid = true;
    return output;
}

} // namespace latebrake::competition
