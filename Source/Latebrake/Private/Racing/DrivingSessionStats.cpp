#include "Racing/DrivingSessionStats.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace latebrake
{
void DrivingSessionStats::Reset() noexcept
{
    DistanceCm = 0.0;
    PeakSpeedKmH = 0.0;
    ActiveSeconds = 0.0;
    BrakingSeconds = 0.0;
    HighSlipSeconds = 0.0;
    BrakeApplications = 0;
    AcceptedSamples = 0;
    PreviousBraking = false;
    SpeedDistribution.Reset();
    Coach.Reset();
}

void DrivingSessionStats::Suspend() noexcept
{
    // A new valid frame following pause/teleport is a fresh input
    // sequence; never join a held brake across discontinuities.
    PreviousBraking = false;
    Coach.Suspend();
}

bool DrivingSessionStats::Observe(DrivingSample sample) noexcept
{
    constexpr double MaxDistanceCm = 1000000000.0;
    constexpr double MaxSeconds = 10000000.0;
    // One input policy now guards both existing telemetry and the new
    // observation-only coach. Invalid samples close only local histories.
    if (!Coach.Observe({sample.DeltaSeconds,
        sample.PlanarSpeedCmPerSecond,sample.LateralSpeedCmPerSecond,
        sample.Braking}))
    {
        PreviousBraking = false;
        return false;
    }

    const double speed = sample.PlanarSpeedCmPerSecond;
    SpeedDistribution.Observe(sample.DeltaSeconds,speed,sample.Braking);
    DistanceCm = std::min(
        MaxDistanceCm, DistanceCm + speed * sample.DeltaSeconds);
    ActiveSeconds = std::min(
        MaxSeconds, ActiveSeconds + sample.DeltaSeconds);
    PeakSpeedKmH = std::max(PeakSpeedKmH, speed * 0.036);
    if (sample.Braking)
    {
        BrakingSeconds = std::min(
            MaxSeconds, BrakingSeconds + sample.DeltaSeconds);
        if (!PreviousBraking && BrakeApplications <
                std::numeric_limits<int>::max())
        {
            ++BrakeApplications;
        }
    }
    PreviousBraking = sample.Braking;

    const double slipMagnitude =
        std::abs(sample.LateralSpeedCmPerSecond);
    if (speed >= 900.0 && slipMagnitude >= 500.0 &&
        slipMagnitude / std::max(speed, 1.0) >= 0.35)
    {
        HighSlipSeconds = std::min(
            MaxSeconds, HighSlipSeconds + sample.DeltaSeconds);
    }
    if (AcceptedSamples < std::numeric_limits<int>::max())
    {
        ++AcceptedSamples;
    }
    return true;
}
} // namespace latebrake
