#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace latebrake
{
// Read-only observation of the *actual final* human Arcade traction
// intervention. Independent of handling, physics commands and race results.
// Inputs are normalized positive throttle before and after the existing
// measured-yaw spin policy. A new session resets all accumulated telemetry.
struct HumanSpinAssistSample final
{
    double DeltaSeconds = 0.0;
    double CornerThrottle = 0.0;
    double FinalThrottle = 0.0;
    bool EligibleHumanDrive = false;
};

class HumanSpinAssistMeter final
{
public:
    static constexpr double MaxSampleSeconds = 0.10;
    static constexpr double MinDisplayReduction = 0.05;
    static constexpr double MaxExpectedReduction = 0.70;
    static constexpr unsigned MaxInterventions = 65535u;

    void Reset() noexcept
    {
        Suspend();
        PeakFraction = 0.0;
        Interventions = 0;
    }

    // Focus loss, physics interruption, auto/recovery placement and
    // menu transitions clear the current display immediately. Run-local
    // high water and activation count survive ordinary interruptions.
    void Suspend() noexcept
    {
        LiveFraction = 0.0;
        Active = false;
    }

    bool Observe(const HumanSpinAssistSample& s) noexcept
    {
        if (!s.EligibleHumanDrive ||
            !std::isfinite(s.DeltaSeconds) ||
            s.DeltaSeconds <= 0.0 ||
            s.DeltaSeconds > MaxSampleSeconds ||
            !std::isfinite(s.CornerThrottle) ||
            !std::isfinite(s.FinalThrottle) ||
            s.CornerThrottle <= 0.01 || s.CornerThrottle > 1.0 ||
            s.FinalThrottle < 0.0 || s.FinalThrottle > s.CornerThrottle)
        {
            Suspend();
            return false;
        }
        const double reduction =
            (s.CornerThrottle - s.FinalThrottle) / s.CornerThrottle;
        if (!std::isfinite(reduction) ||
            reduction < 0.0 || reduction > MaxExpectedReduction + 1e-9)
        {
            Suspend();
            return false;
        }
        const double fraction =
            std::clamp(reduction, 0.0, MaxExpectedReduction);
        const bool nowActive = fraction >= MinDisplayReduction;
        if (nowActive && !Active && Interventions < MaxInterventions)
        {
            ++Interventions;
        }
        Active = nowActive;
        LiveFraction = nowActive ? fraction : 0.0;
        if (nowActive)
        {
            PeakFraction = std::max(PeakFraction, LiveFraction);
        }
        return true;
    }

    bool IsActive() const noexcept { return Active; }
    double CurrentReductionFraction() const noexcept { return LiveFraction; }
    double PeakReductionFraction() const noexcept { return PeakFraction; }
    unsigned InterventionCount() const noexcept { return Interventions; }

private:
    double LiveFraction = 0.0;
    double PeakFraction = 0.0;
    unsigned Interventions = 0;
    bool Active = false;
};
} // namespace latebrake
