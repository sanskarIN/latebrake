#pragma once

#include "Track/CircuitLayout.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Experimental, single-player arcade pit on the inside of the SOUTH
// straight. It is outside the central AI racing line, but physically on
// the existing paved surface. It is not an authored pit-lane mesh, real
// tire service, or an official racing-series rule.
inline constexpr CircuitPoint RacePitBay{0.0, -1300.0};
inline constexpr double RacePitBayRadiusCm = 250.0;
inline constexpr double RacePitServiceHoldSeconds = 2.5;

struct PitServiceObservation final
{
    double DeltaSeconds = 0.0;
    double X = 0.0;
    double Y = 0.0;
    double PlanarSpeedCmPerSecond = 0.0;
    double RequestedThrottle = 0.0;
    bool ButtonHeld = false;
    bool NitroHeld = false;
    bool Authorized = false;
    bool MaintenanceNeeded = false;
};

inline bool IsInRacePitBay(double x, double y) noexcept
{
    if (!std::isfinite(x) || !std::isfinite(y) ||
        !IsOnPavedSurface({x, y}))
    {
        return false;
    }
    const double dx = x - RacePitBay.X, dy = y - RacePitBay.Y;
    return dx * dx + dy * dy <=
        RacePitBayRadiusCm * RacePitBayRadiusCm;
}

// A single service is earned by a CONTINUOUS 2.5s valid, nearly stationary
// hold inside the bay. Exiting the bay is required to arm another stop.
// This policy changes no score, physics, saved garage, timer, or checkpoint.
class RacePitService final
{
public:
    void Reset() noexcept
    {
        HeldSeconds = 0.0;
        Stops = 0;
        Armed = true;
        InBay = false;
    }
    void Suspend() noexcept
    {
        HeldSeconds = 0.0;
        InBay = false;
    }
    bool Observe(PitServiceObservation sample) noexcept
    {
        if (!std::isfinite(sample.DeltaSeconds) ||
            sample.DeltaSeconds <= 0.0 || sample.DeltaSeconds > 0.10 ||
            !std::isfinite(sample.X) || !std::isfinite(sample.Y) ||
            !std::isfinite(sample.PlanarSpeedCmPerSecond) ||
            sample.PlanarSpeedCmPerSecond < 0.0 ||
            sample.PlanarSpeedCmPerSecond > 10000.0 ||
            !std::isfinite(sample.RequestedThrottle) ||
            std::abs(sample.RequestedThrottle) > 1.0 ||
            !sample.Authorized)
        {
            Suspend();
            return false;
        }
        InBay = IsInRacePitBay(sample.X, sample.Y);
        if (!InBay)
        {
            Armed = true;
            HeldSeconds = 0.0;
            return false;
        }
        if (!Armed || !sample.ButtonHeld || sample.NitroHeld ||
            !sample.MaintenanceNeeded ||
            sample.PlanarSpeedCmPerSecond > 120.0 ||
            std::abs(sample.RequestedThrottle) > 0.05)
        {
            HeldSeconds = 0.0;
            return false;
        }
        HeldSeconds = std::min(
            RacePitServiceHoldSeconds, HeldSeconds + sample.DeltaSeconds);
        if (HeldSeconds + 1.0e-9 < RacePitServiceHoldSeconds)
        {
            return false;
        }
        HeldSeconds = 0.0;
        Armed = false;
        if (Stops < 1000) ++Stops;
        return true;
    }
    bool IsInBay() const noexcept { return InBay; }
    bool IsArmed() const noexcept { return Armed; }
    double ProgressFraction() const noexcept
    {
        return HeldSeconds / RacePitServiceHoldSeconds;
    }
    int StopCount() const noexcept { return Stops; }

private:
    double HeldSeconds = 0.0;
    int Stops = 0;
    bool Armed = true;
    bool InBay = false;
};
} // namespace latebrake
