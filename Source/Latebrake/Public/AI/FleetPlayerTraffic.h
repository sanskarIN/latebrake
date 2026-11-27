#pragma once

#include "AI/ExperimentalFleetLiveRecovery.h"
#include "AI/RivalFleetCollisionForecast.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>

namespace latebrake
{
// Player-facing proximity information from all five real rivals. This is
// deliberately independent of the AI brake supervisor: a rear threat must
// be visible to the player even though the AI would not brake for it.
// All projections are planar, constant measured velocity, padded chassis.
// No actual contact, line of sight, collision-free route or rank is claimed.
inline constexpr double FleetPlayerNearbyCm = 850.0;
inline constexpr double FleetPlayerApproachRangeCm = 1800.0;
inline constexpr double FleetPlayerApproachMinClosingCmS = 200.0;
inline constexpr double FleetPlayerTrafficMaxFrameSeconds = 0.10;
inline constexpr double FleetPlayerTrafficClockToleranceSeconds = 1.0e-5;
inline constexpr double FleetPlayerTrafficMaxClockSeconds = 604800.0;

enum class FleetPlayerTrafficDirection
{
    Unavailable,
    Ahead,
    AheadRight,
    Right,
    BehindRight,
    Behind,
    BehindLeft,
    Left,
    AheadLeft,
    SameCentre
};

enum class FleetPlayerTrafficSeverity
{
    Unavailable,
    Clear,
    Nearby,
    Approaching,
    ProjectedOverlap,
    PaddedOverlap
};

struct FleetPlayerTrafficRow final
{
    int Slot = -1;
    FleetPlayerTrafficDirection Direction = FleetPlayerTrafficDirection::Unavailable;
    FleetPlayerTrafficSeverity Severity = FleetPlayerTrafficSeverity::Unavailable;
    double CentreDistanceCm = 0.0;
    double ForwardCm = 0.0;
    double RightCm = 0.0;
    double RadialClosingCmS = 0.0;
    bool Nearby = false;
    bool ForecastOverlap = false;
    bool PaddedOverlap = false;
    double ForecastSeconds = 0.0;
};

struct FleetPlayerTrafficReadout final
{
    bool Valid = false;
    std::uint64_t Attempt = 0;
    std::uint64_t Frame = 0;
    int NearestSlot = -1;
    int StrongestSlot = -1;
    int NearbyCount = 0;
    int WarningCount = 0;
    int ProjectedOverlapCount = 0;
    int PaddedOverlapCount = 0;
    std::array<FleetPlayerTrafficRow, StartGridCapacity - 1> Rivals{};
};

inline FleetPlayerTrafficDirection FleetTrafficDirection(double forward, double right) noexcept
{
    if (!std::isfinite(forward) || !std::isfinite(right))
        return FleetPlayerTrafficDirection::Unavailable;
    if (std::hypot(forward, right) <= 1.0e-6)
        return FleetPlayerTrafficDirection::SameCentre;
    // Yaw-relative octants; Unreal's positive right axis is (-sin, cos).
    // atan2 gives [-pi,pi], so the signed nearest octant is [-4,4].
    const double octant = std::atan2(right, forward) / (std::numbers::pi / 4.0);
    const int index = static_cast<int>(std::round(octant));
    switch (index)
    {
        case 0: return FleetPlayerTrafficDirection::Ahead;
        case 1: return FleetPlayerTrafficDirection::AheadRight;
        case 2: return FleetPlayerTrafficDirection::Right;
        case 3: return FleetPlayerTrafficDirection::BehindRight;
        case 4: case -4: return FleetPlayerTrafficDirection::Behind;
        case -3: return FleetPlayerTrafficDirection::BehindLeft;
        case -2: return FleetPlayerTrafficDirection::Left;
        case -1: return FleetPlayerTrafficDirection::AheadLeft;
        default: return FleetPlayerTrafficDirection::Unavailable;
    }
}

inline const char* FleetTrafficDirectionName(FleetPlayerTrafficDirection direction) noexcept
{
    switch (direction)
    {
        case FleetPlayerTrafficDirection::Ahead: return "AHEAD";
        case FleetPlayerTrafficDirection::AheadRight: return "FRONT RIGHT";
        case FleetPlayerTrafficDirection::Right: return "RIGHT";
        case FleetPlayerTrafficDirection::BehindRight: return "REAR RIGHT";
        case FleetPlayerTrafficDirection::Behind: return "BEHIND";
        case FleetPlayerTrafficDirection::BehindLeft: return "REAR LEFT";
        case FleetPlayerTrafficDirection::Left: return "LEFT";
        case FleetPlayerTrafficDirection::AheadLeft: return "FRONT LEFT";
        case FleetPlayerTrafficDirection::SameCentre: return "SAME CENTRE";
        default: return "UNKNOWN";
    }
}

inline const char* FleetTrafficSeverityName(FleetPlayerTrafficSeverity severity) noexcept
{
    switch (severity)
    {
        case FleetPlayerTrafficSeverity::PaddedOverlap: return "PADDED OVERLAP";
        case FleetPlayerTrafficSeverity::ProjectedOverlap: return "PROJECTED PATH";
        case FleetPlayerTrafficSeverity::Approaching: return "APPROACHING";
        case FleetPlayerTrafficSeverity::Nearby: return "NEARBY";
        case FleetPlayerTrafficSeverity::Clear: return "NO LOCAL ALERT";
        default: return "UNAVAILABLE";
    }
}

inline bool IsFleetPlayerTrafficWarning(FleetPlayerTrafficSeverity severity) noexcept
{
    return severity == FleetPlayerTrafficSeverity::Nearby ||
        severity == FleetPlayerTrafficSeverity::Approaching ||
        severity == FleetPlayerTrafficSeverity::ProjectedOverlap ||
        severity == FleetPlayerTrafficSeverity::PaddedOverlap;
}

inline int FleetPlayerTrafficPriority(FleetPlayerTrafficSeverity severity) noexcept
{
    switch (severity)
    {
        case FleetPlayerTrafficSeverity::PaddedOverlap: return 4;
        case FleetPlayerTrafficSeverity::ProjectedOverlap: return 3;
        case FleetPlayerTrafficSeverity::Approaching: return 2;
        case FleetPlayerTrafficSeverity::Nearby: return 1;
        default: return 0;
    }
}

inline bool PreferFleetPlayerTrafficRow(const FleetPlayerTrafficRow& candidate,
                                       const FleetPlayerTrafficRow& current) noexcept
{
    if (!IsFleetPlayerTrafficWarning(candidate.Severity))
        return false;
    if (!IsFleetPlayerTrafficWarning(current.Severity))
        return true;
    const int a = FleetPlayerTrafficPriority(candidate.Severity);
    const int b = FleetPlayerTrafficPriority(current.Severity);
    if (a != b)
        return a > b;
    // Equal predicted-path severity: earliest swept overlap matters more
    // than centre distance. For ordinary proximity, show the nearest car.
    if (candidate.ForecastOverlap && current.ForecastOverlap &&
        candidate.ForecastSeconds != current.ForecastSeconds)
        return candidate.ForecastSeconds < current.ForecastSeconds;
    if (candidate.CentreDistanceCm != current.CentreDistanceCm)
        return candidate.CentreDistanceCm < current.CentreDistanceCm;
    return candidate.Slot < current.Slot;
}

inline FleetPlayerTrafficRow MeasureFleetPlayerTrafficRow(const RecoveryObstacle& player,
                                                          const RecoveryObstacle& rival) noexcept
{
    FleetPlayerTrafficRow row;
    if (!ValidFleetPhysicalObservation(player) ||
        !ValidFleetPhysicalObservation(rival) ||
        player.Slot != HumanGridSlot || rival.Slot == HumanGridSlot ||
        player.ActorToken == rival.ActorToken)
        return row;

    const auto axes = MakeStartGridRect(player.Pose);
    const double dx = rival.Pose.Position.X - player.Pose.Position.X;
    const double dy = rival.Pose.Position.Y - player.Pose.Position.Y;
    const double vx = rival.VelocityX - player.VelocityX;
    const double vy = rival.VelocityY - player.VelocityY;
    row.CentreDistanceCm = std::hypot(dx, dy);
    row.ForwardCm = dx * axes.ForwardX + dy * axes.ForwardY;
    row.RightCm = dx * axes.RightX + dy * axes.RightY;
    row.RadialClosingCmS = row.CentreDistanceCm > 1.0e-6
        ? -(dx * vx + dy * vy) / row.CentreDistanceCm : 0.0;
    if (!std::isfinite(row.CentreDistanceCm) || !std::isfinite(row.ForwardCm) ||
        !std::isfinite(row.RightCm) || !std::isfinite(row.RadialClosingCmS))
        return {};

    row.Direction = FleetTrafficDirection(row.ForwardCm, row.RightCm);
    if (row.Direction == FleetPlayerTrafficDirection::Unavailable)
        return {};
    row.Slot = rival.Slot;
    row.Nearby = row.CentreDistanceCm <= FleetPlayerNearbyCm;
    row.PaddedOverlap = StartGridFootprintsOverlap(player.Pose, rival.Pose);
    const auto forecast = ForecastFleetContact(player, rival);
    if (forecast.Intersects &&
        (!std::isfinite(forecast.FirstContactSeconds) ||
         forecast.FirstContactSeconds < 0.0 ||
         forecast.FirstContactSeconds > FleetForecastHorizonSeconds))
        return {};
    row.ForecastOverlap = forecast.Intersects;
    row.ForecastSeconds = forecast.Intersects ? forecast.FirstContactSeconds : 0.0;
    if (row.PaddedOverlap)
        row.Severity = FleetPlayerTrafficSeverity::PaddedOverlap;
    else if (row.ForecastOverlap)
        row.Severity = FleetPlayerTrafficSeverity::ProjectedOverlap;
    else if (row.CentreDistanceCm <= FleetPlayerApproachRangeCm &&
             row.RadialClosingCmS >= FleetPlayerApproachMinClosingCmS)
        row.Severity = FleetPlayerTrafficSeverity::Approaching;
    else if (row.Nearby)
        row.Severity = FleetPlayerTrafficSeverity::Nearby;
    else
        row.Severity = FleetPlayerTrafficSeverity::Clear;
    return row;
}

inline FleetPlayerTrafficReadout MeasureFleetPlayerTraffic(const FleetPhysicalFrame& live,
                                                          const SixCarIdentity& identity) noexcept
{
    FleetPlayerTrafficReadout out;
    if (live.Inactive || live.AttemptToken == 0 || live.FrameToken == 0 ||
        !MatchLiveFleetIdentity(live, identity))
        return out;
    const auto& player = live.Cars[0];
    FleetPlayerTrafficRow strongest;
    double nearest = std::numeric_limits<double>::infinity();
    for (int slot = 1; slot < StartGridCapacity; ++slot)
    {
        const auto index = static_cast<std::size_t>(slot);
        const auto row = MeasureFleetPlayerTrafficRow(player, live.Cars[index]);
        if (row.Slot != slot || row.Severity == FleetPlayerTrafficSeverity::Unavailable)
            return {};
        out.Rivals[index - 1] = row;
        if (row.CentreDistanceCm < nearest)
        {
            nearest = row.CentreDistanceCm;
            out.NearestSlot = slot;
        }
        if (row.Nearby) ++out.NearbyCount;
        if (row.ForecastOverlap) ++out.ProjectedOverlapCount;
        if (row.PaddedOverlap) ++out.PaddedOverlapCount;
        if (IsFleetPlayerTrafficWarning(row.Severity)) ++out.WarningCount;
        if (PreferFleetPlayerTrafficRow(row, strongest)) strongest = row;
    }
    out.StrongestSlot = strongest.Slot;
    out.Attempt = live.AttemptToken;
    out.Frame = live.FrameToken;
    out.Valid = true;
    return out;
}

// History is observational, not collision counting. Accumulate only the
// interval between two contiguous authorized physics observations where
// both endpoints agree on the same physical rival's condition. Never
// interpolate through a pause, hitch, recovery or replaced roster.
struct FleetPlayerTrafficSample final
{
    FleetPhysicalFrame Physical{};
    SixCarIdentity Identity{};
    std::uint64_t EngineFrame = 0;
    std::uint64_t SessionEpoch = 0;
    std::array<std::uint64_t, StartGridCapacity> TeleportRevisions{};
    double ActiveSeconds = 0.0;
    double AdvancedSeconds = 0.0;
};

struct FleetPlayerTrafficSlotSummary final
{
    int Slot = -1;
    double NearbySeconds = 0.0;
    double WarningSeconds = 0.0;
    double ProjectedSeconds = 0.0;
    double PaddedOverlapSeconds = 0.0;
    bool HasDistance = false;
    double MinimumCentreDistanceCm = 0.0;
};

struct FleetPlayerTrafficSummary final
{
    bool HasSamples = false;
    int MaximumConcurrentWarnings = 0;
    std::uint64_t AcceptedSamples = 0;
    std::uint64_t ContiguousIntervals = 0;
    std::uint64_t DiscontinuousSamples = 0;
    double ObservedSeconds = 0.0;
    double AnyNearbySeconds = 0.0;
    double AnyWarningSeconds = 0.0;
    double AnyProjectedSeconds = 0.0;
    double AnyPaddedOverlapSeconds = 0.0;
    bool HasDistance = false;
    double MinimumCentreDistanceCm = 0.0;
    int NearestObservedSlot = -1;
    std::array<FleetPlayerTrafficSlotSummary, StartGridCapacity - 1> Rivals{};
};

class FleetPlayerTrafficJournal final
{
public:
    void Reset() noexcept
    {
        Attempt = 0;
        Identity = {};
        LastSeenFrame = 0;
        LastSeenEngineFrame = 0;
        LastSeenActiveSeconds = 0.0;
        HaveSeenFrame = false;
        Previous = {};
        HavePrevious = false;
        Summary = {};
    }

