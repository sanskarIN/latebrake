#include "AI/RivalSurfaceDamagePace.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool ok, const char* explanation)
{
    if (!ok)
    {
        std::cerr << "RivalSurfaceDamagePace FAIL: " << explanation << '\n';
        std::exit(EXIT_FAILURE);
    }
}
latebrake::RivalSurfaceDamageSample Sample(
    bool dirt = false, double speed = 1600.0,
    double lateral = 200.0, double impactGrip = 1.0)
{
    return {0.05, speed, lateral,
            dirt ? latebrake::ArcadeSurfaceTraction::OffPavementGripMultiplier
                 : 1.0,
            impactGrip, dirt, true, true};
}
latebrake::PilotCommand Pilot(double steer = 0.65)
{
    return {0.85, steer, false, false};
}
} // namespace

int main()
{
    using latebrake::RivalSurfaceDamagePace;
    using latebrake::PilotCommand;
    RivalSurfaceDamagePace pace;
    const auto normal = Pilot();

    auto outcome = pace.Apply(normal, Sample());
    Check(!outcome.Brake && outcome.Throttle == normal.Throttle &&
          outcome.Steering == normal.Steering &&
          pace.RemainingHoldSeconds() == 0.0,
          "unhurt rival on real pavement preserves current corner pace");

    // Actual 150-ms off-road confirmation belongs to ArcadeSurfaceTraction,
    // not to this layer. That component's measured flag is passed here.
    outcome = pace.Apply(normal, Sample(true, 2600.0, 500.0));
    Check(outcome.Brake && outcome.Throttle == 0.0 &&
          outcome.Steering == normal.Steering &&
          pace.RemainingHoldSeconds() == 0.12,
          "fast confirmed dirt triggers modest speed-reduction braking");
    for (int i = 0; i < 2; ++i)
    {
        outcome = pace.Apply(normal, Sample(true, 1500.0, 100.0));
        Check(outcome.Brake && outcome.Throttle == 0.0,
              "brake hold avoids alternating brake/throttle on dirt");
    }
    outcome = pace.Apply(normal, Sample(true, 1500.0, 100.0));
    Check(!outcome.Brake && outcome.Throttle > 0.0 &&
          outcome.Throttle < normal.Throttle &&
          outcome.Steering == normal.Steering,
          "confirmed dirt can recover bounded throttle without stalling");
    outcome = pace.Apply(normal, Sample(true, 350.0, 80.0));
    Check(!outcome.Brake && outcome.Throttle == normal.Throttle,
          "low-speed dirt recovery keeps full requested launch torque");

    pace.Reset();
    outcome = pace.Apply(normal, Sample(false, 3000.0, 400.0, 0.88));
    Check(outcome.Brake && outcome.Throttle == 0.0,
          "severe real collision damage tempers fast corner approach");
    pace.Reset();
    outcome = pace.Apply(Pilot(0.1),
                         Sample(false, 3500.0, 50.0, 0.856));
    Check(!outcome.Brake && outcome.Throttle == 0.85,
          "damaged straight does not spuriously brake for corner risk");
    outcome = pace.Apply(normal, Sample(false, 3500.0, 50.0, 0.98));
    Check(!outcome.Brake && outcome.Throttle == normal.Throttle,
          "minimal impact wear alone does not trigger braking");

    pace.Reset();
    outcome = pace.Apply(normal, Sample(true, 2600.0, 500.0));
    PilotCommand priorBrake{0.0, -0.6, true, false};
    outcome = pace.Apply(priorBrake, Sample(true));
    Check(outcome.Brake && outcome.Steering == -0.6 &&
          outcome.Throttle == 0.0 && pace.RemainingHoldSeconds() == 0.0,
          "ordinary route braking remains authoritative");
    PilotCommand arrived{0.0, 0.0, false, true};
    outcome = pace.Apply(arrived, Sample(true));
    Check(outcome.Arrived && !outcome.Brake,
          "route arrival remains authoritative and does not add braking");

    auto Reject = [&](latebrake::RivalSurfaceDamageSample sample,
                      const char* reason)
    {
        pace.Apply(normal, Sample(true, 2500.0, 500.0));
        const auto blocked = pace.Apply(normal, sample);
        Check(blocked.Brake && blocked.Throttle == 0.0 &&
              blocked.Steering == 0.0 &&
              pace.RemainingHoldSeconds() == 0.0, reason);
    };
    {
        auto s = Sample(); s.SurfaceReady = false;
        Reject(s, "missing measured surface observation");
    }
    {
        auto s = Sample(); s.ImpactFrameReady = false;
        Reject(s, "stale impact eligibility");
    }
    {
        auto s = Sample(); s.OffPavement = true;
        Reject(s, "inconsistent surface grip and dirt flag");
    }
    {
        auto s = Sample(true); s.SurfaceGripMultiplier = 1.0;
        Reject(s, "inconsistent confirmed dirt coefficient");
    }
    {
        auto s = Sample(); s.ImpactGripMultiplier = 0.85;
        Reject(s, "fabricated grip below collision policy floor");
    }
    {
        auto s = Sample(); s.ImpactGripMultiplier = 1.01;
        Reject(s, "invalid artificial impact grip bonus");
    }
    {
        auto s = Sample(); s.DeltaSeconds = 0.11;
        Reject(s, "frame hitch fails closed and clears stale hold");
    }
    {
        auto s = Sample(); s.DeltaSeconds = 0.0;
        Reject(s, "invalid time fails closed");
    }
    {
        auto s = Sample(); s.LateralSpeedCmPerSecond = 9000.0;
        Reject(s, "impossible measured lateral velocity");
    }
    for (double bad : {std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity()})
    {
        { auto s = Sample(); s.PlanarSpeedCmPerSecond = bad;
          Reject(s, "invalid chassis speed"); }
        { auto s = Sample(); s.LateralSpeedCmPerSecond = bad;
          Reject(s, "invalid measured lateral velocity"); }
        { auto s = Sample(); s.SurfaceGripMultiplier = bad;
          Reject(s, "invalid pavement grip multiplier"); }
        { auto s = Sample(); s.ImpactGripMultiplier = bad;
          Reject(s, "invalid collision multiplier"); }
        { auto s = Sample(); s.DeltaSeconds = bad;
          Reject(s, "nonfinite engine frame"); }
    }
    outcome = pace.Apply({2.0, 0.0, false, false}, Sample());
    Check(outcome.Brake && outcome.Throttle == 0.0,
          "invalid above-unit throttle must fail closed");
    outcome = pace.Apply({0.85, 2.0, false, false}, Sample());
    Check(outcome.Brake && outcome.Steering == 0.0,
          "invalid steering demand must fail closed");

    for (int i = 0; i < 200000; ++i)
    {
        if (i % 401 == 0)
        {
            pace.Reset();
        }
        const bool offroad = (i % 73) < 23;
        const double speed = 200.0 + static_cast<double>(i % 8700);
        const double lateral = (static_cast<double>(i % 101) / 200.0) * speed;
        const double impactGrip = 0.856 +
            (static_cast<double>(i % 145) / 1000.0);
        const auto cmd = Pilot(static_cast<double>(i % 91) / 90.0);
        const auto result = pace.Apply(cmd,
            Sample(offroad, speed, lateral, impactGrip));
        Check(std::isfinite(result.Throttle) &&
              result.Throttle >= 0.0 && result.Throttle <= cmd.Throttle &&
              result.Steering == cmd.Steering &&
              (!result.Brake || result.Throttle == 0.0) &&
              pace.RemainingHoldSeconds() >= 0.0 &&
              pace.RemainingHoldSeconds() <= RivalSurfaceDamagePace::BrakeHoldSeconds,
              "200000 real-sample combinations never boost or redirect AI");
    }
    pace.Reset();
    Check(pace.RemainingHoldSeconds() == 0.0,
          "race recovery and reset must clear prior brake hold");

    std::cout << "RivalSurfaceDamagePace: 200000 bounded samples passed (no UE)\n";
    return EXIT_SUCCESS;
}
