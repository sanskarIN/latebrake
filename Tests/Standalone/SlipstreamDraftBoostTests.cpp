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