    void Suspend() noexcept
    {
        Previous = {};
        HavePrevious = false;
    }

    bool Configure(std::uint64_t attempt, const SixCarIdentity& identity) noexcept
    {
        if (Attempt != 0 || attempt == 0)
            return false;
        for (int slot = 0; slot < StartGridCapacity; ++slot)
        {
            const auto token = identity[static_cast<std::size_t>(slot)];
            if (token == 0) return false;
            for (int other = 0; other < slot; ++other)
                if (token == identity[static_cast<std::size_t>(other)]) return false;
        }
        Attempt = attempt;
        Identity = identity;
        for (int slot = 1; slot < StartGridCapacity; ++slot)
            Summary.Rivals[static_cast<std::size_t>(slot - 1)].Slot = slot;
        return true;
    }

    bool Observe(const FleetPlayerTrafficSample& sample) noexcept
    {
        if (Attempt == 0 || sample.Physical.AttemptToken != Attempt ||
            sample.Identity != Identity)
            return false;
        // A malformed same-attempt sample breaks continuity, but cannot
        // erase a completed history or replace the monotonic high-water mark.
        if (!ValidClock(sample.ActiveSeconds, sample.AdvancedSeconds) ||
            sample.Physical.FrameToken == 0 ||
            (HaveSeenFrame &&
             (sample.Physical.FrameToken <= LastSeenFrame ||
              sample.EngineFrame <= LastSeenEngineFrame ||
              sample.ActiveSeconds <= LastSeenActiveSeconds)))
        {
            Suspend();
            return false;
        }
        const auto now = MeasureFleetPlayerTraffic(sample.Physical, Identity);
        if (!now.Valid)
        {
            Suspend();
            return false;
        }

        const bool contiguous = HavePrevious && IsContiguous(sample);
        if (contiguous)
        {
            const double duration = sample.ActiveSeconds - Previous.ActiveSeconds;
            AccumulateInterval(Previous.Readout, now, duration);
            Increment(Summary.ContiguousIntervals);
        }
        else if (HaveSeenFrame)
        {
            Increment(Summary.DiscontinuousSamples);
        }
        ObserveEndpoint(now);
        Summary.HasSamples = true;
        Increment(Summary.AcceptedSamples);

        LastSeenFrame = sample.Physical.FrameToken;
        LastSeenEngineFrame = sample.EngineFrame;
        LastSeenActiveSeconds = sample.ActiveSeconds;
        HaveSeenFrame = true;
        Previous = {sample.Physical.FrameToken, sample.EngineFrame, sample.SessionEpoch,
            sample.TeleportRevisions, sample.ActiveSeconds, now};
        HavePrevious = true;
        return true;
    }

