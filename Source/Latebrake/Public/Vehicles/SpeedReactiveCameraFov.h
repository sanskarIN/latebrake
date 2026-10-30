#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Purely visual, bounded FOV widening based on actual positive longitudinal
// chassis speed. No authority over driving forces, races, lap timing or saves.
class SpeedReactiveCameraFov final
{
public:
    void Reset() noexcept
    {
        Active = false;
        Current = 0.0;
    }

    bool HasActiveSample() const noexcept { return Active; }

    double Sample(double baseFovDegrees, double forwardSpeedKmH,
                  double frameSeconds, bool eligible, bool lookingBack) noexcept
    {
        // Camera presets are validated independently. Do not return a NaN
        // to Unreal even if a future/malformed caller violates that contract.
        if (!std::isfinite(baseFovDegrees) ||
            baseFovDegrees < 60.0 || baseFovDegrees > 105.0)
        {
            Reset();
            return 90.0;
        }
        if (!eligible || lookingBack ||
            !std::isfinite(forwardSpeedKmH) || forwardSpeedKmH < 0.0 ||
            forwardSpeedKmH > 300.0 ||
            !std::isfinite(frameSeconds) || frameSeconds <= 0.0 ||
            frameSeconds > 0.25)
        {
            Reset();
            return baseFovDegrees;
        }

        // A six-degree maximum avoids aggressive, disorienting FOV changes.
        // Start widening at road speeds and saturate by racing speed.
        const double fraction = std::clamp(
            (forwardSpeedKmH - 40.0) / 150.0, 0.0, 1.0);
        const double extra = std::min(6.0, 105.0 - baseFovDegrees);
        const double target = baseFovDegrees + fraction * extra;

        if (!Active)
        {
            Current = baseFovDegrees;
            Active = true;
        }
        // Exponential frame-rate-independent smoothing (0.3s response).
        const double response = 1.0 - std::exp(-frameSeconds / 0.3);
        Current += response * (target - Current);
        Current = std::clamp(Current, baseFovDegrees,
                             baseFovDegrees + extra);
        return Current;
    }

private:
    double Current = 0.0;
    bool Active = false;
};
} // namespace latebrake
