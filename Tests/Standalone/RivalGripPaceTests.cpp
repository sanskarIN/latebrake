#include "AI/RivalGripPace.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool ok, const char* why)
{
    if (!ok)
    {
        std::cerr << "RivalGripPace FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
latebrake::RivalGripPaceSample Sample(
    double speed = 2400.0, double lateral = 600.0,
    double grip = 0.80)
{
    return {0.05, speed, lateral, grip, true};
}
latebrake::PilotCommand Command(double steer = 0.7)
{
    return {0.85, steer, false, false};
}
}
int main()
{
    using latebrake::RivalGripPace;
    using latebrake::PilotCommand;
    RivalGripPace pace;
    const PilotCommand original = Command();
    auto output = pace.Apply(original, Sample());
    Check(output.Brake && output.Throttle == 0.0 &&
          output.Steering == original.Steering,
          "high-slip low-grip fast corner should brake without steering change");
    Check(pace.WasGripLimited() && pace.LastTargetSpeed() > 0.0,
          "real grip event exposes a bounded target");

    auto s = Sample(1600.0, 400.0, 0.80);
    pace.Reset();
    output = pace.Apply(original, s);
    Check(!output.Brake && output.Throttle >= 0.0 &&
          output.Throttle < original.Throttle &&
          output.Steering == original.Steering,
          "slower corner limits throttle without stalling or changing steering");

    pace.Reset();
    output = pace.Apply(Command(0.0), Sample(3500.0, 0.0, 0.68));
    Check(!output.Brake && output.Throttle == 0.85 &&
          !pace.WasGripLimited(), "straight does not punish valid tire wear");

    pace.Reset();
    output = pace.Apply(original, Sample(850.0, 220.0, 0.69));
    Check(!output.Brake && output.Throttle == original.Throttle,
          "launch is allowed even when tire grip is low");

    pace.Reset();
    output = pace.Apply(original, Sample());
    Check(output.Brake, "stateful braking begins");
    for (int i = 0; i < 2; ++i)
    {
        output = pace.Apply(Command(0.0), Sample(1800.0, 0.0, 1.0));
        Check(output.Brake && output.Throttle == 0.0,
              "short brake hold prevents instant chatter");
    }
    output = pace.Apply(Command(0.0), Sample(1800.0, 0.0, 1.0));
    Check(!output.Brake && output.Throttle == 0.85 &&
          pace.RemainingHoldSeconds() == 0.0,
          "brake hold expires without retaining stale motor restrictions");

    pace.Apply(original, Sample());
    PilotCommand prebraking{0.0, 0.8, true, false};
    output = pace.Apply(prebraking, Sample());
    Check(output.Brake && output.Throttle == 0.0 &&
          output.Steering == prebraking.Steering &&
          pace.RemainingHoldSeconds() == 0.0,
          "authoritative corner braking passes through untouched");

    pace.Apply(original, Sample());
    PilotCommand arrived{0.0, -0.4, false, true};
    output = pace.Apply(arrived, Sample());
    Check(output.Arrived && !output.Brake && output.Steering == -0.4 &&
          pace.RemainingHoldSeconds() == 0.0,
          "route checkpoint arrival passes through untouched");

    auto Reject = [&](latebrake::RivalGripPaceSample sample,
                      const char* reason)
    {
        pace.Apply(original, Sample());
        auto failed = pace.Apply(original, sample);
        Check(failed.Brake && failed.Throttle == 0.0 &&
              failed.Steering == 0.0 &&
              pace.RemainingHoldSeconds() == 0.0, reason);
    };
    {auto x = Sample(); x.RealPhysicsSampleReady = false; Reject(x, "no actual physics");}
    {auto x = Sample(); x.FrameSeconds = 0.0; Reject(x, "no time");}
    {auto x = Sample(); x.FrameSeconds = 0.11; Reject(x, "physics hitch");}
    {auto x = Sample(); x.PlanarSpeedCmPerSecond = -1.0; Reject(x, "negative speed");}
    {auto x = Sample(); x.PlanarSpeedCmPerSecond = 10001.0; Reject(x, "impossible speed");}
    {auto x = Sample(); x.LateralSpeedCmPerSecond = 3000.0; Reject(x, "invalid slip");}
    {auto x = Sample(); x.TireGripMultiplier = 0.67; Reject(x, "invalid low grip");}
    {auto x = Sample(); x.TireGripMultiplier = 1.01; Reject(x, "invalid high grip");}
    for (double bad : {std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity()})
    {
        {auto x = Sample(); x.FrameSeconds = bad; Reject(x, "bad frame");}
        {auto x = Sample(); x.PlanarSpeedCmPerSecond = bad; Reject(x, "bad speed");}
        {auto x = Sample(); x.LateralSpeedCmPerSecond = bad; Reject(x, "bad slip");}
        {auto x = Sample(); x.TireGripMultiplier = bad; Reject(x, "bad grip");}
    }
    pace.Apply(original, Sample());
    output = pace.Apply({2.0, 0.0, false, false}, Sample());
    Check(output.Brake && output.Throttle == 0.0, "malformed throttle rejected");
    output = pace.Apply({0.85, 2.0, false, false}, Sample());
    Check(output.Brake && output.Steering == 0.0, "malformed steering rejected");

    for (int session = 0; session < 5000; ++session)
    {
        pace.Reset();
        for (int frame = 0; frame < 100; ++frame)
        {
            const double speed = 900.0 + (frame * 23.0);
            const double grip = 0.68 + (frame % 33) * 0.01;
            const auto cmd = Command((frame % 2 == 0) ? 0.6 : -0.6);
            const auto out = pace.Apply(cmd, Sample(speed, speed * 0.20, grip));
            Check(std::isfinite(out.Throttle) && std::isfinite(out.Steering) &&
                  out.Throttle >= 0.0 && out.Throttle <= cmd.Throttle &&
                  out.Steering == cmd.Steering,
                  "stress preserves bounded throttle and steering");
        }
        pace.Reset();
        Check(pace.RemainingHoldSeconds() == 0.0 &&
              pace.LastTargetSpeed() == 0.0 && !pace.WasGripLimited(),
              "reset must remove stale grip pacing state");
    }
    std::cout << "RivalGripPace passed\n";
}
