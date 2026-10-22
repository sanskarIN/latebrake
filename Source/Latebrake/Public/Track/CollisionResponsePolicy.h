#pragma once

namespace latebrake
{
// The LateBrake starter circuit is intended to block the pawn, simulated
// vehicle physics body and other dynamic world objects. CollisionEnabled
// alone is insufficient if a collision profile ignores key channels.
struct TrackCollisionResponses final
{
    bool BlocksPawn = false;
    bool BlocksPhysicsBody = false;
    bool BlocksWorldDynamic = false;
};

enum class TrackCollisionResponseDecision
{
    Ready,
    PawnIgnored,
    PhysicsBodyIgnored,
    WorldDynamicIgnored
};

inline constexpr TrackCollisionResponseDecision CheckTrackCollisionResponses(
    TrackCollisionResponses responses) noexcept
{
    if (!responses.BlocksPawn)
        return TrackCollisionResponseDecision::PawnIgnored;
    if (!responses.BlocksPhysicsBody)
        return TrackCollisionResponseDecision::PhysicsBodyIgnored;
    if (!responses.BlocksWorldDynamic)
        return TrackCollisionResponseDecision::WorldDynamicIgnored;
    return TrackCollisionResponseDecision::Ready;
}

inline constexpr const char* TrackCollisionResponseDecisionName(
    TrackCollisionResponseDecision decision) noexcept
{
    switch (decision)
    {
    case TrackCollisionResponseDecision::Ready: return "ready";
    case TrackCollisionResponseDecision::PawnIgnored: return "pawn-not-blocked";
    case TrackCollisionResponseDecision::PhysicsBodyIgnored: return "physics-body-not-blocked";
    case TrackCollisionResponseDecision::WorldDynamicIgnored: return "world-dynamic-not-blocked";
    }
    return "unknown";
}
} // namespace latebrake
