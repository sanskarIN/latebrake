#pragma once

#include "Racing/SlipstreamDraftBoost.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// Experimental six-car extension of the existing bounded single-rival
// arcade slipstream. Exactly one real leader can contribute on a frame.
// Never sum wakes or transfer confirmation between two different bodies.
inline constexpr int FleetDraftCarCount = 6;
inline constexpr int FleetDraftRivalCount = FleetDraftCarCount - 1;
using FleetDraftIdentity = std::array<std::uint64_t, FleetDraftCarCount>;

struct FleetDraftFollower final
{
    std::uint64_t Token = 0;
    std::uint64_t TeleportRevision = 0;
    double FrameSeconds = 0.0;
    double PositionX = 0.0;
    double PositionY = 0.0;
    double PositionZ = 0.0;
    double ForwardX = 1.0;
    double ForwardY = 0.0;
    double ForwardSpeed = 0.0;
    double LateralSpeed = 0.0;
    double Throttle = 0.0;
    bool Braking = false;
    bool GroundSupported = false;
    bool EligibleHumanRace = false;
};

struct FleetDraftCandidate final
{
    int Slot = -1;
    std::uint64_t Token = 0;
    std::uint64_t TeleportRevision = 0;
    bool PhysicalStateValid = false;
    bool GroundSupported = false;
    bool RouteComplete = false;
    bool ClearPhysicalPath = false;
    double PositionX = 0.0;
    double PositionY = 0.0;
    double PositionZ = 0.0;
    double ForwardX = 1.0;
    double ForwardY = 0.0;
    double VelocityX = 0.0;
    double VelocityY = 0.0;
};

struct FleetDraftFrame final
{
    std::uint64_t Attempt = 0;
    std::uint64_t LocalFrame = 0;
    std::uint64_t EngineFrame = 0;
    std::uint64_t SessionRevision = 0;
    FleetDraftIdentity BoundIdentity{};
    FleetDraftFollower Follower{};
    std::array<FleetDraftCandidate, FleetDraftRivalCount> Candidates{};
};

enum class FleetDraftStatus
{
    Unavailable,
    Searching,
    Confirming,
    Active
};

struct FleetDraftReadout final
{
    FleetDraftStatus Status = FleetDraftStatus::Unavailable;
    int TargetSlot = -1;
    std::uint64_t TargetToken = 0;
    double QualifiedSeconds = 0.0;
    double ExtraForce = 0.0;
    int EligibleCandidates = 0;
    double ForwardGapCm = 0.0;
    double LateralGapCm = 0.0;

    bool Active() const noexcept
    {
        return Status == FleetDraftStatus::Active && ExtraForce > 0.0;
    }
};

inline bool FiniteFleetDraftFollower(const FleetDraftFollower& follower) noexcept
{
    return follower.Token != 0 &&
        std::isfinite(follower.FrameSeconds) &&
        follower.FrameSeconds >= SlipstreamDraftBoost::MinFrameSeconds &&
        follower.FrameSeconds <= SlipstreamDraftBoost::MaxFrameSeconds &&
        std::isfinite(follower.PositionX) && std::isfinite(follower.PositionY) &&
        std::isfinite(follower.PositionZ) &&
        std::isfinite(follower.ForwardX) && std::isfinite(follower.ForwardY) &&
        std::isfinite(follower.ForwardSpeed) && std::isfinite(follower.LateralSpeed) &&
        std::isfinite(follower.Throttle);
}

inline bool FiniteFleetDraftCandidate(const FleetDraftCandidate& candidate) noexcept
{
    return candidate.PhysicalStateValid && candidate.Token != 0 &&
        std::isfinite(candidate.PositionX) && std::isfinite(candidate.PositionY) &&
        std::isfinite(candidate.PositionZ) &&
        std::isfinite(candidate.ForwardX) && std::isfinite(candidate.ForwardY) &&
        std::isfinite(candidate.VelocityX) && std::isfinite(candidate.VelocityY);
}

inline bool ValidateFleetDraftFrame(const FleetDraftFrame& frame) noexcept
{
    if (frame.Attempt == 0 || frame.LocalFrame == 0 ||
        !FiniteFleetDraftFollower(frame.Follower) ||
        frame.BoundIdentity[0] != frame.Follower.Token)
        return false;

    // Every rival slot is represented even when that particular car has
    // finished, is off the road or has an obstructed wake. An omitted
    // body may otherwise conceal a closer obstructing opponent.
    for (int slot = 0; slot < FleetDraftCarCount; ++slot)
    {
        const auto token = frame.BoundIdentity[static_cast<std::size_t>(slot)];
        if (token == 0)
            return false;
        for (int earlier = 0; earlier < slot; ++earlier)
            if (token == frame.BoundIdentity[static_cast<std::size_t>(earlier)])
                return false;
        if (slot == 0)
            continue;
        const auto& candidate = frame.Candidates[static_cast<std::size_t>(slot - 1)];
        if (candidate.Slot != slot || candidate.Token != token ||
            !FiniteFleetDraftCandidate(candidate))
            return false;
    }
    return true;
}

