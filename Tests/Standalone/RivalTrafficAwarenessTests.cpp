#include "AI/RivalTrafficAwareness.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "RivalTrafficAwareness FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
latebrake::RivalTrafficObservation Traffic(
    double distance, double side, double rivalSpeed = 2200.0,
    double playerSpeed = 900.0)
{
    return {0.0, 0.0, 0.0, rivalSpeed, distance, side,
            playerSpeed, 0.0, true, rivalSpeed, 0.0, 684.0};
}
}

int main()
{
    using namespace latebrake;
    const PilotCommand cruise{0.85, -0.25, false, false};
    const auto close = YieldRivalToPlayer(cruise, Traffic(420, 12));
    Require(close.Brake && close.Throttle == 0.0 &&
            close.Steering == cruise.Steering && !close.Arrived,
            "closing on a player in the same lane triggers courtesy braking");

    const auto approach = YieldRivalToPlayer(cruise, Traffic(2500, 0));
    Require(!approach.Brake && approach.Throttle >= 0.15 &&
            approach.Throttle < cruise.Throttle &&
            approach.Steering == cruise.Steering,
            "a distant approach reduces rival motor power without stealing steering");

    for (const auto& observation : {
        Traffic(-300, 0), Traffic(0, 0), Traffic(7000, 0),
        Traffic(500, 250), Traffic(500, -250),
        Traffic(400, 10, 350.0, 0.0),
        Traffic(400, 0, 1800.0, 1700.0),
        Traffic(400, 0, 1400.0, 1900.0)
    })
    {
        const auto untouched = YieldRivalToPlayer(cruise, observation);
        Require(untouched.Throttle == cruise.Throttle &&
                !untouched.Brake && untouched.Steering == cruise.Steering,
                "player behind, adjacent, far or not closing cannot trigger brake");
    }

    const auto overlapApproach = YieldRivalToPlayer(cruise, Traffic(90, 0));
    Require(overlapApproach.Brake && overlapApproach.Throttle == 0.0,
            "a nearby in-lane player ahead cannot bypass emergency braking");

    // At fast closing speed, the original 600 cm-only emergency threshold
    // could react too late even in a straight, occupied lane.
    const auto predictive = YieldRivalToPlayer(cruise, Traffic(1050, 0));
    Require(predictive.Brake && predictive.Throttle == 0.0 &&
            predictive.Steering == cruise.Steering,
            "predicted short time-to-contact brakes before 600 cm threshold");
    const auto fastCatch = YieldRivalToPlayer(
        cruise, Traffic(1400, 0, 3400.0, 500.0));
    Require(fastCatch.Brake && fastCatch.Throttle == 0.0,
            "high closing speed triggers early braking even at fourteen meters");
    const auto longRangeCatch = YieldRivalToPlayer(
        cruise, Traffic(5000, 0, 3400.0, 500.0));
    Require(longRangeCatch.Brake && longRangeCatch.Throttle == 0.0 &&
            longRangeCatch.Steering == cruise.Steering,
            "measured fast closing begins braking at fifty metres, not fourteen");
    const auto beyondThrottleCap = YieldRivalToPlayer(
        cruise, Traffic(7000, 0, 3400.0, 500.0));
    Require(beyondThrottleCap.Brake && beyondThrottleCap.Throttle == 0.0,
            "physical braking deficit must override the 65-metre throttle cap");
    const auto ordinaryCatch = YieldRivalToPlayer(
        cruise, Traffic(1800, 0, 2200.0, 900.0));
    Require(ordinaryCatch.Brake && ordinaryCatch.Throttle == 0.0,
            "predicted stopping distance now brakes before the old fixed cutoff");
    auto faded = Traffic(1800, 0, 2500.0, 2000.0);
    const auto healthyBrakes = YieldRivalToPlayer(cruise, faded);
    faded.QualifiedBrakeDecelerationCmPerSecondSquared = 80.0;
    const auto weakBrakes = YieldRivalToPlayer(cruise, faded);
    Require(!healthyBrakes.Brake && healthyBrakes.Throttle == cruise.Throttle &&
            weakBrakes.Brake && weakBrakes.Throttle == 0.0,
            "real low-grip/faded brake forecast expands the stopping decision");
    const auto farDistantLane = YieldRivalToPlayer(
        cruise, Traffic(7000, 250.0, 3400.0, 500.0));
    Require(!farDistantLane.Brake &&
            farDistantLane.Throttle == cruise.Throttle,
            "emergency beyond cap still respects true other-lane clearance");
    const auto distantLane = YieldRivalToPlayer(
        cruise, Traffic(5000, 250.0, 3400.0, 500.0));
    Require(!distantLane.Brake && distantLane.Throttle == cruise.Throttle,
            "farther same-straight prediction never brakes another lane");
    const auto matchedSpeed = YieldRivalToPlayer(
        cruise, Traffic(3500, 0, 2200.0, 2150.0));
    Require(!matchedSpeed.Brake && matchedSpeed.Throttle == cruise.Throttle,
            "nearly equal physical speed at safe gap does not invent a collision");
    const auto bumperOverlap = YieldRivalToPlayer(
        cruise, Traffic(150, 0, 1700.0, 1700.0));
    Require(bumperOverlap.Brake && bumperOverlap.Throttle == 0.0,
            "genuine near bumper occupancy brakes even without relative closing");
    const auto slowCatch = YieldRivalToPlayer(
        cruise, Traffic(1050, 0, 1500.0, 900.0));
    Require(!slowCatch.Brake && slowCatch.Throttle > 0.0 &&
            slowCatch.Throttle < cruise.Throttle,
            "plenty of time before contact keeps a smooth throttle reduction");
    const auto outsideLane = YieldRivalToPlayer(
        cruise, Traffic(1050, 226.0, 3400.0, 500.0));
    Require(!outsideLane.Brake && outsideLane.Throttle == cruise.Throttle,
            "prediction cannot brake for a player outside the lane corridor");
    // An otherwise adjacent player can merge into the real physical
    // passing lane BEFORE a rapidly closing rival reaches its rear gap.
    // The previous source policy only inspected the CURRENT 225cm lane.
    auto imminentMerge = Traffic(1500, 400.0, 2200.0, 500.0);
    imminentMerge.PlayerWorldVelocityY = -600.0;
    const auto crossing = YieldRivalToPlayer(cruise, imminentMerge);
    Require(crossing.Brake && crossing.Throttle == 0.0 &&
            crossing.Steering == cruise.Steering,
            "real sideways entry into future lane overlap triggers early brake");
    auto oppositeMerge = imminentMerge;
    oppositeMerge.PlayerY = -400.0;
    oppositeMerge.PlayerWorldVelocityY = 600.0;
    const auto mirrored = YieldRivalToPlayer(cruise, oppositeMerge);
    Require(mirrored.Brake && mirrored.Throttle == 0.0,
            "opposite side merging toward lane is symmetric");
    auto diverging = imminentMerge;
    diverging.PlayerWorldVelocityY = 600.0;
    const auto outward = YieldRivalToPlayer(cruise, diverging);
    Require(!outward.Brake && outward.Throttle == cruise.Throttle,
            "same-side travel AWAY from lane cannot invent contact");
    auto nearMiss = imminentMerge;
    nearMiss.PlayerWorldVelocityY = -200.0;
    const auto grazing = YieldRivalToPlayer(cruise, nearMiss);
    Require(!grazing.Brake && grazing.Throttle == cruise.Throttle,
            "lateral projection staying outside clearance remains untouched");
    auto distantCross = imminentMerge;
    distantCross.PlayerX = 5500.0;
    const auto farUncertain = YieldRivalToPlayer(cruise, distantCross);
    Require(!farUncertain.Brake && farUncertain.Throttle == cruise.Throttle,
            "distant future lane-entry guess cannot freeze the rival");
    auto matchedAdjacent = imminentMerge;
    matchedAdjacent.PlayerWorldVelocityX = 2150.0;
    const auto noCatch = YieldRivalToPlayer(cruise, matchedAdjacent);
    Require(!noCatch.Brake && noCatch.Throttle == cruise.Throttle,
            "lateral merging with no longitudinal closing is not a rear-catch");

    const auto mildClose = YieldRivalToPlayer(
        cruise, Traffic(350, 0, 1600.0, 1400.0));
    Require(!mildClose.Brake && mildClose.Throttle < cruise.Throttle,
            "relative approach below emergency threshold is gentle, not full stop");

    const auto priorCornerBrake = YieldRivalToPlayer(
        {0.0, 0.25, true, false}, Traffic(400, 0));
    Require(priorCornerBrake.Brake && priorCornerBrake.Throttle == 0.0 &&
            priorCornerBrake.Steering == 0.25,
            "already braking for a corner is preserved");
    const auto arrival = YieldRivalToPlayer(
        {0.0, 0.0, false, true}, Traffic(350, 0));
    Require(arrival.Arrived && !arrival.Brake && arrival.Throttle == 0.0,
            "waypoint arrival is never reclassified as traffic");
    const auto idle = YieldRivalToPlayer(
        {0.0, 0.4, false, false}, Traffic(380, 0));
    Require(idle.Brake && idle.Throttle == 0.0 &&
            idle.Steering == 0.4,
            "a coasting but fast-closing rival still brakes and never adds throttle");

    constexpr double HalfPi = 1.57079632679489661923;
    const RivalTrafficObservation ninety{
        1200.0, -700.0, HalfPi, 2000.0,
        1190.0, -300.0, 0.0, 900.0, true, 0.0, 2000.0, 684.0
    };
    const RivalTrafficObservation northboundMerge{
        1200.0, -700.0, HalfPi, 2200.0,
        800.0, 800.0, 600.0, 500.0, true, 0.0, 2200.0, 684.0
    };
    const auto rotatedMerge = YieldRivalToPlayer(cruise, northboundMerge);
    Require(rotatedMerge.Brake && rotatedMerge.Throttle == 0.0,
            "world-X sideways merge near northbound forward collision brakes");
    auto rotatedAway = northboundMerge;
    rotatedAway.PlayerWorldVelocityX = -600.0;
    const auto rotatedExit = YieldRivalToPlayer(cruise, rotatedAway);
    Require(!rotatedExit.Brake && rotatedExit.Throttle == cruise.Throttle,
            "northbound player moving outward from lane remains unblocked");

    const auto rotating = YieldRivalToPlayer(cruise, ninety);
    Require(rotating.Brake && rotating.Throttle == 0.0 &&
            rotating.Steering == cruise.Steering,
            "90-degree world-Y driving uses correct forward lane projections");

    // A sideways-sliding rival has genuine planar speed but no forward
    // approach. Treating speed magnitude as forward motion invents contact.
    auto sliding = Traffic(850, 0, 2200.0, 0.0);
    sliding.RivalWorldVelocityX = 0.0;
    sliding.RivalWorldVelocityY = 2200.0;
    const auto lateral = YieldRivalToPlayer(cruise, sliding);
    Require(!lateral.Brake && lateral.Throttle == cruise.Throttle &&
            lateral.Steering == cruise.Steering,
            "sideways speed alone cannot invent forward collision risk");
    auto reversing = Traffic(850, 0, 2200.0, 0.0);
    reversing.RivalWorldVelocityX = -2200.0;
    const auto backing = YieldRivalToPlayer(cruise, reversing);
    Require(!backing.Brake && backing.Throttle == cruise.Throttle,
            "physical reversing motion does not register as closing");
    auto inconsistent = Traffic(850, 0, 2200.0, 0.0);
    inconsistent.RivalWorldVelocityX = 200.0;
    const auto invalidSpeed = YieldRivalToPlayer(cruise, inconsistent);
    Require(invalidSpeed.Brake && invalidSpeed.Throttle == 0.0,
            "mismatched rival speed and world velocity fail closed");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    const RivalTrafficObservation invalid[] = {
        {0, 0, 0, 2000, 420, 0, 0, 0, false, 2000, 0},
        {nan, 0, 0, 2000, 420, 0, 0, 0, true, 2000, 0},
        {0, inf, 0, 2000, 420, 0, 0, 0, true, 2000, 0},
        {0, 0, nan, 2000, 420, 0, 0, 0, true, 2000, 0},
        {0, 0, 8, 2000, 420, 0, 0, 0, true, 2000, 0},
        {0, 0, 0, -1, 420, 0, 0, 0, true, 0, 0},
        {0, 0, 0, 15001, 420, 0, 0, 0, true, 15001, 0},
        {0, 0, 0, 2000, nan, 0, 0, 0, true, 2000, 0},
        {0, 0, 0, 2000, 420, inf, 0, 0, true, 2000, 0},
        {0, 0, 0, 2000, 420, 0, inf, 0, true, 2000, 0},
        {0, 0, 0, 2000, 420, 0, 15001, 0, true, 2000, 0},
        {0, 0, 0, 2000, 420, 0, 0, 0, true, nan, 0},
        {0, 0, 0, 2000, 420, 0, 0, 0, true, 0, inf},
    };
    for (const auto& corrupt : invalid)
    {
        auto independentlyCorrupt = corrupt;
        independentlyCorrupt.QualifiedBrakeDecelerationCmPerSecondSquared =
            684.0;
        const auto result = YieldRivalToPlayer(cruise, independentlyCorrupt);
        Require(result.Throttle == 0.0 && result.Brake &&
                result.Steering == 0.0 && !result.Arrived,
                "missing player or invalid physics must fail closed to no motor");
    }
    auto noBrakeAuthority = Traffic(5000, 0, 3400.0, 500.0);
    noBrakeAuthority.QualifiedBrakeDecelerationCmPerSecondSquared = 0.0;
    const auto noAuthority = YieldRivalToPlayer(cruise, noBrakeAuthority);
    Require(noAuthority.Brake && noAuthority.Throttle == 0.0 &&
            noAuthority.Steering == 0.0,
            "missing measured grip/braking forecast fails closed to neutral brake");
    noBrakeAuthority.QualifiedBrakeDecelerationCmPerSecondSquared = nan;
    Require(YieldRivalToPlayer(cruise, noBrakeAuthority).Brake,
            "nonfinite braking forecast fails closed");
    noBrakeAuthority.QualifiedBrakeDecelerationCmPerSecondSquared = inf;
    Require(YieldRivalToPlayer(cruise, noBrakeAuthority).Brake,
            "infinite braking forecast fails closed");
    noBrakeAuthority.QualifiedBrakeDecelerationCmPerSecondSquared = -10.0;
    Require(YieldRivalToPlayer(cruise, noBrakeAuthority).Brake,
            "negative braking forecast cannot authorize propulsion");
    const auto badPilot = YieldRivalToPlayer(
        {nan, 0, false, false}, Traffic(400, 0));
    Require(badPilot.Brake && badPilot.Throttle == 0.0,
            "invalid pilot commands never authorize motor output");

    for (int d = 110; d <= 1600; ++d)
    {
        const auto output = YieldRivalToPlayer(cruise, Traffic(d, 0));
        Require(std::isfinite(output.Throttle) &&
                output.Throttle >= 0 && output.Throttle <= cruise.Throttle &&
                output.Steering == cruise.Steering && !output.Arrived,
                "every observed distance retains safe bounded speed and steering");
    }
    // Large deterministic sweep: worse physical stopping capability
    // cannot turn a required brake into permission to accelerate.
    for (int distance = 250; distance <= 6500; distance += 25)
    {
        for (int closing = 150; closing <= 3500; closing += 50)
        {
            auto physicallyHealthy = Traffic(
                distance, 0, 3600.0, 3600.0 - closing);
            auto physicallyWeak = physicallyHealthy;
            physicallyWeak.QualifiedBrakeDecelerationCmPerSecondSquared = 80.0;
            const auto withHealthyBrakes =
                YieldRivalToPlayer(cruise, physicallyHealthy);
            const auto withWeakBrakes =
                YieldRivalToPlayer(cruise, physicallyWeak);
            Require(std::isfinite(withHealthyBrakes.Throttle) &&
                    std::isfinite(withWeakBrakes.Throttle) &&
                    withHealthyBrakes.Throttle >= 0.0 &&
                    withWeakBrakes.Throttle >= 0.0 &&
                    withHealthyBrakes.Throttle <= cruise.Throttle &&
                    withWeakBrakes.Throttle <= cruise.Throttle &&
                    withHealthyBrakes.Steering == cruise.Steering &&
                    withWeakBrakes.Steering == cruise.Steering,
                    "all speeds, grip states and gaps keep bounded safe commands");
            Require(!withHealthyBrakes.Brake || withWeakBrakes.Brake,
                    "weaker measured brakes never dismiss a healthy-brake stop");
        }
    }
    // 12,000 deterministic, mirrored lane-entry scenarios across real
    // measured approach gaps. Inward lateral velocity may reduce throttle;
    // outward travel must never trigger phantom braking. The guard must
    // preserve steering and never add throttle or create route progress.
    int predictedBrakes = 0;
    for (int index = 0; index < 12000; ++index)
    {
        const double forward = 400.0 + static_cast<double>(index % 1800);
        const double side = 250.0 + static_cast<double>(index % 350);
        auto inward = Traffic(forward, side, 2200.0, 500.0);
        inward.PlayerWorldVelocityY = -600.0;
        auto away = inward;
        away.PlayerWorldVelocityY = 600.0;
        const auto toward = YieldRivalToPlayer(cruise, inward);
        const auto retreat = YieldRivalToPlayer(cruise, away);
        Require(toward.Throttle <= cruise.Throttle &&
                toward.Throttle <= retreat.Throttle &&
                toward.Steering == cruise.Steering &&
                retreat.Steering == cruise.Steering &&
                !retreat.Brake && retreat.Throttle == cruise.Throttle,
                "12k converging/retreating lane pairs stay conservative");
        if (toward.Brake) ++predictedBrakes;
    }
    Require(predictedBrakes > 5000,
            "physically converging lateral traffic is detected broadly");
    std::cout << "Rival lane-approach safety policy passed.\n";
    return EXIT_SUCCESS;
}
