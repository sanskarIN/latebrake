#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

namespace latebrake
{
// Visual-only, run-local ghost. No physics, gate, lap-score, network,
// progression, replay-file or credit authority is derived from these poses.
struct LapGhostPose final
{
    double Seconds = 0.0;
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;
    double YawDegrees = 0.0;
};

class BestLapGhost final
{
public:
    // Fixed-capacity storage: no tick-time allocations or external assets.
    // 10 Hz recording for at most 360 seconds (4,096 slots per trace).
    static constexpr std::size_t MaxSamples = 4096;
    static constexpr double MaxLapSeconds = 360.0;

    void ResetRun() noexcept
    {
        BestCount = 0;
        BestSeconds = 0.0;
        StartLap();
    }

    // Called only after a genuine ordered lap completion. Never try to
    // stitch a poisoned recovery/pause frame into the next candidate.
    void StartLap() noexcept
    {
        CandidateCount = 0;
        CandidateAllowed = true;
    }

    void InvalidateCurrentLap() noexcept
    {
        CandidateAllowed = false;
        CandidateCount = 0;
    }

    bool Record(const LapGhostPose& pose, bool authorizedTick) noexcept
    {
        if (!CandidateAllowed || !authorizedTick || !IsFinitePose(pose) ||
            pose.Seconds <= 0.0 || pose.Seconds > MaxLapSeconds)
        {
            InvalidateCurrentLap();
            return false;
        }

        if (CandidateCount == 0)
        {
            if (pose.Seconds > 0.45)
            {
                InvalidateCurrentLap();
                return false;
            }
        }
        else
        {
            const LapGhostPose& last = Candidate[CandidateCount - 1];
            const double dt = pose.Seconds - last.Seconds;
            if (!std::isfinite(dt) || dt < 0.0 || dt > 0.45)
            {
                InvalidateCurrentLap();
                return false;
            }
            // Unreal may Tick several times between 10-Hz keyframes.
            if (dt < 0.09)
            {
                return true;
            }
            const double dx = pose.X - last.X;
            const double dy = pose.Y - last.Y;
            const double dz = pose.Z - last.Z;
            const double distance = std::hypot(std::hypot(dx, dy), dz);
            // Conservative real-motion envelope, not a race speed limit.
            // This detects teleports or corrupt transforms, not driving.
            if (!std::isfinite(distance) || distance > dt * 15000.0 + 200.0)
            {
                InvalidateCurrentLap();
                return false;
            }
        }
        if (CandidateCount >= MaxSamples)
        {
            InvalidateCurrentLap();
            return false;
        }
        Candidate[CandidateCount++] = pose;
        return true;
    }

    // Only GameMode's successful ordered-gate lap transition may call
    // this. activeTiming is backed by RaceProgress, not this recorder.
    bool FinishLap(double officialLapSeconds, bool activeTiming,
                   bool penaltyFree) noexcept
    {
        const bool complete =
            CandidateAllowed && activeTiming && penaltyFree &&
            std::isfinite(officialLapSeconds) &&
            officialLapSeconds > 1.0 &&
            officialLapSeconds <= MaxLapSeconds &&
            CandidateCount >= 8 &&
            Candidate[CandidateCount - 1].Seconds <= officialLapSeconds &&
            officialLapSeconds - Candidate[CandidateCount - 1].Seconds <= 0.45;
        bool newBest = false;
        if (complete && (BestCount == 0 || officialLapSeconds < BestSeconds))
        {
            Best = Candidate;
            BestCount = CandidateCount;
            BestSeconds = officialLapSeconds;
            newBest = true;
        }
        StartLap();
        return newBest;
    }

    bool Evaluate(double lapSeconds, LapGhostPose& out) const noexcept
    {
        out = {};
        if (BestCount < 8 || !std::isfinite(lapSeconds) ||
            lapSeconds < 0.0 || lapSeconds > BestSeconds)
        {
            return false;
        }
        if (lapSeconds <= Best[0].Seconds)
        {
            out = Best[0];
            return true;
        }
        if (lapSeconds >= Best[BestCount - 1].Seconds)
        {
            out = Best[BestCount - 1];
            return true;
        }

        // Coherent monotone time keys; binary search is bounded to 12
        // comparisons at MaxSamples, even on very high refresh rates.
        std::size_t lower = 0;
        std::size_t upper = BestCount - 1;
        while (upper - lower > 1)
        {
            const std::size_t middle = lower + (upper - lower) / 2;
            if (Best[middle].Seconds <= lapSeconds)
            {
                lower = middle;
            }
            else
            {
                upper = middle;
            }
        }
        const LapGhostPose& a = Best[lower];
        const LapGhostPose& b = Best[upper];
        const double span = b.Seconds - a.Seconds;
        if (!std::isfinite(span) || span <= 0.0)
        {
            return false;
        }
        const double alpha = (lapSeconds - a.Seconds) / span;
        const double dyaw = std::remainder(b.YawDegrees - a.YawDegrees, 360.0);
        out = {lapSeconds,
            a.X + (b.X - a.X) * alpha,
            a.Y + (b.Y - a.Y) * alpha,
            a.Z + (b.Z - a.Z) * alpha,
            std::remainder(a.YawDegrees + dyaw * alpha, 360.0)};
        return IsFinitePose(out);
    }

    bool HasBest() const noexcept { return BestCount >= 8; }
    bool IsCandidateAllowed() const noexcept { return CandidateAllowed; }
    std::size_t GetCandidateCount() const noexcept { return CandidateCount; }
    std::size_t GetBestCount() const noexcept { return BestCount; }
    double GetBestSeconds() const noexcept { return BestSeconds; }

private:
    static bool IsFinitePose(const LapGhostPose& p) noexcept
    {
        constexpr double WorldBound = 500000.0;
        return std::isfinite(p.Seconds) && std::isfinite(p.X) &&
            std::isfinite(p.Y) && std::isfinite(p.Z) &&
            std::isfinite(p.YawDegrees) &&
            std::abs(p.X) <= WorldBound &&
            std::abs(p.Y) <= WorldBound &&
            std::abs(p.Z) <= WorldBound &&
            std::abs(p.YawDegrees) <= 3600.0;
    }

    std::array<LapGhostPose, MaxSamples> Candidate{};
    std::array<LapGhostPose, MaxSamples> Best{};
    std::size_t CandidateCount = 0;
    std::size_t BestCount = 0;
    double BestSeconds = 0.0;
    bool CandidateAllowed = true;
};
} // namespace latebrake
