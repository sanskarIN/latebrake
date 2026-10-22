#pragma once

#include "Track/TrackDecorations.h"

namespace latebrake
{
// Deterministic, Unreal-independent instance plan for the ONE starter circuit.
// Each batch contains one cube mesh/material style. Unreal spawns one holder
// actor with seven ISM components instead of an actor for every cosmetic cube.
enum class DecorationBatch
{
    LaneStripe = 0,
    CurbRed,
    CurbWhite,
    GrandstandBase,
    GrandstandTierLight,
    GrandstandTierDark,
    StartFinishBanner,
    Count
};

struct DecorationPlacement final
{
    CircuitPoint Center{};
    double Height = 0.0;
    double ScaleX = 0.0;
    double ScaleY = 0.0;
    double ScaleZ = 0.0;
    DecorationBatch Batch = DecorationBatch::LaneStripe;
    bool Valid = false;
};

inline constexpr int DecorationBatchCount = static_cast<int>(DecorationBatch::Count);
inline constexpr int DecorationRoadPieces = StraightCount * MarkersPerStraight * 3;
inline constexpr int DecorationStandPieces = GrandstandCount * 4;
inline constexpr int DecorationPieceCount = DecorationRoadPieces + DecorationStandPieces + 1;

constexpr DecorationPlacement PlannedDecoration(int index) noexcept
{
    if (index < 0 || index >= DecorationPieceCount) return {};

    if (index < DecorationRoadPieces)
    {
        const int straight = index / (MarkersPerStraight * 3);
        const int segment = (index / 3) % MarkersPerStraight;
        const int type = index % 3;
        if (type == 0)
        {
            const TrackMarker marker = LaneStripe(straight, segment);
            return {marker.Center, 4.0, marker.ScaleX, marker.ScaleY, 0.025,
                    DecorationBatch::LaneStripe, marker.Valid};
        }

        const bool outer = type == 2;
        const TrackMarker marker = TrackCurb(straight, segment, outer);
        const bool red = (segment + (outer ? 1 : 0)) % 2 == 0;
        return {marker.Center, 6.0, marker.ScaleX, marker.ScaleY, 0.08,
                red ? DecorationBatch::CurbRed : DecorationBatch::CurbWhite,
                marker.Valid};
    }

    if (index < DecorationRoadPieces + DecorationStandPieces)
    {
        const int groupIndex = index - DecorationRoadPieces;
        const int stand = groupIndex / 4;
        const int tier = groupIndex % 4 - 1;
        const CircuitPoint center = GrandstandCenter(stand);
        if (tier < 0)
        {
            return {center, 5.0, 15.0, 5.5, 0.9,
                    DecorationBatch::GrandstandBase, ValidGrandstand(stand)};
        }
        const double direction = stand == 0 ? 1.0 : -1.0;
        return {{center.X, center.Y + direction * (tier - 1) * 155.0},
                62.0 + tier * 32.0, 13.0, 1.25, 0.48,
                tier % 2 == 0 ? DecorationBatch::GrandstandTierLight
                              : DecorationBatch::GrandstandTierDark,
                ValidGrandstand(stand)};
    }

    return {{-2150.0, SouthY}, 390.0, 8.0, 0.32, 1.5,
            DecorationBatch::StartFinishBanner, true};
}

constexpr int DecorationBatchIndex(DecorationBatch batch) noexcept
{
    const int index = static_cast<int>(batch);
    return index >= 0 && index < DecorationBatchCount ? index : -1;
}

constexpr int PlannedBatchSize(DecorationBatch batch) noexcept
{
    if (DecorationBatchIndex(batch) < 0) return 0;
    int count = 0;
    for (int i = 0; i < DecorationPieceCount; ++i)
    {
        const auto marker = PlannedDecoration(i);
        if (marker.Valid && marker.Batch == batch) ++count;
    }
    return count;
}
} // namespace latebrake