    FleetPlayerTrafficSummary Readout() const noexcept { return Summary; }

private:
    struct Endpoint final
    {
        std::uint64_t Frame = 0;
        std::uint64_t EngineFrame = 0;
        std::uint64_t SessionEpoch = 0;
        std::array<std::uint64_t, StartGridCapacity> TeleportRevisions{};
        double ActiveSeconds = 0.0;
        FleetPlayerTrafficReadout Readout{};
    };

    static bool ValidClock(double active, double delta) noexcept
    {
        return std::isfinite(active) && std::isfinite(delta) &&
            active > 0.0 && active <= FleetPlayerTrafficMaxClockSeconds &&
            delta > 0.0 && delta <= FleetPlayerTrafficMaxFrameSeconds &&
            active + FleetPlayerTrafficClockToleranceSeconds >= delta;
    }

    static void Increment(std::uint64_t& counter) noexcept
    {
        if (counter != std::numeric_limits<std::uint64_t>::max()) ++counter;
    }

    bool IsContiguous(const FleetPlayerTrafficSample& sample) const noexcept
    {
        if (Previous.Frame == std::numeric_limits<std::uint64_t>::max() ||
            Previous.EngineFrame == std::numeric_limits<std::uint64_t>::max() ||
            sample.Physical.FrameToken != Previous.Frame + 1 ||
            sample.EngineFrame != Previous.EngineFrame + 1 ||
            sample.SessionEpoch != Previous.SessionEpoch ||
            sample.TeleportRevisions != Previous.TeleportRevisions)
            return false;
        const double actual = sample.ActiveSeconds - Previous.ActiveSeconds;
        return actual > 0.0 &&
            actual <= FleetPlayerTrafficMaxFrameSeconds + FleetPlayerTrafficClockToleranceSeconds &&
            std::abs(actual - sample.AdvancedSeconds) <= FleetPlayerTrafficClockToleranceSeconds;
    }

