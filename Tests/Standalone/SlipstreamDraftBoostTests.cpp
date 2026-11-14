#include "Racing/SlipstreamDraftBoost.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool value, const char* why)
{
    if (!value)
    {
        std::cerr << "SlipstreamDraftBoost FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
latebrake::DraftingSample Sample(double gap = 1200.0, double lateral = 0.0)
{
    latebrake::DraftingSample s{};
    s.FrameSeconds = 0.05;
    s.FollowerSpeedCmPerSecond = 2400.0;
    s.LeadSpeedCmPerSecond = 2300.0;
    s.LeadOffsetX = gap;
    s.LeadOffsetY = lateral;
    s.ForwardThrottle = 1.0;
    s.EligibleHumanRace = true;
    s.ClearPhysicalDraftPath = true;
    s.FollowerVerticalSpeedCmPerSecond = 0.0;
    s.LeadVerticalSpeedCmPerSecond = 0.0;
    s.LeadIdentity = 17;
    s.FollowerWorldX = 0.0;
    s.FollowerWorldY = 0.0;
    s.FollowerWorldZ = 0.0;
    return s;
}
void Confirm(latebrake::SlipstreamDraftBoost& boost,
             latebrake::DraftingSample s = Sample())
{
    for (int i = 0; i < 3; ++i)
    {
        Check(boost.Observe(s) == 0.0 && !boost.IsActive(),
              "single frame/short draft never earns active boost");
    }
    Check(boost.Observe(s) > 0.0 && boost.IsActive(),
          "sustained same-direction draft earns modest boost");
}
}
int main()
{
    using latebrake::SlipstreamDraftBoost;
    SlipstreamDraftBoost boost;
    Check(!boost.IsActive() && boost.ExtraForce() == 0.0,
          "fresh policy has no free speed");
    Confirm(boost);
    const double nearBonus = boost.ExtraForce();
    Check(nearBonus > 0.0 && nearBonus <= SlipstreamDraftBoost::MaxForceExtraFraction,
          "bonus is strictly bounded to 6 percent");
    boost.Reset();
    Confirm(boost, Sample(3200.0, 140.0));
    Check(boost.ExtraForce() < nearBonus, "distance and lateral offset attenuate drafting");

    const double bad = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (double dt : {bad, inf, 0.0, -0.1, 0.11})
    {
        boost.Reset();
        Confirm(boost);
        auto s = Sample(); s.FrameSeconds = dt;
        Check(boost.Observe(s) == 0.0 && !boost.IsActive(), "invalid/hitch dt clears boost");
    }
    for (double value : {bad, inf, -inf})
    {
        boost.Reset(); Confirm(boost);
        auto s = Sample(); s.LeadOffsetX = value;
        Check(boost.Observe(s) == 0.0 && !boost.IsActive(), "bad position clears boost");
        boost.Reset(); Confirm(boost);
        s = Sample(); s.LeadSpeedCmPerSecond = value;
        Check(boost.Observe(s) == 0.0 && !boost.IsActive(), "bad rival velocity clears boost");
        boost.Reset(); Confirm(boost);
        s = Sample(); s.FollowerForwardY = value;
        Check(boost.Observe(s) == 0.0 && !boost.IsActive(), "bad orientation clears boost");
    }
    auto Reject = [&](latebrake::DraftingSample s, const char* message)
    {
        boost.Reset(); Confirm(boost);
        Check(boost.Observe(s) == 0.0 && !boost.IsActive() &&
              boost.QualifiedSeconds() == 0.0, message);
    };
    {auto s = Sample(); s.Braking = true; Reject(s, "brake takes priority");}
    {auto s = Sample(); s.EligibleHumanRace = false; Reject(s, "no AI/pause or menu eligibility");}
    {auto s = Sample(); s.ClearPhysicalDraftPath = false;
        Reject(s, "solid barrier cancels already-earned draft immediately");}
    {auto s = Sample(); s.LeadIdentity = 0;
        Reject(s, "unknown live rival actor identity fails closed");}
    for (const double invalid : {bad, inf, -inf})
    {
        auto s = Sample(); s.FollowerWorldX = invalid;
        Reject(s, "missing or nonfinite follower world X fails closed");
        s = Sample(); s.FollowerWorldY = invalid;
        Reject(s, "missing or nonfinite follower world Y fails closed");
        s = Sample(); s.FollowerWorldZ = invalid;
        Reject(s, "missing or nonfinite follower world Z fails closed");
    }
    {auto s = Sample(); s.ForwardThrottle = -0.4; Reject(s, "reverse input denied");}
    {auto s = Sample(); s.FollowerSpeedCmPerSecond = 800.0; Reject(s, "stationary car denied");}
    {auto s = Sample(); s.LeadSpeedCmPerSecond = 0.0; Reject(s, "stationary front car denied");}
    {auto s = Sample(); s.LeadSpeedCmPerSecond = 1100.0; Reject(s, "too rapid closing denied");}
    {auto s = Sample(); s.LeadForwardX = -1.0; Reject(s, "oncoming rival denied");}
    {auto s = Sample(); s.LeadOffsetX = -1200.0; Reject(s, "rival behind denied");}
    {auto s = Sample(); s.LeadOffsetX = 4500.0; Reject(s, "rival too distant denied");}
    {auto s = Sample(); s.LeadOffsetY = 390.0; Reject(s, "rival in another lane denied");}
    {auto s = Sample(); s.LeadOffsetZ = 300.0; Reject(s, "vertical separation denied");}
    {auto s = Sample(); s.FollowerForwardX = 0.5; Reject(s, "non-unit forward vector denied");}
    {auto s = Sample(); s.FollowerLateralSpeedCmPerSecond = 501.0;
        Reject(s, "sliding follower does not earn forward-only aerodynamic help");}
    {auto s = Sample(); s.LeadLateralSpeedCmPerSecond = -501.0;
        Reject(s, "sliding real rival cannot become a valid leading target");}
    {auto s = Sample(); s.FollowerSpeedCmPerSecond = 1200.0;
        s.LeadSpeedCmPerSecond = 1000.0;
        s.FollowerLateralSpeedCmPerSecond = 301.0;
        Reject(s, "low-speed slip threshold scales with real forward travel");}
    {auto s = Sample(); s.LeadSpeedCmPerSecond = 1000.0;
        s.LeadLateralSpeedCmPerSecond = 251.0;
        Reject(s, "slow leader uses fractional slip envelope, not just absolute cap");}
    for (const double invalid : {bad, inf, -inf})
    {
        auto s = Sample();
        s.FollowerLateralSpeedCmPerSecond = invalid;
        Reject(s, "nonfinite follower lateral velocity cannot grant draft");
        s = Sample();
        s.LeadLateralSpeedCmPerSecond = invalid;
        Reject(s, "nonfinite rival lateral velocity cannot grant draft");
    }


    // Missing/nonphysical vertical observations must fail closed even
    // if every planar sample otherwise looks like a valid drafting lane.
    for (const double invalid : {bad, inf, -inf})
    {
        auto s = Sample(); s.FollowerVerticalSpeedCmPerSecond = invalid;
        Reject(s, "invalid follower vertical velocity clears draft");
        s = Sample(); s.LeadVerticalSpeedCmPerSecond = invalid;
        Reject(s, "invalid lead vertical velocity clears draft");
    }
    {auto s = Sample(); s.FollowerVerticalSpeedCmPerSecond = 10001.0;
        Reject(s, "implausibly fast vertical follower denied");}
    {auto s = Sample(); s.LeadVerticalSpeedCmPerSecond = -10001.0;
        Reject(s, "implausibly fast vertical leader denied");}
    {auto s = Sample(); s.FollowerVerticalSpeedCmPerSecond = bad;
        boost.Reset();
        Check(boost.Observe(s) == 0.0 && !boost.IsActive() &&
              boost.QualifiedSeconds() == 0.0,
              "missing Z evidence on an uninitialized frame is closed");}

    // Unit-axis normalization keeps slightly imperfect source vectors
    // from falsifying true chassis alignment. The raw dot product of two
    // slightly oversized headings can pass a raw dot-product check
    // despite a physically invalid real angle.
    boost.Reset();
    auto scaledAndSkewed = Sample();
    const double permittedScale = std::sqrt(1.04);
    scaledAndSkewed.FollowerForwardX = permittedScale;
    scaledAndSkewed.FollowerForwardY = 0.0;
    scaledAndSkewed.LeadForwardX = permittedScale * std::cos(0.43);
    scaledAndSkewed.LeadForwardY = permittedScale * std::sin(0.43);
    Check(scaledAndSkewed.FollowerForwardX *
              scaledAndSkewed.LeadForwardX >
              SlipstreamDraftBoost::MinimumHeadingAlignment,
          "adversarial raw scaled dot appears aligned");
    Check(std::cos(0.43) <
              SlipstreamDraftBoost::MinimumHeadingAlignment,
          "adversarial normalized angle is actually disallowed");
    Reject(scaledAndSkewed,
           "scaled headings cannot fake the physical alignment threshold");

    // A valid slightly nonunit direction remains accepted after both
    // axes are normalized; the 5% source tolerance is not a false
    // rejection of small heading-axis measurement error.
    boost.Reset();
    auto scaledAndAligned = Sample();
    scaledAndAligned.FollowerForwardX = permittedScale;
    scaledAndAligned.LeadForwardX = permittedScale * std::cos(0.32);
    scaledAndAligned.LeadForwardY = permittedScale * std::sin(0.32);
    Confirm(boost, scaledAndAligned);
    Check(boost.IsActive(),
          "properly aligned near-boundary headings retain drafting");

    // Planar gap, lane lateral distance and predicted relative motion
    // must use normalized axes, not a fictitious 5% longer heading.
    boost.Reset();
    auto scaledForward = Sample(3780.0);
    scaledForward.FollowerForwardX = permittedScale;
    scaledForward.LeadForwardX = permittedScale;
    for (int frame = 0; frame < 3; ++frame)
    {
        Check(boost.Observe(scaledForward) == 0.0 &&
              boost.QualifiedSeconds() < SlipstreamDraftBoost::ConfirmSeconds,
              "true gap near 38m remains eligible before confirmation");
    }
    Check(boost.Observe(scaledForward) > 0.0 && boost.IsActive(),
          "normalization prevents false far-edge gap rejection");
    boost.Reset();
    auto scaledSide = Sample(1200.0, 377.0);
    scaledSide.FollowerForwardX = permittedScale;
    scaledSide.LeadForwardX = permittedScale;
    Confirm(boost, scaledSide);
    Check(boost.IsActive(),
          "normalization prevents near-edge lane offset inflation");

    // Physically admissible side slip at both boundaries still receives
    // help, but a single sideways frame immediately clears all stored
    // eligibility rather than silently resuming at the prior duration.
    boost.Reset();
    auto boundedSlip = Sample();
    boundedSlip.FollowerLateralSpeedCmPerSecond = 500.0;
    boundedSlip.LeadLateralSpeedCmPerSecond = -500.0;
    Confirm(boost, boundedSlip);
    Check(boost.IsActive(), "bounded physical side movement can still draft");
    boundedSlip.LeadLateralSpeedCmPerSecond = -500.0001;
    Check(boost.Observe(boundedSlip) == 0.0 && !boost.IsActive() &&
          boost.QualifiedSeconds() == 0.0,
          "first over-slip frame revokes boost and accumulated qualification");
    boundedSlip.LeadLateralSpeedCmPerSecond = 0.0;
    Check(boost.Observe(boundedSlip) == 0.0 &&
          boost.QualifiedSeconds() < SlipstreamDraftBoost::ConfirmSeconds,
          "drift recovery cannot inherit previous confirmation time");

    // Deterministic positive/negative lateral slip sweeps against real
    // permitted forward speeds: neither car may earn assistance from
    // excessive sideways travel, even when both headings point forward.
    for (int i = 0; i < 20000; ++i)
    {
        auto s = Sample();
        s.FollowerSpeedCmPerSecond = 1200.0 + (i % 6000);
        s.LeadSpeedCmPerSecond = s.FollowerSpeedCmPerSecond - 100.0;
        s.FollowerLateralSpeedCmPerSecond =
            (i & 1 ? -1.0 : 1.0) *
            (0.25 * s.FollowerSpeedCmPerSecond + 600.0);
        boost.Reset();
        Check(boost.Observe(s) == 0.0 && !boost.IsActive() &&
              boost.QualifiedSeconds() == 0.0,
              "20k varying-speed sideways follower frames never qualify");
        s.FollowerLateralSpeedCmPerSecond = 0.0;
        s.LeadLateralSpeedCmPerSecond =
            (i & 1 ? 1.0 : -1.0) *
            (0.25 * s.LeadSpeedCmPerSecond + 600.0);
        Check(boost.Observe(s) == 0.0 && !boost.IsActive() &&
              boost.QualifiedSeconds() == 0.0,
              "20k varying-speed sideways rival frames never qualify");
    }

    // A genuine relative gap change matching both measured chassis
    // velocities must keep the qualification alive without a timer reset.
    boost.Reset();
    auto continuous = Sample();
    Confirm(boost, continuous);
    continuous.LeadOffsetX -= 5.0; // (2300 - 2400) cm/s for 0.05s
    Check(boost.Observe(continuous) > 0.0 && boost.IsActive(),
          "ordinary real relative movement retains qualified draft");

    // A recovering/teleported rival may reappear in the same eligible lane.
    // Snapshot geometry alone is insufficient: the prior boost must die.
    boost.Reset();
    Confirm(boost);
    auto jumped = Sample();
    jumped.LeadOffsetX = 2000.0; // still a valid standalone drafting gap
    Check(boost.Observe(jumped) == 0.0 && !boost.IsActive() &&
          boost.QualifiedSeconds() == 0.0,
          "in-lane rival teleport revokes full drafting confirmation");
    Check(boost.Observe(jumped) == 0.0 && !boost.IsActive() &&
          boost.QualifiedSeconds() < SlipstreamDraftBoost::ConfirmSeconds,
          "post-teleport geometry must establish an independent new window");

    // World-space continuity also works on the Y axis, rather than only
    // using horizontal map X or the leading vehicle's location.
    boost.Reset();
    auto north = Sample();
    north.FollowerForwardX = 0.0;
    north.FollowerForwardY = 1.0;
    north.LeadForwardX = 0.0;
    north.LeadForwardY = 1.0;
    north.LeadOffsetX = 0.0;
    north.LeadOffsetY = 1200.0;
    Confirm(boost, north);
    north.LeadOffsetY -= 5.0;
    Check(boost.Observe(north) > 0.0 && boost.IsActive(),
          "rotated northbound pair preserves genuine relative continuity");
    north.LeadOffsetY = 1700.0;
    Check(boost.Observe(north) == 0.0 && !boost.IsActive() &&
          boost.QualifiedSeconds() == 0.0,
          "northbound in-lane jump also clears drafting");


    // Physically consistent vertical motion may preserve drafting; an
    // impossible up/down displacement within the usual 2.3 m geometry
    // envelope must not inherit an earned boost or confirmation time.
    boost.Reset();
    auto rising = Sample();
    rising.FollowerVerticalSpeedCmPerSecond = 80.0;
    rising.LeadVerticalSpeedCmPerSecond = 280.0;
    Confirm(boost, rising);
    rising.LeadOffsetZ = 10.0; // (280 - 80) cm/s * 0.05s
    Check(boost.Observe(rising) > 0.0 && boost.IsActive(),
          "measured ordinary relative 3D motion preserves drafting");
    rising.LeadOffsetZ = -220.0; // valid gap, but incompatible with velocity
    Check(boost.Observe(rising) == 0.0 && !boost.IsActive() &&
          boost.QualifiedSeconds() == 0.0,
          "same-lane vertical relocation immediately revokes 3D drafting");
    Check(boost.Observe(rising) == 0.0 && !boost.IsActive() &&
          boost.QualifiedSeconds() < SlipstreamDraftBoost::ConfirmSeconds,
          "post-vertical-jump frames must earn a fresh confirmation window");

    // The total 2m three-axis tolerance alone permits a 90cm vertical
    // relocation inside a perfectly ordinary drafting height window.
    // Unmeasured modest tick-order position skew is tolerated, but an
    // impossible isolated vertical jump immediately revokes earned credit.
    boost.Reset();
    Confirm(boost);
    auto deceptiveRise = Sample();
    deceptiveRise.LeadOffsetZ = 60.0;
    Check(boost.Observe(deceptiveRise) > 0.0 && boost.IsActive(),
          "small vertical observation skew preserves a qualified draft");
    deceptiveRise.LeadOffsetZ = 150.0; // 90cm unreported delta, <2m 3D
    Check(boost.Observe(deceptiveRise) == 0.0 && !boost.IsActive() &&
          boost.QualifiedSeconds() == 0.0,
          "90cm impossible vertical relocation fails the independent Z gate");
    Check(boost.Observe(deceptiveRise) == 0.0 &&
          boost.QualifiedSeconds() < SlipstreamDraftBoost::ConfirmSeconds,
          "isolated Z jump cannot reuse previous confirmation time");
    for (int frame = 0; frame < 3; ++frame)
    {
        boost.Observe(deceptiveRise);
    }
    Check(boost.IsActive(),
          "stable recovered real Z separation may qualify again");


    // Symmetry matters: an unmeasured downward 90cm relocation is no
    // more physically consistent than the upward case.
    boost.Reset();
    Confirm(boost);
    auto deceptiveDrop = Sample();
    deceptiveDrop.LeadOffsetZ = -90.0;
    Check(boost.Observe(deceptiveDrop) == 0.0 && !boost.IsActive() &&
          boost.QualifiedSeconds() == 0.0,
          "downward in-lane jump also revokes drafting immediately");

    // The vertical policy still follows REAL measured descent: both
    // cars can be dropping rapidly without a false teleport detection.
    boost.Reset();
    auto measuredDescent = Sample();
    measuredDescent.FollowerVerticalSpeedCmPerSecond = -75.0;
    measuredDescent.LeadVerticalSpeedCmPerSecond = -275.0;
    Confirm(boost, measuredDescent);
    measuredDescent.LeadOffsetZ = -10.0;
    Check(boost.Observe(measuredDescent) > 0.0 && boost.IsActive(),
          "physically predicted falling pair preserves draft");

    // A 75cm residual boundary is eligible; the Z policy must reject
    // only strictly beyond tolerance while preserving the wider 3D gate.
    boost.Reset();
    Confirm(boost);
    auto boundedVerticalSkew = Sample();
    boundedVerticalSkew.LeadOffsetZ =
        SlipstreamDraftBoost::MaxVerticalMotionResidualCm;
    Check(boost.Observe(boundedVerticalSkew) > 0.0 && boost.IsActive(),
          "vertical threshold itself is inclusive");
    boundedVerticalSkew.LeadOffsetZ += 0.01 +
        SlipstreamDraftBoost::MaxVerticalMotionResidualCm;
    Check(boost.Observe(boundedVerticalSkew) == 0.0 &&
          boost.QualifiedSeconds() == 0.0,
          "vertical continuity denies threshold exceedance in one frame");

    // A substituted rival may have exactly the same speed, world heading,
    // gap and elevation as the previously observed car. Its UNIQUE actor
    // identity must invalidate the earlier fully earned drafting window.
    boost.Reset();
    Confirm(boost);
    auto replacedRival = Sample();
    replacedRival.LeadIdentity = 18;
    Check(boost.Observe(replacedRival) == 0.0 && !boost.IsActive() &&
          boost.QualifiedSeconds() > 0.0 &&
          boost.QualifiedSeconds() < SlipstreamDraftBoost::ConfirmSeconds,
          "identical-geometry replacement restarts rival confirmation");
    for (int frame = 0; frame < 3; ++frame)
    {
        boost.Observe(replacedRival);
    }
    Check(boost.IsActive(),
          "replacement rival must earn its own complete drafting interval");
    replacedRival.LeadIdentity = 0;
    Check(boost.Observe(replacedRival) == 0.0 && !boost.IsActive() &&
          boost.QualifiedSeconds() == 0.0,
          "unknown actor after replacement clears all earned credit");

    // Rapid rival identity churn cannot synthesize a continuous 0.18s
    // aerodynamic interval, even when snapshots are geometrically valid.
    boost.Reset();
    for (int frame = 0; frame < 5000; ++frame)
    {
        auto alternatingRival = Sample();
        alternatingRival.LeadIdentity = frame % 2 == 0 ? 17 : 18;
        Check(boost.Observe(alternatingRival) == 0.0 && !boost.IsActive() &&
              boost.QualifiedSeconds() < SlipstreamDraftBoost::ConfirmSeconds,
              "rapid opponent swaps cannot inherit drafting charge");
    }

    // Relative gap and heading do not change when two vehicles relocate
    // identically. The independent follower world-position check must
    // revoke even a fully qualified draft instead of inheriting its charge.
    boost.Reset();
    Confirm(boost);
    auto commonWorldJump = Sample();
    commonWorldJump.FollowerWorldX = 500.0;
    Check(boost.Observe(commonWorldJump) == 0.0 && !boost.IsActive() &&
          boost.QualifiedSeconds() == 0.0,
          "common-mode X relocation revokes unchanged relative drafting");
    Check(boost.Observe(commonWorldJump) == 0.0 &&
          boost.QualifiedSeconds() < SlipstreamDraftBoost::ConfirmSeconds,
          "common-mode relocation must requalify from scratch");

    boost.Reset();
    Confirm(boost);
    commonWorldJump = Sample();
    commonWorldJump.FollowerWorldY = 400.0;
    Check(boost.Observe(commonWorldJump) == 0.0 && !boost.IsActive() &&
          boost.QualifiedSeconds() == 0.0,
          "common-mode lateral relocation cannot carry old drafting");

    boost.Reset();
    Confirm(boost);
    commonWorldJump = Sample();
    commonWorldJump.FollowerWorldZ = 100.0;
    Check(boost.Observe(commonWorldJump) == 0.0 && !boost.IsActive() &&
          boost.QualifiedSeconds() == 0.0,
          "common-mode vertical relocation violates absolute Z continuity");

    // Normal real player movement matching the measured physical velocity
    // must NOT be classified as a relocation or reset the draft window.
    boost.Reset();
    Confirm(boost);
    auto worldContinues = Sample();
    worldContinues.FollowerWorldX = 120.0; // 2400 cm/s for 50 ms
    Check(boost.Observe(worldContinues) > 0.0 && boost.IsActive(),
          "genuine measured follower world movement preserves draft");
    worldContinues.FollowerWorldX += 120.0;
    Check(boost.Observe(worldContinues) > 0.0 && boost.IsActive(),
          "repeated genuine forward world movement preserves draft");

    // A sustained HIGH-SPEED physical convoy must still qualify. This
    // exercises real world travel rather than relying on zeroed fixture
    // positions and tests the 90m/s observed high-speed safety envelope.
    boost.Reset();
    for (int frame = 0; frame < 60; ++frame)
    {
        auto highSpeed = Sample();
        highSpeed.FollowerSpeedCmPerSecond = 9000.0;
        highSpeed.LeadSpeedCmPerSecond = 8900.0;
        highSpeed.FollowerWorldX = 450.0 * frame;
        highSpeed.LeadOffsetX = 1200.0 - 5.0 * frame;
        const double force = boost.Observe(highSpeed);
        Check(frame < 3
                  ? (force == 0.0 && !boost.IsActive())
                  : (force > 0.0 && boost.IsActive()),
              "physically continuous high-speed world motion keeps draft");
    }

    // Genuine world-space motion is not restricted to an X-aligned circuit.
    // Both absolute vehicle displacement and relative lead/follower motion
    // must be reconstructed correctly after rotating the driving axes.
    boost.Reset();
    const double roadX = std::cos(0.55);
    const double roadY = std::sin(0.55);
    for (int frame = 0; frame < 60; ++frame)
    {
        auto rotatedRoad = Sample();
        rotatedRoad.FollowerForwardX = roadX;
        rotatedRoad.FollowerForwardY = roadY;
        rotatedRoad.LeadForwardX = roadX;
        rotatedRoad.LeadForwardY = roadY;
        rotatedRoad.FollowerWorldX = 120.0 * frame * roadX;
        rotatedRoad.FollowerWorldY = 120.0 * frame * roadY;
        rotatedRoad.LeadOffsetX = (1200.0 - 5.0 * frame) * roadX;
        rotatedRoad.LeadOffsetY = (1200.0 - 5.0 * frame) * roadY;
        const double force = boost.Observe(rotatedRoad);
        Check(frame < 3
                  ? (force == 0.0 && !boost.IsActive())
                  : (force > 0.0 && boost.IsActive()),
              "rotated track world/relative motion qualifies without false teleport");
    }

    // A live road bend changes actual world velocity every frame. The
    // previous tests rotate a straight road once; this verifies continuously
    // turning physics with time-integrated positions for BOTH moving cars.
    // The same heading/velocity vectors passed to the policy MUST predict
    // the observed world displacement and relative lead position, rather
    // than resetting legitimate slipstream assistance each steering tick.
    boost.Reset();
    double bendFollowerX = 0.0;
    double bendFollowerY = 0.0;
    double bendLeaderX = 1200.0;
    double bendLeaderY = 0.0;
    for (int frame = 0; frame < 64; ++frame)
    {
        auto bend = Sample();
        const double yaw = frame * 0.003;
        const double directionX = std::cos(yaw);
        const double directionY = std::sin(yaw);
        if (frame > 0)
        {
            bendFollowerX += bend.FrameSeconds *
                bend.FollowerSpeedCmPerSecond * directionX;
            bendFollowerY += bend.FrameSeconds *
                bend.FollowerSpeedCmPerSecond * directionY;
            bendLeaderX += bend.FrameSeconds *
                bend.LeadSpeedCmPerSecond * directionX;
            bendLeaderY += bend.FrameSeconds *
                bend.LeadSpeedCmPerSecond * directionY;
        }
        bend.FollowerForwardX = directionX;
        bend.FollowerForwardY = directionY;
        bend.LeadForwardX = directionX;
        bend.LeadForwardY = directionY;
        bend.FollowerWorldX = bendFollowerX;
        bend.FollowerWorldY = bendFollowerY;
        bend.LeadOffsetX = bendLeaderX - bendFollowerX;
        bend.LeadOffsetY = bendLeaderY - bendFollowerY;
        const double force = boost.Observe(bend);
        Check(frame < 3
                  ? (force == 0.0 && !boost.IsActive())
                  : (force > 0.0 && boost.IsActive()),
              "64-frame physically integrated bend retains drafting");
    }

    // A matching world trajectory can be inverted on the Y axis without
    // treating legitimate right-handed and left-handed corners differently.
    boost.Reset();
    bendFollowerX = 0.0;
    bendFollowerY = 0.0;
    bendLeaderX = 1200.0;
    bendLeaderY = 0.0;
    for (int frame = 0; frame < 64; ++frame)
    {
        auto bend = Sample();
        const double yaw = -frame * 0.003;
        const double directionX = std::cos(yaw);
        const double directionY = std::sin(yaw);
        if (frame > 0)
        {
            bendFollowerX += bend.FrameSeconds *
                bend.FollowerSpeedCmPerSecond * directionX;
            bendFollowerY += bend.FrameSeconds *
                bend.FollowerSpeedCmPerSecond * directionY;
            bendLeaderX += bend.FrameSeconds *
                bend.LeadSpeedCmPerSecond * directionX;
            bendLeaderY += bend.FrameSeconds *
                bend.LeadSpeedCmPerSecond * directionY;
        }
        bend.FollowerForwardX = directionX;
        bend.FollowerForwardY = directionY;
        bend.LeadForwardX = directionX;
        bend.LeadForwardY = directionY;
        bend.FollowerWorldX = bendFollowerX;
        bend.FollowerWorldY = bendFollowerY;
        bend.LeadOffsetX = bendLeaderX - bendFollowerX;
        bend.LeadOffsetY = bendLeaderY - bendFollowerY;
        const double force = boost.Observe(bend);
        Check(frame < 3
                  ? (force == 0.0 && !boost.IsActive())
                  : (force > 0.0 && boost.IsActive()),
              "64-frame opposite-handed bend retains real drafting");
    }

    // 12k independent discontinuous in-lane snapshots must never inherit
    // an old qualification, even when both headings and speeds are valid.
    for (int run = 0; run < 12000; ++run)
    {
        boost.Reset();
        Confirm(boost);
        auto s = Sample();
        s.LeadOffsetX = 1600.0 + static_cast<double>(run % 500);
        Check(boost.Observe(s) == 0.0 && !boost.IsActive() &&
              boost.QualifiedSeconds() == 0.0,
              "12k bounded but discontinuous rival gaps revoke boost");
    }

    // A blocked real-world sightline never grants help even when the
    // chassis are forward-moving, aligned, supported and within one lane.
    // Immediately losing visibility must erase the entire old window.
    boost.Reset();
    Confirm(boost);
    auto blocked = Sample();
    blocked.ClearPhysicalDraftPath = false;
    Check(boost.Observe(blocked) == 0.0 && !boost.IsActive() &&
          boost.QualifiedSeconds() == 0.0,
          "solid barrier must revoke even a fully qualified physical draft");
    blocked.ClearPhysicalDraftPath = true;
    Check(boost.Observe(blocked) == 0.0 && !boost.IsActive() &&
          boost.QualifiedSeconds() < SlipstreamDraftBoost::ConfirmSeconds,
          "reopening the real sightline must not restore stored draft time");
    for (int frame = 0; frame < 3; ++frame)
    {
        boost.Observe(blocked);
    }
    Check(boost.IsActive(),
          "four newly unobstructed samples reestablish drafting normally");

    // Deterministic adversarial visibility sequences: an occlusion cannot
    // borrow the previously accepted time even as the valid gap changes.
    for (int run = 0; run < 12000; ++run)
    {
        boost.Reset();
        Confirm(boost);
        auto s = Sample();
        s.LeadOffsetX = 1200.0 - static_cast<double>(run % 80);
        s.ClearPhysicalDraftPath = false;
        Check(boost.Observe(s) == 0.0 && !boost.IsActive() &&
              boost.QualifiedSeconds() == 0.0,
              "12k occluded but otherwise valid racing pairs deny draft");
        s.ClearPhysicalDraftPath = true;
        Check(boost.Observe(s) == 0.0 && !boost.IsActive() &&
              boost.QualifiedSeconds() < SlipstreamDraftBoost::ConfirmSeconds,
              "post-occlusion physics samples always start fresh");
    }

    boost.Reset();
    for (int run = 0; run < 5000; ++run)
    {
        Confirm(boost);
        Check(std::isfinite(boost.ExtraForce()) &&
              boost.ExtraForce() >= 0.0 &&
              boost.ExtraForce() <= SlipstreamDraftBoost::MaxForceExtraFraction,
              "stress never exceeds safe force fraction");
        boost.Suspend();
        Check(!boost.IsActive() && boost.QualifiedSeconds() == 0.0,
              "pause/teleport clears entire confirmation window");
    }
    std::cout << "SlipstreamDraftBoost passed\n";
}
