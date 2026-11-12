#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// This model is a pre-integration diagnostic of two already-observed cars
// traveling forward along the SAME unwrapped lane coordinate in metres.
// It neither commands AI brakes nor replaces swept Chaos collision queries.
struct PortableFollowingObservation final
{
    double FollowerStationMeters = 0.0;
    double LeaderStationMeters = 0.0;
    double FollowerSpeedMetersPerSecond = 0.0;
    double LeaderSpeedMetersPerSecond = 0.0;
    double FollowerLengthMeters = 4.4;
    double LeaderLengthMeters = 4.4;
    double FollowerHalfWidthMeters = 0.95;
    double LeaderHalfWidthMeters = 0.95;
    double LateralSeparationMeters = 0.0;
    double DriverReactionSeconds = 0.8;
    double FollowerBrakingMetersPerSecondSquared = 6.0;
    double LeaderBrakingMetersPerSecondSquared = 8.0;
    double ClearanceMarginMeters = 2.0;
};

enum class PortableFollowingRisk
{
    Unavailable,
    DifferentLane,
    Clear,
    Caution,
    BrakeSoon,
    Emergency
};

struct PortableFollowingAssessment final
{
    bool Valid = false;
    PortableFollowingRisk Risk = PortableFollowingRisk::Unavailable;
    bool OverlappingFootprints = false;
    double CurrentClearanceMeters = 0.0;
    double ConservativeStoppingClearanceMeters = 0.0;
    double PredictedClosingMetersPerSecond = 0.0;
    // Negative means no constant-speed closing collision is predicted.
    double ConstantSpeedContactSeconds = -1.0;
    double ClearanceShortfallMeters = 0.0;
};

inline bool ValidPortableFollowingObservation(
    const PortableFollowingObservation& sample) noexcept
{
    const auto finite = [](double value) noexcept
    {
        return std::isfinite(value);
    };
    return finite(sample.FollowerStationMeters) &&
        finite(sample.LeaderStationMeters) &&
        sample.FollowerStationMeters >= 0.0 &&
        sample.LeaderStationMeters >= sample.FollowerStationMeters &&
        sample.LeaderStationMeters <= 1000000.0 &&
        finite(sample.FollowerSpeedMetersPerSecond) &&
        finite(sample.LeaderSpeedMetersPerSecond) &&
        sample.FollowerSpeedMetersPerSecond >= 0.0 &&
        sample.FollowerSpeedMetersPerSecond <= 120.0 &&
        sample.LeaderSpeedMetersPerSecond >= 0.0 &&
        sample.LeaderSpeedMetersPerSecond <= 120.0 &&
        finite(sample.FollowerLengthMeters) &&
        finite(sample.LeaderLengthMeters) &&
        sample.FollowerLengthMeters >= 2.0 &&
        sample.LeaderLengthMeters >= 2.0 &&
        sample.FollowerLengthMeters <= 30.0 &&
        sample.LeaderLengthMeters <= 30.0 &&
        finite(sample.FollowerHalfWidthMeters) &&
        finite(sample.LeaderHalfWidthMeters) &&
        sample.FollowerHalfWidthMeters >= 0.5 &&
        sample.LeaderHalfWidthMeters >= 0.5 &&
        sample.FollowerHalfWidthMeters <= 4.0 &&
        sample.LeaderHalfWidthMeters <= 4.0 &&
        finite(sample.LateralSeparationMeters) &&
        std::abs(sample.LateralSeparationMeters) <= 1000.0 &&
        finite(sample.DriverReactionSeconds) &&
        sample.DriverReactionSeconds >= 0.0 &&
        sample.DriverReactionSeconds <= 3.0 &&
        finite(sample.FollowerBrakingMetersPerSecondSquared) &&
        finite(sample.LeaderBrakingMetersPerSecondSquared) &&
        sample.FollowerBrakingMetersPerSecondSquared >= 0.5 &&
        sample.LeaderBrakingMetersPerSecondSquared >= 0.5 &&
        sample.FollowerBrakingMetersPerSecondSquared <= 30.0 &&
        sample.LeaderBrakingMetersPerSecondSquared <= 30.0 &&
        finite(sample.ClearanceMarginMeters) &&
        sample.ClearanceMarginMeters >= 0.0 &&
        sample.ClearanceMarginMeters <= 20.0;
}

inline PortableFollowingAssessment AssessPortableFollowingGap(
    PortableFollowingObservation sample) noexcept
{
    if (!ValidPortableFollowingObservation(sample))
    {
        return {};
    }
    PortableFollowingAssessment result;
    result.Valid = true;
    const double centreDistance =
        sample.LeaderStationMeters - sample.FollowerStationMeters;
    const double bumperOffset =
        (sample.FollowerLengthMeters + sample.LeaderLengthMeters) / 2.0;
    result.CurrentClearanceMeters = centreDistance - bumperOffset;
    result.OverlappingFootprints = result.CurrentClearanceMeters < 0.0;
    if (std::abs(sample.LateralSeparationMeters) >
        sample.FollowerHalfWidthMeters + sample.LeaderHalfWidthMeters)
    {
        result.Risk = PortableFollowingRisk::DifferentLane;
        // The scalar road coordinate does not prove future lane-changing
        // safety; this simply refuses to invent longitudinal contact.
        return result;
    }

    const double followerSpeed = sample.FollowerSpeedMetersPerSecond;
    const double leaderSpeed = sample.LeaderSpeedMetersPerSecond;
    const double followerStop = followerSpeed * followerSpeed /
        (2.0 * sample.FollowerBrakingMetersPerSecondSquared);
    const double leaderStop = leaderSpeed * leaderSpeed /
        (2.0 * sample.LeaderBrakingMetersPerSecondSquared);
    const double reactionTravel = followerSpeed * sample.DriverReactionSeconds;
    result.ConservativeStoppingClearanceMeters = std::max(
        sample.ClearanceMarginMeters,
        sample.ClearanceMarginMeters + reactionTravel + followerStop - leaderStop);
    result.ClearanceShortfallMeters = std::max(0.0,
        result.ConservativeStoppingClearanceMeters -
        result.CurrentClearanceMeters);
    result.PredictedClosingMetersPerSecond =
        std::max(0.0, followerSpeed - leaderSpeed);
    if (result.PredictedClosingMetersPerSecond > 0.0)
    {
        result.ConstantSpeedContactSeconds =
            std::max(0.0, result.CurrentClearanceMeters) /
            result.PredictedClosingMetersPerSecond;
    }
    if (result.OverlappingFootprints ||
        result.CurrentClearanceMeters <= 0.0 ||
        (result.ConstantSpeedContactSeconds >= 0.0 &&
         result.ConstantSpeedContactSeconds < 1.0))
    {
        result.Risk = PortableFollowingRisk::Emergency;
    }
    else if (result.ClearanceShortfallMeters > 0.0 ||
        (result.ConstantSpeedContactSeconds >= 0.0 &&
         result.ConstantSpeedContactSeconds < 2.0))
    {
        result.Risk = PortableFollowingRisk::BrakeSoon;
    }
    else if ((result.ConstantSpeedContactSeconds >= 0.0 &&
              result.ConstantSpeedContactSeconds < 4.0) ||
             result.CurrentClearanceMeters <
                 result.ConservativeStoppingClearanceMeters * 1.5)
    {
        result.Risk = PortableFollowingRisk::Caution;
    }
    else
    {
        result.Risk = PortableFollowingRisk::Clear;
    }
    return result;
}
} // namespace latebrake