inline DraftingSample MakeFleetDraftSample(const FleetDraftFollower& follower,
                                           const FleetDraftCandidate& candidate) noexcept
{
    DraftingSample sample;
    sample.FrameSeconds = follower.FrameSeconds;
    sample.FollowerSpeedCmPerSecond = follower.ForwardSpeed;
    sample.LeadSpeedCmPerSecond =
        candidate.VelocityX * candidate.ForwardX + candidate.VelocityY * candidate.ForwardY;
    sample.FollowerForwardX = follower.ForwardX;
    sample.FollowerForwardY = follower.ForwardY;
    sample.LeadForwardX = candidate.ForwardX;
    sample.LeadForwardY = candidate.ForwardY;
    sample.LeadOffsetX = candidate.PositionX - follower.PositionX;
    sample.LeadOffsetY = candidate.PositionY - follower.PositionY;
    sample.LeadOffsetZ = candidate.PositionZ - follower.PositionZ;
    sample.ForwardThrottle = follower.Throttle;
    sample.Braking = follower.Braking;
    sample.EligibleHumanRace = follower.EligibleHumanRace && follower.GroundSupported &&
        candidate.GroundSupported && !candidate.RouteComplete;
    sample.FollowerLateralSpeedCmPerSecond = follower.LateralSpeed;
    sample.LeadLateralSpeedCmPerSecond =
        -candidate.VelocityX * candidate.ForwardY + candidate.VelocityY * candidate.ForwardX;
    sample.ClearPhysicalDraftPath = candidate.ClearPhysicalPath;
    return sample;
}

struct FleetDraftPotential final
{
    bool Eligible = false;
    double Weight = 0.0;
    double ForwardGapCm = 0.0;
    double LateralGapCm = 0.0;
};

// Use the established policy for single-frame admission, without touching
// the real confirmation meter. A temporary meter never becomes a force.
inline FleetDraftPotential MeasureFleetDraftPotential(const DraftingSample& sample) noexcept
{
    SlipstreamDraftBoost probe;
    probe.Observe(sample);
    if (probe.QualifiedSeconds() <= 0.0)
        return {};
    const double forward = sample.LeadOffsetX * sample.FollowerForwardX +
        sample.LeadOffsetY * sample.FollowerForwardY;
    const double lateral = std::abs(sample.LeadOffsetX * sample.FollowerForwardY -
        sample.LeadOffsetY * sample.FollowerForwardX);
    const double distanceWeight = (SlipstreamDraftBoost::MaxForwardDistance - forward) /
        (SlipstreamDraftBoost::MaxForwardDistance - SlipstreamDraftBoost::MinForwardDistance);
    const double laneWeight = 1.0 - lateral / SlipstreamDraftBoost::MaxLateralDistance;
    return {true, std::clamp(distanceWeight * laneWeight, 0.0, 1.0), forward, lateral};
}

class FleetSlipstreamDraft final
{
public:
    // Explicit new race/reset. Input suspension alone preserves the
    // high-water marks so a stale frame cannot be replayed after pause.
    void Reset() noexcept
    {
        Suspend();
        HaveStamp = false;
        LastAttempt = 0;
        LastLocalFrame = 0;
        LastEngineFrame = 0;
        LastSessionRevision = 0;
        LastFollowerToken = 0;
        LastFollowerTeleport = 0;
        LastIdentity = {};
    }

    void Suspend() noexcept
    {
        Meter.Suspend();
        TargetSlot = -1;
        TargetToken = 0;
        TargetTeleport = 0;
        Output = {};
    }

