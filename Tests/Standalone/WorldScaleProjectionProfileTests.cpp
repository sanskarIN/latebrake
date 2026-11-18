#include "Track/WorldScaleRacePacePlan.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

// Strict, standalone source-only QA; this does not execute Unreal Tick,
// real Chaos physics, a cooked level, or a six-actor gameplay simulation.
// A separate --profile mode times the existing full-scan XY route query;
// geometry generation and ALL validity assertions are outside the timed loop.
namespace
{
void Require(bool condition, const char* reason)
{
    if (!condition)
    {
        std::cerr << "WorldScaleProjectionProfile FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void VerifyAllReferenceNodes(const latebrake::WorldScalePacePlan& plan)
{
    using namespace latebrake;
    Require(plan.Valid && WorldScalePaceStations == 512,
            "valid 512-node original world-scale pace reference");
    for (unsigned i = 0; i < WorldScalePaceStations; ++i)
    {
        const CircuitPoint position = plan.Nodes[i].Position;
        const auto first = ProjectWorldScaleRoute(plan, position);
        const auto second = ProjectWorldScaleRoute(plan, position);
        Require(first.Valid && second.Valid &&
                    first.InsidePhysicalCandidateRoad &&
                    first.CentrelineSeparationCm <= 1.0e-5 &&
                    std::isfinite(first.SuggestedSpeedCmps),
                "every authored centerline node projects onto real candidate road");
        Require(first.LapDistanceCm == second.LapDistanceCm &&
                    first.SignedLateralCm == second.SignedLateralCm &&
                    first.CentrelineSeparationCm == second.CentrelineSeparationCm &&
                    first.InsidePhysicalCandidateRoad ==
                        second.InsidePhysicalCandidateRoad,
                "repeated physical projection is deterministic");
        // Independent halfway-between-nodes positions and real lane offsets
        // exercise bends and the start/finish seam, not just exact knots.
        const auto halfway = WorldScaleCenterlineAt(
            (static_cast<double>(i) + 0.5) * WorldScalePaceStepCm);
        Require(halfway.Valid, "halfway circuit pose is authored and finite");
        const double rad = halfway.Pose.HeadingDegrees *
            WorldScalePaceRadPerDegree;
        for (double lateral : {-180.0, 0.0, 180.0})
        {
            const CircuitPoint road{
                halfway.Pose.Position.X - std::sin(rad) * lateral,
                halfway.Pose.Position.Y + std::cos(rad) * lateral
            };
            const auto projected = ProjectWorldScaleRoute(plan, road);
            Require(projected.Valid && projected.InsidePhysicalCandidateRoad &&
                        std::isfinite(projected.LapDistanceCm) &&
                        std::isfinite(projected.SignedLateralCm) &&
                        std::isfinite(projected.CentrelineSeparationCm) &&
                        projected.CentrelineSeparationCm <= 300.0,
                    "half-segment lane offsets remain road-backed and finite");
        }
    }
    const WorldScalePacePlan invalid{};
    Require(!ProjectWorldScaleRoute(invalid, plan.Nodes[0].Position).Valid,
            "invalid reference cannot authorize projection");
    for (const double value : {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        2000000.0})
    {
        Require(!ProjectWorldScaleRoute(
                    plan, CircuitPoint{value, 0.0}).Valid,
                "untrusted X coordinates fail closed");
        Require(!ProjectWorldScaleRoute(
                    plan, CircuitPoint{0.0, value}).Valid,
                "untrusted Y coordinates fail closed");
    }
}

std::vector<latebrake::CircuitPoint> PrepareSixCarObservations(
    unsigned framesPerSecond, unsigned virtualSeconds)
{
    using namespace latebrake;
    constexpr std::size_t roster = 6;
    const std::size_t frames = static_cast<std::size_t>(
        framesPerSecond) * virtualSeconds;
    std::vector<CircuitPoint> observations;
    observations.reserve(frames * roster);
    // Six separate geometry inputs at staggered route stations. The
    // geometry is a deterministic synthetic workload, NOT live actors.
    // Alternate left, center and right source positions, including bends.
    for (std::size_t frame = 0; frame < frames; ++frame)
    {
        for (std::size_t slot = 0; slot < roster; ++slot)
        {
            const double station =
                (static_cast<double>((frame * 13u + slot * 83u +
                                     frame / 5u) % WorldScalePaceStations) +
                 0.37) * WorldScalePaceStepCm;
            const auto pose = WorldScaleCenterlineAt(station);
            Require(pose.Valid, "precomputed synthetic circuit pose is valid");
            const double rad = pose.Pose.HeadingDegrees *
                WorldScalePaceRadPerDegree;
            const double lateral = slot % 3u == 0 ? -220.0 :
                                   slot % 3u == 1 ? 0.0 : 220.0;
            const CircuitPoint road{
                pose.Pose.Position.X - std::sin(rad) * lateral,
                pose.Pose.Position.Y + std::cos(rad) * lateral
            };
            Require(IsOnWorldScaleRoad(road),
                    "synthetic lane sample remains on original source road");
            observations.push_back(road);
        }
    }
    Require(observations.size() == frames * roster,
            "exact six-source-observation workload");
    return observations;
}

void VerifyResults(
    const std::vector<latebrake::WorldScaleRouteProjection>& results)
{
    double checksum = 0.0;
    for (std::size_t i = 0; i < results.size(); ++i)
    {
        const auto& result = results[i];
        Require(result.Valid && result.InsidePhysicalCandidateRoad &&
                    std::isfinite(result.SignedLateralCm) &&
                    std::isfinite(result.SuggestedSpeedCmps) &&
                    result.CentrelineSeparationCm <= 350.0,
                "profile query must produce road-backed, finite geometry");
        checksum += result.LapDistanceCm *
            (1.0 + static_cast<double>(i % 6u) * 1.0e-6);
    }
    Require(std::isfinite(checksum) && checksum > 0.0,
            "no optimizer-elided or empty projection workload");
}

void VerifyProfileInputContract(const latebrake::WorldScalePacePlan& plan)
{
    // The opt-in benchmark must not pass CTest while its input generator
    // or +/-220 cm lane-offset geometry is broken. Keep this short:
    // 30 synthetic frames, six source points per frame, no stopwatch.
    const auto observations = PrepareSixCarObservations(30u, 1u);
    Require(observations.size() == 180u,
            "baseline must cover 180 real source-route projection queries");
    std::vector<latebrake::WorldScaleRouteProjection> results;
    results.reserve(observations.size());
    for (const auto& xy : observations)
    {
        results.push_back(latebrake::ProjectWorldScaleRoute(plan, xy));
    }
    VerifyResults(results);
}

void ProfileRate(const latebrake::WorldScalePacePlan& plan, unsigned fps)
{
    // Sixty seconds of source-only precomputed six-position observations
    // for each rate. Do not turn noisy CPU timings into CTest assertions.
    const auto observations = PrepareSixCarObservations(fps, 60u);
    std::vector<latebrake::WorldScaleRouteProjection> results(
        observations.size());
    using Clock = std::chrono::steady_clock;
    std::array<double, 3> samplesNs{};
    for (std::size_t trial = 0; trial < samplesNs.size(); ++trial)
    {
        // Only projector calls and the result writes are timed. No source
        // generation, IsOnWorldScaleRoad, validation or checksum inside.
        const auto start = Clock::now();
        for (std::size_t i = 0; i < observations.size(); ++i)
        {
            results[i] = latebrake::ProjectWorldScaleRoute(
                plan, observations[i]);
        }
        const auto finish = Clock::now();
        samplesNs[trial] =
            std::chrono::duration<double, std::nano>(
                finish - start).count() /
            static_cast<double>(observations.size());
        Require(std::isfinite(samplesNs[trial]) && samplesNs[trial] >= 0.0,
                "finite portable stopwatch receipt");
        // Every trial's full result buffer is consumed AFTER the timer.
        VerifyResults(results);
    }
    std::sort(samplesNs.begin(), samplesNs.end());
    const double medianNs = samplesNs[1];
    std::cout << fps << " Hz | six lane-offset source points/frame | "
              << observations.size() << " projections/trial | "
              << std::fixed << std::setprecision(1)
              << medianNs << " median ns/query | "
              << std::setprecision(3)
              << 6.0 * medianNs / 1000000.0
              << " ms/6 queries (estimated, portable only)\n";
}
} // namespace

int main(int argc, char** argv)
{
    // CI always uses the default correctness mode, never timing gates.
    // Manually opt into source-only CPU research with --profile.
    const bool profile = argc == 2 &&
        std::string_view(argv[1]) == "--profile";
    if (argc != 1 && !profile)
    {
        std::cerr << "Usage: WorldScaleProjectionProfileTests [--profile]\n";
        return EXIT_FAILURE;
    }
    const auto plan = latebrake::BuildWorldScalePacePlan();
    VerifyAllReferenceNodes(plan);
    VerifyProfileInputContract(plan);
    if (profile)
    {
        std::cout << "Portable C++20 original route, three trial median; "
                     "not licensed UE5.8 or playable Win64 evidence\n";
        for (unsigned fps : std::array<unsigned, 3>{30u, 60u, 120u})
        {
            ProfileRate(plan, fps);
        }
    }
    else
    {
        std::cout << "512 source knots, 1536 half-station lane-offset "
                     "projections, invalid-input guards passed; "
                     "NOT native Unreal/Win64 testing\n";
    }
    return EXIT_SUCCESS;
}
