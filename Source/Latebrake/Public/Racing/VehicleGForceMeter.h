#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace latebrake
{
// Optional physical-motion telemetry, never a race clock, performance rating,
// reward, suspension-force input or medical/driving safety instrument.
// Velocities are WORLD XY Chaos rigid-body values in cm/s, not actor-local
// components. Their difference is projected onto the current chassis yaw;
// differentiating local speed alone would miss steady cornering acceleration.
struct VehicleMotionSample final
{
    double DeltaSeconds = 0.0;
    double WorldVelocityX = 0.0;
    double WorldVelocityY = 0.0;
    double HeadingDegrees = 0.0;
    bool BrakeApplied = false;
};

class VehicleGForceMeter final
{
public:
    static constexpr double GravityCmPerSecondSquared = 980.665;
    static constexpr double MaxSampleSeconds = 0.25;
    static constexpr double MinSampleSeconds = 0.001;
    static constexpr double MaxSpeedCmPerSecond = 10000.0;
    static constexpr double MaxResultantG = 6.0;
    static constexpr double SmoothingSeconds = 0.12;

    void Reset() noexcept
    {
        Suspend();
        PeakBrakingG = 0.0;
        PeakCorneringG = 0.0;
        AcceptedPairs = 0;
    }

    // Physics validity loss, menu/focus pause, restart or teleport must not
    // manufacture a velocity impulse across a discontinuity. Preserve
    // peaks during normal interruptions, but clear current HUD readings.
    void Suspend() noexcept
    {
        HavePrevious = false;
        PreviousX = 0.0;
        PreviousY = 0.0;
        SmoothedLongitudinalG = 0.0;
        SmoothedLateralG = 0.0;
    }

    // First valid frame arms a baseline but is NOT an acceleration sample.
    // Returns true only for a plausible consecutive world-velocity pair.
    bool Observe(const VehicleMotionSample& sample) noexcept
    {
        if (!std::isfinite(sample.DeltaSeconds) ||
            sample.DeltaSeconds < MinSampleSeconds ||
            sample.DeltaSeconds > MaxSampleSeconds ||
            !std::isfinite(sample.WorldVelocityX) ||
            !std::isfinite(sample.WorldVelocityY) ||
            !std::isfinite(sample.HeadingDegrees) ||
            std::abs(sample.HeadingDegrees) > 1000000.0 ||
            std::hypot(sample.WorldVelocityX, sample.WorldVelocityY) >
                MaxSpeedCmPerSecond)
        {
            Suspend();
            return false;
        }

        if (!HavePrevious)
        {
            Arm(sample);
            return false;
        }

        const double accelerationX =
            (sample.WorldVelocityX - PreviousX) / sample.DeltaSeconds;
        const double accelerationY =
            (sample.WorldVelocityY - PreviousY) / sample.DeltaSeconds;
        if (!std::isfinite(accelerationX) ||
            !std::isfinite(accelerationY) ||
            std::hypot(accelerationX, accelerationY) >
                MaxResultantG * GravityCmPerSecondSquared)
        {
            // Collisions, resets and numerical spikes may be real events,
            // but cannot be promoted to trusted smooth-driver g statistics.
            Suspend();
            return false;
        }

        constexpr double Pi = 3.14159265358979323846;
        const double yaw = std::remainder(sample.HeadingDegrees, 360.0) *
            (Pi / 180.0);
        const double forwardX = std::cos(yaw);
        const double forwardY = std::sin(yaw);
        const double rightX = -forwardY;
        const double rightY = forwardX;
        const double longG =
            (accelerationX * forwardX + accelerationY * forwardY) /
            GravityCmPerSecondSquared;
        const double lateralG =
            (accelerationX * rightX + accelerationY * rightY) /
            GravityCmPerSecondSquared;
        if (!std::isfinite(longG) || !std::isfinite(lateralG))
        {
            Suspend();
            return false;
        }

        const double weight = sample.DeltaSeconds /
            (SmoothingSeconds + sample.DeltaSeconds);
        SmoothedLongitudinalG += weight *
            (longG - SmoothedLongitudinalG);
        SmoothedLateralG += weight * (lateralG - SmoothedLateralG);

        // Braking G is a *deceleration while the brake is held and moving
        // forward*, not engine drag, reverse acceleration or a collision.
        const double forwardSpeed =
            sample.WorldVelocityX * forwardX + sample.WorldVelocityY * forwardY;
        if (sample.BrakeApplied && forwardSpeed >= 280.0 && longG < -0.01)
        {
            PeakBrakingG = std::max(PeakBrakingG, -longG);
        }
        if (std::hypot(sample.WorldVelocityX, sample.WorldVelocityY) >= 500.0)
        {
            PeakCorneringG = std::max(PeakCorneringG, std::abs(lateralG));
        }
        if (AcceptedPairs < std::numeric_limits<int>::max())
        {
            ++AcceptedPairs;
        }
        Arm(sample);
        return true;
    }

    double GetLiveLongitudinalG() const noexcept
    {
        return SmoothedLongitudinalG;
    }
    double GetLiveLateralG() const noexcept { return SmoothedLateralG; }
    double GetPeakBrakingG() const noexcept { return PeakBrakingG; }
    double GetPeakCorneringG() const noexcept { return PeakCorneringG; }
    int GetAcceptedPairs() const noexcept { return AcceptedPairs; }

private:
    void Arm(const VehicleMotionSample& sample) noexcept
    {
        PreviousX = sample.WorldVelocityX;
        PreviousY = sample.WorldVelocityY;
        HavePrevious = true;
    }

    double PreviousX = 0.0;
    double PreviousY = 0.0;
    double SmoothedLongitudinalG = 0.0;
    double SmoothedLateralG = 0.0;
    double PeakBrakingG = 0.0;
    double PeakCorneringG = 0.0;
    int AcceptedPairs = 0;
    bool HavePrevious = false;
};
} // namespace latebrake
