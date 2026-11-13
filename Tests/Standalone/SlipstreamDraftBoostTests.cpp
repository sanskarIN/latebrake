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
