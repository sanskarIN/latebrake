#pragma once
#include "Racecraft/RacecraftFrames.h"
#include "Track/CircuitLayout.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
inline constexpr double OriginalRoadHalfWidthCm =
    latebrake::RoadHalfWidth;
inline constexpr double SouthCenterY = latebrake::SouthY;
inline constexpr double NorthCenterY = latebrake::NorthY;
inline constexpr double EastCenterX = latebrake::EastX;
inline constexpr double WestCenterX = latebrake::WestX;
inline constexpr double OriginalStraightLengthCm =
    EastCenterX - WestCenterX;
inline constexpr double OriginalSideLengthCm =
    NorthCenterY - SouthCenterY;
inline constexpr double OriginalCircuitLapCm =
    2.0 * (OriginalStraightLengthCm + OriginalSideLengthCm);
inline constexpr int OriginalSegmentCount = 4;
inline constexpr int OriginalAsphaltSlabCount = 4;

struct CircuitSegment final
{
    Segment2 Centerline{};
    double StartDistanceCm = 0.0;
    double LengthCm = 0.0;
    double HeadingRadians = 0.0;
    double RoadHalfWidthCm = OriginalRoadHalfWidthCm;
    int Index = -1;
};
struct CircuitPointProjection final
{
    Vec2 NearestCenterline{};
    double DistanceAlongLapCm = 0.0;
    double CrossTrackCm = 0.0;
    double CenterlineDistanceCm = 0.0;
    double HeadingRadians = 0.0;
    double TangentDot = 0.0;
    double RoadClearanceCm = 0.0;
    double CurvatureAheadPerCm = 0.0;
    int Segment = -1;
    bool PavedCenter = false;
    bool WithinTrackWidth = false;
    bool Valid = false;
};
struct AsphaltSlab final
{
    Aabb2 Bounds{};
    int Index = -1;
    bool Drivable = true;
};
struct CircuitBodyState final
{
    Rect2 Footprint{};
    double RoadAreaFraction = 0.0;
    double MinimumCornerClearanceCm = 0.0;
    int ContainingSlabs = 0;
    int TouchingSlabs = 0;
    int ClosestSegment = -1;
    bool CenterPaved = false;
    bool BodyFullyOnAsphalt = false;
    bool BarrierEnvelopeQualified = false;
    bool SourceGeometryOnly = true;
};
inline constexpr std::array<CircuitSegment, 4> OriginalSegments = {{
    {{{WestCenterX, SouthCenterY}, {EastCenterX, SouthCenterY}},
     0.0, OriginalStraightLengthCm, 0.0,
     OriginalRoadHalfWidthCm, 0},
    {{{EastCenterX, SouthCenterY}, {EastCenterX, NorthCenterY}},
     OriginalStraightLengthCm, OriginalSideLengthCm,
     Pi * 0.5, OriginalRoadHalfWidthCm, 1},
    {{{EastCenterX, NorthCenterY}, {WestCenterX, NorthCenterY}},
     OriginalStraightLengthCm + OriginalSideLengthCm,
     OriginalStraightLengthCm, Pi,
     OriginalRoadHalfWidthCm, 2},
    {{{WestCenterX, NorthCenterY}, {WestCenterX, SouthCenterY}},
     2.0 * OriginalStraightLengthCm + OriginalSideLengthCm,
     OriginalSideLengthCm, -Pi * 0.5,
     OriginalRoadHalfWidthCm, 3}
}};
inline constexpr std::array<AsphaltSlab, 4> OriginalAsphalt = {{
    {{{-4000.0, SouthCenterY - OriginalRoadHalfWidthCm},
      {4000.0, SouthCenterY + OriginalRoadHalfWidthCm}}, 0, true},
    {{{EastCenterX - OriginalRoadHalfWidthCm, -2600.0},
      {EastCenterX + OriginalRoadHalfWidthCm, 2600.0}}, 1, true},
    {{{-4000.0, NorthCenterY - OriginalRoadHalfWidthCm},
      {4000.0, NorthCenterY + OriginalRoadHalfWidthCm}}, 2, true},
    {{{WestCenterX - OriginalRoadHalfWidthCm, -2600.0},
      {WestCenterX + OriginalRoadHalfWidthCm, 2600.0}}, 3, true}
}};

