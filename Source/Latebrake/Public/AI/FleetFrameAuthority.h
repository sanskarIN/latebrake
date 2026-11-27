#pragma once

#include <cstdint>

namespace latebrake
{
// A single experimental GameMode observation is a short-lived capability.
// The engine adapter supplies the actual global engine frame counter and
// the authoritative gameplay session epoch. No counter here advances race
// time, integrates physics, simulates a clock or grants a checkpoint.
struct FleetFrameStamp final
{
    std::uint64_t Attempt=0;
    std::uint64_t Observation=0;
    std::uint64_t EngineFrame=0;
    std::uint64_t SessionEpoch=0;
};

inline constexpr bool SameFleetFrameStamp(FleetFrameStamp a,FleetFrameStamp b) noexcept
{
    return a.Attempt==b.Attempt && a.Observation==b.Observation &&
        a.EngineFrame==b.EngineFrame && a.SessionEpoch==b.SessionEpoch;
}

enum class FleetFrameAuthorityStatus
{
    Unconfigured,
    AwaitingFrame,
    Sampling,
    Denied,
    Admitted
};

struct FleetFrameAuthorityReadout final
{
    FleetFrameStamp Stamp{};
    FleetFrameAuthorityStatus Status=FleetFrameAuthorityStatus::Unconfigured;
    bool HasEngineFrame=false;
};

class FleetFrameAuthority final
{
public:
    bool Begin(std::uint64_t attempt) noexcept
    {
        if(attempt==0 || attempt<=State.Stamp.Attempt)return false;
        State={};
        State.Stamp.Attempt=attempt;
        State.Status=FleetFrameAuthorityStatus::AwaitingFrame;
        return true;
    }

    // Invalidate the prior capability BEFORE collecting a new roster. A
    // repeated or backward engine frame cannot spend a second countdown
    // or clock tick, and cannot leave a previously admitted frame usable.
    bool BeginFrame(FleetFrameStamp stamp) noexcept
    {
        if(stamp.Attempt==0 || State.Status==FleetFrameAuthorityStatus::Unconfigured ||
            stamp.Attempt!=State.Stamp.Attempt)
            return false;
        if(stamp.Observation==0 || stamp.Observation<=State.Stamp.Observation ||
            (State.HasEngineFrame && stamp.EngineFrame<=State.Stamp.EngineFrame))
        {
            State.Status=FleetFrameAuthorityStatus::Denied;
            return false;
        }
        State.Stamp=stamp;
        State.HasEngineFrame=true;
        State.Status=FleetFrameAuthorityStatus::Sampling;
        return true;
    }

    bool Commit(FleetFrameStamp stamp,bool qualified) noexcept
    {
        // A delayed result from a replaced attempt must not mutate the
        // current attempt. Wrong observation or epoch within this attempt
        // is a denied sample, not a usable old admission.
        // A default-constructed authority has no attempt to deny or admit.
        if(stamp.Attempt==0 || State.Status==FleetFrameAuthorityStatus::Unconfigured ||
            stamp.Attempt!=State.Stamp.Attempt)return false;
        if(State.Status!=FleetFrameAuthorityStatus::Sampling ||
            !SameFleetFrameStamp(stamp,State.Stamp))
        {
            State.Status=FleetFrameAuthorityStatus::Denied;
            return false;
        }
        State.Status=qualified?FleetFrameAuthorityStatus::Admitted
                              :FleetFrameAuthorityStatus::Denied;
        return qualified;
    }

    bool IsCurrent(FleetFrameStamp stamp) const noexcept
    {
        return stamp.Attempt!=0 && State.Status==FleetFrameAuthorityStatus::Admitted &&
            State.HasEngineFrame && SameFleetFrameStamp(stamp,State.Stamp);
    }

    void Revoke() noexcept
    {
        if(State.Status!=FleetFrameAuthorityStatus::Unconfigured)
            State.Status=FleetFrameAuthorityStatus::Denied;
    }

    FleetFrameAuthorityReadout Readout() const noexcept{return State;}

private:
    FleetFrameAuthorityReadout State{};
};
} // namespace latebrake
