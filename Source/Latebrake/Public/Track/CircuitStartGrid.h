#pragma once

#include "Track/CircuitSpawnSafety.h"

#include <array>
#include <cmath>

namespace latebrake
{
// Proposed six-place original-circuit start layout: one physical human and
// five *potential* rival slots. The current GameMode still spawns ONE rival.
// These coordinates are a collision preflight, NOT proof of a cooked map or
// a full-grid racing AI. All distances are original Unreal centimetres.
inline constexpr int StartGridCapacity = 6;
inline constexpr int HumanGridSlot = 0;
inline constexpr int PrimaryRivalGridSlot = 1;

inline constexpr std::array<CircuitPose, StartGridCapacity> StartGridPoses{{
    PlayerStart,
    RivalStart,
    {{-2890.0, -2200.0}, 0.0},
    {{-2890.0, -1350.0}, 0.0},
    {{-3400.0, -2200.0}, 0.0},
    {{-3400.0, -1350.0}, 0.0}
}};

// Conservative plan for the stock 380 x 180 cm chassis: plus 45 cm on
// each side. A padded body may not overlap another padded body or a real
// barrier; all four padded corners must still lie on actual paved slabs.
inline constexpr double GridBodyHalfLengthCm = 190.0;
inline constexpr double GridBodyHalfWidthCm = 90.0;
inline constexpr double GridSafetyMarginCm = 45.0;
inline constexpr double GridPaddedHalfLengthCm =
    GridBodyHalfLengthCm + GridSafetyMarginCm;
inline constexpr double GridPaddedHalfWidthCm =
    GridBodyHalfWidthCm + GridSafetyMarginCm;

enum class StartGridFailure
{
    None,
    InvalidPose,
    OffPavement,
    BarrierOverlap,
    CarOverlap
};

struct StartGridValidation final
{
    StartGridFailure Failure = StartGridFailure::None;
    int Slot = -1;
    int OtherSlot = -1;
    bool Safe() const noexcept { return Failure == StartGridFailure::None; }
};

inline bool TryGetStartGridPose(int slot, CircuitPose& out) noexcept
{
    out = {};
    if (slot < 0 || slot >= StartGridCapacity)
    {
        return false; // No silent fallback to occupied player or rival grid.
    }
    out = StartGridPoses[static_cast<std::size_t>(slot)];
    return true;
}

struct StartGridRect final
{
    double X;
    double Y;
    double ForwardX;
    double ForwardY;
    double RightX;
    double RightY;
    double HalfLength;
    double HalfWidth;
};

inline bool IsValidStartGridPose(CircuitPose pose) noexcept
{
    return std::isfinite(pose.Position.X) &&
        std::isfinite(pose.Position.Y) &&
        std::isfinite(pose.HeadingDegrees) &&
        std::abs(pose.Position.X) <= 100000.0 &&
        std::abs(pose.Position.Y) <= 100000.0 &&
        std::abs(pose.HeadingDegrees) <= 36000.0;
}

inline StartGridRect MakeStartGridRect(CircuitPose pose) noexcept
{
    constexpr double RadiansPerDegree = 3.14159265358979323846 / 180.0;
    const double angle = std::remainder(pose.HeadingDegrees, 360.0) *
        RadiansPerDegree;
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    return {pose.Position.X, pose.Position.Y, c, s, -s, c,
        GridPaddedHalfLengthCm, GridPaddedHalfWidthCm};
}

inline double StartGridProjectionRadius(
    const StartGridRect& r, double axisX, double axisY) noexcept
{
    return r.HalfLength * std::abs(r.ForwardX * axisX + r.ForwardY * axisY) +
        r.HalfWidth * std::abs(r.RightX * axisX + r.RightY * axisY);
}

inline bool StartGridFootprintsOverlap(
    CircuitPose a, CircuitPose b) noexcept
{
    if (!IsValidStartGridPose(a) || !IsValidStartGridPose(b))
    {
        return true; // Invalid geometry never authorizes a free grid.
    }
    const StartGridRect ar = MakeStartGridRect(a);
    const StartGridRect br = MakeStartGridRect(b);
    const double dx = br.X - ar.X;
    const double dy = br.Y - ar.Y;
    const std::array<CircuitPoint, 4> axes{{
        {ar.ForwardX, ar.ForwardY}, {ar.RightX, ar.RightY},
        {br.ForwardX, br.ForwardY}, {br.RightX, br.RightY}
    }};
    for (const CircuitPoint axis : axes)
    {
        const double distance = std::abs(dx * axis.X + dy * axis.Y);
        const double radius =
            StartGridProjectionRadius(ar, axis.X, axis.Y) +
            StartGridProjectionRadius(br, axis.X, axis.Y);
        if (distance > radius)
        {
            return false;
        }
    }
    return true; // Touching safety margins are conservatively rejected.
}

inline bool StartGridFootprintTouchesBarrier(CircuitPose pose) noexcept
{
    if (!IsValidStartGridPose(pose))
    {
        return true;
    }
    const StartGridRect r = MakeStartGridRect(pose);
    for (int i = 0; i < CircuitCollisionBlockCount; ++i)
    {
        const CircuitCollisionBlock wall = PlannedCollisionBlock(i);
        if (wall.Style != CollisionBlockStyle::Barrier ||
            !IsValidCollisionBlock(wall))
        {
            continue;
        }
        const double dx = r.X - wall.Center.X;
        const double dy = r.Y - wall.Center.Y;
        const double hx = wall.ScaleX * 50.0;
        const double hy = wall.ScaleY * 50.0;
        // Exact separating-axis test for one rotated padded chassis and
        // the existing unrotated 100-cm-base barrier slab in world XY.
        if (std::abs(dx) > hx + StartGridProjectionRadius(r, 1.0, 0.0) ||
            std::abs(dy) > hy + StartGridProjectionRadius(r, 0.0, 1.0))
        {
            continue;
        }
        const double along = std::abs(dx * r.ForwardX + dy * r.ForwardY);
        const double wallAlong =
            hx * std::abs(r.ForwardX) + hy * std::abs(r.ForwardY);
        if (along > r.HalfLength + wallAlong)
        {
            continue;
        }
        const double across = std::abs(dx * r.RightX + dy * r.RightY);
        const double wallAcross =
            hx * std::abs(r.RightX) + hy * std::abs(r.RightY);
        if (across > r.HalfWidth + wallAcross)
        {
            continue;
        }
        return true;
    }
    return false;
}

inline bool StartGridFootprintOnPavement(CircuitPose pose) noexcept
{
    if (!IsValidStartGridPose(pose))
    {
        return false;
    }
    const StartGridRect r = MakeStartGridRect(pose);
    // Sampling four rectangle corners is conservative for the straight
    // grid patches, but is not a general polygon-containment proof for
    // arbitrary future curved / disjoint surfaces.
    for (const double lengthSign : {-1.0, 1.0})
    {
        for (const double widthSign : {-1.0, 1.0})
        {
            const CircuitPoint corner{
                r.X + lengthSign * r.HalfLength * r.ForwardX +
                    widthSign * r.HalfWidth * r.RightX,
                r.Y + lengthSign * r.HalfLength * r.ForwardY +
                    widthSign * r.HalfWidth * r.RightY
            };
            if (!IsOnPavedSurface(corner))
            {
                return false;
            }
        }
    }
    return IsOnPavedSurface(pose.Position);
}

inline StartGridValidation ValidateStartGrid(
    const std::array<CircuitPose, StartGridCapacity>& poses) noexcept
{
    for (int i = 0; i < StartGridCapacity; ++i)
    {
        const CircuitPose p = poses[static_cast<std::size_t>(i)];
        if (!IsValidStartGridPose(p) ||
            EvaluateCircuitSpawnPose(p) == CircuitSpawnDecision::InvalidLocation)
        {
            return {StartGridFailure::InvalidPose, i, -1};
        }
        if (!StartGridFootprintOnPavement(p))
        {
            return {StartGridFailure::OffPavement, i, -1};
        }
        if (StartGridFootprintTouchesBarrier(p) ||
            EvaluateCircuitSpawnPose(p) == CircuitSpawnDecision::BarrierOverlap)
        {
            return {StartGridFailure::BarrierOverlap, i, -1};
        }
        for (int j = 0; j < i; ++j)
        {
            if (StartGridFootprintsOverlap(
                p, poses[static_cast<std::size_t>(j)]))
            {
                return {StartGridFailure::CarOverlap, i, j};
            }
        }
    }
    return {};
}

inline bool IsPlannedStartGridSafe() noexcept
{
    return ValidateStartGrid(StartGridPoses).Safe();
}
} // namespace latebrake
