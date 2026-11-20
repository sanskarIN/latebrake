#include "../../Source/Latebrake/Public/Competition/ProvisionalRaceControl.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "Race control: %s at line %d\n", \
                 #condition, __LINE__); \
    std::abort(); \
} } while (false)

namespace {
using namespace latebrake::competition;

QualifyingStartGridPlan VerifiedProposedGrid() {
    ProvisionalQualifyingGrid grid{};
    ProvisionalSectorOrder sectors{};
    PhysicalGridRoster roster{};
    grid.valid = true;
    grid.count = MaximumQualifyingCars;
    grid.qualifiedCount = 2;
    sectors.valid = true;
    sectors.count = MaximumQualifyingCars;
    sectors.timedDrivers = 2;
    roster.session = 200;
    roster.actorTokens = {106, 105, 104, 103, 102, 101};
    roster.actorObservationVerified.fill(true);
    roster.originalCircuitVerified = true;
    roster.engineRosterVerified = true;
    roster.physicsEpochCurrent = true;
    for (std::size_t i = 0; i < MaximumQualifyingCars; ++i) {
        grid.actors[i] = 101 + i;
        grid.qualified[i] = i < 2;
        grid.bestLapOrdinals[i] = i < 2 ? 1 : 0;
        grid.bestLapSeconds[i] = i < 2 ?
            50.0 + static_cast<double>(i) :
            std::numeric_limits<double>::infinity();
        auto& s = sectors.entries[i];
        s.actor = grid.actors[i];
        s.hasEvidence = i < 2;
        s.reviewedLaps = i < 2 ? 1 : 0;
        s.measuredBestLapSeconds = grid.bestLapSeconds[i];
        s.bestSectorCombinationSeconds = grid.bestLapSeconds[i];
    }
    const auto audit = AuditQualifyingGrid(grid, sectors);
    CHECK(audit.fullyAgrees);
    const auto plan = PrepareProvisionalQualifyingStartGrid(audit, roster);
    CHECK(plan.valid && plan.count == 6 && plan.qualifiedCount == 2);
    return plan;
}

RaceControlSettings Settings() {
    RaceControlSettings s{};
    s.raceSession = 900;
    s.qualifyingSession = 200;
    s.circuit = 77;
    s.authorizedMarshal = 11;
    s.sessionStartSeconds = 100.0;
    s.maximumClockSeconds = 200.0;
    s.minimumFormationSeconds = 5.0;
    s.minimumYellowSeconds = 3.0;
    s.minimumRedSeconds = 8.0;
    s.formationAdvisorySpeedMps = 8.0;
    s.yellowAdvisorySpeedMps = 20.0;
    s.maximumSignals = 10;
    return s;
}

PhysicalRaceControlSignal Signal(ProvisionalRaceCommand command,
                                 std::uint64_t frame,
                                 double time) {
    PhysicalRaceControlSignal s{};
    s.raceSession = 900;
    s.circuit = 77;
    s.marshal = 11;
    s.engineFrame = frame;
    s.engineClockSeconds = time;
    s.command = command;
    s.originalCircuitVerified = true;
    s.observedFromEngine = true;
    s.samePhysicalRosterVerified = true;
    s.engineClockVerified = true;
    s.raceControlAuthorized = true;
    s.gameModeAcknowledged = true;
    return s;
}

void TestPhaseTimeline() {
    ProvisionalRaceControl director;
    const auto proposal = VerifiedProposedGrid();
    auto settings = Settings();
    CHECK(!director.Advice().valid);
    CHECK(director.Observe(Signal(ProvisionalRaceCommand::StartGreen,
                                  1, 105.0)).admission ==
          RaceControlAdmission::NoSession);
    settings.authorizedMarshal = 0;
    CHECK(!director.Begin(settings, proposal));
    settings = Settings();
    CHECK(director.Begin(settings, proposal));
    CHECK(!director.Begin(settings, proposal));
    CHECK(director.Active());
    CHECK(director.Phase() == ProvisionalRacePhase::Formation);
    auto advice = director.Advice();
    CHECK(advice.valid && advice.mustHoldStartingGrid);
    CHECK(advice.noPassingRecommended);
    CHECK(advice.recommendedSpeedCapMps == 8.0);
    CHECK(advice.formationPeriods == 1);
    CHECK(director.Record(0) == nullptr);

    auto receipt = director.Observe(Signal(
        ProvisionalRaceCommand::StartGreen, 1, 104.0));
    CHECK(receipt.admission ==
          RaceControlAdmission::MinimumHoldNotElapsed);
    CHECK(!receipt.accepted && director.AcceptedSignals() == 0);
    receipt = director.Observe(Signal(
        ProvisionalRaceCommand::StartGreen, 2, 105.0));
    CHECK(receipt.admission == RaceControlAdmission::Accepted);
    CHECK(receipt.accepted && receipt.eventOrdinal == 1);
    CHECK(receipt.previousPhaseSeconds == 5.0);
    CHECK(director.Phase() == ProvisionalRacePhase::Green);
    CHECK(director.Record(0)->from == ProvisionalRacePhase::Formation);
    CHECK(director.Record(0)->to == ProvisionalRacePhase::Green);
    advice = director.Advice();
    CHECK(!advice.speedCapRecommended);
    CHECK(!advice.noPassingRecommended);
    CHECK(!advice.mustHoldStartingGrid);
    CHECK(std::isinf(advice.recommendedSpeedCapMps));
    CHECK(advice.completedFormationSeconds == 5.0);

    auto corrupt = Signal(ProvisionalRaceCommand::DeployYellow, 3, 106.0);
    corrupt.raceSession = 200;
    receipt = director.Observe(corrupt);
    CHECK(receipt.admission == RaceControlAdmission::WrongSession);
    corrupt = Signal(ProvisionalRaceCommand::DeployYellow, 3, 106.0);
    corrupt.marshal = 12;
    CHECK(director.Observe(corrupt).admission ==
          RaceControlAdmission::UnverifiedSource);
    corrupt = Signal(ProvisionalRaceCommand::DeployYellow, 3, 106.0);
    corrupt.gameModeAcknowledged = false;
    CHECK(director.Observe(corrupt).admission ==
          RaceControlAdmission::UnverifiedSource);
    corrupt = Signal(ProvisionalRaceCommand::DeployYellow, 2, 106.0);
    CHECK(director.Observe(corrupt).admission ==
          RaceControlAdmission::StaleSignal);
    corrupt = Signal(ProvisionalRaceCommand::DeployYellow, 3, 104.0);
    CHECK(director.Observe(corrupt).admission ==
          RaceControlAdmission::StaleSignal);
    corrupt = Signal(ProvisionalRaceCommand::DeployYellow, 3,
                     std::numeric_limits<double>::quiet_NaN());
    CHECK(director.Observe(corrupt).admission ==
          RaceControlAdmission::InvalidTiming);
    corrupt = Signal(static_cast<ProvisionalRaceCommand>(255), 3, 106.0);
    CHECK(director.Observe(corrupt).admission ==
          RaceControlAdmission::InvalidTransition);
    CHECK(director.AcceptedSignals() == 1);

    receipt = director.Observe(Signal(
        ProvisionalRaceCommand::DeployYellow, 3, 108.0));
    CHECK(receipt.accepted);
    CHECK(director.Phase() == ProvisionalRacePhase::FullCourseYellow);
    advice = director.Advice();
    CHECK(advice.yellowPeriods == 1);
    CHECK(advice.noPassingRecommended);
    CHECK(advice.speedCapRecommended);
    CHECK(!advice.mustHoldStartingGrid);
    CHECK(advice.recommendedSpeedCapMps == 20.0);

    receipt = director.Observe(Signal(
        ProvisionalRaceCommand::ResumeGreen, 4, 110.0));
    CHECK(receipt.admission ==
          RaceControlAdmission::MinimumHoldNotElapsed);
    receipt = director.Observe(Signal(
        ProvisionalRaceCommand::ResumeGreen, 5, 111.5));
    CHECK(receipt.accepted);
    CHECK(director.Advice().completedYellowSeconds == 3.5);
    CHECK(director.Phase() == ProvisionalRacePhase::Green);

    receipt = director.Observe(Signal(
        ProvisionalRaceCommand::StopRed, 6, 112.0));
    CHECK(receipt.accepted);
    CHECK(director.Phase() == ProvisionalRacePhase::RedFlag);
    advice = director.Advice();
    CHECK(advice.redPeriods == 1);
    CHECK(advice.mustHoldStartingGrid);
    CHECK(advice.noPassingRecommended);
    CHECK(advice.recommendedSpeedCapMps == 0.0);
    CHECK(advice.completedGreenSeconds == 3.5);

    receipt = director.Observe(Signal(
        ProvisionalRaceCommand::Reform, 7, 115.0));
    CHECK(receipt.admission ==
          RaceControlAdmission::MinimumHoldNotElapsed);
    receipt = director.Observe(Signal(
        ProvisionalRaceCommand::Reform, 8, 120.5));
    CHECK(receipt.accepted);
    CHECK(director.Phase() == ProvisionalRacePhase::Formation);
    CHECK(director.Advice().formationPeriods == 2);
    CHECK(director.Advice().completedRedSeconds == 8.5);

    receipt = director.Observe(Signal(
        ProvisionalRaceCommand::StartGreen, 9, 124.0));
    CHECK(receipt.admission ==
          RaceControlAdmission::MinimumHoldNotElapsed);
    receipt = director.Observe(Signal(
        ProvisionalRaceCommand::StartGreen, 10, 126.0));
    CHECK(receipt.accepted);
    CHECK(director.Advice().completedFormationSeconds == 10.5);
    CHECK(director.Phase() == ProvisionalRacePhase::Green);

    receipt = director.Observe(Signal(
        ProvisionalRaceCommand::Finish, 11, 150.0));
    CHECK(receipt.accepted);
    CHECK(receipt.eventOrdinal == 7);
    CHECK(director.Phase() == ProvisionalRacePhase::Finished);
    advice = director.Advice();
    CHECK(advice.completedGreenSeconds == 27.5);
    CHECK(advice.completedYellowSeconds == 3.5);
    CHECK(advice.completedRedSeconds == 8.5);
    CHECK(advice.noPassingRecommended);
    CHECK(advice.mustHoldStartingGrid);
    CHECK(advice.speedCapRecommended);
    CHECK(advice.recommendedSpeedCapMps == 0.0);
    CHECK(director.Record(6)->command == ProvisionalRaceCommand::Finish);
    CHECK(director.Record(7) == nullptr);
    CHECK(director.Observe(Signal(
        ProvisionalRaceCommand::StartGreen, 12, 160.0)).admission ==
          RaceControlAdmission::InvalidTransition);
    CHECK(director.AcceptedSignals() == 7);
}

void TestEmergencyStopAndBoundedLog() {
    const auto proposal = VerifiedProposedGrid();
    auto settings = Settings();
    ProvisionalRaceControl director;
    CHECK(director.Begin(settings, proposal));
    auto receipt = director.Observe(Signal(
        ProvisionalRaceCommand::StopRed, 1, 100.1));
    CHECK(receipt.accepted);
    CHECK(director.Phase() == ProvisionalRacePhase::RedFlag);
    CHECK(director.Advice().completedFormationSeconds > 0.0);
    CHECK(director.Observe(Signal(
        ProvisionalRaceCommand::StartGreen, 2, 101.0)).admission ==
          RaceControlAdmission::InvalidTransition);
    receipt = director.Observe(Signal(
        ProvisionalRaceCommand::Abandon, 3, 101.5));
    CHECK(receipt.accepted);
    CHECK(director.Phase() == ProvisionalRacePhase::Finished);

    settings.maximumSignals = 2;
    ProvisionalRaceControl bounded;
    CHECK(bounded.Begin(settings, proposal));
    CHECK(bounded.Observe(Signal(
        ProvisionalRaceCommand::StartGreen, 10, 106.0)).accepted);
    CHECK(bounded.Observe(Signal(
        ProvisionalRaceCommand::DeployYellow, 11, 107.0)).accepted);
    receipt = bounded.Observe(Signal(
        ProvisionalRaceCommand::ResumeGreen, 12, 111.0));
    CHECK(receipt.admission == RaceControlAdmission::CapacityReached);
    CHECK(!receipt.accepted);
    CHECK(bounded.AcceptedSignals() == 2);
    CHECK(bounded.Phase() == ProvisionalRacePhase::FullCourseYellow);
}

void TestUntrustedPlanRejection() {
    auto settings = Settings();
    auto plan = VerifiedProposedGrid();
    plan.session = 0;
    ProvisionalRaceControl rejected;
    CHECK(!rejected.Begin(settings, plan));
    plan = VerifiedProposedGrid();
    plan.slots[1].actor = plan.slots[0].actor;
    CHECK(!rejected.Begin(settings, plan));
    plan = VerifiedProposedGrid();
    plan.slots[2].plannedSlot = 0;
    CHECK(!rejected.Begin(settings, plan));
    plan = VerifiedProposedGrid();
    plan.slots[0].originalPose.Position.X += 1.0;
    CHECK(!rejected.Begin(settings, plan));
    plan = VerifiedProposedGrid();
    plan.qualifiedCount = 5;
    CHECK(!rejected.Begin(settings, plan));
    plan = VerifiedProposedGrid();
    plan.valid = false;
    CHECK(!rejected.Begin(settings, plan));
    plan = VerifiedProposedGrid();
    settings.minimumYellowSeconds =
        std::numeric_limits<double>::infinity();
    CHECK(!rejected.Begin(settings, plan));
    settings = Settings();
    settings.raceSession = settings.qualifyingSession;
    CHECK(!rejected.Begin(settings, plan));
    CHECK(!rejected.Active());
    CHECK(rejected.Record(0) == nullptr);
}

} // namespace

int main() {
    TestPhaseTimeline();
    TestEmergencyStopAndBoundedLog();
    TestUntrustedPlanRejection();
    return 0;
}
