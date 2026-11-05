#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Lightweight, per-car NON-SHIPPING observation cadence. This is not a
// physics solver, vehicle controller or automatic gameplay QA verdict.
// Keep the existing v=1 log schema so the offline inspector stays strict.
//
// Capture the first valid reading, a changed supported-wheel mask or
// grid-to-GO phase at most 5 times/second, plus a heartbeat every 2 seconds.
// Changes are observed EVERY valid Tick; a change that reverses in under
// 200 ms cannot be reconstructed from sampled diagnostic logs.
inline constexpr double SuspensionQaMinimumEventSeconds = 0.20;
inline constexpr double SuspensionQaHeartbeatSeconds = 2.0;
inline constexpr double SuspensionQaMaximumFrameSeconds = 0.10;

class SuspensionQaCadence final
{
public:
    void Reset() noexcept
    {
        SecondsSinceReport = 0.0;
        LastLoadMask = 0;
        bLastGrid = false;
        bHasReport = false;
    }

    bool ShouldReport(double deltaSeconds, unsigned loadedCornerMask,
                      bool isGrid) noexcept
    {
        // A malformed frame is not valid telemetry. Clear any cached
        // observations so a fresh valid session begins with a report.
        if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0 ||
            deltaSeconds > SuspensionQaMaximumFrameSeconds ||
            loadedCornerMask > 0x0Fu)
        {
            Reset();
            return false;
        }

        if (!bHasReport)
        {
            Commit(loadedCornerMask, isGrid);
            return true;
        }

        SecondsSinceReport = std::min(
            SuspensionQaHeartbeatSeconds, SecondsSinceReport + deltaSeconds);
        if (SecondsSinceReport < SuspensionQaMinimumEventSeconds)
        {
            return false;
        }

        const bool changed = loadedCornerMask != LastLoadMask ||
                             isGrid != bLastGrid;
        if (!changed && SecondsSinceReport < SuspensionQaHeartbeatSeconds)
        {
            return false;
        }

        Commit(loadedCornerMask, isGrid);
        return true;
    }

private:
    void Commit(unsigned mask, bool grid) noexcept
    {
        LastLoadMask = mask;
        bLastGrid = grid;
        bHasReport = true;
        SecondsSinceReport = 0.0;
    }

    double SecondsSinceReport = 0.0;
    unsigned LastLoadMask = 0;
    bool bLastGrid = false;
    bool bHasReport = false;
};
} // namespace latebrake
