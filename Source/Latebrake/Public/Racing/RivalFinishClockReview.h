#pragma once

#include <cmath>

namespace latebrake
{
// Conservative, session-only comparison of observed Quick Race final-gate
// events on the *shared active* post-GO player clock (recovery fees removed).
// This is NOT official race classification: actor tick order, physics-frame
// sampling and unverified rival state prevent certified place/winner claims.
enum class RivalFinishClockState
{
    Unavailable,
    RivalEarlierObserved,
    NearSimultaneousUncertain
};

struct RivalFinishClockComparison final
{
    RivalFinishClockState State = RivalFinishClockState::Unavailable;
    // Positive difference in the sampled ACTIVE clock: rival's last
    // verified gate was observed earlier than the player's last gate.
    double RivalEarlierSeconds = 0.0;
};

class RivalFinishClockReview final
{
public:
    static constexpr double MaxActiveClockSeconds = 7.0 * 24.0 * 3600.0;
    // Do not classify close frame-to-frame callback ordering as a timing
    // lead; this is intentionally larger than normal frame duration.
    static constexpr double NearFinishToleranceSeconds = 0.25;

    void Configure(int checkpointCount, int requiredLaps) noexcept
    {
        TotalGates = checkpointCount >= 1 && checkpointCount <= 256 &&
                     requiredLaps >= 1 && requiredLaps <= 100
            ? checkpointCount * requiredLaps : 0;
        Reset();
    }

    void Reset() noexcept
    {
        RivalFinalGateObserved = false;
        RivalFinalActiveSeconds = 0.0;
    }

    // Invoked ONLY after the actual rival's forward-gate trace and
    // RivalMatchedGateSplits::RecordRival both accepted this ordinal.
    // Reject corrupt event, absent readiness, bad time and duplicate final
    // callbacks. Never rewrites a first qualified observation.
    bool RecordRivalAcceptedGate(
        int ordinal, double activeSeconds, bool qualified) noexcept
    {
        if (!qualified || !TotalGates || ordinal != TotalGates ||
            RivalFinalGateObserved || !ValidTime(activeSeconds))
        {
            return false;
        }
        RivalFinalActiveSeconds = activeSeconds;
        RivalFinalGateObserved = true;
        return true;
    }

    RivalFinishClockComparison CompareAtPlayerFinish(
        int playerFinalOrdinal, double playerActiveSeconds,
        bool qualifiedPlayerFinish, bool qualifiedRivalFinish) const noexcept
    {
        if (!qualifiedPlayerFinish || !qualifiedRivalFinish ||
            !RivalFinalGateObserved || !TotalGates ||
            playerFinalOrdinal != TotalGates ||
            !ValidTime(playerActiveSeconds) ||
            // A backward clock or nonsensical wall sequence is not a win.
            playerActiveSeconds < RivalFinalActiveSeconds)
        {
            return {};
        }
        const double difference =
            playerActiveSeconds - RivalFinalActiveSeconds;
        if (!std::isfinite(difference))
        {
            return {};
        }
        if (difference <= NearFinishToleranceSeconds)
        {
            // Equal sampled clock values may mean same-frame callbacks.
            return {RivalFinishClockState::NearSimultaneousUncertain, 0.0};
        }
        return {RivalFinishClockState::RivalEarlierObserved, difference};
    }

    bool HasRivalFinalGate() const noexcept
    {
        return RivalFinalGateObserved;
    }

private:
    static bool ValidTime(double seconds) noexcept
    {
        return std::isfinite(seconds) && seconds > 0.0 &&
               seconds <= MaxActiveClockSeconds;
    }

    int TotalGates = 0;
    bool RivalFinalGateObserved = false;
    double RivalFinalActiveSeconds = 0.0;
};
} // namespace latebrake