    static void AddSeconds(double& total, double value) noexcept
    {
        // Keep summaries bounded to the same finite run-local clock domain.
        total = std::min(FleetPlayerTrafficMaxClockSeconds, total + value);
    }

    void AccumulateInterval(const FleetPlayerTrafficReadout& before,
                            const FleetPlayerTrafficReadout& after,
                            double duration) noexcept
    {
        AddSeconds(Summary.ObservedSeconds, duration);
        bool near = false;
        bool warning = false;
        bool projected = false;
        bool padded = false;
        for (std::size_t index = 0; index < Summary.Rivals.size(); ++index)
        {
            const auto& a = before.Rivals[index];
            const auto& b = after.Rivals[index];
            auto& slot = Summary.Rivals[index];
            if (a.Nearby && b.Nearby)
            {
                AddSeconds(slot.NearbySeconds, duration);
                near = true;
            }
            if (IsFleetPlayerTrafficWarning(a.Severity) &&
                IsFleetPlayerTrafficWarning(b.Severity))
            {
                AddSeconds(slot.WarningSeconds, duration);
                warning = true;
            }
            if (a.ForecastOverlap && b.ForecastOverlap)
            {
                AddSeconds(slot.ProjectedSeconds, duration);
                projected = true;
            }
            if (a.PaddedOverlap && b.PaddedOverlap)
            {
                AddSeconds(slot.PaddedOverlapSeconds, duration);
                padded = true;
            }
        }
        // Count the union once even when several rivals are nearby.
        if (near) AddSeconds(Summary.AnyNearbySeconds, duration);
        if (warning) AddSeconds(Summary.AnyWarningSeconds, duration);
        if (projected) AddSeconds(Summary.AnyProjectedSeconds, duration);
        if (padded) AddSeconds(Summary.AnyPaddedOverlapSeconds, duration);
    }

