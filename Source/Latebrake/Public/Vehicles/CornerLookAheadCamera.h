#pragma once

#include "Vehicles/CameraPresets.h"
#include <algorithm>
#include <cmath>

namespace latebrake
{
// Visual-only camera yaw lead for the possessed human driver's chase views.
// Steering is a requested view direction, not a steering-physics multiplier.
// It cannot affect RaceProgress, input ownership, saves or rewards.
class CornerLookAheadCamera final
{
public:
    void Reset() noexcept
    {
        Active = false;
        YawDegrees = 0.0;
    }

    bool HasActiveSample() const noexcept { return Active; }

    double Sample(double forwardSpeedCmPerSecond, double steeringAxis,
                  double frameSeconds, bool humanDriver, bool lookingBack,
                  CameraView view) noexcept
    {
        // Pauses, reverse, hood/rear view, corrupt chassis samples and
        // long stalls return to neutral. Never pass NaN to Unreal's arm.
        if (!humanDriver || lookingBack || view == CameraView::Hood ||
            !std::isfinite(forwardSpeedCmPerSecond) ||
            forwardSpeedCmPerSecond < 0.0 ||
            forwardSpeedCmPerSecond > 20000.0 ||
            !std::isfinite(steeringAxis) || std::abs(steeringAxis) > 1.01 ||
            !std::isfinite(frameSeconds) || frameSeconds <= 0.0 ||
            frameSeconds > 0.25)
        {
            Reset();
            return 0.0;
        }

        // No camera turn below ~12.6 km/h; full effect by ~91.8 km/h.
        // High chase moves less to preserve route visibility.
        const double maxYaw = view == CameraView::High ? 4.0 : 6.0;
        const double speedWeight = std::clamp(
            (forwardSpeedCmPerSecond - 350.0) / 2200.0, 0.0, 1.0);
        const double target = std::clamp(steeringAxis, -1.0, 1.0)
            * maxYaw * speedWeight;
        if (!Active)
        {
            Active = true;
            YawDegrees = 0.0;
        }
        // Exponential time constant: equal elapsed time gives identical
        // visual easing at 30/60/120 Hz, with no Chaos dependency.
        const double response = 1.0 - std::exp(-frameSeconds / 0.24);
        YawDegrees += response * (target - YawDegrees);
        YawDegrees = std::clamp(YawDegrees, -maxYaw, maxYaw);
        return YawDegrees;
    }

private:
    double YawDegrees = 0.0;
    bool Active = false;
};
} // namespace latebrake
