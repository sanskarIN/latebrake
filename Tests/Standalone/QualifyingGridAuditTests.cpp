#include "../../Source/Latebrake/Public/Competition/QualifyingGridAudit.h"
#include "../../Source/Latebrake/Public/Competition/QualifyingEvidencePipeline.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

#define CHECK(expr) do { if (!(expr)) { \
    std::fprintf(stderr,"Grid audit failed: %s line %d\n",#expr,__LINE__); \
    std::abort(); \
} } while (false)

namespace {
using namespace latebrake::competition;

std::array<std::uint64_t, MaximumQualifyingCars> Roster() {
    return {100, 101, 102, 103, 104, 105};
}

QualifyingPipelineSettings Settings() {
    QualifyingPipelineSettings p{};
    p.gates.session = 700;
    p.gates.circuit = 900;
    p.gates.gatesPerLap = 3;
    p.gates.minimumGateIntervalSeconds = 0.01;
    p.gates.maximumGateIntervalSeconds = 100.0;
    p.gates.maximumObservedSpeedCmS = 15000.0;
    p.gates.maximumPositionNoiseCm = 30.0;
    p.laps.session = p.gates.session;
    p.laps.circuit = p.gates.circuit;
    p.laps.gatesPerLap = 3;
    p.laps.maximumRecordedLaps = 4;
    p.laps.minimumLapSeconds = 5.0;
    p.laps.maximumLapSeconds = 150.0;
    p.sectors.session = p.gates.session;
    p.sectors.circuit = p.gates.circuit;
    p.sectors.gatesPerLap = 3;
    p.sectors.sectorsPerLap = 2;
    p.sectors.minimumSectorSeconds = 0.5;
    p.sectors.maximumSectorSeconds = 100.0;
    p.sectorGateBoundaries[0] = 0;
    p.sectorGateBoundaries[1] = 2;
    p.sectorGateBoundaries[2] = 3;
    return p;
}

VerifiedGateObservation Gate(std::uint64_t actor,int lap,int gate,
                             double secondsPerGate) {
    VerifiedGateObservation sample{};
    sample.session = 700;
    sample.circuit = 900;
    sample.actor = actor;
    sample.lapOrdinal = lap;
    sample.gateOrdinal = gate;
    sample.engineFrame = static_cast<std::uint64_t>(
        lap * 12000 + gate * 600 + 1);
    sample.clockSeconds = (lap - 1) * 60.0 +
        gate * secondsPerGate;
    sample.position = {
        static_cast<double>(gate) * 8000.0,0.0,100.0
    };
    sample.measuredSpeedCmS = 1000.0;
    sample.fromOriginalEngine = true;
    sample.actualPhysicalCar = true;
    sample.physicalGateCrossing = true;
    sample.onAuthoredCircuit = true;
    sample.uninterruptedRace = true;
    sample.legalTrackLimits = true;
    sample.raceControlAuthorized = true;
    return sample;
}

void Admit(QualifyingEvidencePipeline& pipeline,std::uint64_t actor,
           int lap,double rate) {
    for (int gate = 0;gate <= 3;++gate) {
        const auto result = pipeline.Observe(
            Gate(actor,lap,gate,rate));
        if (gate == 3) {
            CHECK(result.accepted);
            CHECK(result.status ==
                  QualifyingPipelineStatus::QualifyingLapAccepted);
        } else {
            CHECK(result.status == QualifyingPipelineStatus::GateAccepted);
        }
    }
}

void ValidateAudit() {
    QualifyingEvidencePipeline pipeline;
    CHECK(pipeline.Begin(Settings(),Roster(),6));
    auto audit = AuditQualifyingGrid(
        pipeline.Grid(),pipeline.SectorLeaderboard());
    CHECK(audit.fullyAgrees);
    CHECK(audit.count == 6);
    CHECK(audit.qualifiedCount == 0);
    CHECK(audit.sectorQualifiedCount == 0);
    CHECK(audit.sectorOrderingDifferences == 0);
    CHECK(pipeline.AuditGrid().fullyAgrees);

    Admit(pipeline,101,1,10.0);
    Admit(pipeline,100,1,11.0);
    Admit(pipeline,102,1,9.0);
    audit = AuditQualifyingGrid(
        pipeline.Grid(),pipeline.SectorLeaderboard());
    CHECK(audit.fullyAgrees);
    CHECK(audit.fault == GridAuditFault::None);
    CHECK(audit.qualifiedCount == 3);
    CHECK(audit.byGridSlot[0].actor == 102);
    CHECK(audit.byGridSlot[0].gridPosition == 1);
    CHECK(audit.byGridSlot[0].measuredBestLapSeconds == 27.0);
    CHECK(audit.maximumObservedBestLapDifferenceSeconds == 0.0);
    CHECK(audit.maximumObservedTheoreticalGainSeconds == 0.0);
    CHECK(pipeline.AuditGrid().fullyAgrees);
    CHECK(audit.byGridSlot[3].hasValidLap == false);

    // Two valid laps with different sector minima may offer an advisory
    // theoretical time lower than the real best complete lap.
    Admit(pipeline,100,2,10.5);
    audit = AuditQualifyingGrid(
        pipeline.Grid(),pipeline.SectorLeaderboard());
    CHECK(audit.fullyAgrees && audit.qualifiedCount == 3);
    CHECK(audit.byGridSlot[2].actor == 100);
    CHECK(audit.byGridSlot[2].measuredBestLapSeconds == 31.5);

    auto grid = pipeline.Grid();
    auto sectors = pipeline.SectorLeaderboard();

    grid.actors[1] = grid.actors[0];
    audit = AuditQualifyingGrid(grid,sectors);
    CHECK(!audit.fullyAgrees);
    CHECK(audit.fault == GridAuditFault::DuplicateActor);

    grid = pipeline.Grid();
    sectors.entries[1].actor = sectors.entries[0].actor;
    audit = AuditQualifyingGrid(grid,sectors);
    CHECK(audit.fault == GridAuditFault::ForeignActor ||
          audit.fault == GridAuditFault::DuplicateActor);

    grid = pipeline.Grid();
    sectors = pipeline.SectorLeaderboard();
    grid.qualified[0] = false;
    audit = AuditQualifyingGrid(grid,sectors);
    CHECK(audit.fault == GridAuditFault::InconsistentQualification);

    grid = pipeline.Grid();
    sectors = pipeline.SectorLeaderboard();
    sectors.entries[0].measuredBestLapSeconds += 0.25;
    audit = AuditQualifyingGrid(grid,sectors);
    CHECK(audit.fault == GridAuditFault::ConflictingBestLap);

    grid = pipeline.Grid();
    sectors = pipeline.SectorLeaderboard();
    sectors.entries[0].bestSectorCombinationSeconds += 5.0;
    audit = AuditQualifyingGrid(grid,sectors);
    CHECK(audit.fault == GridAuditFault::InvalidTheoreticalPace);

    grid = pipeline.Grid();
    sectors = pipeline.SectorLeaderboard();
    grid.bestLapSeconds[0] = std::numeric_limits<double>::quiet_NaN();
    audit = AuditQualifyingGrid(grid,sectors);
    CHECK(audit.fault == GridAuditFault::InvalidLapTime);

    grid = pipeline.Grid();
    sectors = pipeline.SectorLeaderboard();
    grid.qualifiedCount = 8;
    audit = AuditQualifyingGrid(grid,sectors);
    CHECK(audit.fault == GridAuditFault::InvalidCount);

    grid = pipeline.Grid();
    sectors = pipeline.SectorLeaderboard();
    grid.bestLapSeconds[5] = -std::numeric_limits<double>::infinity();
    audit = AuditQualifyingGrid(grid,sectors);
    CHECK(audit.fault == GridAuditFault::InconsistentQualification);

    grid = pipeline.Grid();
    sectors = pipeline.SectorLeaderboard();
    grid.valid = false;
    audit = AuditQualifyingGrid(grid,sectors);
    CHECK(audit.fault == GridAuditFault::MissingEvidence);

    grid = pipeline.Grid();
    sectors = pipeline.SectorLeaderboard();
    GridAuditSettings invalidSettings{};
    invalidSettings.maximumBestLapDifferenceSeconds =
        std::numeric_limits<double>::infinity();
    audit = AuditQualifyingGrid(grid,sectors,invalidSettings);
    CHECK(audit.fault == GridAuditFault::MissingEvidence);
}

} // namespace

int main() {
    ValidateAudit();
    return 0;
}
