#pragma once

#include "Track/CircuitLayout.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace latebrake
{
// Allocation-free starter-circuit track-limit rules. The real pavement
// classifier is IsOnPavedSurface, shared with existing drive physics.
// A future continuous circuit needs its own validated surface classifier
// before it is allowed to use this policy; no fake road is inferred.
struct TrackLimitsRules final
{
    static constexpr double MinimumMovingSpeedKmH = 8.0;
    static constexpr double WarningAfterSeconds = 0.6;
    static constexpr double ViolationAfterSeconds = 1.5;
    static constexpr double CleanRejoinSeconds = 0.5;
    static constexpr double MaximumTickSeconds = 0.25;
    static constexpr double PenaltySeconds = 5.0;
    static constexpr int ViolationsPerPenalty = 3;
    static constexpr int MaximumViolations = 9999;
};

enum class TrackLimitsEvent
{
    None,
    WarningStarted,
    ViolationRecorded,
    PenaltyApplied,
    PenaltyRefused,
    CleanRejoin,
    Suspended
};

struct TrackLimitsSnapshot final
{
    int Violations = 0;
    int Penalties = 0;
    double OffPavementMovingSeconds = 0.0;
    bool Warning = false;
    bool OffPavement = false;
    bool PendingCleanRejoin = false;
};

// Race time is changed only by the GameMode through its existing guarded
// RaceProgress::AddTimePenalty; this class never edits saves or rewards.
class TrackLimitsSteward final
{
public:
    // A new race event or accepted RESTART clears prior strikes.
    void Reset() noexcept
    {
        Violations = 0;
        Penalties = 0;
        Latched = false;
        Suspend();
    }

    // Pause, focus loss, hitch, driver swap, physics failure and teleport
    // must not stitch two independent off-pavement episodes together.
    // A previous strike stays latched until a clean paved rejoin.
    void Suspend() noexcept
    {
        OffPavementMovingSeconds = 0.0;
        CleanRejoinElapsed = 0.0;
        Warning = false;
        OffPavement = false;
    }

    TrackLimitsSnapshot Snapshot() const noexcept
    {
        return {Violations, Penalties, OffPavementMovingSeconds,
                Warning, OffPavement, Latched};
    }

    // Caller must provide authenticated local-driver/GO/race-clock
    // eligibility and physical chassis speed, not an input axis.
    // The callback must succeed atomically before a third/sixth strike
    // can be committed; failures cannot produce invented penalty credit.
    template <typename ChargePenalty>
    TrackLimitsEvent Update(CircuitPoint position,
                            double speedKmH,
                            double deltaSeconds,
                            bool raceAuthorized,
                            ChargePenalty&& chargePenalty) noexcept
    {
        if (!raceAuthorized || !std::isfinite(position.X) ||
            !std::isfinite(position.Y) || !std::isfinite(speedKmH) ||
            !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0 ||
            deltaSeconds > TrackLimitsRules::MaximumTickSeconds)
        {
            Suspend();
            return TrackLimitsEvent::Suspended;
        }

        const bool outside = !IsOnPavedSurface(position);
        OffPavement = outside;
        if (!outside)
        {
            OffPavementMovingSeconds = 0.0;
            Warning = false;
            if (Latched)
            {
                CleanRejoinElapsed = std::min(
                    TrackLimitsRules::CleanRejoinSeconds,
                    CleanRejoinElapsed + deltaSeconds);
                if (CleanRejoinElapsed >= TrackLimitsRules::CleanRejoinSeconds)
                {
                    Latched = false;
                    CleanRejoinElapsed = 0.0;
                    return TrackLimitsEvent::CleanRejoin;
                }
            }
            else
            {
                CleanRejoinElapsed = 0.0;
            }
            return TrackLimitsEvent::None;
        }

        // Being stranded without moving offroad is not repeated cutting.
        CleanRejoinElapsed = 0.0;
        if (std::abs(speedKmH) < TrackLimitsRules::MinimumMovingSpeedKmH)
        {
            return TrackLimitsEvent::None;
        }

        OffPavementMovingSeconds = std::min(
            TrackLimitsRules::ViolationAfterSeconds,
            OffPavementMovingSeconds + deltaSeconds);
        if (!Warning &&
            OffPavementMovingSeconds >= TrackLimitsRules::WarningAfterSeconds)
        {
            Warning = true;
            if (OffPavementMovingSeconds < TrackLimitsRules::ViolationAfterSeconds)
            {
                return TrackLimitsEvent::WarningStarted;
            }
        }
        if (Latched ||
            OffPavementMovingSeconds < TrackLimitsRules::ViolationAfterSeconds)
        {
            return TrackLimitsEvent::None;
        }

        // Latch before invoking external scoring: a rejected charge must
        // not be retried every tick while the same car remains on grass.
        Latched = true;
        if (Violations >= TrackLimitsRules::MaximumViolations)
        {
            return TrackLimitsEvent::None;
        }
        const int nextViolation = Violations + 1;
        if (nextViolation % TrackLimitsRules::ViolationsPerPenalty == 0)
        {
            if (!std::forward<ChargePenalty>(chargePenalty)(
                    TrackLimitsRules::PenaltySeconds))
            {
                return TrackLimitsEvent::PenaltyRefused;
            }
            ++Penalties;
            Violations = nextViolation;
            return TrackLimitsEvent::PenaltyApplied;
        }
        Violations = nextViolation;
        return TrackLimitsEvent::ViolationRecorded;
    }

private:
    int Violations = 0;
    int Penalties = 0;
    double OffPavementMovingSeconds = 0.0;
    double CleanRejoinElapsed = 0.0;
    bool Warning = false;
    bool OffPavement = false;
    bool Latched = false;
};
} // namespace latebrake
