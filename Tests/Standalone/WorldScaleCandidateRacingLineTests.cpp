#include "Track/WorldScaleCandidateRacingLine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool yes, const char* message)
{
    if (!yes)
    {
        std::cerr << "WorldScaleCandidateRacingLine FAIL: "
                  << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

latebrake::WorldScaleLineRequest Qualified()
{
    latebrake::WorldScaleLineRequest req{};
    req.ProposalIdentity = 42u;
    req.AuthoredReferenceConfirmed = true;
    req.SourceOnlyAdvisory = true;
    return req;
}

void CheckEveryEdge(
    const latebrake::WorldScalePacePlan& source,
    const latebrake::WorldScaleLinePlan& candidate)
{
    using namespace latebrake;
    Require(candidate.Valid &&
            candidate.VerifiedSegments == WorldScaleLineStationCount &&
            candidate.ProposalIdentity == 42u &&
            candidate.RelaxationPasses >= 1u &&
            candidate.RelaxationPasses <= WorldScaleLineStationCount &&
            candidate.SmoothingPasses > 0u &&
            candidate.OriginalPhysicalRoadUnchanged &&
            candidate.NoOfficialProgressAuthority &&
            candidate.DoesNotSteerChaos &&
            candidate.EstimatedLapSeconds >=
                source.EstimatedLapSeconds - 1e-6,
            "source-only closed loop solves without changing official game");

    constexpr double acceleration =
        2.0 * WorldScalePaceDriveAccelCmps2 * WorldScalePaceStepCm;
    constexpr double brake =
        2.0 * WorldScalePaceBrakeAccelCmps2 * WorldScalePaceStepCm;
    double lap = 0.0;
    for (std::size_t i = 0; i < WorldScaleLineStationCount; ++i)
    {
        const auto next = (i + 1u) % WorldScaleLineStationCount;
        const auto& a = candidate.Nodes[i];
        const auto& b = candidate.Nodes[next];
        Require(a.PhysicallySupportedAtSource &&
                IsOnWorldScaleRoad(a.CandidateCenter) &&
                WorldScaleLineSpanSupported(
                    a.CandidateCenter, b.CandidateCenter),
                "all source center nodes and sampled chords are paved");
        Require(std::abs(a.OffsetCm) <= 250.0 + 1e-6 &&
                std::abs(a.OffsetCm - b.OffsetCm) <= 65.0 + 1e-6 &&
                a.MinimumBodyEdgeClearanceCm >= 55.0 - 1e-6 &&
                a.SourceStationCm == i * WorldScalePaceStepCm,
                "all lateral/slope/body-clearance limits hold including seam");
        Require(std::isfinite(a.CurvaturePerCm) &&
                a.CurvaturePerCm >= 0.0 &&
                a.CurvaturePerCm < 0.002 &&
                a.TargetSpeedCmps >= WorldScalePaceMinSpeedCmps - 1e-6 &&
                a.TargetSpeedCmps <= source.Nodes[i].TargetCmps + 1e-6 &&
                a.TargetSpeedCmps <= a.LocalSpeedLimitCmps + 1e-6 &&
                a.TargetSpeedCmps * a.TargetSpeedCmps <=
                    b.TargetSpeedCmps * b.TargetSpeedCmps +
                        brake + 0.01 &&
                b.TargetSpeedCmps * b.TargetSpeedCmps <=
                    a.TargetSpeedCmps * a.TargetSpeedCmps +
                        acceleration + 0.01,
                "candidate may not exceed authored lateral, braking, "
                "acceleration or nominal speed limits");
        lap += 2.0 * WorldScalePaceStepCm /
            (a.TargetSpeedCmps + b.TargetSpeedCmps);
    }
    Require(std::abs(lap - candidate.EstimatedLapSeconds) < 1e-6,
            "computed source-only lap estimate covers all 512 edges");
}

void BaselineAndSmooth(
    const latebrake::WorldScalePacePlan& reference)
{
    using namespace latebrake;
    auto request = Qualified();
    auto plan = BuildWorldScaleCandidateRacingLine(reference, request);
    CheckEveryEdge(reference, plan);
    Require(plan.MaximumAbsoluteLateralOffsetCm < 1e-10 &&
            plan.MaximumNeighborLateralChangeCm < 1e-10,
            "zero requested lane shift stays on true source centerline");
    const auto repeated = BuildWorldScaleCandidateRacingLine(
        reference, request);
    Require(repeated.Valid &&
            repeated.EstimatedLapSeconds == plan.EstimatedLapSeconds,
            "unchanged source request is deterministic");

    for (std::size_t i = 0; i < WorldScaleLineStationCount; ++i)
    {
        request.DesiredOffsetCm[i] =
            175.0 * std::sin(2.0 * RoundedPi *
                static_cast<double>(i) /
                static_cast<double>(WorldScaleLineStationCount));
    }
    plan = BuildWorldScaleCandidateRacingLine(reference, request);
    CheckEveryEdge(reference, plan);
    Require(plan.MaximumAbsoluteLateralOffsetCm > 125.0 &&
            plan.MaximumAbsoluteLateralOffsetCm <= 175.0 &&
            plan.MaximumNeighborLateralChangeCm <= 65.0,
            "coherent race-line bend offsets remain road-supported");
    request.DesiredOffsetCm[511] = 250.0;
    request.DesiredOffsetCm[0] = -250.0;
    plan = BuildWorldScaleCandidateRacingLine(reference, request);
    CheckEveryEdge(reference, plan);
    Require(std::abs(plan.Nodes[511].OffsetCm -
                     plan.Nodes[0].OffsetCm) <= 65.0,
            "opposite requested seam offsets cannot produce a teleport");

    request = Qualified();
    request.DesiredOffsetCm[42] = 250.0;
    plan = BuildWorldScaleCandidateRacingLine(reference, request);
    CheckEveryEdge(reference, plan);
    Require(plan.MaximumNeighborLateralChangeCm <= 65.0,
            "single-frame steering spike is softened before road guidance");
}

void SampleSignedLaps(
    const latebrake::WorldScalePacePlan& source)
{
    using namespace latebrake;
    auto request = Qualified();
    for (std::size_t i = 0; i < WorldScaleLineStationCount; ++i)
    {
        request.DesiredOffsetCm[i] =
            120.0 * std::cos(
                2.0 * RoundedPi * static_cast<double>(i) /
                WorldScaleLineStationCount);
    }
    const auto plan = BuildWorldScaleCandidateRacingLine(source,request);
    CheckEveryEdge(source,plan);

    for (double station : {-WorldScaleCircuitLapCm, 0.0,
                           WorldScaleCircuitLapCm,
                           2.0 * WorldScaleCircuitLapCm})
    {
        const auto sample = SampleWorldScaleCandidateRacingLine(
            plan,station);
        Require(sample.Valid && sample.WrappedStationCm == 0.0 &&
                sample.InsideOriginalPhysicalRoad &&
                sample.TargetSpeedCmps <= source.Nodes[0].TargetCmps + 1e-6,
                "negative/multiple lap wraps return a paved candidate lane");
    }
    const auto before = SampleWorldScaleCandidateRacingLine(
        plan, WorldScaleCircuitLapCm - 0.01);
    const auto after = SampleWorldScaleCandidateRacingLine(plan, 0.01);
    Require(before.Valid && after.Valid &&
            std::hypot(before.CandidateCenter.X -
                           after.CandidateCenter.X,
                       before.CandidateCenter.Y -
                           after.CandidateCenter.Y) < 2.0,
            "continuous selected line meets on both sides of the seam");

    for (double fps : {30.0,60.0,120.0})
    {
        const double dt = 1.0 / fps;
        for (int frame = -300; frame <= 300; ++frame)
        {
            const double s = WorldScaleCircuitLapCm +
                frame * 2200.0 * dt;
            const auto sample = SampleWorldScaleCandidateRacingLine(
                plan,s);
            Require(sample.Valid &&
                    sample.InsideOriginalPhysicalRoad &&
                    std::isfinite(sample.DirectionRadians) &&
                    std::isfinite(sample.TargetSpeedCmps) &&
                    std::abs(sample.OffsetCm) <= 250.0 + 1e-6 &&
                    sample.TargetSpeedCmps <=
                        WorldScalePaceMaxSpeedCmps + 1e-6,
                    "30/60/120 source-only frame sampling stays road-backed");
        }
    }
}

void RejectUntrustedCandidates(
    const latebrake::WorldScalePacePlan& source)
{
    using namespace latebrake;
    auto req = Qualified();
    Require(!BuildWorldScaleCandidateRacingLine({},req).Valid,
            "absent authored physical route fails closed");
    req.ProposalIdentity = 0;
    Require(!BuildWorldScaleCandidateRacingLine(source,req).Valid,
            "anonymous race-line proposals cannot authorize changes");
    req = Qualified();
    req.AuthoredReferenceConfirmed = false;
    Require(!BuildWorldScaleCandidateRacingLine(source,req).Valid,
            "unqualified source route does not create a race line");
    req = Qualified();
    req.SourceOnlyAdvisory = false;
    Require(!BuildWorldScaleCandidateRacingLine(source,req).Valid,
            "candidate path cannot be used as direct Chaos control");
    req = Qualified();
    req.DesiredOffsetCm[511] =
        std::numeric_limits<double>::quiet_NaN();
    Require(!BuildWorldScaleCandidateRacingLine(source,req).Valid,
            "NaN proposed road offset must fail on full-ring validation");
    req = Qualified();
    req.DesiredOffsetCm[73] = 500.0;
    Require(!BuildWorldScaleCandidateRacingLine(source,req).Valid,
            "requests beyond conservative chassis/road corridor are denied");

    auto settings = WorldScaleLinePlannerSettings{};
    settings.MaximumCandidateOffsetCm = 280.0;
    settings.RoadEdgeSafetyMarginCm = 100.0;
    Require(!BuildWorldScaleCandidateRacingLine(
                source,Qualified(),settings).Valid,
            "source corridor cannot exceed original 900cm roadway");
    settings = {};
    settings.MaximumLocalLateralAccelerationCmS2 *= 1.1;
    Require(!BuildWorldScaleCandidateRacingLine(
                source,Qualified(),settings).Valid,
            "candidate guidance may not invent tire lateral grip");
    settings = {};
    settings.NoActualVehicleSteeringAuthority = false;
    Require(!BuildWorldScaleCandidateRacingLine(
                source,Qualified(),settings).Valid,
            "no direct steering or actor control can be requested");
    settings = {};
    settings.ForbidOfficialCheckpointMutation = false;
    Require(!BuildWorldScaleCandidateRacingLine(
                source,Qualified(),settings).Valid,
            "official checkpoint and lap state are never touched");
    Require(!SampleWorldScaleCandidateRacingLine(
               {},0.0).Valid,
           "missing plan produces no fictitious steering target");
    const auto valid =
        BuildWorldScaleCandidateRacingLine(source,Qualified());
    Require(valid.Valid &&
            !SampleWorldScaleCandidateRacingLine(
                valid,std::numeric_limits<double>::infinity()).Valid,
            "invalid source sample cannot be treated as real driving");
}
} // namespace

int main()
{
    const auto reference = latebrake::BuildWorldScalePacePlan();
    Require(reference.Valid, "original future circuit is available");
    BaselineAndSmooth(reference);
    SampleSignedLaps(reference);
    RejectUntrustedCandidates(reference);
    std::cout << "WorldScaleCandidateRacingLine PASS: 512-node loop, "
                 "smooth lateral offsets, 68-piece road union, "
                 "conservative corner/brake/drive speeds, 30/60/120Hz "
                 "sampling and fail-closed source inputs (NO Unreal)\n";
    return EXIT_SUCCESS;
}
