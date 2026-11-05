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
        LastObservedLoadMask = 0;
        bLastGrid = false;
        bLastObservedGrid = false;
        bPendingTransition = false;
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

        // Preserve any brief load-loss/recontact or grid/GO edge observed
        // DURING the cooldown. A noisy 120 Hz alternating load mask may
        // equal the last reported mask on every 200 ms boundary; looking
        // only at that boundary would otherwise erase every real edge.
        bPendingTransition = bPendingTransition ||
            loadedCornerMask != LastObservedLoadMask ||
            isGrid != bLastObservedGrid;
        LastObservedLoadMask = loadedCornerMask;
        bLastObservedGrid = isGrid;
        SecondsSinceReport = std::min(
            SuspensionQaHeartbeatSeconds, SecondsSinceReport + deltaSeconds);
        if (SecondsSinceReport + 1.0e-12 < SuspensionQaMinimumEventSeconds)
        {
            return false;
        }

        const bool changed = bPendingTransition ||
                             loadedCornerMask != LastLoadMask ||
                             isGrid != bLastGrid;
        if (!changed && SecondsSinceReport + 1.0e-12 < SuspensionQaHeartbeatSeconds)
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
        LastObservedLoadMask = mask;
        bLastGrid = grid;
        bLastObservedGrid = grid;
        bPendingTransition = false;
        bHasReport = true;
        SecondsSinceReport = 0.0;
    }

    double SecondsSinceReport = 0.0;
    unsigned LastLoadMask = 0;
    unsigned LastObservedLoadMask = 0;
    bool bLastGrid = false;
    bool bLastObservedGrid = false;
    bool bPendingTransition = false;
    bool bHasReport = false;
};
} // namespace latebrake
