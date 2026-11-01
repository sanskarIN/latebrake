#include "Racing/StuckRecoveryHint.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

static void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "StuckRecoveryHint FAIL: " << message << '\n';
        std::exit(1);
    }
}

int main()
{
    using latebrake::StuckRecoveryHint;
    using latebrake::StuckRecoverySample;

    StuckRecoveryHint hint;
    StuckRecoverySample sample{0.1, 1000.0, 2000.0, 0.0, true, true};
    for (int i = 0; i < 39; ++i)
        Check(!hint.Observe(sample), "no premature alert");
    Check(hint.Observe(sample), "eligible 4 second hold alerts");
    Check(hint.Observe(sample), "persists while stationary and pressing gas");
    Check(hint.QualifiedStillSeconds() == 4.0, "duration bounded");

    sample.ForwardThrottleRequested = false;
    Check(!hint.Observe(sample), "throttle release clears alert");
    Check(hint.QualifiedStillSeconds() == 0.0, "cleared history");
    sample.ForwardThrottleRequested = true;
    Check(!hint.Observe(sample), "a fresh hold needs another four seconds");

    sample.DrivingEligible = false;
    Check(!hint.Observe(sample), "countdown/pause/finish disallow hint");
    sample.DrivingEligible = true;
    for (int i = 0; i < 39; ++i)
        Check(!hint.Observe(sample), "post-pause starts fresh");
    Check(hint.Observe(sample), "fresh post-pause hold can alert");
    sample.PlanarSpeedCmPerSecond = StuckRecoveryHint::SlowSpeedCmPerSecond;
    Check(!hint.Observe(sample), "normal speed clears false warning");
    sample.PlanarSpeedCmPerSecond = 0;

    for (int i = 0; i < 20; ++i)
    {
        sample.PositionXcm += 8;
        Check(!hint.Observe(sample), "moving car not flagged as stuck");
    }
    Check(hint.QualifiedStillSeconds() < 4.0,
          "anchor resets after real displacement");

    // Teleport and race restart explicitly reset the caller-owned tracker.
    hint.Reset();
    sample.PositionXcm += 10000;
    Check(!hint.Observe(sample), "new world anchor after teleport");

    sample.DeltaSeconds = 0.2;
    Check(!hint.Observe(sample), "long frame resets instead of aging");
    sample.DeltaSeconds = 0.1;
    sample.PositionXcm = std::numeric_limits<double>::infinity();
    Check(!hint.Observe(sample), "bad position fails closed");
    sample.PositionXcm = 1000;
    sample.PlanarSpeedCmPerSecond = std::numeric_limits<double>::quiet_NaN();
    Check(!hint.Observe(sample), "bad speed fails closed");
    sample.PlanarSpeedCmPerSecond = 0;
    sample.DeltaSeconds = std::numeric_limits<double>::quiet_NaN();
    Check(!hint.Observe(sample), "bad frame time fails closed");
    sample.DeltaSeconds = 0.1;

    for (int i = 0; i < 39; ++i)
        Check(!hint.Observe(sample), "invalid frames cannot accumulate time");
    Check(hint.Observe(sample), "valid duration after invalid data");
    sample.PositionXcm += 151;
    Check(!hint.Observe(sample), "large motion resets active hint");
    Check(!hint.Observe(sample), "reset requires new observation sequence");

    std::cout << "Stuck recovery advisory tests passed.\n";
    return 0;
}
