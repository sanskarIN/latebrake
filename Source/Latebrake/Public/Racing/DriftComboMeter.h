#pragma once

#include "Racing/DrivingSessionStats.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace latebrake
{
// Purely optional, session-local ARCADE drift scoring from observed chassis
// movement. Never influences tire forces, safety controls, checkpoint timing,
// credits, career medals, achievements, leaderboards, or save data.
class DriftComboMeter final
{
public:
    void Reset() noexcept
    {
        BankedScore = 0.0;
        BestScore = 0.0;
        BankedDriftSeconds = 0.0;
        CurrentSeconds = 0.0;
        CurrentScore = 0.0;
        CandidateSeconds = 0.0;
        GapSeconds = 0.0;
        CompletedCombos = 0;
        Active = false;
    }

    // On pause, focus loss, recovery, finish or invalid sampling, end the
    // combo. Earlier advisory points are retained until the next race reset.
    void Suspend() noexcept
    {
        if (Active)
        {
            BankedScore = std::min(MaxScore, BankedScore + CurrentScore);
            BankedDriftSeconds = std::min(
                MaxSessionSeconds, BankedDriftSeconds + CurrentSeconds);
            BestScore = std::max(BestScore, CurrentScore);
        }
        Active = false;
        CandidateSeconds = 0.0;
        GapSeconds = 0.0;
        CurrentSeconds = 0.0;
        CurrentScore = 0.0;
    }

    bool Observe(DrivingSample sample) noexcept
    {
        // Mirror production chassis observation limits, plus a physical
        // consistency guard: sideways velocity cannot exceed planar speed.
        if (!std::isfinite(sample.DeltaSeconds) ||
            sample.DeltaSeconds <= 0.0 || sample.DeltaSeconds > 0.25 ||
            !std::isfinite(sample.PlanarSpeedCmPerSecond) ||
            sample.PlanarSpeedCmPerSecond < 0.0 ||
            sample.PlanarSpeedCmPerSecond > 10000.0 ||
            !std::isfinite(sample.LateralSpeedCmPerSecond) ||
            std::abs(sample.LateralSpeedCmPerSecond) > 10000.0 ||
            std::abs(sample.LateralSpeedCmPerSecond) >
                sample.PlanarSpeedCmPerSecond + 0.01)
        {
            Suspend();
            return false;
        }

        const double speed = sample.PlanarSpeedCmPerSecond;
        const double sideways = std::abs(sample.LateralSpeedCmPerSecond);
        const double slipRatio = speed > 0.0 ? sideways / speed : 0.0;
        const bool drifting =
            speed >= 900.0 && sideways >= 450.0 && slipRatio >= 0.35;
        if (!drifting)
        {
            if (CandidateSeconds > 0.0)
            {
                GapSeconds += sample.DeltaSeconds;
                if (GapSeconds >= 0.3)
                {
                    Suspend();
                }
            }
            return true;
        }

        GapSeconds = 0.0;
        if (!Active)
        {
            CandidateSeconds += sample.DeltaSeconds;
            // A single slip impulse, braking frame or corrective wobble
            // must not become a credited drift combo.
            if (CandidateSeconds >= 0.55)
            {
                Active = true;
                if (CompletedCombos < std::numeric_limits<int>::max())
                {
                    ++CompletedCombos;
                }
                CurrentSeconds = CandidateSeconds;
                CurrentScore = std::min(MaxScore, CandidateSeconds * 100.0);
            }
        }
        else
        {
            CurrentSeconds = std::min(
                MaxSessionSeconds, CurrentSeconds + sample.DeltaSeconds);
            const double multiplier = std::min(
                3.0, 1.0 + 0.25 * std::floor(CurrentSeconds / 3.0));
            const double gain = sample.DeltaSeconds *
                (100.0 + 120.0 * slipRatio) * multiplier;
            CurrentScore = std::min(MaxScore, CurrentScore + gain);
        }
        return true;
    }

    bool HasActiveCombo() const noexcept { return Active; }
    int GetComboCount() const noexcept { return CompletedCombos; }
    double GetCurrentComboPoints() const noexcept
    {
        return Active ? CurrentScore : 0.0;
    }
    double GetCurrentComboSeconds() const noexcept
    {
        return Active ? CurrentSeconds : 0.0;
    }
    double GetBestComboPoints() const noexcept
    {
        return std::max(BestScore, GetCurrentComboPoints());
    }
    double GetTotalPoints() const noexcept
    {
        return std::min(MaxScore, BankedScore + GetCurrentComboPoints());
    }
    double GetTotalDriftSeconds() const noexcept
    {
        return std::min(
            MaxSessionSeconds, BankedDriftSeconds + GetCurrentComboSeconds());
    }

private:
    static constexpr double MaxScore = 1000000000.0;
    static constexpr double MaxSessionSeconds = 10000000.0;
    double BankedScore = 0.0;
    double BestScore = 0.0;
    double BankedDriftSeconds = 0.0;
    double CurrentSeconds = 0.0;
    double CurrentScore = 0.0;
    double CandidateSeconds = 0.0;
    double GapSeconds = 0.0;
    int CompletedCombos = 0;
    bool Active = false;
};
} // namespace latebrake