    FleetDraftReadout Observe(const FleetDraftFrame& frame) noexcept
    {
        if (frame.Attempt == 0 || frame.LocalFrame == 0)
        {
            Suspend();
            return Output;
        }

        if (HaveStamp &&
            (frame.Attempt < LastAttempt ||
             (frame.Attempt == LastAttempt &&
              (frame.LocalFrame <= LastLocalFrame || frame.EngineFrame <= LastEngineFrame))))
        {
            Suspend();
            return Output;
        }

        // A newly admitted stamp starts a fresh confirmation if there is
        // any discontinuity in the attempt, race epoch, roster or player.
        const bool continuous = HaveStamp && frame.Attempt == LastAttempt &&
            frame.LocalFrame - LastLocalFrame == 1 &&
            frame.EngineFrame - LastEngineFrame == 1 &&
            frame.SessionRevision == LastSessionRevision &&
            frame.BoundIdentity == LastIdentity &&
            frame.Follower.Token == LastFollowerToken &&
            frame.Follower.TeleportRevision == LastFollowerTeleport;
        if (!continuous)
            Suspend();

        HaveStamp = true;
        LastAttempt = frame.Attempt;
        LastLocalFrame = frame.LocalFrame;
        LastEngineFrame = frame.EngineFrame;
        LastSessionRevision = frame.SessionRevision;
        LastIdentity = frame.BoundIdentity;
        LastFollowerToken = frame.Follower.Token;
        LastFollowerTeleport = frame.Follower.TeleportRevision;

        if (!ValidateFleetDraftFrame(frame))
        {
            Suspend();
            return Output;
        }

        std::array<DraftingSample, FleetDraftRivalCount> samples{};
        std::array<FleetDraftPotential, FleetDraftRivalCount> potential{};
        int eligible = 0;
        int best = -1;
        for (int index = 0; index < FleetDraftRivalCount; ++index)
        {
            const auto i = static_cast<std::size_t>(index);
            samples[i] = MakeFleetDraftSample(frame.Follower, frame.Candidates[i]);
            potential[i] = MeasureFleetDraftPotential(samples[i]);
            if (!potential[i].Eligible)
                continue;
            ++eligible;
            if (best < 0 || potential[i].Weight > potential[static_cast<std::size_t>(best)].Weight)
                best = index;
        }

        if (best < 0)
        {
            Suspend();
            Output.Status = FleetDraftStatus::Searching;
            return Output;
        }

        // Keep the same real leader as long as its current wake remains
        // eligible. This avoids alternating between adjacent opponents
        // merely because their instantaneous strengths are almost equal.
        int selected = best;
        if (TargetSlot >= 1 && TargetSlot <= FleetDraftRivalCount)
        {
            const auto current = static_cast<std::size_t>(TargetSlot - 1);
            if (potential[current].Eligible &&
                frame.Candidates[current].Token == TargetToken &&
                frame.Candidates[current].TeleportRevision == TargetTeleport)
                selected = TargetSlot - 1;
        }

        const auto i = static_cast<std::size_t>(selected);
        const auto& leader = frame.Candidates[i];
        if (leader.Slot != TargetSlot || leader.Token != TargetToken ||
            leader.TeleportRevision != TargetTeleport)
        {
            Meter.Reset();
            TargetSlot = leader.Slot;
            TargetToken = leader.Token;
            TargetTeleport = leader.TeleportRevision;
        }

        // Only the selected sample can drive the real confirmation. Even
        // five fully eligible bodies cannot sum five motor-force bonuses.
        const double force = Meter.Observe(samples[i]);
        if (Meter.QualifiedSeconds() <= 0.0)
        {
            // The established policy detected a relative-motion jump.
            // Do not try another candidate on the same sampled frame.
            Suspend();
            Output.Status = FleetDraftStatus::Searching;
            Output.EligibleCandidates = eligible;
            return Output;
        }

        Output.Status = force > 0.0 ? FleetDraftStatus::Active : FleetDraftStatus::Confirming;
        Output.TargetSlot = TargetSlot;
        Output.TargetToken = TargetToken;
        Output.QualifiedSeconds = Meter.QualifiedSeconds();
        Output.ExtraForce = force;
        Output.EligibleCandidates = eligible;
        Output.ForwardGapCm = potential[i].ForwardGapCm;
        Output.LateralGapCm = potential[i].LateralGapCm;
        return Output;
    }

    FleetDraftReadout Readout() const noexcept { return Output; }

private:
    SlipstreamDraftBoost Meter;
    FleetDraftReadout Output{};
    int TargetSlot = -1;
    std::uint64_t TargetToken = 0;
    std::uint64_t TargetTeleport = 0;
    bool HaveStamp = false;
    std::uint64_t LastAttempt = 0;
    std::uint64_t LastLocalFrame = 0;
    std::uint64_t LastEngineFrame = 0;
    std::uint64_t LastSessionRevision = 0;
    std::uint64_t LastFollowerToken = 0;
    std::uint64_t LastFollowerTeleport = 0;
    FleetDraftIdentity LastIdentity{};
};

} // namespace latebrake
