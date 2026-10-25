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
}

void DrivingSessionStats::Suspend() noexcept
{
    // A new valid frame following pause/teleport is a fresh input
    // sequence; never join a held brake across discontinuities.
    PreviousBraking = false;
}

bool DrivingSessionStats::Observe(DrivingSample sample) noexcept
{
    constexpr double MaxSpeed = 10000.0; // 360 km/h
    constexpr double MaxDistanceCm = 1000000000.0;
    constexpr double MaxSeconds = 10000000.0;
    if (!std::isfinite(sample.DeltaSeconds) ||
        sample.DeltaSeconds <= 0.0 || sample.DeltaSeconds > 0.25 ||
        !std::isfinite(sample.PlanarSpeedCmPerSecond) ||
        sample.PlanarSpeedCmPerSecond < 0.0 ||
        sample.PlanarSpeedCmPerSecond > MaxSpeed ||
        !std::isfinite(sample.LateralSpeedCmPerSecond) ||
        std::abs(sample.LateralSpeedCmPerSecond) > MaxSpeed)
    {
        Suspend();
        return false;
    }

    const double speed = sample.PlanarSpeedCmPerSecond;
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
