#include "AI/PortableFollowingGapEnvelope.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool value, const char* message)
{
    if (!value)
    {
        std::cerr << "PortableFollowingGap FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b)
{
    return std::abs(a - b) < 1e-9;
}
}

int main()
{
    using namespace latebrake;
    PortableFollowingObservation sample;
    sample.FollowerStationMeters = 100.0;
    sample.LeaderStationMeters = 160.0;
    sample.FollowerSpeedMetersPerSecond = 20.0;
    sample.LeaderSpeedMetersPerSecond = 20.0;
    auto result = AssessPortableFollowingGap(sample);
    Check(result.Valid && result.Risk == PortableFollowingRisk::Clear &&
          Near(result.CurrentClearanceMeters, 55.6) &&
          Near(result.PredictedClosingMetersPerSecond, 0.0) &&
          result.ConstantSpeedContactSeconds == -1.0,
          "coasting separation and no fictitious collision");
    sample.LeaderSpeedMetersPerSecond = 5.0;
    sample.LeaderStationMeters = 112.0;
    result = AssessPortableFollowingGap(sample);
    Check(result.Valid && result.Risk == PortableFollowingRisk::Emergency &&
          result.ConstantSpeedContactSeconds < 1.0 &&
          result.ClearanceShortfallMeters > 0.0,
          "closing into braking leader requires emergency review");
    sample.LeaderStationMeters = 130.0;
    result = AssessPortableFollowingGap(sample);
    Check(result.Valid && result.Risk == PortableFollowingRisk::BrakeSoon &&
          result.ConservativeStoppingClearanceMeters > 0.0,
          "reaction and stopping distance exceed current clearance");
    sample.LeaderStationMeters = 500.0;
    result = AssessPortableFollowingGap(sample);
    Check(result.Valid && result.Risk == PortableFollowingRisk::Clear,
          "long distance suppresses warning");
    sample.LeaderStationMeters = 105.0;
    sample.FollowerSpeedMetersPerSecond = 0.0;
    sample.LeaderSpeedMetersPerSecond = 0.0;
    result = AssessPortableFollowingGap(sample);
    Check(result.Valid && !result.OverlappingFootprints &&
          result.Risk == PortableFollowingRisk::BrakeSoon,
          "stationary close spacing is below margin without closing speed");
    sample.LeaderStationMeters = 100.0;
    result = AssessPortableFollowingGap(sample);
    Check(result.OverlappingFootprints &&
          result.Risk == PortableFollowingRisk::Emergency,
          "overlap is urgent even when both velocities zero");
    sample.LateralSeparationMeters = 4.0;
    result = AssessPortableFollowingGap(sample);
    Check(result.Valid &&
          result.Risk == PortableFollowingRisk::DifferentLane &&
          result.ConstantSpeedContactSeconds == -1.0,
          "laterally separated observed cars do not invent contact");
    sample.LateralSeparationMeters = 0.0;
    sample.LeaderStationMeters = 90.0;
    Check(!AssessPortableFollowingGap(sample).Valid,
          "unwrapped leader-behind input fails closed");
    sample.LeaderStationMeters = 120.0;
    sample.FollowerSpeedMetersPerSecond =
        std::numeric_limits<double>::quiet_NaN();
    Check(!AssessPortableFollowingGap(sample).Valid,
          "nonfinite velocity rejected");
    sample.FollowerSpeedMetersPerSecond = 30.0;
    sample.DriverReactionSeconds = -1.0;
    Check(!AssessPortableFollowingGap(sample).Valid,
          "negative reaction time refused");
    sample.DriverReactionSeconds = 3.0;
    sample.LeaderSpeedMetersPerSecond = 0.0;
    sample.LeaderStationMeters = 140.0;
    auto slowReaction = AssessPortableFollowingGap(sample);
    sample.DriverReactionSeconds = 0.0;
    auto quickReaction = AssessPortableFollowingGap(sample);
    Check(slowReaction.Valid && quickReaction.Valid &&
          slowReaction.ConservativeStoppingClearanceMeters >
          quickReaction.ConservativeStoppingClearanceMeters,
          "larger reaction interval must not lower required clearance");
    sample.FollowerBrakingMetersPerSecondSquared = 12.0;
    auto strongerBrakes = AssessPortableFollowingGap(sample);
    Check(strongerBrakes.Valid &&
          strongerBrakes.ConservativeStoppingClearanceMeters <
          quickReaction.ConservativeStoppingClearanceMeters,
          "stronger declared brakes reduce stopping deficit");
    sample.FollowerLengthMeters = std::numeric_limits<double>::infinity();
    Check(!AssessPortableFollowingGap(sample).Valid,
          "nonfinite physical dimensions rejected");
    std::cout << "Portable following gap tests passed.\n";
}