inline bool ValidCircuitSegment(CircuitSegment s) noexcept
{
    return Finite(s.Centerline.A) && Finite(s.Centerline.B) &&
        Finite(s.StartDistanceCm) && Finite(s.LengthCm) &&
        s.LengthCm > PhysicsEpsilon &&
        Finite(s.HeadingRadians) &&
        Finite(s.RoadHalfWidthCm) && s.RoadHalfWidthCm > 0.0 &&
        s.Index >= 0 && s.Index < OriginalSegmentCount;
}
inline bool CenterOnOriginalRoad(Vec2 point) noexcept
{
    return Finite(point) &&
        latebrake::IsOnPavedSurface({point.X, point.Y});
}
inline bool SlabContainsPoint(AsphaltSlab slab,
                              Vec2 point) noexcept
{
    return AabbContains(slab.Bounds, point);
}
inline bool SlabContainsBody(AsphaltSlab slab,
                             Rect2 footprint) noexcept
{
    return AabbContains(slab.Bounds, footprint);
}
inline bool AnyOriginalSlabContains(Rect2 footprint) noexcept
{
    for (AsphaltSlab slab : OriginalAsphalt)
    {
        if (SlabContainsBody(slab, footprint)) return true;
    }
    return false;
}
inline CircuitSegment SegmentAt(int index) noexcept
{
    return index >= 0 && index < OriginalSegmentCount ?
        OriginalSegments[static_cast<std::size_t>(index)] :
        CircuitSegment{};
}
inline CircuitSegment SegmentForDistance(double distanceCm) noexcept
{
    if (!Finite(distanceCm)) return {};
    const double wrapped = WrapDistance(
        distanceCm, OriginalCircuitLapCm);
    for (const CircuitSegment& segment : OriginalSegments)
    {
        if (wrapped >= segment.StartDistanceCm &&
            wrapped < segment.StartDistanceCm +
                segment.LengthCm)
            return segment;
    }
    return OriginalSegments.back();
}
inline Vec2 PositionAtDistance(double distanceCm) noexcept
{
    if (!Finite(distanceCm)) return {};
    const double wrapped = WrapDistance(
        distanceCm, OriginalCircuitLapCm);
    const CircuitSegment segment = SegmentForDistance(wrapped);
    const double fraction = Clamp01(
        SafeDivision(wrapped - segment.StartDistanceCm,
                     segment.LengthCm));
    return Lerp(segment.Centerline.A,
                segment.Centerline.B, fraction);
}
inline Pose2 PoseAtDistance(double distanceCm) noexcept
{
    const CircuitSegment s = SegmentForDistance(distanceCm);
    return {PositionAtDistance(distanceCm), s.HeadingRadians};
}
inline Vec2 PositionWithOffset(double distanceCm,
                                double lateralCm) noexcept
{
    const Pose2 pose = PoseAtDistance(distanceCm);
    return pose.Position +
        PerpendicularLeft(HeadingVector(pose.HeadingRadians)) *
        lateralCm;
}
inline double SegmentFinishDistance(int index) noexcept
{
    const CircuitSegment s = SegmentAt(index);
    return s.StartDistanceCm + s.LengthCm;
}
inline CircuitPointProjection ProjectOriginalCircuit(
    Vec2 world, double headingRadians = 0.0,
    bool preferForward = true) noexcept
{
    CircuitPointProjection best;
    if (!Finite(world) || !Finite(headingRadians)) return best;
    double minimumError = std::numeric_limits<double>::infinity();
    for (const CircuitSegment& segment : OriginalSegments)
    {
        const SegmentProjection projection =
            ProjectToSegment(world, segment.Centerline);
        if (!projection.Valid) continue;
        const double tangentAlignment =
            Dot(HeadingVector(headingRadians),
                HeadingVector(segment.HeadingRadians));
        const double headingPenalty =
            preferForward && tangentAlignment < 0.0 ?
                Square(125.0 * tangentAlignment) : 0.0;
        const double error =
            projection.DistanceSquared + headingPenalty;
        if (error >= minimumError) continue;
        minimumError = error;
        best.NearestCenterline = projection.Point;
        best.DistanceAlongLapCm =
            segment.StartDistanceCm +
            projection.AlongFraction * segment.LengthCm;
        best.CrossTrackCm = projection.SignedSideCm;
        best.CenterlineDistanceCm =
            std::sqrt(projection.DistanceSquared);
        best.HeadingRadians = segment.HeadingRadians;
        best.TangentDot = tangentAlignment;
        best.RoadClearanceCm = segment.RoadHalfWidthCm -
            std::abs(projection.SignedSideCm);
        best.CurvatureAheadPerCm = 0.0;
        best.Segment = segment.Index;
        best.WithinTrackWidth =
            std::abs(best.CrossTrackCm) <=
            segment.RoadHalfWidthCm;
        best.PavedCenter = CenterOnOriginalRoad(world);
        best.Valid = true;
    }
    return best;
}
inline double ForwardRemainingToGate(
    double lapDistanceCm, int gateIndex) noexcept
{
    if (!Finite(lapDistanceCm) ||
        gateIndex < 0 || gateIndex >= OriginalSegmentCount)
        return 0.0;
    const double gateCm = SegmentFinishDistance(gateIndex);
    return WrapDistance(gateCm -
        WrapDistance(lapDistanceCm, OriginalCircuitLapCm),
        OriginalCircuitLapCm);
}
inline int NextGateFromDistance(double progressCm) noexcept
{
    return SegmentForDistance(progressCm).Index;
}
inline double ProgressBetween(Vec2 from, Vec2 to,
                              double headingRadians) noexcept
{
    const auto a = ProjectOriginalCircuit(from, headingRadians);
    const auto b = ProjectOriginalCircuit(to, headingRadians);
    if (!a.Valid || !b.Valid) return 0.0;
    return SignedShortestLapDistance(
        a.DistanceAlongLapCm, b.DistanceAlongLapCm,
        OriginalCircuitLapCm);
}
inline double LineCenterDistance(Vec2 world) noexcept
{
    const auto p = ProjectOriginalCircuit(world);
    return p.Valid ? p.CenterlineDistanceCm :
        std::numeric_limits<double>::infinity();
}
inline bool RoadCornerSamplePasses(
    Vec2 corner, double toleranceCm = 0.0) noexcept
{
    if (!Finite(corner) || !Finite(toleranceCm) ||
        toleranceCm < 0.0) return false;
    for (AsphaltSlab slab : OriginalAsphalt)
    {
        Aabb2 box = slab.Bounds;
        box.Minimum.X -= toleranceCm;
        box.Minimum.Y -= toleranceCm;
        box.Maximum.X += toleranceCm;
        box.Maximum.Y += toleranceCm;
        if (AabbContains(box, corner)) return true;
    }
    return false;
}
inline bool EveryBodyCornerPaved(Rect2 footprint) noexcept
{
    if (!ValidRect(footprint)) return false;
    const auto corners = RectCorners(footprint);
    for (Vec2 corner : corners)
    {
        if (!RoadCornerSamplePasses(corner)) return false;
    }
    return true;
}
inline int SlabsCoveringBody(Rect2 footprint) noexcept
{
    if (!ValidRect(footprint)) return 0;
    int count = 0;
    for (AsphaltSlab slab : OriginalAsphalt)
    {
        count += SlabContainsBody(slab, footprint) ? 1 : 0;
    }
    return count;
}
inline int SlabsTouchedByBody(Rect2 footprint) noexcept
{
    if (!ValidRect(footprint)) return 0;
    const auto points = RectCorners(footprint);
    int count = 0;
    for (AsphaltSlab slab : OriginalAsphalt)
    {
        bool touched = AabbContains(
            slab.Bounds, footprint.Center);
        for (Vec2 point : points)
        {
            touched = touched ||
                AabbContains(slab.Bounds, point);
        }
        count += touched ? 1 : 0;
    }
    return count;
}
inline double CornerPavementFraction(Rect2 footprint) noexcept
{
    if (!ValidRect(footprint)) return 0.0;
    const auto corners = RectCorners(footprint);
    int onRoad = 0;
    for (Vec2 corner : corners)
    {
        onRoad += RoadCornerSamplePasses(corner) ? 1 : 0;
    }
    return onRoad / 4.0;
}
inline CircuitBodyState InspectCircuitBody(
    Rect2 footprint, double minimumRoadMarginCm = 0.0) noexcept
{
    CircuitBodyState out;
    out.Footprint = footprint;
    if (!ValidRect(footprint) ||
        !Finite(minimumRoadMarginCm) ||
        minimumRoadMarginCm < 0.0) return out;
    out.CenterPaved = CenterOnOriginalRoad(footprint.Center);
    out.ContainingSlabs = SlabsCoveringBody(footprint);
    out.TouchingSlabs = SlabsTouchedByBody(footprint);
    out.RoadAreaFraction = CornerPavementFraction(footprint);
    const auto projection =
        ProjectOriginalCircuit(footprint.Center,
                               footprint.RotationRadians);
    out.ClosestSegment = projection.Segment;
    out.MinimumCornerClearanceCm =
        projection.RoadClearanceCm -
        ContactRadiusAlongAxis(
            footprint,
            PerpendicularLeft(HeadingVector(
                projection.HeadingRadians)));
    out.BodyFullyOnAsphalt =
        out.ContainingSlabs > 0 &&
        out.MinimumCornerClearanceCm >=
            minimumRoadMarginCm;
    out.BarrierEnvelopeQualified = false;
    out.SourceGeometryOnly = true;
    return out;
}
inline bool PaddedBodyStaysWithinOneSlab(
    Pose2 pose, Vec2 halfSize,
    double paddingCm) noexcept
{
    if (!Finite(pose) || !Finite(halfSize) ||
        !Finite(paddingCm) || paddingCm < 0.0)
        return false;
    return AnyOriginalSlabContains(
        {pose.Position,
         {halfSize.X + paddingCm, halfSize.Y + paddingCm},
         pose.HeadingRadians});
}
inline double DistanceToNearestSlabEdge(Vec2 world) noexcept
{
    if (!Finite(world)) return 0.0;
    double best = -std::numeric_limits<double>::infinity();
    for (AsphaltSlab slab : OriginalAsphalt)
    {
        if (!AabbContains(slab.Bounds, world)) continue;
        const double margin = std::min({
            world.X - slab.Bounds.Minimum.X,
            slab.Bounds.Maximum.X - world.X,
            world.Y - slab.Bounds.Minimum.Y,
            slab.Bounds.Maximum.Y - world.Y
        });
        best = std::max(best, margin);
    }
    return Finite(best) ? best : -1.0;
}
inline double LookaheadDistance(double speedCmS,
                                double minimumCm = 350.0,
                                double seconds = 0.75) noexcept
{
    return Finite(speedCmS) && Finite(minimumCm) &&
        Finite(seconds) && speedCmS >= 0.0 &&
        minimumCm > 0.0 && seconds >= 0.0 ?
        std::clamp(speedCmS * seconds, minimumCm, 2400.0) :
        minimumCm;
}
inline Pose2 LookaheadPose(
    Vec2 world, double yawRadians,
    double speedCmS) noexcept
{
    const CircuitPointProjection snap =
        ProjectOriginalCircuit(world, yawRadians);
    if (!snap.Valid) return {};
    return PoseAtDistance(snap.DistanceAlongLapCm +
        LookaheadDistance(speedCmS));
}
inline double LapDistanceToWaypoint(
    double ownDistanceCm, double targetDistanceCm) noexcept
{
    return WrapDistance(targetDistanceCm - ownDistanceCm,
                        OriginalCircuitLapCm);
}
inline bool CircuitGeometryValid() noexcept
{
    double totalLength = 0.0;
    for (CircuitSegment segment : OriginalSegments)
    {
        if (!ValidCircuitSegment(segment)) return false;
        if (std::abs(segment.StartDistanceCm -
                     totalLength) > 1.0e-6) return false;
        totalLength += segment.LengthCm;
    }
    return std::abs(totalLength -
        OriginalCircuitLapCm) < 1.0e-6 &&
        OriginalCircuitLapCm > 10000.0;
}
} // namespace latebrake::racecraft
