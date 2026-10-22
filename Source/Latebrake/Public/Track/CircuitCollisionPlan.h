#pragma once

#include "Track/CircuitLayout.h"

#include <array>
#include <cmath>

namespace latebrake
{
// Original collision-bearing circuit construction plan in Unreal centimetres.
// The nine real collision pieces (ground, four asphalt slabs, four barriers)
// are authored once here and consumed by Unreal, tests and startup audit.
// Decorative instanced meshes remain in DecorationBatchPlan.h.
enum class CollisionBlockStyle
{
    Ground,
    Asphalt,
    Barrier
};

struct CircuitCollisionBlock final
{
    CircuitPoint Center{};
    double Height = 0.0;
    double ScaleX = 0.0;
    double ScaleY = 0.0;
    double ScaleZ = 0.0;
    CollisionBlockStyle Style = CollisionBlockStyle::Ground;
};

inline constexpr std::array<CircuitCollisionBlock, 9> CircuitCollisionBlocks{{
    {{0.0, 0.0}, -100.0, 110.0, 85.0, 1.25, CollisionBlockStyle::Ground},
    {{0.0, SouthY}, -22.0, 80.0, 16.0, 0.42, CollisionBlockStyle::Asphalt},
    {{0.0, NorthY}, -22.0, 80.0, 16.0, 0.42, CollisionBlockStyle::Asphalt},
    {{EastX, 0.0}, -22.0, 16.0, 52.0, 0.42, CollisionBlockStyle::Asphalt},
    {{WestX, 0.0}, -22.0, 16.0, 52.0, 0.42, CollisionBlockStyle::Asphalt},
    {{0.0, 2750.0}, 65.0, 85.0, 1.0, 2.0, CollisionBlockStyle::Barrier},
    {{0.0, -2750.0}, 65.0, 85.0, 1.0, 2.0, CollisionBlockStyle::Barrier},
    {{3750.0, 0.0}, 65.0, 1.0, 60.0, 2.0, CollisionBlockStyle::Barrier},
    {{-3750.0, 0.0}, 65.0, 1.0, 60.0, 2.0, CollisionBlockStyle::Barrier}
}};
inline constexpr int CircuitCollisionBlockCount =
    static_cast<int>(CircuitCollisionBlocks.size());

inline constexpr CircuitCollisionBlock PlannedCollisionBlock(int index) noexcept
{
    return index >= 0 && index < CircuitCollisionBlockCount
        ? CircuitCollisionBlocks[static_cast<unsigned>(index)]
        : CircuitCollisionBlock{};
}

inline bool IsValidCollisionBlock(CircuitCollisionBlock block) noexcept
{
    return std::isfinite(block.Center.X) &&
           std::isfinite(block.Center.Y) && std::isfinite(block.Height) &&
           std::isfinite(block.ScaleX) && std::isfinite(block.ScaleY) &&
           std::isfinite(block.ScaleZ) &&
           block.ScaleX > 0.0 && block.ScaleY > 0.0 && block.ScaleZ > 0.0;
}
} // namespace latebrake
