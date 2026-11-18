#include "Track/WorldScaleRacePacePlan.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <vector>

// Native, portable geometry-only source QA. This program is not a
// performance claim about Unreal Tick, Chaos physics or six live actors.
// With --profile it times ONLY the unchanged nearest-segment projector;
// generating synthetic observations and building the reference happen
// outside the timed loop.
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
                "repeated projection of one physical position is deterministic");
    }
    const WorldScalePacePlan invalid{};
    Require(!ProjectWorldScaleRoute(invalid, plan.Nodes[0].Position).Valid,
            "invalid reference cannot be projected");
}

std::vector<latebrake::CircuitPoint> PrepareSixCarObservations(
    const latebrake::WorldScalePacePlan& plan,
    unsigned framesPerSecond, unsigned virtualSeconds)
{
    using namespace latebrake;
    constexpr unsigned roster = 6;
    const std::size_t frames = static_cast<std::size_t>(
        framesPerSecond) * virtualSeconds;
    std::vector<CircuitPoint> observations;
    observations.reserve(frames * roster);
    // Six independent source-coordinate streams cover every part of
    // the candidate route; these are NOT synthetic Unreal actors.
    for (std::size_t frame = 0; frame < frames; ++frame)
    {
        for (unsigned slot = 0; slot < roster; ++slot)
        {
            const std::size_t index =
                (frame * 13u + static_cast<std::size_t>(slot) * 83u +
                 frame / 5u) % WorldScalePaceStations;
            observations.push_back(plan.Nodes[index].Position);
        }
    }
    Require(observations.size() == frames * roster,
            "exact six-observation synthetic workload");
    return observations;
}

void ProfileRate(const latebrake::WorldScalePacePlan& plan, unsigned fps)
{
    // Sixty simulated seconds at each stated observation rate; no
    // wall-clock deadline and no brittle CI timing pass/fail threshold.
    const auto observations = PrepareSixCarObservations(plan, fps, 60u);
    double checksum = 0.0;
    using Clock = std::chrono::steady_clock;
    const auto start = Clock::now();
    for (std::size_t i = 0; i < observations.size(); ++i)
    {
        const auto result = latebrake::ProjectWorldScaleRoute(
            plan, observations[i]);
        Require(result.Valid && result.InsidePhysicalCandidateRoad &&
                    result.CentrelineSeparationCm <= 1.0e-5,
                "profile must not conceal an invalid geometric projection");
        checksum += result.LapDistanceCm *
            (1.0 + static_cast<double>(i % 6u) * 1.0e-6);
    }
    const auto finish = Clock::now();
    const double nanoseconds =
        std::chrono::duration<double, std::nano>(finish - start).count();
    Require(std::isfinite(checksum) && checksum > 0.0 &&
                std::isfinite(nanoseconds) && nanoseconds >= 0.0,
            "profile produced a finite nonempty observation receipt");
    const double perProjectionNs =
        nanoseconds / static_cast<double>(observations.size());
    std::cout << fps << " Hz | 6 synthetic positions/frame | "
              << observations.size() << " projections | "
              << std::fixed << std::setprecision(1)
              << perProjectionNs << " ns/projection | checksum="
              << std::setprecision(3) << checksum << '\n';
}
} // namespace

int main(int argc, char** argv)
{
    // Default mode is a quick deterministic CTest smoke check.
    // Timings are opt-in because CI virtual machines have variable CPU
    // contention and cannot certify a frame-time budget for an actual game.
    const bool profile = argc == 2 &&
        std::string_view(argv[1]) == "--profile";
    if (argc != 1 && !profile)
    {
        std::cerr << "Usage: WorldScaleProjectionProfileTests [--profile]\n";
        return EXIT_FAILURE;
    }
    const auto plan = latebrake::BuildWorldScalePacePlan();
    VerifyAllReferenceNodes(plan);
    if (profile)
    {
        std::cout << "Portable C++20 geometry-only measurement; "
                     "not native Unreal gameplay evidence\n";
        for (unsigned fps : std::array<unsigned, 3>{30u, 60u, 120u})
        {
            ProfileRate(plan, fps);
        }
    }
    else
    {
        std::cout << "512 authored centerline node projection checks passed; "
                     "run with --profile for 30/60/120 Hz synthetic costs; "
                     "NOT a UE5.8 or Win64 playtest\n";
    }
    return EXIT_SUCCESS;
}
