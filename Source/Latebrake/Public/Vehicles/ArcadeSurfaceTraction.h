#pragma once

#include "Track/CircuitLayout.h"

#include <cmath>

namespace latebrake
{
// A deterministic arcade surface response for the existing rigid-body
// vehicle. This samples the actual XY chassis location against the same
// original paved circuit used for wayfinding. It is NOT Chaos tire friction,
// terrain collision, suspension or an asphalt material query.
struct ArcadeSurfaceSample final
{
    double DeltaSeconds = 0.0;
    double WorldX = 0.0;
    double WorldY = 0.0;
    bool DrivingReady = false;
};

struct ArcadeSurfaceOutput final
{
    // Invalid samples never authorize motor force. The Unreal caller must
    // neutralize physics rather than interpreting invalid as ordinary road.
    bool Valid = false;
    bool OffPavement = false;
    double MotorMultiplier = 0.0;
    double LateralGripMultiplier = 0.0;
};

class ArcadeSurfaceTraction final
{
public:
    static constexpr double MaximumFrameSeconds = 0.10;
    static constexpr double MaximumWorldCoordinate = 25000.0;
    static constexpr double OffPavementConfirmSeconds = 0.15;
    static constexpr double OnPavementConfirmSeconds = 0.20;
    static constexpr double OffPavementMotorMultiplier = 0.78;
    static constexpr double OffPavementGripMultiplier = 0.72;

    void Reset() noexcept
    {
        OffPavement = false;
        PendingOffPavement = false;
        PendingSeconds = 0.0;
        Valid = false;
    }
    void Suspend() noexcept { Reset(); }

    ArcadeSurfaceOutput Observe(ArcadeSurfaceSample sample) noexcept
    {
        if (!sample.DrivingReady ||
            !std::isfinite(sample.DeltaSeconds) ||
            sample.DeltaSeconds <= 0.0 ||
            sample.DeltaSeconds > MaximumFrameSeconds ||
            !std::isfinite(sample.WorldX) ||
            !std::isfinite(sample.WorldY) ||
            std::abs(sample.WorldX) > MaximumWorldCoordinate ||
            std::abs(sample.WorldY) > MaximumWorldCoordinate)
        {
            Suspend();
            return {};
        }

        const bool sensedOffRoad = !IsOnPavedSurface(
            {sample.WorldX, sample.WorldY});
        if (sensedOffRoad == OffPavement)
        {
            PendingSeconds = 0.0;
            PendingOffPavement = OffPavement;
        }
        else
        {
            if (PendingOffPavement != sensedOffRoad)
            {
                PendingOffPavement = sensedOffRoad;
                PendingSeconds = 0.0;
            }
            PendingSeconds += sample.DeltaSeconds;
            const double threshold = sensedOffRoad
                ? OffPavementConfirmSeconds : OnPavementConfirmSeconds;
            if (PendingSeconds >= threshold)
            {
                OffPavement = sensedOffRoad;
                PendingOffPavement = OffPavement;
                PendingSeconds = 0.0;
            }
        }
        Valid = true;
        return {
            true, OffPavement,
            OffPavement ? OffPavementMotorMultiplier : 1.0,
            OffPavement ? OffPavementGripMultiplier : 1.0
        };
    }

    bool HasValidSample() const noexcept { return Valid; }
    bool IsOffPavement() const noexcept { return Valid && OffPavement; }
    double PendingConfirmationSeconds() const noexcept
    {
        return PendingSeconds;
    }

private:
    bool OffPavement = false;
    bool PendingOffPavement = false;
    double PendingSeconds = 0.0;
    bool Valid = false;
};
} // namespace latebrake
