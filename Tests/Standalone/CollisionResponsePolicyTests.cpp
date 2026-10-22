#include "Track/CollisionResponsePolicy.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "Collision response policy FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    using D = TrackCollisionResponseDecision;
    static_assert(CheckTrackCollisionResponses({true, true, true}) == D::Ready);
    static_assert(CheckTrackCollisionResponses({false, true, true}) == D::PawnIgnored);
    static_assert(CheckTrackCollisionResponses({true, false, true}) == D::PhysicsBodyIgnored);
    static_assert(CheckTrackCollisionResponses({true, true, false}) == D::WorldDynamicIgnored);

    Require(CheckTrackCollisionResponses({}) == D::PawnIgnored,
            "unknown default channel configuration is never ready");
    Require(CheckTrackCollisionResponses({true, true, true}) == D::Ready,
            "all three blocking collision channels are required");
    Require(CheckTrackCollisionResponses({false, true, true}) == D::PawnIgnored,
            "pawn Ignore/Overlap is an explicit rejection");
    Require(CheckTrackCollisionResponses({true, false, true}) == D::PhysicsBodyIgnored,
            "vehicle physics Ignore/Overlap is an explicit rejection");
    Require(CheckTrackCollisionResponses({true, true, false}) == D::WorldDynamicIgnored,
            "dynamic world Ignore/Overlap is an explicit rejection");

    Require(CheckTrackCollisionResponses({false, false, false}) == D::PawnIgnored,
            "multiple wrong channels deterministically report first failure");
    Require(CheckTrackCollisionResponses({true, false, false}) == D::PhysicsBodyIgnored,
            "remaining failure order never falsely reports ready");

    Require(TrackCollisionResponseDecisionName(D::PawnIgnored)[0] == 'p' &&
            TrackCollisionResponseDecisionName(D::PhysicsBodyIgnored)[0] == 'p' &&
            TrackCollisionResponseDecisionName(D::WorldDynamicIgnored)[0] == 'w',
            "stable local diagnostic strings for field investigation");
    Require(TrackCollisionResponseDecisionName(static_cast<D>(999))[0] == 'u',
            "unexpected enum values safely print unknown");
    std::cout << "Track collision-channel policy passed.\n";
    return EXIT_SUCCESS;
}
