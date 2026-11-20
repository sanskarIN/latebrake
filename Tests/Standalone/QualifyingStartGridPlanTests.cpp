#include "../../Source/Latebrake/Public/Competition/QualifyingStartGridPlan.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

#define VERIFY(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "Qualifying start grid failed: %s line %d\n", \
                 #condition, __LINE__); \
    std::abort(); \
} } while (false)

using namespace latebrake::competition;

namespace {

ProvisionalQualifyingGrid SourceGrid(std::size_t qualifiers) {
    ProvisionalQualifyingGrid grid{};
    grid.valid = true;
    grid.count = MaximumQualifyingCars;
    grid.qualifiedCount = qualifiers;
    for (std::size_t i = 0; i < MaximumQualifyingCars; ++i) {
        grid.actors[i] = 101 + i;
        grid.qualified[i] = i < qualifiers;
        grid.bestLapOrdinals[i] = i < qualifiers ? 1 : 0;
        grid.bestLapSeconds[i] = i < qualifiers ?
            45.0 + static_cast<double>(i) :
            std::numeric_limits<double>::infinity();
    }
    return grid;
}

ProvisionalSectorOrder SourceSectors(std::size_t qualifiers) {
    ProvisionalSectorOrder sectors{};
    sectors.valid = true;
    sectors.count = MaximumQualifyingCars;
    sectors.timedDrivers = qualifiers;
    for (std::size_t i = 0; i < MaximumQualifyingCars; ++i) {
        auto& entry = sectors.entries[i];
        entry.actor = 101 + i;
        entry.hasEvidence = i < qualifiers;
        entry.reviewedLaps = i < qualifiers ? 1 : 0;
        entry.measuredBestLapSeconds = i < qualifiers ?
            45.0 + static_cast<double>(i) :
            std::numeric_limits<double>::infinity();
        entry.bestSectorCombinationSeconds =
            entry.measuredBestLapSeconds;
    }
    return sectors;
}

PhysicalGridRoster ObservedRoster() {
    PhysicalGridRoster roster{};
    roster.session = 9001;
    // Physical telemetry can arrive in an independent fixed slot order.
    roster.actorTokens = {106, 105, 104, 103, 102, 101};
    roster.actorObservationVerified.fill(true);
    roster.originalCircuitVerified = true;
    roster.engineRosterVerified = true;
    roster.physicsEpochCurrent = true;
    return roster;
}

void ValidatePhysicalMapping() {
    const auto grid = SourceGrid(2);
    const auto sectors = SourceSectors(2);
    const auto audit = AuditQualifyingGrid(grid, sectors);
    VERIFY(audit.fullyAgrees && audit.count == 6);
    VERIFY(audit.qualifiedCount == 2);
    VERIFY(latebrake::IsPlannedStartGridSafe());

    auto roster = ObservedRoster();
    auto plan = PrepareProvisionalQualifyingStartGrid(audit, roster);
    VERIFY(plan.valid);
    VERIFY(plan.fault == QualifyingGridPlanFault::None);
    VERIFY(plan.faultSlot == -1);
    VERIFY(plan.session == 9001);
    VERIFY(plan.count == 6);
    VERIFY(plan.qualifiedCount == 2);
    for (std::size_t slot = 0; slot < 6; ++slot) {
        const auto& expected = latebrake::StartGridPoses[slot];
        const auto& assigned = plan.slots[slot];
        VERIFY(assigned.actor == 101 + slot);
        VERIFY(assigned.plannedSlot == static_cast<int>(slot));
        VERIFY(assigned.rosterObserved);
        VERIFY(assigned.qualified == (slot < 2));
        VERIFY(assigned.originalPose.Position.X == expected.Position.X);
        VERIFY(assigned.originalPose.Position.Y == expected.Position.Y);
        VERIFY(assigned.originalPose.HeadingDegrees ==
               expected.HeadingDegrees);
        VERIFY(latebrake::StartGridFootprintOnPavement(
            assigned.originalPose));
        if (slot < 2) {
            VERIFY(assigned.bestQualifyingSeconds ==
                   45.0 + static_cast<double>(slot));
        } else {
            VERIFY(std::isinf(assigned.bestQualifyingSeconds));
        }
        for (std::size_t other = 0; other < slot; ++other) {
            VERIFY(!latebrake::StartGridFootprintsOverlap(
                assigned.originalPose,
                plan.slots[other].originalPose));
        }
    }

    roster.session = 0;
    plan = PrepareProvisionalQualifyingStartGrid(audit, roster);
    VERIFY(!plan.valid &&
           plan.fault == QualifyingGridPlanFault::InvalidSession);
    roster = ObservedRoster();
    roster.originalCircuitVerified = false;
    VERIFY(PrepareProvisionalQualifyingStartGrid(audit, roster).fault ==
           QualifyingGridPlanFault::UnverifiedSource);
    roster = ObservedRoster();
    roster.physicsEpochCurrent = false;
    VERIFY(PrepareProvisionalQualifyingStartGrid(audit, roster).fault ==
           QualifyingGridPlanFault::UnverifiedSource);
    roster = ObservedRoster();
    roster.actorTokens[0] = roster.actorTokens[1];
    VERIFY(PrepareProvisionalQualifyingStartGrid(audit, roster).fault ==
           QualifyingGridPlanFault::WrongActorRoster);
    roster = ObservedRoster();
    roster.actorTokens[0] = 99999;
    VERIFY(PrepareProvisionalQualifyingStartGrid(audit, roster).fault ==
           QualifyingGridPlanFault::WrongActorRoster);
    roster = ObservedRoster();
    roster.actorObservationVerified[5] = false;
    plan = PrepareProvisionalQualifyingStartGrid(audit, roster);
    VERIFY(!plan.valid);
    VERIFY(plan.fault == QualifyingGridPlanFault::MissingPhysicalActor);
    VERIFY(plan.faultSlot == 0);

    roster = ObservedRoster();
    auto corruptAudit = audit;
    corruptAudit.fullyAgrees = false;
    VERIFY(PrepareProvisionalQualifyingStartGrid(corruptAudit, roster).fault ==
           QualifyingGridPlanFault::InvalidEvidence);
    corruptAudit = audit;
    corruptAudit.count = 5;
    VERIFY(PrepareProvisionalQualifyingStartGrid(corruptAudit, roster).fault ==
           QualifyingGridPlanFault::InvalidEvidence);
    corruptAudit = audit;
    corruptAudit.qualifiedCount = 0;
    corruptAudit.sectorQualifiedCount = 0;
    VERIFY(PrepareProvisionalQualifyingStartGrid(corruptAudit, roster).fault ==
           QualifyingGridPlanFault::MissingQualifiedLap);
    corruptAudit = audit;
    corruptAudit.byGridSlot[3].gridPosition = 6;
    VERIFY(PrepareProvisionalQualifyingStartGrid(corruptAudit, roster).fault ==
           QualifyingGridPlanFault::InvalidEvidence);
    corruptAudit = audit;
    corruptAudit.byGridSlot[0].actor = 0;
    VERIFY(PrepareProvisionalQualifyingStartGrid(corruptAudit, roster).fault ==
           QualifyingGridPlanFault::InvalidEvidence);
    corruptAudit = audit;
    corruptAudit.byGridSlot[0].measuredBestLapSeconds =
        std::numeric_limits<double>::quiet_NaN();
    VERIFY(PrepareProvisionalQualifyingStartGrid(corruptAudit, roster).fault ==
           QualifyingGridPlanFault::InvalidEvidence);
}

void ValidateCompleteQualifyingGrid() {
    auto roster = ObservedRoster();
    auto audit = AuditQualifyingGrid(SourceGrid(6), SourceSectors(6));
    VERIFY(audit.fullyAgrees && audit.qualifiedCount == 6);
    auto plan = PrepareProvisionalQualifyingStartGrid(audit, roster);
    VERIFY(plan.valid && plan.qualifiedCount == 6);
    for (const auto& slot : plan.slots) {
        VERIFY(slot.qualified);
        VERIFY(std::isfinite(slot.bestQualifyingSeconds));
    }

    // Unqualified actors may enter at the rear only when there is at least
    // one valid qualifying lap, never from a fabricated six-car result.
    audit = AuditQualifyingGrid(SourceGrid(0), SourceSectors(0));
    VERIFY(audit.fullyAgrees && audit.qualifiedCount == 0);
    plan = PrepareProvisionalQualifyingStartGrid(audit, roster);
    VERIFY(!plan.valid &&
           plan.fault == QualifyingGridPlanFault::MissingQualifiedLap);
}

} // namespace

int main() {
    ValidatePhysicalMapping();
    ValidateCompleteQualifyingGrid();
    return 0;
}
