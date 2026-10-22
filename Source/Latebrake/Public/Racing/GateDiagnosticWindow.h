#pragma once

#include "Racing/GateDiagnosticCounters.h"

#include <cmath>

namespace latebrake
{
// One bounded QA reporting interval. Runs without the Unreal SDK and does
// not collect player identity, world positions, or a per-frame event buffer.
//
// Record() returns true once ten seconds of VALID, bounded frame-time
// samples have accrued. The caller can inspect Counts(), emit an explicitly
// opted-in local report, and Reset() for the next interval.
// Large hitches count at most one second so a stalled frame cannot generate
// a misleading "ten seconds of driving" QA summary by itself.
class GateDiagnosticWindow final
{
public:
    static constexpr double IntervalSeconds = 10.0;
    static constexpr double MaxFrameSeconds = 1.0;

    bool Record(CheckpointTraceResult result, double deltaSeconds) noexcept
    {
        Statistics.Observe(result);
        if (std::isfinite(deltaSeconds) && deltaSeconds > 0.0)
        {
            const double bounded = deltaSeconds > MaxFrameSeconds
                ? MaxFrameSeconds : deltaSeconds;
            const double updated = Elapsed + bounded;
            Elapsed = updated >= IntervalSeconds ? IntervalSeconds : updated;
        }
        return IsReady();
    }

    bool IsReady() const noexcept { return Elapsed >= IntervalSeconds; }
    double ElapsedSeconds() const noexcept { return Elapsed; }

    const GateDiagnosticCounters& Counts() const noexcept
    {
        return Statistics;
    }

    void Reset() noexcept
    {
        Statistics.Reset();
        Elapsed = 0.0;
    }

private:
    GateDiagnosticCounters Statistics{};
    double Elapsed = 0.0;
};
} // namespace latebrake
