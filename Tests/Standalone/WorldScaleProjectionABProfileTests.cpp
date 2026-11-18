#include "Track/WorldScaleProjectionBlockIndex.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <vector>

namespace
{
using Clock = std::chrono::steady_clock;
using Projection = latebrake::WorldScaleRouteProjection;

void Require(bool condition, const char* reason)
{
    if (!condition)
    {
        std::cerr << "WorldScaleProjectionABProfile FAIL: "
                  << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Same(const Projection& a, const Projection& b)
{
    return a.Valid == b.Valid && (!a.Valid ||
            (a.LapDistanceCm == b.LapDistanceCm &&
             a.SignedLateralCm == b.SignedLateralCm &&
             a.CentrelineSeparationCm == b.CentrelineSeparationCm &&
             a.InsidePhysicalCandidateRoad == b.InsidePhysicalCandidateRoad &&
             a.SuggestedSpeedCmps == b.SuggestedSpeedCmps));
}

std::vector<latebrake::CircuitPoint> BuildWorkload(unsigned fps)
{
    using namespace latebrake;
    const std::size_t frames = static_cast<std::size_t>(fps) * 60u;
    std::vector<CircuitPoint> positions;
    positions.reserve(frames * 6u);
    for (std::size_t tick = 0; tick < frames; ++tick)
    {
        for (unsigned slot = 0; slot < 6; ++slot)
        {
            // Six deliberately separated locations and lateral offsets,
            // not a claim of six physics-driven actors in Unreal.
            const double distance =
                (static_cast<double>((tick * 19u + slot * 79u) % 512u) +
                 0.23) * WorldScalePaceStepCm;
            const auto pose = WorldScaleCenterlineAt(distance);
            Require(pose.Valid, "source centerline sample exists");
            const double radians = pose.Pose.HeadingDegrees *
                WorldScalePaceRadPerDegree;
            const double lateral = (static_cast<int>(slot % 3u) - 1) * 175.0;
            const CircuitPoint at{
                pose.Pose.Position.X - std::sin(radians) * lateral,
                pose.Pose.Position.Y + std::cos(radians) * lateral
            };
            Require(IsOnWorldScaleRoad(at),
                    "benchmark source samples remain on projected road");
            positions.push_back(at);
        }
    }
    return positions;
}

void AuditPair(const std::vector<Projection>& baseline,
               const std::vector<Projection>& candidate,
               const std::vector<latebrake::WorldScaleProjectionBlockStats>&
                   stats,
               std::size_t& checked,
               std::size_t& skipped)
{
    Require(baseline.size() == candidate.size() &&
                baseline.size() == stats.size(),
            "complete baseline/candidate result buffers");
    for (std::size_t i = 0; i < baseline.size(); ++i)
    {
        Require(Same(baseline[i], candidate[i]),
                "block search must match EVERY original result field");
        Require(stats[i].TestedSegments <= latebrake::WorldScalePaceStations &&
                    stats[i].TestedSegments %
                    latebrake::WorldScaleProjectionBlockWidth == 0 &&
                    stats[i].SkippedBlocks <=
                    latebrake::WorldScaleProjectionBlockCount,
                "bounded block segment accounting");
        ++checked;
        skipped += latebrake::WorldScalePaceStations -
            stats[i].TestedSegments;
    }
}

struct TrialResult final
{
    double BaselineNs = 0.0;
    double IndexedNs = 0.0;
    std::size_t Checked = 0;
    std::size_t AvoidedSegments = 0;
};

TrialResult RunTrial(
    const latebrake::WorldScalePacePlan& plan,
    const latebrake::WorldScaleProjectionBlockIndex& index,
    const std::vector<latebrake::CircuitPoint>& source,
    bool reverseOrder)
{
    std::vector<Projection> baseline(source.size());
    std::vector<Projection> candidate(source.size());
    std::vector<latebrake::WorldScaleProjectionBlockStats> stats(
        source.size());

    const auto doReference = [&] {
        const auto begin = Clock::now();
        for (std::size_t i = 0; i < source.size(); ++i)
        {
            baseline[i] = latebrake::ProjectWorldScaleRoute(
                plan, source[i]);
        }
        return std::chrono::duration<double, std::nano>(
            Clock::now() - begin).count();
    };

    const auto doIndexed = [&] {
        const auto begin = Clock::now();
        for (std::size_t i = 0; i < source.size(); ++i)
        {
            candidate[i] = latebrake::ProjectWorldScaleRouteBlocks(
                plan, index, source[i], &stats[i]);
        }
        return std::chrono::duration<double, std::nano>(
            Clock::now() - begin).count();
    };

    // Alternate run order to partially compensate for cache warming.
    // No input generation, diagnostic assertion or scoring inside timer.
    double referenceNs = 0.0;
    double candidateNs = 0.0;
    if (reverseOrder)
    {
        candidateNs = doIndexed();
        referenceNs = doReference();
    }
    else
    {
        referenceNs = doReference();
        candidateNs = doIndexed();
    }

    std::size_t checked = 0;
    std::size_t avoided = 0;
    AuditPair(baseline, candidate, stats, checked, avoided);
    Require(checked == source.size() && checked > 0 &&
                avoided > source.size() * 16u &&
                std::isfinite(referenceNs) && referenceNs >= 0.0 &&
                std::isfinite(candidateNs) && candidateNs >= 0.0,
            "both measured paths return valid original results");
    return {
        referenceNs / static_cast<double>(source.size()),
        candidateNs / static_cast<double>(source.size()),
        checked, avoided
    };
}

void Profile(const latebrake::WorldScalePacePlan& plan,
             const latebrake::WorldScaleProjectionBlockIndex& index,
             unsigned fps)
{
    const auto source = BuildWorkload(fps);
    std::array<double, 5> reference{};
    std::array<double, 5> candidate{};
    std::size_t avoided = 0;
    for (std::size_t i = 0; i < reference.size(); ++i)
    {
        const auto trial = RunTrial(plan, index, source, i % 2u == 1u);
        reference[i] = trial.BaselineNs;
        candidate[i] = trial.IndexedNs;
        avoided = trial.AvoidedSegments;
    }
    std::sort(reference.begin(), reference.end());
    std::sort(candidate.begin(), candidate.end());
    const double medianReferenceNs = reference[2];
    const double medianCandidateNs = candidate[2];
    std::cout << fps << "Hz | samples=" << source.size()
              << " | full scan ns/query=" << std::fixed
              << std::setprecision(1) << medianReferenceNs
              << " | block search ns/query=" << medianCandidateNs
              << " | relative=" << std::setprecision(3)
              << (medianReferenceNs / std::max(1.0, medianCandidateNs))
              << "x | avoided segment evaluations=" << avoided
              << " per trial\n";
}

void Smoke(const latebrake::WorldScalePacePlan& plan,
           const latebrake::WorldScaleProjectionBlockIndex& index)
{
    const auto source = BuildWorkload(30u);
    std::vector<Projection> baseline, candidate;
    baseline.reserve(180u);
    candidate.reserve(180u);
    for (std::size_t i = 0; i < 180u; ++i)
    {
        baseline.push_back(latebrake::ProjectWorldScaleRoute(plan, source[i]));
        candidate.push_back(latebrake::ProjectWorldScaleRouteBlocks(
            plan, index, source[i]));
    }
    for (std::size_t i = 0; i < baseline.size(); ++i)
    {
        Require(Same(baseline[i], candidate[i]),
                "180 consecutive six-stream positions match exactly");
    }
}
} // namespace

int main(int argc, char** argv)
{
    const bool profile = argc == 2 &&
        std::string_view(argv[1]) == "--profile";
    if (argc != 1 && !profile)
    {
        std::cerr << "Usage: WorldScaleProjectionABProfile [--profile]\n";
        return EXIT_FAILURE;
    }
    const auto plan = latebrake::BuildWorldScalePacePlan();
    const auto index = latebrake::BuildWorldScaleProjectionBlockIndex(plan);
    Require(plan.Valid && index.Valid,
            "original pace and conservative block reference both valid");
    Smoke(plan, index);
    if (profile)
    {
        std::cout << "SOURCE-ONLY optimized portable A/B; "
                     "not Unreal Tick or Win64 driving performance\n";
        for (unsigned fps : {30u, 60u, 120u})
        {
            Profile(plan, index, fps);
        }
    }
    else
    {
        std::cout << "Portable A/B baseline/index exact-match smoke passed; "
                     "run --profile for five-trial 30/60/120Hz data\n";
    }
}
