#pragma once

#include "Track/CircuitLayout.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Experimental, single-player arcade pit on the inside of the SOUTH
// straight. It is outside the central AI racing line, but physically on
// the existing paved surface. It is not an authored pit-lane mesh, real
// tire service, or an official racing-series rule.
inline constexpr CircuitPoint RacePitBay{0.0, -1300.0};
inline constexpr double RacePitBayRadiusCm = 250.0;
inline constexpr double RacePitServiceHoldSeconds = 2.5;
inline constexpr int RacePitStopLimit=1000;

struct PitServiceObservation final
{
    double DeltaSeconds = 0.0;
    double X = 0.0;
    double Y = 0.0;
    double PlanarSpeedCmPerSecond = 0.0;
    double RequestedThrottle = 0.0;
    bool ButtonHeld = false;
    bool NitroHeld = false;
    bool Authorized = false;
    bool MaintenanceNeeded = false;
    // Measured separately from driver authorization so HUD can explain
    // unstable contact without pretending a service was authorized.
    bool StableGround = false;
};

inline bool IsInRacePitBay(double x, double y) noexcept
{
    if (!std::isfinite(x) || !std::isfinite(y) ||
        !IsOnPavedSurface({x, y}))
    {
        return false;
    }
    const double dx = x - RacePitBay.X, dy = y - RacePitBay.Y;
    return dx * dx + dy * dy <=
        RacePitBayRadiusCm * RacePitBayRadiusCm;
}

// The diagnostic is a READ-ONLY explanation of why a real physical
// service hold can or cannot progress. It never grants service or credits.
// Ordered priorities are deterministic when several reasons coexist.
enum class PitServiceGuidance
{
    Unavailable,
    ApproachBay,
    ExitToRearm,
    NoMaintenance,
    SlowDown,
    ReleaseThrottle,
    ReleaseNitro,
    SettleChassis,
    HoldButton,
    Servicing,
    Completed,
    StopLimitReached
};

inline PitServiceGuidance DiagnosePitService(
    const PitServiceObservation& sample, bool armed) noexcept
{
    if (!std::isfinite(sample.DeltaSeconds) ||
        sample.DeltaSeconds <= 0.0 || sample.DeltaSeconds > 0.10 ||
        !std::isfinite(sample.X) || !std::isfinite(sample.Y) ||
        !std::isfinite(sample.PlanarSpeedCmPerSecond) ||
        sample.PlanarSpeedCmPerSecond < 0.0 ||
        sample.PlanarSpeedCmPerSecond > 10000.0 ||
        !std::isfinite(sample.RequestedThrottle) ||
        std::abs(sample.RequestedThrottle) > 1.0 ||
        !sample.Authorized)
    {
        return PitServiceGuidance::Unavailable;
    }
    if (!IsInRacePitBay(sample.X, sample.Y))
    {
        return PitServiceGuidance::ApproachBay;
    }
    if (!armed) return PitServiceGuidance::ExitToRearm;
    if (!sample.MaintenanceNeeded)
        return PitServiceGuidance::NoMaintenance;
    if (sample.PlanarSpeedCmPerSecond > 120.0)
        return PitServiceGuidance::SlowDown;
    if (std::abs(sample.RequestedThrottle) > 0.05)
        return PitServiceGuidance::ReleaseThrottle;
    if (sample.NitroHeld)
        return PitServiceGuidance::ReleaseNitro;
    if (!sample.StableGround)
        return PitServiceGuidance::SettleChassis;
    if (!sample.ButtonHeld)
        return PitServiceGuidance::HoldButton;
    return PitServiceGuidance::Servicing;
}

struct PitServiceFrameStamp final
{
    std::uint64_t EngineFrame=0;
    std::uint64_t SessionRevision=0;
    std::uint64_t TeleportRevision=0;
    std::uint64_t ControllerToken=0;

    bool Valid() const noexcept {return EngineFrame!=0 && ControllerToken!=0;}

    bool FreshAfter(const PitServiceFrameStamp& previous) const noexcept
    {
        return Valid() && previous.Valid() && EngineFrame>previous.EngineFrame &&
            SessionRevision>=previous.SessionRevision && TeleportRevision>=previous.TeleportRevision;
    }

    bool ContinuousAfter(const PitServiceFrameStamp& previous) const noexcept
    {
        return FreshAfter(previous) && EngineFrame-previous.EngineFrame==1 &&
            SessionRevision==previous.SessionRevision && TeleportRevision==previous.TeleportRevision &&
            ControllerToken==previous.ControllerToken;
    }
};

struct PitServiceFrameReview final
{
    std::uint64_t AcceptedFrames=0;
    std::uint64_t RefusedFrames=0;
    std::uint64_t Discontinuities=0;
    std::uint64_t InterruptedHolds=0;
    double LongestInterruptedHoldSeconds=0.0;
};

