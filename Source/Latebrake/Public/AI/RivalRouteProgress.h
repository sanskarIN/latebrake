#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Pure C++20 route bookkeeping for the experimental autonomous car.
// Waypoint progress is approximate; it is not an authoritative race result.
struct RouteObservation final
{
    bool Advanced = false;
    bool NeedsRecovery = false;
    bool Completed = false;
};

class RivalRouteProgress final
{
public:
    // Keep the number of autonomous teleports within an observable, bounded
    // session budget. Never request a recovery without recording it.
    static constexpr int MaxRecoveries = 100000;

    // Besides the original stationary trigger, recognize a rival moving
    // against a wall or circling without materially getting closer to its
    // NEXT waypoint. This is approximate AI recovery, never player timing.
    static constexpr double NoApproachRecoverSeconds = 8.0;
    static constexpr double MeaningfulApproachCm = 260.0;
    static constexpr double NoApproachMinSpeedCmPerSecond = 450.0;
    static constexpr double NoApproachMaxFrameSeconds = 0.10;
    static constexpr double MaxQualifiedWaypointDistanceCm = 50000.0;

    explicit RivalRouteProgress(int waypointCount = 4, int requiredLaps = 3) noexcept
        : WaypointCount(std::clamp(waypointCount, 1, 256)),
          RequiredLaps(std::clamp(requiredLaps, 1, 100))
    {
    }

    // Reconfigure when switching quick races/career rounds. All route,
    // recovery and stuck-timer state belongs to the previous attempt and
    // must not leak into the new event's lap count.
    void Configure(int waypointCount, int requiredLaps) noexcept
    {
        WaypointCount = std::clamp(waypointCount, 1, 256);
        RequiredLaps = std::clamp(requiredLaps, 1, 100);
        Reset();
    }

    void Reset() noexcept
    {
        NextWaypoint = 0;
        CompletedLaps = 0;
        StationarySeconds = 0.0;
        ResetApproachWindow();
        Recoveries = 0;
        SuccessfulRecoveries = 0;
        FailedRecoveries = 0;
        RecoveryPending = false;
    }

    RouteObservation Observe(double distanceCm, double speedCmPerSecond,
                             double deltaSeconds) noexcept
    {
        // Until the engine has reported the outcome, a pending recovery
        // cannot silently advance a waypoint or trigger another attempt.
        if (RecoveryPending || IsFinished() ||
            !std::isfinite(distanceCm) || distanceCm < 0.0 ||
            !std::isfinite(speedCmPerSecond) || speedCmPerSecond < 0.0 ||
            !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0)
        {
            ResetApproachWindow();
            return {};
        }

        if (distanceCm <= 440.0)
        {
            StationarySeconds = 0.0;
            ResetApproachWindow();
            ++NextWaypoint;
            if (NextWaypoint == WaypointCount)
            {
                NextWaypoint = 0;
                ++CompletedLaps;
            }
            return {true, false, IsFinished()};
        }

        bool noApproachStall = false;
        if (speedCmPerSecond >= 80.0)
        {
            // Preserve the original low-speed recovery behavior. A moving
            // car can still be stranded if it never gets closer to the
            // waypoint (wheelspin, sideways wall slides, endless circling).
            StationarySeconds = 0.0;
            if (speedCmPerSecond >= NoApproachMinSpeedCmPerSecond &&
                speedCmPerSecond <= 10000.0 &&
                deltaSeconds <= NoApproachMaxFrameSeconds &&
                distanceCm <= MaxQualifiedWaypointDistanceCm)
            {
                if (!HasApproachReference)
                {
                    ApproachReferenceDistanceCm = distanceCm;
                    HasApproachReference = true;
                    NoApproachSeconds = 0.0;
                }
                // Only REAL distance improvement counts, not world speed,
                // sideways slip, reversing or motion away from the gate.
                if (distanceCm <=
                    ApproachReferenceDistanceCm - MeaningfulApproachCm)
                {
                    ApproachReferenceDistanceCm = distanceCm;
                    NoApproachSeconds = 0.0;
                }
                else
                {
                    NoApproachSeconds = std::min(
                        NoApproachRecoverSeconds,
                        NoApproachSeconds + deltaSeconds);
                }
                noApproachStall = NoApproachSeconds >=
                    NoApproachRecoverSeconds - 1.0e-9;
            }
            else
            {
                // A hitch, implausible distance or marginal speed cannot
                // be interpreted as evidence of continuous failed travel.
                ResetApproachWindow();
            }
        }
        else
        {
            ResetApproachWindow();
            // The original eight-second low-speed stall still qualifies,
            // including the existing bounded, synchronously audited
            // attempt path.
            StationarySeconds = std::min(8.0,
                StationarySeconds + deltaSeconds);
        }

        if (StationarySeconds >= 8.0 || noApproachStall)
        {
            StationarySeconds = 0.0;
            ResetApproachWindow();
            // The telemetry counter and the actual teleport request must
            // never diverge. Once the finite budget is exhausted the rival
            // remains stopped instead of receiving free untracked recoveries.
            if (Recoveries >= MaxRecoveries)
            {
                return {};
            }
            ++Recoveries;
            RecoveryPending = true;
            return {false, true, false};
        }
        return {};
    }

    // Caller must synchronously report the actual verified Unreal teleport
    // outcome; repeated or unsolicited results cannot modify telemetry.
    // Every issued request consumes one finite budget slot regardless of
    // success. A failed move may be retried only after a fresh 8s stall.
    bool ReportRecoveryResult(bool enginePlacementVerified) noexcept
    {
        if (!RecoveryPending)
        {
            return false;
        }
        RecoveryPending = false;
        if (enginePlacementVerified)
        {
            ++SuccessfulRecoveries;
        }
        else
        {
            ++FailedRecoveries;
        }
        return true;
    }

    double NoApproachObservationSeconds() const noexcept
    {
        return NoApproachSeconds;
    }
    int GetNextWaypoint() const noexcept { return NextWaypoint; }
    int GetWaypointCount() const noexcept { return WaypointCount; }
    int GetRequiredLaps() const noexcept { return RequiredLaps; }
    int GetCompletedLaps() const noexcept { return CompletedLaps; }
    // Historical recovery count remains the number of ATTEMPTS, not
    // successes; separate audit counters make failed engine moves visible.
    int GetRecoveryCount() const noexcept { return Recoveries; }
    int GetSuccessfulRecoveryCount() const noexcept { return SuccessfulRecoveries; }
    int GetFailedRecoveryCount() const noexcept { return FailedRecoveries; }
    bool HasPendingRecovery() const noexcept { return RecoveryPending; }
    bool IsFinished() const noexcept { return CompletedLaps >= RequiredLaps; }

private:
    void ResetApproachWindow() noexcept
    {
        HasApproachReference = false;
        ApproachReferenceDistanceCm = 0.0;
        NoApproachSeconds = 0.0;
    }

    int WaypointCount;
    int RequiredLaps;
    int NextWaypoint = 0;
    int CompletedLaps = 0;
    int Recoveries = 0;
    int SuccessfulRecoveries = 0;
    int FailedRecoveries = 0;
    bool RecoveryPending = false;
    double StationarySeconds = 0.0;
    double ApproachReferenceDistanceCm = 0.0;
    double NoApproachSeconds = 0.0;
    bool HasApproachReference = false;
};
} // namespace latebrake
