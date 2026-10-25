#pragma once

#include "Track/RaceMiniMap.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Advisory ONLY: measured change in direct planar rival separation.
// This is NOT relative vehicle velocity, race order, collision prediction,
// time-to-contact, lap delta or authoritative scoring data.
enum class RivalRelativeTrend
{
    None,
    Closing,
    Opening
};

struct RivalRelativeMotionAdvice final
{
    RivalRelativeTrend Trend = RivalRelativeTrend::None;
    double DirectDistanceMeters = 0.0;
    double SeparationChangeMetersPerSecond = 0.0;
};

class RivalRelativeMotionTracker final
{
public:
    void Reset() noexcept
    {
        HavePrevious = false;
        PreviousDistance = 0.0;
        Candidate = RivalRelativeTrend::None;
        Confirmed = RivalRelativeTrend::None;
        CandidateSeconds = 0.0;
        SmoothedSpeed = 0.0;
    }

    RivalRelativeMotionAdvice Update(
        const RaceMiniMapSnapshot& radar,
        double deltaSeconds, bool raceEligible) noexcept
    {
        // A gap, teleport, missing physics, ambiguous radar point or
        // UI suspension must never be interpreted as rival motion.
        if (!raceEligible || !std::isfinite(deltaSeconds) ||
            deltaSeconds < 0.001 || deltaSeconds > 0.25 ||
            !radar.Player.Position.Valid || !radar.Rival.Valid ||
            !radar.DirectRivalSeparationValid ||
            !std::isfinite(radar.DirectRivalSeparationMeters) ||
            radar.DirectRivalSeparationMeters < 0.0 ||
            radar.DirectRivalSeparationMeters > 100.0)
        {
            Reset();
            return {};
        }

        const double distance = radar.DirectRivalSeparationMeters;
        if (!HavePrevious)
        {
            HavePrevious = true;
            PreviousDistance = distance;
            return {};
        }

        // Positive means the direct planar distance is decreasing, not
        // that the rival is behind/ahead or travelling faster.
        const double radial = (PreviousDistance - distance) / deltaSeconds;
        PreviousDistance = distance;
        // Reject discontinuities rather than describing jumps as >300km/h
        // apparent relative speed or continuing a previous warning.
        if (!std::isfinite(radial) || std::abs(radial) > 90.0)
        {
            Reset();
            return {};
        }

        constexpr double MinMotionMetersPerSecond = 2.0;
        constexpr double ConfirmSeconds = 0.4;
        RivalRelativeTrend next = RivalRelativeTrend::None;
        if (radial >= MinMotionMetersPerSecond)
        {
            next = RivalRelativeTrend::Closing;
        }
        else if (radial <= -MinMotionMetersPerSecond)
        {
            next = RivalRelativeTrend::Opening;
        }

        if (next == RivalRelativeTrend::None)
        {
            Candidate = RivalRelativeTrend::None;
            Confirmed = RivalRelativeTrend::None;
            CandidateSeconds = 0.0;
            SmoothedSpeed = 0.0;
            return {};
        }

        if (Candidate != next)
        {
            Candidate = next;
            Confirmed = RivalRelativeTrend::None;
            CandidateSeconds = 0.0;
            SmoothedSpeed = std::abs(radial);
        }
        else
        {
            // Bounded exponential smoothing dampens the visible readout.
            SmoothedSpeed = SmoothedSpeed * 0.6 + std::abs(radial) * 0.4;
        }
        CandidateSeconds = std::min(ConfirmSeconds, CandidateSeconds + deltaSeconds);
        if (CandidateSeconds >= ConfirmSeconds)
        {
            Confirmed = Candidate;
        }
        return Confirmed == RivalRelativeTrend::None
            ? RivalRelativeMotionAdvice{}
            : RivalRelativeMotionAdvice{Confirmed, distance, SmoothedSpeed};
    }

    RivalRelativeTrend GetConfirmedTrend() const noexcept { return Confirmed; }
    double GetCandidateSeconds() const noexcept { return CandidateSeconds; }

private:
    bool HavePrevious = false;
    double PreviousDistance = 0.0;
    RivalRelativeTrend Candidate = RivalRelativeTrend::None;
    RivalRelativeTrend Confirmed = RivalRelativeTrend::None;
    double CandidateSeconds = 0.0;
    double SmoothedSpeed = 0.0;
};
} // namespace latebrake