// A single service is earned by a CONTINUOUS 2.5s valid, nearly stationary
// hold inside the bay. Exiting the bay is required to arm another stop.
// This policy changes no score, physics, saved garage, timer, or checkpoint.
class RacePitService final
{
public:
    void Reset() noexcept
    {
        HeldSeconds = 0.0;
        LastFrame={};
        FrameReview={};
        Stops = 0;
        Armed = true;
        InBay = false;
        LastGuidance = PitServiceGuidance::Unavailable;
    }
    void Suspend() noexcept
    {
        ClearHold();
        InBay = false;
        LastGuidance = PitServiceGuidance::Unavailable;
    }
    // Production callers must submit one actual local player physics
    // frame. A duplicate or rollback cannot spend hold time or rearm a
    // completed stop. Explicit Suspend leaves the high-water stamp intact.
    bool Observe(PitServiceObservation sample,const PitServiceFrameStamp& stamp) noexcept
    {
        if(!stamp.Valid() || (LastFrame.Valid() && !stamp.FreshAfter(LastFrame)))
        {
            Increment(FrameReview.RefusedFrames);
            Suspend();
            return false;
        }
        if(LastFrame.Valid() && !stamp.ContinuousAfter(LastFrame))
        {
            Increment(FrameReview.Discontinuities);
            Suspend();
        }
        LastFrame=stamp;
        Increment(FrameReview.AcceptedFrames);
        return Observe(sample);
    }

    // Portable state-machine observation; native pawn uses stamped overload.
    bool Observe(PitServiceObservation sample) noexcept
    {
        LastGuidance = DiagnosePitService(sample, Armed);
        if (!std::isfinite(sample.DeltaSeconds) ||
            sample.DeltaSeconds <= 0.0 || sample.DeltaSeconds > 0.10 ||
            !std::isfinite(sample.X) || !std::isfinite(sample.Y) ||
            !std::isfinite(sample.PlanarSpeedCmPerSecond) ||
            sample.PlanarSpeedCmPerSecond < 0.0 ||
            sample.PlanarSpeedCmPerSecond > 10000.0 ||
            !std::isfinite(sample.RequestedThrottle) ||
            std::abs(sample.RequestedThrottle) > 1.0 ||
            !sample.Authorized)
        {
            Suspend();
            return false;
        }
        InBay = IsInRacePitBay(sample.X, sample.Y);
        if(Stops>=RacePitStopLimit)
        {
            ClearHold();
            Armed=false;
            LastGuidance=PitServiceGuidance::StopLimitReached;
            return false;
        }
        if (!InBay)
        {
            Armed = true;
            ClearHold();
            return false;
        }
        if (!Armed || !sample.ButtonHeld || sample.NitroHeld ||
            !sample.MaintenanceNeeded || !sample.StableGround ||
            sample.PlanarSpeedCmPerSecond > 120.0 ||
            std::abs(sample.RequestedThrottle) > 0.05)
        {
            ClearHold();
            return false;
        }
        HeldSeconds = std::min(
            RacePitServiceHoldSeconds, HeldSeconds + sample.DeltaSeconds);
        if (HeldSeconds + 1.0e-9 < RacePitServiceHoldSeconds)
        {
            return false;
        }
        HeldSeconds = 0.0;
        Armed = false;
        LastGuidance = PitServiceGuidance::Completed;
        ++Stops;
        return true;
    }
    bool IsInBay() const noexcept { return InBay; }
    bool IsArmed() const noexcept { return Armed; }
    double ProgressFraction() const noexcept
    {
        return HeldSeconds / RacePitServiceHoldSeconds;
    }
    int StopCount() const noexcept { return Stops; }
    PitServiceFrameReview ReviewFrames() const noexcept {return FrameReview;}
    PitServiceGuidance Guidance() const noexcept { return LastGuidance; }

private:
    static void Increment(std::uint64_t& counter) noexcept
    {
        if(counter!=std::numeric_limits<std::uint64_t>::max()) ++counter;
    }

    void ClearHold() noexcept
    {
        if(HeldSeconds>0.0)
        {
            Increment(FrameReview.InterruptedHolds);
            FrameReview.LongestInterruptedHoldSeconds=std::max(
                FrameReview.LongestInterruptedHoldSeconds,HeldSeconds);
        }
        HeldSeconds=0.0;
    }

    PitServiceFrameStamp LastFrame{};
    PitServiceFrameReview FrameReview{};
    double HeldSeconds = 0.0;
    int Stops = 0;
    bool Armed = true;
    bool InBay = false;
    PitServiceGuidance LastGuidance = PitServiceGuidance::Unavailable;
};
} // namespace latebrake
