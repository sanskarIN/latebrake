#include "Track/WorldScaleProjectionBlockIndex.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* why)
{
    if (!ok)
    {
        std::cerr << "WorldScaleProjectionBlockIndex FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool ExactlySame(const latebrake::WorldScaleRouteProjection& a,
                 const latebrake::WorldScaleRouteProjection& b)
{
    if (a.Valid != b.Valid) return false;
    if (!a.Valid) return true;
    return a.LapDistanceCm == b.LapDistanceCm &&
           a.SignedLateralCm == b.SignedLateralCm &&
           a.CentrelineSeparationCm == b.CentrelineSeparationCm &&
           a.InsidePhysicalCandidateRoad == b.InsidePhysicalCandidateRoad &&
           a.SuggestedSpeedCmps == b.SuggestedSpeedCmps;
}

void Compare(const latebrake::WorldScalePacePlan& plan,
             const latebrake::WorldScaleProjectionBlockIndex& index,
             latebrake::CircuitPoint at,
             std::size_t& cases,
             std::size_t& blocksAvoided,
             std::size_t& segmentsAvoided)
{
    using namespace latebrake;
    const auto reference = ProjectWorldScaleRoute(plan, at);
    WorldScaleProjectionBlockStats stats{};
    const auto optimized = ProjectWorldScaleRouteBlocks(plan, index, at, &stats);
    Require(ExactlySame(reference, optimized),
            "indexed answer must equal original full-scan answer exactly");
    Require(stats.TestedSegments <= WorldScalePaceStations &&
            stats.SkippedBlocks <= WorldScaleProjectionBlockCount,
            "search stats are bounded by the original 512 segments");
    if (reference.Valid)
    {
        Require(stats.TestedSegments >= WorldScaleProjectionBlockWidth &&
                stats.TestedSegments %
                    WorldScaleProjectionBlockWidth == 0,
                "tested segments represent whole source blocks");
    }
    ++cases;
    blocksAvoided += stats.SkippedBlocks;
    segmentsAvoided +=
        WorldScalePaceStations - stats.TestedSegments;
}

void InvalidSourceAndCoordinates(
    const latebrake::WorldScalePacePlan& plan,
    const latebrake::WorldScaleProjectionBlockIndex& index)
{
    using namespace latebrake;
    WorldScalePacePlan absent{};
    Require(!BuildWorldScaleProjectionBlockIndex(absent).Valid,
            "empty source plan never produces a valid index");
    const auto sameContentDifferentObject = plan;
    Require(!ProjectWorldScaleRouteBlocks(
                sameContentDifferentObject, index,
                plan.Nodes[0].Position).Valid,
            "index cannot be reused with a different pace-plan object");
    WorldScaleProjectionBlockIndex emptyIndex{};
    Require(!ProjectWorldScaleRouteBlocks(
                plan, emptyIndex, plan.Nodes[0].Position).Valid,
            "uninitialized index cannot produce a position");
    WorldScalePacePlan broken = plan;
    broken.Nodes[100].Position = broken.Nodes[101].Position;
    Require(!BuildWorldScaleProjectionBlockIndex(broken).Valid,
            "a zero-length source segment must invalidate the whole index");
    broken = plan;
    broken.Nodes[100].Position.X =
        std::numeric_limits<double>::quiet_NaN();
    Require(!BuildWorldScaleProjectionBlockIndex(broken).Valid,
            "NaN source geometry must invalidate the whole index");
    for (double bad : {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        2000000.0})
    {
        Require(!ProjectWorldScaleRouteBlocks(
                    plan, index, {bad, 0.0}).Valid &&
                !ProjectWorldScaleRouteBlocks(
                    plan, index, {0.0, bad}).Valid,
                "invalid world XY source fails closed");
    }
}

void RoadAndSeamEquivalence(
    const latebrake::WorldScalePacePlan& plan,
    const latebrake::WorldScaleProjectionBlockIndex& index,
    std::size_t& cases, std::size_t& blocksAvoided,
    std::size_t& segmentsAvoided)
{
    using namespace latebrake;
    // Include true start/end seam, straight/bend intermediate positions,
    // physical 9-m road shoulders and up-to-9.6-m off-road offsets.
    for (unsigned station = 0; station < WorldScalePaceStations; ++station)
    {
        for (double fraction : {0.0, 0.125, 0.5, 0.875})
        {
            const double s = (station + fraction) * WorldScalePaceStepCm;
            const auto pose = WorldScaleCenterlineAt(s);
            Require(pose.Valid, "reference source centerline exists");
            const double rad = pose.Pose.HeadingDegrees *
                WorldScalePaceRadPerDegree;
            for (double lateral : {
                -480.0, -350.0, -220.0, 0.0, 220.0, 350.0, 480.0})
            {
                const CircuitPoint point{
                    pose.Pose.Position.X - std::sin(rad) * lateral,
                    pose.Pose.Position.Y + std::cos(rad) * lateral};
                Compare(plan, index, point, cases, blocksAvoided,
                        segmentsAvoided);
            }
        }
    }
    // Crossing outside a selected 1500 cm projection radius must also
    // match the reference's failure behavior, not spuriously succeed.
    for (unsigned i = 0; i < WorldScalePaceStations; i += 13)
    {
        const auto pose = WorldScaleCenterlineAt(
            (i + 0.75) * WorldScalePaceStepCm);
        const double rad = pose.Pose.HeadingDegrees *
            WorldScalePaceRadPerDegree;
        for (double offset : {-2400.0, 2400.0})
        {
            const CircuitPoint point{
                pose.Pose.Position.X - std::sin(rad) * offset,
                pose.Pose.Position.Y + std::cos(rad) * offset};
            Compare(plan, index, point, cases, blocksAvoided, segmentsAvoided);
        }
    }
}

void SixSourceCoordinateStreams(
    const latebrake::WorldScalePacePlan& plan,
    const latebrake::WorldScaleProjectionBlockIndex& index,
    std::size_t& cases, std::size_t& blocksAvoided,
    std::size_t& segmentsAvoided)
{
    using namespace latebrake;
    for (unsigned fps : {30u, 60u, 120u})
    {
        for (unsigned frame = 0; frame < fps; ++frame)
        {
            for (unsigned slot = 0; slot < 6; ++slot)
            {
                const double station = (
                    static_cast<double>((frame * 17u + slot * 89u) %
                                       WorldScalePaceStations) + 0.31) *
                    WorldScalePaceStepCm;
                const auto pose = WorldScaleCenterlineAt(station);
                Require(pose.Valid, "synthetic source geometry exists");
                Compare(plan, index, pose.Pose.Position,
                        cases, blocksAvoided, segmentsAvoided);
            }
        }
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    static_assert(WorldScaleProjectionBlockWidth == 16);
    static_assert(WorldScaleProjectionBlockCount == 32);
    const auto plan = BuildWorldScalePacePlan();
    Require(plan.Valid, "original world-scale source pace plan exists");
    const auto index = BuildWorldScaleProjectionBlockIndex(plan);
    Require(index.Valid && index.Source == &plan,
            "immutable source reference is indexed correctly");
    InvalidSourceAndCoordinates(plan, index);
    std::size_t cases = 0;
    std::size_t blocksAvoided = 0;
    std::size_t segmentsAvoided = 0;
    RoadAndSeamEquivalence(plan, index, cases,
                           blocksAvoided, segmentsAvoided);
    SixSourceCoordinateStreams(plan, index, cases,
                               blocksAvoided, segmentsAvoided);
    Require(cases > 15000 && blocksAvoided > 0 &&
                segmentsAvoided > cases * 16,
            "the index must demonstrate genuine source-only broad-phase work avoidance");
    std::cout << "Exact 512-segment source equivalence: " << cases
              << " road/seam/lane/6-source observations; "
              << segmentsAvoided << " candidate segment evaluations "
              << "avoided; no Unreal Tick or playable game claimed\n";
    return EXIT_SUCCESS;
}
