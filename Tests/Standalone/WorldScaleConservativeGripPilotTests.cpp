#include "AI/WorldScaleConservativeGripPilot.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool ok, const char* reason)
{
    if (!ok)
    {
        std::cerr << "WorldScaleConservativeGripPilot FAIL: "
                  << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}

latebrake::WorldScaleGripPaceInput QualifiedRoad(
    const latebrake::WorldScalePacePlan& base,
    double fraction)
{
    using namespace latebrake;
    WorldScaleGripPaceInput input{};
    input.SessionIdentity = 12345u;
    input.SnapshotClockSeconds = 400.0;
    input.OriginalWorldScaleRoadVerified = true;
    input.CompleteMeasuredStationCoverage = true;
    for (std::size_t i = 0; i < WorldScalePaceStations; ++i)
    {
        input.Stations[i] = {
            base.Nodes[i].DistanceCm,
            fraction,
            0.99,
            399.9,
            static_cast<std::uint64_t>(i + 1u),
            true,
            true
        };
    }
    return input;
}

latebrake::WorldScaleGripPilotObservation OnRoad(
    double station, double speed, double seconds = 400.2)
{
    using namespace latebrake;
    const auto pose = WorldScaleCenterlineAt(station);
    Check(pose.Valid, "all fixture poses originate on source circuit");
    const double heading = pose.Pose.HeadingDegrees *
        WorldScalePaceRadPerDegree;
    WorldScalePilotSample car{
        pose.Pose.Position,
        heading, speed, speed, 1.0 / 60.0,
        true, true
    };
    return {car, 12345u, seconds, true, true};
}

void DryAndWetSource(
    const latebrake::WorldScalePacePlan& reference)
{
    using namespace latebrake;
    const auto dryPlan = BuildWorldScaleVariableGripPace(
        reference, QualifiedRoad(reference, 1.0));
    const auto wetPlan = BuildWorldScaleVariableGripPace(
        reference, QualifiedRoad(reference, 0.65));
    Check(dryPlan.Valid && wetPlan.Valid,
          "independently qualified dry/wet source pace plans exist");

    WorldScaleConservativeGripPilot dry;
    WorldScaleConservativeGripPilot wet;
    WorldScaleCandidateRivalPilot baseline;
    bool wetBrakeSeen = false;
    bool dryParitySeen = false;
    for (unsigned i = 0; i < WorldScalePaceStations; ++i)
    {
        const double station = i * WorldScalePaceStepCm;
        const double speed = 0.5 * (
            wetPlan.Nodes[i].TargetSpeedCmps +
            reference.Nodes[i].TargetCmps);
        const auto input = OnRoad(station, speed);
        dry.Reset();
        wet.Reset();
        baseline.Reset();
        const auto plain = baseline.Step(reference, input.Physical);
        const auto permitted = dry.Step(reference, dryPlan, input);
        const auto conservative = wet.Step(reference, wetPlan, input);
        Check(plain.Valid && permitted.Valid && conservative.Valid &&
              dryPlan.SourceSessionIdentity == input.SessionIdentity,
              "source qualified all three neutral baseline observations");
        Check(plain.Command.Steering ==
                  permitted.Decision.Command.Steering &&
              plain.Command.Brake ==
                  permitted.Decision.Command.Brake &&
              plain.Command.Throttle ==
                  permitted.Decision.Command.Throttle &&
              plain.SpeedReferenceCmps ==
                  permitted.Decision.SpeedReferenceCmps,
              "all-dry grip integration exactly preserves original pilot");
        Check(conservative.Decision.Command.Steering ==
                  plain.Command.Steering &&
              conservative.Decision.Command.Throttle <=
                  plain.Command.Throttle &&
              conservative.Decision.SpeedReferenceCmps <=
                  plain.SpeedReferenceCmps + 1.0e-6 &&
              (!plain.Command.Brake ||
                  conservative.Decision.Command.Brake) &&
              (!conservative.Decision.Command.Brake ||
                  conservative.Decision.Command.Throttle == 0.0),
              "wet grip may restrict only throttle/brake/pace, never steer");
        dryParitySeen = true;
        if (conservative.ConservativeBrakeRequested)
        {
            wetBrakeSeen = true;
        }
    }
    Check(dryParitySeen && wetBrakeSeen,
          "complete 512-station comparison demonstrates grip braking");
}

void RejectedSensorsAndRecovery(
    const latebrake::WorldScalePacePlan& reference)
{
    using namespace latebrake;
    const auto grip = BuildWorldScaleVariableGripPace(
        reference, QualifiedRoad(reference, 0.80));
    Check(grip.Valid, "valid source for negative control cases");
    WorldScaleConservativeGripPilot pilot;
    const auto first = OnRoad(5000.0, 0.0);
    Check(pilot.Step(reference, grip, first).Valid,
          "real road and tire observation admits source-only pilot");

    auto bad = first;
    bad.CurrentTireContactVerified = false;
    const auto lost = pilot.Step(reference, grip, bad);
    Check(!lost.Valid && lost.Decision.Command.Brake &&
          lost.Decision.Command.Throttle == 0.0,
          "missing physical wheel contact resets and denies all drive");

    bad = first;
    bad.SourceRoadIdentityVerified = false;
    Check(!pilot.Step(reference, grip, bad).Valid,
          "unverified source roadway may not inherit last car's pace");

    bad = first;
    bad.SessionIdentity = 100000u;
    Check(!pilot.Step(reference, grip, bad).Valid,
          "a different race session cannot reuse the measured grip plan");

    bad = first;
    bad.PhysicsClockSeconds = 399.5;
    Check(!pilot.Step(reference, grip, bad).Valid,
          "future-dated source plan rejects older physical timestamps");

    bad = first;
    bad.PhysicsClockSeconds = 401.0;
    Check(!pilot.Step(reference, grip, bad).Valid,
          "source plan older than 750ms fails instead of reusing dry grip");

    bad = first;
    bad.PhysicsClockSeconds =
        std::numeric_limits<double>::quiet_NaN();
    Check(!pilot.Step(reference, grip, bad).Valid,
          "NaN physics timestamp has no throttle authority");

    bad = first;
    bad.Physical.RaceGreen = false;
    Check(!pilot.Step(reference, grip, bad).Valid,
          "unqualified race green cannot resume a previous pace command");

    bad = first;
    bad.Physical.ForwardSpeedCmps = -300.0;
    bad.Physical.FlatSpeedCmps = 300.0;
    const auto reverse = pilot.Step(reference, grip, bad);
    Check(reverse.Valid && reverse.Decision.Command.Brake &&
          reverse.Decision.Command.Throttle == 0.0,
          "honest reverse movement retains original brake authority");

    Check(!pilot.Step(reference,
                      WorldScaleVariableGripPacePlan{}, first).Valid,
          "unqualified grip plan cannot be replaced by invented dry data");
    Check(pilot.Step(reference, grip, first).Valid,
          "after failed source observation new valid physics may reacquire");
}

void TemporalContinuityAtSeam(
    const latebrake::WorldScalePacePlan& reference)
{
    using namespace latebrake;
    auto plan = BuildWorldScaleVariableGripPace(
        reference, QualifiedRoad(reference, 0.70));
    Check(plan.Valid, "wet plan for across-seam source input");
    for (double fps : {30.0, 60.0, 120.0})
    {
        WorldScaleConservativeGripPilot pilot;
        const double speed = 2200.0;
        const double dt = 1.0 / fps;
        const double start = WorldScaleCircuitLapCm - 300.0;
        for (unsigned frame = 0; frame < 30; ++frame)
        {
            auto observation = OnRoad(
                start + frame * dt * speed, speed,
                400.0 + frame * dt);
            observation.Physical.DeltaSeconds = dt;
            auto out = pilot.Step(reference, plan, observation);
            Check(out.Valid && out.Decision.Command.Throttle >= 0.0 &&
                  out.Decision.Command.Throttle <= 1.0 &&
                  out.Decision.Command.Steering >= -1.0 &&
                  out.Decision.Command.Steering <= 1.0 &&
                  out.GripConstrainedReferenceCmps <=
                      out.DrySpeedReferenceCmps + 1.0e-6,
                  "30/60/120Hz seam drive respects measured grip bounds");
        }
        auto stale = OnRoad(start + 31.0 * dt * speed,
                            speed, 401.0);
        stale.Physical.DeltaSeconds = dt;
        Check(!pilot.Step(reference, plan, stale).Valid,
              "stale grip snapshot ends continuous control authority");
    }
}
} // namespace

int main()
{
    const auto reference = latebrake::BuildWorldScalePacePlan();
    Check(reference.Valid, "source-only original 1.7 km pace exists");
    DryAndWetSource(reference);
    RejectedSensorsAndRecovery(reference);
    TemporalContinuityAtSeam(reference);
    std::cout << "WorldScaleConservativeGripPilot dry parity, wet-only "
                 "braking, source freshness, reverse, session gating and "
                 "30/60/120Hz seam PASS (source-only, not Unreal)\n";
    return EXIT_SUCCESS;
}
