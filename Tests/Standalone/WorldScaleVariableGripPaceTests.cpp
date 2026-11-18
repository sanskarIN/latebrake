#include "Track/WorldScaleVariableGripPace.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Expect(bool condition, const char* reason)
{
    if (!condition)
    {
        std::cerr << "WorldScaleVariableGripPace FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double a, double b, double tolerance = 0.00001)
{
    return std::isfinite(a) && std::isfinite(b) &&
        std::abs(a - b) <= tolerance;
}

latebrake::WorldScaleGripPaceInput Qualify(
    const latebrake::WorldScalePacePlan& baseline)
{
    using namespace latebrake;
    WorldScaleGripPaceInput input{};
    input.SessionIdentity = 99;
    input.SnapshotClockSeconds = 170.5;
    input.OriginalWorldScaleRoadVerified = true;
    input.CompleteMeasuredStationCoverage = true;
    for (std::size_t i = 0; i < WorldScaleGripPaceStationCount; ++i)
    {
        input.Stations[i] = {
            baseline.Nodes[i].DistanceCm, 1.0, 0.97,
            170.25, static_cast<std::uint64_t>(i + 1u),
            true, true
        };
    }
    return input;
}

void CheckPhysicsBounds(
    const latebrake::WorldScalePacePlan& reference,
    const latebrake::WorldScaleVariableGripPacePlan& wet)
{
    using namespace latebrake;
    Expect(wet.Valid && wet.SourceSessionIdentity != 0 &&
           wet.RelaxationPasses > 0 &&
           wet.RelaxationPasses <= WorldScalePaceStations,
           "qualified source is solved in bounded closed-lap passes");
    double integrated = 0.0;
    for (std::size_t i = 0; i < WorldScalePaceStations; ++i)
    {
        const std::size_t next = (i + 1u) % WorldScalePaceStations;
        const double a = wet.Nodes[i].TargetSpeedCmps;
        const double b = wet.Nodes[next].TargetSpeedCmps;
        Expect(std::isfinite(a) && std::isfinite(b) &&
               a >= WorldScalePaceMinSpeedCmps - 1.0e-5 &&
               a <= reference.Nodes[i].TargetCmps + 1.0e-6 &&
               a <= wet.Nodes[i].LocalSpeedLimitCmps + 1.0e-6 &&
               wet.Nodes[i].VerifiedGripFraction >= 0.50 &&
               wet.Nodes[i].VerifiedGripFraction <= 1.0 &&
               a * a <= b * b +
                  wet.Nodes[i].MaxNextBrakeDeltaSpeedSquared + 0.01 &&
               b * b <= a * a +
                  wet.Nodes[i].MaxNextDriveDeltaSpeedSquared + 0.01,
               "every edge including source seam obeys brake, drive "
               "and baseline upper-speed limits");
        integrated += 2.0 * WorldScalePaceStepCm / (a + b);
    }
    Expect(Near(integrated, wet.EstimatedLapSeconds, 1.0e-6) &&
           wet.EstimatedLapSeconds >=
               reference.EstimatedLapSeconds - 1.0e-6,
           "lap estimate is geometry-only and never improves on dry plan");
}

void UniformDry(const latebrake::WorldScalePacePlan& baseline)
{
    using namespace latebrake;
    const auto input = Qualify(baseline);
    const auto same = BuildWorldScaleVariableGripPace(baseline, input);
    CheckPhysicsBounds(baseline, same);
    Expect(Near(same.MinimumMeasuredGripFraction, 1.0) &&
           Near(same.MaximumMeasurementAgeSeconds, 0.25) &&
           same.ReducedSpeedStations == 0,
           "qualified 100% dry measurements preserve every source speed");
    for (std::size_t i = 0; i < WorldScalePaceStations; ++i)
    {
        Expect(Near(same.Nodes[i].TargetSpeedCmps,
                    baseline.Nodes[i].TargetCmps, 1.0e-8),
               "dry-case baseline speeds are not boosted or recomputed");
    }
}

void UniformWetAndTransitions(
    const latebrake::WorldScalePacePlan& baseline)
{
    using namespace latebrake;
    auto input = Qualify(baseline);
    for (auto& station : input.Stations)
    {
        station.MeasuredGripFraction = 0.65;
    }
    const auto wet = BuildWorldScaleVariableGripPace(baseline, input);
    CheckPhysicsBounds(baseline, wet);
    Expect(Near(wet.MinimumMeasuredGripFraction, 0.65) &&
           wet.ReducedSpeedStations > 0 &&
           wet.EstimatedLapSeconds > baseline.EstimatedLapSeconds,
           "genuine lower grip reduces permitted speed and lengthens "
           "geometric lap estimate");
    for (std::size_t i = 0; i < WorldScalePaceStations; ++i)
    {
        const auto s = i * WorldScalePaceStepCm;
        const auto sampled = SampleWorldScaleVariableGripPace(wet, s);
        Expect(sampled.Valid &&
               Near(sampled.TargetSpeedCmps,
                    wet.Nodes[i].TargetSpeedCmps),
               "all grip-constrained authored stations can be sampled");
    }

    // A fixed speed target sampled immediately across lap start remains
    // continuous even when the low-grip patch crosses the seam.
    input = Qualify(baseline);
    for (unsigned i = 0; i < 15u; ++i)
    {
        input.Stations[i].MeasuredGripFraction = 0.52;
        input.Stations[WorldScalePaceStations - 1u - i]
            .MeasuredGripFraction = 0.52;
    }
    const auto seam = BuildWorldScaleVariableGripPace(baseline, input);
    CheckPhysicsBounds(baseline, seam);
    Expect(Near(seam.MinimumMeasuredGripFraction, 0.52) &&
           seam.ReducedSpeedStations > 0,
           "a 30-node grip patch crossing the 511 -> 0 edge remains valid");
    for (double distance : {-WorldScaleCircuitLapCm, 0.0,
                            WorldScaleCircuitLapCm,
                            2.0 * WorldScaleCircuitLapCm})
    {
        const auto at = SampleWorldScaleVariableGripPace(seam, distance);
        Expect(at.Valid &&
               Near(at.TargetSpeedCmps, seam.Nodes[0].TargetSpeedCmps) &&
               Near(at.WrappedStationCm, 0.0),
               "all complete signed-lap wraps agree at start/finish");
    }
    const double delta = 0.001;
    const auto before =
        SampleWorldScaleVariableGripPace(seam, -delta);
    const auto after =
        SampleWorldScaleVariableGripPace(seam, delta);
    Expect(before.Valid && after.Valid &&
           std::abs(before.TargetSpeedCmps -
                    after.TargetSpeedCmps) <= 0.1,
           "speed-squared interpolation is continuous at lap seam");
}

void RejectIncompleteOrInventedGrip(
    const latebrake::WorldScalePacePlan& reference)
{
    using namespace latebrake;
    auto input = Qualify(reference);
    Expect(!BuildWorldScaleVariableGripPace(
               WorldScalePacePlan{}, input).Valid,
           "invalid world-scale source geometry never authorizes pace");
    auto invalid = input;
    invalid.SessionIdentity = 0;
    Expect(!BuildWorldScaleVariableGripPace(reference, invalid).Valid,
           "anonymous observation sessions fail closed");
    invalid = input;
    invalid.CompleteMeasuredStationCoverage = false;
    Expect(!BuildWorldScaleVariableGripPace(reference, invalid).Valid,
           "missing route coverage is never inferred dry");
    invalid = input;
    invalid.OriginalWorldScaleRoadVerified = false;
    Expect(!BuildWorldScaleVariableGripPace(reference, invalid).Valid,
           "nonphysical road sources are rejected");
    invalid = input;
    invalid.Stations[511].PhysicalRoadContactVerified = false;
    Expect(!BuildWorldScaleVariableGripPace(reference, invalid).Valid,
           "one unverified seam contact invalidates entire pace plan");
    invalid = input;
    invalid.Stations[40].RealFrictionMeasurementVerified = false;
    Expect(!BuildWorldScaleVariableGripPace(reference, invalid).Valid,
           "a hypothetical friction measurement is never accepted");
    invalid = input;
    invalid.Stations[40].RoadComponentIdentity = 0;
    Expect(!BuildWorldScaleVariableGripPace(reference, invalid).Valid,
           "every friction observation needs a nonzero road identity");
    invalid = input;
    invalid.Stations[44].SourceConfidence = 0.70;
    Expect(!BuildWorldScaleVariableGripPace(reference, invalid).Valid,
           "uncertain road-friction source fails the confidence gate");
    invalid = input;
    invalid.Stations[51].MeasurementClockSeconds = 167.0;
    Expect(!BuildWorldScaleVariableGripPace(reference, invalid).Valid,
           "one stale sensor sample denies a qualified full-lap plan");
    invalid = input;
    invalid.Stations[51].MeasurementClockSeconds = 171.0;
    Expect(!BuildWorldScaleVariableGripPace(reference, invalid).Valid,
           "future-dated source observation is not trusted");
    invalid = input;
    invalid.Stations[0].StationCm += 100.0;
    Expect(!BuildWorldScaleVariableGripPace(reference, invalid).Valid,
           "swapped or relocated source stations fail closed");
    invalid = input;
    invalid.Stations[12].MeasuredGripFraction =
        std::numeric_limits<double>::quiet_NaN();
    Expect(!BuildWorldScaleVariableGripPace(reference, invalid).Valid,
           "NaN physical grip is never an authorized speed limit");
    invalid = input;
    invalid.Stations[23].MeasuredGripFraction = 0.30;
    Expect(!BuildWorldScaleVariableGripPace(reference, invalid).Valid,
           "extreme grip beyond supported advisory limits fails closed");
    invalid = input;
    invalid.Stations[80].MeasuredGripFraction = 1.15;
    Expect(!BuildWorldScaleVariableGripPace(reference, invalid).Valid,
           "friction gain cannot increase racing target over dry source");
    WorldScaleGripPaceSettings settings{};
    settings.BaselineBrakeAccelCmS2 *= 1.1;
    Expect(!BuildWorldScaleVariableGripPace(reference, input, settings).Valid,
           "configuration cannot secretly exceed baseline braking authority");
    settings = {};
    settings.MaximumMeasurementAgeSeconds =
        std::numeric_limits<double>::infinity();
    Expect(!BuildWorldScaleVariableGripPace(reference, input, settings).Valid,
           "unbounded source observation age is rejected");
}

void SampleAcrossTimeBases(const latebrake::WorldScalePacePlan& reference)
{
    using namespace latebrake;
    auto input = Qualify(reference);
    // Validate a genuinely varying measurement across many intervals.
    for (std::size_t i = 0; i < WorldScalePaceStations; ++i)
    {
        input.Stations[i].MeasuredGripFraction =
            0.65 + 0.30 * static_cast<double>((i * 73u) % 97u) / 96.0;
    }
    const auto plan = BuildWorldScaleVariableGripPace(reference, input);
    CheckPhysicsBounds(reference, plan);
    for (double fps : {30.0, 60.0, 120.0})
    {
        const double step = 2300.0 / fps;
        for (int frame = -300; frame < 300; ++frame)
        {
            const double station =
                WorldScaleCircuitLapCm + frame * step;
            const auto sample =
                SampleWorldScaleVariableGripPace(plan, station);
            Expect(sample.Valid &&
                   std::isfinite(sample.TargetSpeedCmps) &&
                   sample.TargetSpeedCmps <=
                       WorldScalePaceMaxSpeedCmps + 1.0e-6 &&
                   sample.TargetSpeedCmps >=
                       WorldScalePaceMinSpeedCmps - 1.0e-6,
                   "signed multi-frame seam sampling is finite and bounded");
        }
    }
    Expect(!SampleWorldScaleVariableGripPace(
               WorldScaleVariableGripPacePlan{}, 0.0).Valid,
           "uninitialized grip plan cannot supply an AI pace command");
    Expect(!SampleWorldScaleVariableGripPace(
               plan, std::numeric_limits<double>::infinity()).Valid,
           "nonfinite pace stations are rejected");
}
} // namespace

int main()
{
    const auto baseline = latebrake::BuildWorldScalePacePlan();
    Expect(baseline.Valid,
           "the original 1.7km world-scale circuit is reproducible");
    UniformDry(baseline);
    UniformWetAndTransitions(baseline);
    RejectIncompleteOrInventedGrip(baseline);
    SampleAcrossTimeBases(baseline);
    std::cout << "WorldScaleVariableGripPace source-only 512-station "
                 "grip, closed-lap braking, seam, sensor rejection and "
                 "30/60/120Hz sampling PASS; no Unreal qualification\n";
    return EXIT_SUCCESS;
}
