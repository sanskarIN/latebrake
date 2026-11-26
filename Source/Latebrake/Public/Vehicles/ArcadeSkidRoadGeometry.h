#pragma once

#include "Vehicles/SkidTrailGeometry.h"
#include "Track/CircuitCollisionPlan.h"
#include "Vehicles/ChassisGroundContactTraction.h"

#include <cmath>

namespace latebrake
{
// The live arcade chassis has four downward WorldStatic probes, NOT
// individually sprung tires. These helpers admit only actual original
// flat asphalt-block hits and never infer a wheel load or brake slip.
inline constexpr double ArcadeSkidRoadHeightToleranceCm=0.25;
inline constexpr double ArcadeSkidMinimumNormalZ=0.999;
inline constexpr double ArcadeSkidEdgeInsetCm=0.25;

inline bool IsArcadeSkidAsphaltBlock(int blockIndex) noexcept
{
    if (blockIndex<0 || blockIndex>=CircuitCollisionBlockCount) return false;
    const auto block=PlannedCollisionBlock(blockIndex);
    return block.Style==CollisionBlockStyle::Asphalt && IsValidCollisionBlock(block);
}

inline bool IsArcadeSkidRoadPoint(int blockIndex,SkidVector point,
    SkidVector normal) noexcept
{
    if (!IsArcadeSkidAsphaltBlock(blockIndex) ||
        !IsFiniteSkidVector(point,100000.0) ||
        !IsSkidUnitNormal(normal) || normal.Z<ArcadeSkidMinimumNormalZ)
        return false;
    const auto block=PlannedCollisionBlock(blockIndex);
    const double top=block.Height+block.ScaleZ*50.0;
    return std::abs(point.Z-top)<=ArcadeSkidRoadHeightToleranceCm &&
        std::abs(point.X-block.Center.X)<=block.ScaleX*50.0-ArcadeSkidEdgeInsetCm &&
        std::abs(point.Y-block.Center.Y)<=block.ScaleY*50.0-ArcadeSkidEdgeInsetCm;
}

// A complete oriented decorative strip, not just its centre line, has
// to fit inside the exact one asphalt slab that supplied both hits.
// Edge clearance uses projected half extents of the 3D cuboid. A road
// corner shared by two different slabs starts new trail continuity.
inline bool IsArcadeSkidStripInsideRoad(int blockIndex,const SkidStrip& strip) noexcept
{
    if (!strip.Valid || !IsArcadeSkidAsphaltBlock(blockIndex) ||
        !IsFiniteSkidVector(strip.Center,100000.0) ||
        !IsSkidUnitNormal(strip.Up) || strip.Up.Z<ArcadeSkidMinimumNormalZ ||
        !IsFiniteSkidVector(strip.Forward,1.001) ||
        !IsFiniteSkidVector(strip.Right,1.001) ||
        !std::isfinite(strip.LengthCm) || strip.LengthCm<=0.0 ||
        strip.LengthCm>180.501 || !std::isfinite(strip.WidthCm) ||
        strip.WidthCm<=0.0 || strip.WidthCm>18.001 ||
        !std::isfinite(strip.ThicknessCm) || strip.ThicknessCm!=0.08)
        return false;

    const auto block=PlannedCollisionBlock(blockIndex);
    const double extentX=0.5*(std::abs(strip.Forward.X)*strip.LengthCm+
        std::abs(strip.Right.X)*strip.WidthCm+std::abs(strip.Up.X)*strip.ThicknessCm);
    const double extentY=0.5*(std::abs(strip.Forward.Y)*strip.LengthCm+
        std::abs(strip.Right.Y)*strip.WidthCm+std::abs(strip.Up.Y)*strip.ThicknessCm);
    const double extentZ=0.5*(std::abs(strip.Forward.Z)*strip.LengthCm+
        std::abs(strip.Right.Z)*strip.WidthCm+std::abs(strip.Up.Z)*strip.ThicknessCm);
    const double top=block.Height+block.ScaleZ*50.0;
    return std::abs(strip.Center.X-block.Center.X)+extentX<=
            block.ScaleX*50.0-ArcadeSkidEdgeInsetCm &&
        std::abs(strip.Center.Y-block.Center.Y)+extentY<=
            block.ScaleY*50.0-ArcadeSkidEdgeInsetCm &&
        strip.Center.Z-extentZ>=top-0.01 &&
        strip.Center.Z+extentZ<=top+0.75;
}

// Final renderer defense independent of the source evidence type.
// Do not send nonfinite or non-orthonormal transforms to an instance
// component even if a caller accidentally marks a strip as valid.
inline bool IsRenderableSkidStrip(const SkidStrip& strip) noexcept
{
    if (!strip.Valid || !IsFiniteSkidVector(strip.Center,1.0e8) ||
        !IsFiniteSkidVector(strip.Forward,1.001) ||
        !IsFiniteSkidVector(strip.Right,1.001) || !IsSkidUnitNormal(strip.Up) ||
        !std::isfinite(strip.LengthCm) || strip.LengthCm<1.0 ||
        strip.LengthCm>180.501 || !std::isfinite(strip.WidthCm) ||
        strip.WidthCm<14.399 || strip.WidthCm>18.001 ||
        !std::isfinite(strip.ThicknessCm) || strip.ThicknessCm!=0.08 ||
        !std::isfinite(strip.Intensity) || strip.Intensity<0.0 || strip.Intensity>1.0)
        return false;
    constexpr double tolerance=0.002;
    return std::abs(SkidLength(strip.Forward)-1.0)<=tolerance &&
        std::abs(SkidLength(strip.Right)-1.0)<=tolerance &&
        std::abs(SkidLength(strip.Up)-1.0)<=tolerance &&
        std::abs(SkidDot(strip.Forward,strip.Right))<=tolerance &&
        std::abs(SkidDot(strip.Forward,strip.Up))<=tolerance &&
        std::abs(SkidDot(strip.Right,strip.Up))<=tolerance &&
        SkidDot(SkidCross(strip.Forward,strip.Right),strip.Up)>=1.0-tolerance;
}
} // namespace latebrake
