#include "Racing/RecoveryCharge.h"
#include "Racing/PlayerRecoveryClearance.h"
#include "Racing/RecoveryTeleportVerification.h"
#include "Racing/CheckpointSessionTrace.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace
{
void Require(bool ok, const char* label)
{
    if (!ok)
    {
        std::cerr << "Recovery teleport transaction FAIL: " << label << '\n';
        std::exit(EXIT_FAILURE);
    }
}

latebrake::RecoveryTeleportObservation ValidPlacement()
{
    // A four-gate racer's actual 180cm recovery pose.
    latebrake::RecoveryTeleportObservation placement{
        true, 2800., -1300., 180., 90.,
        2800., -1300., 180., 90.
    };
    placement.ChassisReadyAfterMove = true;
    return placement;
}
}

int main()
{
    using namespace latebrake;

    Require(IsRecoveryTeleportVerified(ValidPlacement()),
            "accepted exact physical position and heading");
    Require(std::string(RecoveryTeleportDecisionName(
                RecoveryTeleportDecision::PositionMismatch)) ==
                "position-mismatch",
            "stable failure reason");
    auto rejected = ValidPlacement();
    rejected.EngineAccepted = false;
    Require(EvaluateRecoveryTeleport(rejected) ==
                RecoveryTeleportDecision::EngineRejected,
            "Unreal reject is never a paid recovery");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (double broken : {nan, inf, -inf})
    {
        for (int part = 0; part != 12; ++part)
        {
            auto trial = ValidPlacement();
            double* fields[] = {
                &trial.TargetX, &trial.TargetY, &trial.TargetZ,
                &trial.TargetYawDegrees, &trial.ActualX, &trial.ActualY,
                &trial.ActualZ, &trial.ActualYawDegrees,
                &trial.TargetPitchDegrees, &trial.TargetRollDegrees,
                &trial.ActualPitchDegrees, &trial.ActualRollDegrees
            };
            *fields[part] = broken;
            Require(EvaluateRecoveryTeleport(trial) ==
                        RecoveryTeleportDecision::NonfinitePose,
                    "nonfinite source or observed 3D pose is rejected");
        }
    }
    auto displacement = ValidPlacement();
    displacement.ActualX += 5.01;
    Require(EvaluateRecoveryTeleport(displacement) ==
                RecoveryTeleportDecision::PositionMismatch,
            "partial physical move outside five centimetres cannot charge");
    displacement = ValidPlacement();
    displacement.ActualX += 3.0;
    displacement.ActualY += 4.0;
    Require(IsRecoveryTeleportVerified(displacement),
            "combined 3-4-5 tolerance boundary is included");
    displacement.ActualZ += 0.1;
    Require(!IsRecoveryTeleportVerified(displacement),
            "3D distance, not individual axis bounds, limits misplacement");
    auto facing = ValidPlacement();
    facing.ActualYawDegrees += 2.01;
    Require(EvaluateRecoveryTeleport(facing) ==
                RecoveryTeleportDecision::HeadingMismatch,
            "wrong recovery facing cannot charge");
    facing = ValidPlacement();
    facing.TargetYawDegrees = 179.0;
    facing.ActualYawDegrees = -181.0;
    Require(IsRecoveryTeleportVerified(facing),
            "wrapped Unreal yaw representation is equivalent");

    // A valid yaw cannot disguise a displaced pitch or roll.
    auto orientation = ValidPlacement();
    orientation.ActualPitchDegrees = 2.01;
    Require(EvaluateRecoveryTeleport(orientation) ==
                RecoveryTeleportDecision::OrientationMismatch,
            "pitch mismatch rejects an otherwise accepted recovery");
    orientation = ValidPlacement();
    orientation.ActualRollDegrees = -2.01;
    Require(EvaluateRecoveryTeleport(orientation) ==
                RecoveryTeleportDecision::OrientationMismatch,
            "roll mismatch rejects an otherwise accepted recovery");
    orientation = ValidPlacement();
    orientation.ActualPitchDegrees = 2.0;
    orientation.ActualRollDegrees = -2.0;
    Require(IsRecoveryTeleportVerified(orientation),
            "pitch and roll accept inclusive two-degree bounds");
    orientation = ValidPlacement();
    orientation.TargetRollDegrees = 179.0;
    orientation.ActualRollDegrees = -181.0;
    Require(IsRecoveryTeleportVerified(orientation),
            "wrapped roll is compared modulo full rotations");

    auto unreadable = ValidPlacement();
    unreadable.ChassisReadyAfterMove = false;
    Require(EvaluateRecoveryTeleport(unreadable) ==
                RecoveryTeleportDecision::PostMovePhysicsUnavailable,
            "broken Chaos or collision after teleport denies penalty");
    // Defaults must fail closed for callers that forget post-move readiness.
    Require(EvaluateRecoveryTeleport({
                true, 2800., -1300., 180., 90.,
                2800., -1300., 180., 90.
            }) == RecoveryTeleportDecision::PostMovePhysicsUnavailable,
            "legacy pose-only observations cannot authorize a paid move");
    for (double broken : {nan, inf, -inf})
    {
        for (int part = 0; part != 6; ++part)
        {
            auto trial = ValidPlacement();
            double* fields[] = {
                &trial.PostLinearVelocityX, &trial.PostLinearVelocityY,
                &trial.PostLinearVelocityZ,
                &trial.PostAngularVelocityXDegrees,
                &trial.PostAngularVelocityYDegrees,
                &trial.PostAngularVelocityZDegrees
            };
            *fields[part] = broken;
            Require(EvaluateRecoveryTeleport(trial) ==
                        RecoveryTeleportDecision::NonfinitePostMoveMotion,
                    "nonfinite post-teleport velocity cannot earn a recovery");
        }
    }
    auto drifting = ValidPlacement();
    drifting.PostLinearVelocityX = 1.01;
    Require(EvaluateRecoveryTeleport(drifting) ==
                RecoveryTeleportDecision::ResidualMotion,
            "unexpected linear movement after teleport blocks fee");
    drifting = ValidPlacement();
    drifting.PostAngularVelocityZDegrees = -1.01;
    Require(EvaluateRecoveryTeleport(drifting) ==
                RecoveryTeleportDecision::ResidualMotion,
            "unexpected spin after teleport blocks fee");
    drifting = ValidPlacement();
    drifting.PostLinearVelocityX = 0.8;
    drifting.PostLinearVelocityY = 0.8;
    Require(EvaluateRecoveryTeleport(drifting) ==
                RecoveryTeleportDecision::ResidualMotion,
            "vector magnitude not per-axis velocity check");
    drifting = ValidPlacement();
    drifting.PostLinearVelocityX = 0.6;
    drifting.PostLinearVelocityY = 0.8;
    drifting.PostAngularVelocityZDegrees = -1.0;
    Require(IsRecoveryTeleportVerified(drifting),
            "inclusive 1 cm/s and 1 deg/s combined motion bounds");
    Require(std::string(RecoveryTeleportDecisionName(
                RecoveryTeleportDecision::ResidualMotion)) ==
                "residual-motion",
            "post-teleport failure diagnostic has stable text");

    RaceProgress race(4, 1);
    race.Advance(4.0);
    int count = 0;
    bool called = false;
    auto fakeEngineReject = [&] {
        called = true;
        return IsRecoveryTeleportVerified(rejected);
    };
    const double originalSeconds = race.GetElapsedSeconds();
    Require(!TryChargePlayerRecoveryAfterVerifiedTeleport(
                race, count, true, false, true, fakeEngineReject),
            "an engine-rejected teleport is not charged");
    Require(called && race.GetElapsedSeconds() == originalSeconds &&
            race.GetTimePenaltyCount() == 0 &&
            race.GetTotalPenaltySeconds() == 0. &&
            count == 0,
            "failed teleport rolls back clock, penalty telemetry and counter");

    called = false;
    Require(!TryChargePlayerRecoveryAfterVerifiedTeleport(
                race, count, false, false, true,
                [&] { called = true; return true; }) &&
            !called, "unready circuit never even attempts physical movement");
    Require(!TryChargePlayerRecoveryAfterVerifiedTeleport(
                race, count, true, true, true,
                [&] { called = true; return true; }) &&
            !called, "manual/OS/world pause never attempts a paid move");
    Require(!TryChargePlayerRecoveryAfterVerifiedTeleport(
                race, count, true, false, false,
                [&] { called = true; return true; }) &&
            !called, "invalid player driver never attempts a paid move");
    Require(race.GetElapsedSeconds() == originalSeconds && count == 0,
            "denied readiness cannot charge");

    // Only verified movement authorizes the previously staged fee.
    CheckpointSessionTrace checkpoint;
    const auto before = checkpoint.SampleDetailed(
        0, 0, 0, {{2700., -1800.}, 180.}, 320., 0.);
    Require(!before.Crossed(), "valid pre-recovery baseline");
    int moves = 0;
    const bool paid = TryChargePlayerRecoveryAfterVerifiedTeleport(
        race, count, true, false, true, [&] {
            ++moves;
            // Unreal wrapper increments teleport revision BEFORE SetActor;
            // this test checks resulting trace cannot bridge that attempt.
            return IsRecoveryTeleportVerified(ValidPlacement());
        });
    Require(paid && moves == 1 &&
            race.GetElapsedSeconds() == originalSeconds + RecoveryPenaltySeconds &&
            race.GetTimePenaltyCount() == 1 &&
            race.GetTotalPenaltySeconds() == RecoveryPenaltySeconds &&
            count == 1, "exactly one full fee on verified real placement");

    auto crossing = checkpoint.SampleDetailed(
        0, 1, 0, {{2900., -1800.}, 180.}, 320., 0.);
    Require(!crossing.Crossed() && !checkpoint.IsNewSession(0),
            "successful teleport generation rebases before crossing score");
    Require(!checkpoint.SampleDetailed(
                0, 1, 0, {{2700., -1800.}, 180.}, 320., 0.).Crossed(),
            "new forward-plane approach is fresh");
    Require(checkpoint.SampleDetailed(
                0, 1, 0, {{2900., -1800.}, 180.}, 320., 0.).Crossed(),
            "genuine subsequent forward pass still counts");

    // A physically attempted but unverified placement must not charge.
    const auto beforeRetry = race.GetElapsedSeconds();
    const int beforeCount = count;
    Require(!TryChargePlayerRecoveryAfterVerifiedTeleport(
                race, count, true, false, true, [] {
                    auto bad = ValidPlacement();
                    bad.ActualY += 600.;
                    return IsRecoveryTeleportVerified(bad);
                }),
            "reported accepted move at wrong location is not chargeable");
    Require(race.GetElapsedSeconds() == beforeRetry &&
            count == beforeCount && race.GetTimePenaltyCount() == 1,
            "no partial billing for physically unverifiable retries");
    for (int invalid = 0; invalid < 3; ++invalid)
    {
        Require(!TryChargePlayerRecoveryAfterVerifiedTeleport(
                    race, count, true, false, true, [invalid] {
                        auto bad = ValidPlacement();
                        if (invalid == 0) bad.ActualRollDegrees = 12.0;
                        if (invalid == 1) bad.ChassisReadyAfterMove = false;
                        if (invalid == 2) bad.PostAngularVelocityYDegrees = 2.0;
                        return IsRecoveryTeleportVerified(bad);
                    }),
                "unverified orientation, physics or spin never charges");
        Require(race.GetElapsedSeconds() == beforeRetry &&
                race.GetTimePenaltyCount() == 1 &&
                race.GetTotalPenaltySeconds() == RecoveryPenaltySeconds &&
                count == beforeCount,
                "new failures preserve live race and all penalty telemetry");
    }
    Require(TryChargePlayerRecoveryAfterVerifiedTeleport(
                race, count, true, false, true, [] { return true; }) &&
            count == beforeCount + 1 &&
            race.GetElapsedSeconds() == beforeRetry + RecoveryPenaltySeconds,
            "healthy retry after failure commits exactly one new fee");

    RaceProgress finished(1, 1);
    finished.Advance(1.);
    Require(finished.PassCheckpoint(0) && finished.IsFinished(),
            "finished fixture reaches end of race");
    bool finishTeleportAttempt = false;
    int finishRecoveries = 0;
    Require(!TryChargePlayerRecoveryAfterVerifiedTeleport(
                finished, finishRecoveries, true, false, true,
                [&] { finishTeleportAttempt = true; return true; }) &&
            !finishTeleportAttempt, "no teleport after race finish");

    RaceProgress newRace(4, 1);
    int exhausted = std::numeric_limits<int>::max();
    bool exhaustedCalled = false;
    Require(!TryChargePlayerRecoveryAfterVerifiedTeleport(
                newRace, exhausted, true, false, true,
                [&] { exhaustedCalled = true; return true; }) &&
            !exhaustedCalled &&
            newRace.GetElapsedSeconds() == 0. &&
            exhausted == std::numeric_limits<int>::max(),
            "counter overflow blocks moving before staging a fee");

    RaceProgress enormous(4, 1);
    enormous.Advance(std::ldexp(1.0, 54));
    int extremeRecoveries = 0;
    bool enormousCalled = false;
    const double beforeEnormous = enormous.GetElapsedSeconds();
    Require(!TryChargePlayerRecoveryAfterVerifiedTeleport(
                enormous, extremeRecoveries, true, false, true,
                [&] { enormousCalled = true; return true; }) &&
            !enormousCalled && enormous.GetElapsedSeconds() == beforeEnormous &&
            extremeRecoveries == 0,
            "unrepresentable full five-second fee never moves or charges");

    // Actual recovery clearance must be composed with the staged fee,
    // not merely shown as optional HUD advice.
    RecoveryObstacle other{true,PrimaryRivalGridSlot,42,
        {{10000.0,10000.0},0.0},0.0,0.0};
    for(int gate=0;gate<CircuitGateCount;++gate)
    {
        const auto original=PlayerRecovery(gate);
        const auto solo=EvaluatePlayerRecoveryClearance(gate,false,false,{});
        const auto clear=EvaluatePlayerRecoveryClearance(gate,true,true,other);
        Require(solo.Safe() && clear.Safe() &&
            solo.Pose.Position.X==original.Position.X &&
            solo.Pose.Position.Y==original.Position.Y &&
            clear.Pose.HeadingDegrees==original.HeadingDegrees,
            "solo and clear rival race preserve exact historical landing");
        Require(!EvaluatePlayerRecoveryClearance(gate,true,false,{}).Safe() &&
            !EvaluatePlayerRecoveryClearance(gate,false,true,other).Safe(),
            "scene mismatch is unknown traffic, not empty road");
        auto blocker=other;blocker.Pose=original;
        const auto occupied=EvaluatePlayerRecoveryClearance(gate,true,true,blocker);
        Require(occupied.Status==PlayerRecoveryClearanceStatus::Occupied,
            "actual rival footprint denies landing");

        RaceProgress attempt(4,2);
        attempt.Advance(12.0);
        int charges=0,calls=0;
        const auto callback=[&](CircuitPose pose) {
            ++calls;
            return pose.Position.X==original.Position.X &&
                pose.Position.Y==original.Position.Y;
        };
        Require(TryPaidRecoveryWithClearance(attempt,charges,true,false,true,
            occupied,callback)==PlayerRecoveryNotice::Occupied &&
            calls==0 && charges==0 && attempt.GetElapsedSeconds()==12.0,
            "blocked landing cannot move or charge");
        Require(TryPaidRecoveryWithClearance(attempt,charges,true,true,true,
            clear,callback)==PlayerRecoveryNotice::None && calls==0,
            "paused recovery cannot create a notice or engine request");
        Require(TryPaidRecoveryWithClearance(attempt,charges,true,false,true,
            clear,[](CircuitPose){return false;})==PlayerRecoveryNotice::PlacementUnverified &&
            charges==0 && attempt.GetElapsedSeconds()==12.0,
            "failed physical placement refunds entire tentative fee");
        Require(TryPaidRecoveryWithClearance(attempt,charges,true,false,true,
            clear,callback)==PlayerRecoveryNotice::VerifiedAndCharged &&
            calls==1 && charges==1 && attempt.GetElapsedSeconds()==17.0 &&
            attempt.GetTotalPenaltySeconds()==5.0,
            "one clear and verified recovery commits exactly one fee");
        charges=std::numeric_limits<int>::max();
        Require(TryPaidRecoveryWithClearance(attempt,charges,true,false,true,
            clear,callback)==PlayerRecoveryNotice::FeeUnavailable && calls==1,
            "counter exhaustion cannot attempt engine move");

        blocker.Pose.Position.X-=1200.0;blocker.VelocityX=4000.0;
        Require(EvaluatePlayerRecoveryClearance(gate,true,true,blocker).Status==
            PlayerRecoveryClearanceStatus::Occupied,
            "swept approaching rival blocks recovery");
    }
    auto invalidOther=other;invalidOther.ActorToken=0;
    Require(EvaluatePlayerRecoveryClearance(0,true,true,invalidOther).Status==
        PlayerRecoveryClearanceStatus::UnknownTraffic,"unidentified rival rejected");
    invalidOther=other;invalidOther.Slot=HumanGridSlot;
    Require(!EvaluatePlayerRecoveryClearance(0,true,true,invalidOther).Safe(),
        "human cannot substitute for distinct rival");
    invalidOther=other;invalidOther.VelocityX=inf;
    Require(!EvaluatePlayerRecoveryClearance(0,true,true,invalidOther).Safe(),
        "invalid motion blocks landing");
    Require(!EvaluatePlayerRecoveryClearance(-1,false,false,{}).Safe(),
        "invalid gate never falls back to start");

    PlayerRecoveryNoticeState notice;
    Require(notice.At(0.0)==PlayerRecoveryNotice::None,"no invented feedback");
    Require(notice.Record(PlayerRecoveryNotice::Occupied,12.0) &&
        notice.At(12.0)==PlayerRecoveryNotice::Occupied &&
        notice.At(15.0)==PlayerRecoveryNotice::Occupied &&
        notice.At(15.001)==PlayerRecoveryNotice::None,
        "bounded race-clock lifetime without wall-clock timer");
    Require(notice.At(11.0)==PlayerRecoveryNotice::None,
        "earlier race clock cannot inherit old notice");
    Require(notice.Record(PlayerRecoveryNotice::VerifiedAndCharged,20.0) &&
        notice.At(20.0)==PlayerRecoveryNotice::VerifiedAndCharged,
        "latest actual request replaces prior notice");
    Require(!notice.Record(static_cast<PlayerRecoveryNotice>(999),21.0) &&
        notice.At(21.0)==PlayerRecoveryNotice::None,"unknown status clears notice");
    Require(!notice.Record(PlayerRecoveryNotice::Occupied,nan),
        "invalid clock cannot display feedback");
    notice.Record(PlayerRecoveryNotice::Occupied,0.0);
    notice.Reset();
    Require(notice.At(0.0)==PlayerRecoveryNotice::None,"restart clears request feedback");

    std::cout << "Recovery verified-teleport and live clearance transactions passed (NO Unreal).\n";
    return EXIT_SUCCESS;
}
