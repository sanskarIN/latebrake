#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Deterministic, clock-independent START 3 / 2 / 1 / GO gate.
//
// The caller must supply the same real-game readiness used for checkpoint
// scoring, input and the authoritative race clock. Invalid frames, menu,
// focus loss, pause and unverified placement never consume countdown time.
// No UE types, dynamic allocations, timer handles or save data live here.
enum class RaceStartPhase
{
    Three,
    Two,
    One,
    Go,
    Racing
};

class RaceStartLights final
{
public:
    static constexpr double CountdownSeconds = 3.0;
    static constexpr double GoVisibleSeconds = 0.85;
    static constexpr double MaxAcceptedFrameSeconds = 0.5;

    void Reset() noexcept
    {
        ElapsedCountdownSeconds = 0.0;
        TimeSinceGreenSeconds = 0.0;
        Green = false;
    }

    bool IsGreen() const noexcept { return Green; }
    double SecondsUntilGreen() const noexcept
    {
        return Green ? 0.0 : CountdownSeconds - ElapsedCountdownSeconds;
    }

    RaceStartPhase Phase() const noexcept
    {
        if (!Green)
        {
            if (ElapsedCountdownSeconds < 1.0) return RaceStartPhase::Three;
            if (ElapsedCountdownSeconds < 2.0) return RaceStartPhase::Two;
            return RaceStartPhase::One;
        }
        return TimeSinceGreenSeconds < GoVisibleSeconds
            ? RaceStartPhase::Go : RaceStartPhase::Racing;
    }

    // Returns ONLY the fraction of this eligible frame after GO, suitable
    // for direct forwarding to RaceProgress::Advance. A single frame which
    // crosses the start line never receives the full pre-green duration.
    double Advance(double deltaSeconds, bool raceReady) noexcept
    {
        if (!raceReady || !std::isfinite(deltaSeconds) ||
            deltaSeconds <= 0.0)
        {
            return 0.0;
        }

        // Once the race is live, a genuine positive frame hitch must
        // still count toward the official clock. Applying the red-light
        // anti-skip cap AFTER green would silently undercount a driven
        // lap when the renderer stutters or the device drops frames.
        if (Green)
        {
            TimeSinceGreenSeconds = std::min(
                GoVisibleSeconds, TimeSinceGreenSeconds + deltaSeconds);
            return deltaSeconds;
        }

        // Only the still-red startup countdown rejects an implausible
        // large frame; this prevents an artificial jump straight to GO.
        if (deltaSeconds > MaxAcceptedFrameSeconds)
        {
            return 0.0;
        }

        const double remaining = CountdownSeconds - ElapsedCountdownSeconds;
        if (deltaSeconds < remaining)
        {
            ElapsedCountdownSeconds += deltaSeconds;
            return 0.0;
        }

        // At equality, GO opens on this frame but zero game time was
        // available after the green light. Never invent a timed lap.
        const double liveSeconds = deltaSeconds - remaining;
        Green = true;
        ElapsedCountdownSeconds = CountdownSeconds;
        TimeSinceGreenSeconds = std::min(GoVisibleSeconds, liveSeconds);
        return liveSeconds;
    }

private:
    double ElapsedCountdownSeconds = 0.0;
    double TimeSinceGreenSeconds = 0.0;
    bool Green = false;
};
} // namespace latebrake