    void ObserveEndpoint(const FleetPlayerTrafficReadout& now) noexcept
    {
        Summary.MaximumConcurrentWarnings =
            std::max(Summary.MaximumConcurrentWarnings, now.WarningCount);
        for (std::size_t index = 0; index < Summary.Rivals.size(); ++index)
        {
            auto& slot = Summary.Rivals[index];
            const auto& row = now.Rivals[index];
            if (!slot.HasDistance || row.CentreDistanceCm < slot.MinimumCentreDistanceCm)
            {
                slot.HasDistance = true;
                slot.MinimumCentreDistanceCm = row.CentreDistanceCm;
            }
            if (!Summary.HasDistance ||
                row.CentreDistanceCm < Summary.MinimumCentreDistanceCm ||
                (row.CentreDistanceCm == Summary.MinimumCentreDistanceCm &&
                 row.Slot < Summary.NearestObservedSlot))
            {
                Summary.HasDistance = true;
                Summary.MinimumCentreDistanceCm = row.CentreDistanceCm;
                Summary.NearestObservedSlot = row.Slot;
            }
        }
    }

    std::uint64_t Attempt = 0;
    SixCarIdentity Identity{};
    std::uint64_t LastSeenFrame = 0;
    std::uint64_t LastSeenEngineFrame = 0;
    double LastSeenActiveSeconds = 0.0;
    bool HaveSeenFrame = false;
    Endpoint Previous{};
    bool HavePrevious = false;
    FleetPlayerTrafficSummary Summary{};
};
} // namespace latebrake
