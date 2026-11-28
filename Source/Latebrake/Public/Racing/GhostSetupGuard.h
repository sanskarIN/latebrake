#pragma once

#include "Garage/GarageTuningKey.h"
#include "Racing/BestLapGhost.h"

#include <cstdint>

namespace latebrake
{
// A run-local best-lap ghost is not a cross-configuration comparison.
// Cosmetics and currency are intentionally absent from this physical key.
struct GhostSetupStamp final
{
    std::uint64_t Actor=0;
    GarageTuningKey Tuning{};
    int Handling=-1;

    bool Valid() const noexcept
    {
        return Actor!=0 && GarageState::IsVehicle(Tuning.Selected) &&
            Tuning.Engine>=0 && Tuning.Engine<=3 && Tuning.Grip>=0 && Tuning.Grip<=3 &&
            Handling>=0 && Handling<=1;
    }

    bool operator==(const GhostSetupStamp&) const noexcept=default;
};

enum class GhostSetupTransition : std::uint8_t
{
    Unavailable, InitialBinding, Unchanged, Rebound, Lost
};

class GhostSetupGuard final
{
public:
    void Reset() noexcept
    {
        Bound={};
        EverBound=false;
    }

    GhostSetupTransition Observe(GhostSetupStamp stamp) noexcept
    {
        if(!stamp.Valid())
        {
            if(Bound.Valid())
            {
                Bound={};
                return GhostSetupTransition::Lost;
            }
            return GhostSetupTransition::Unavailable;
        }
        if(Bound.Valid() && stamp==Bound)return GhostSetupTransition::Unchanged;
        const bool first=!EverBound;
        EverBound=true;
        Bound=stamp;
        return first?GhostSetupTransition::InitialBinding:GhostSetupTransition::Rebound;
    }

    bool Matches(GhostSetupStamp stamp) const noexcept
    {return stamp.Valid() && Bound.Valid() && stamp==Bound;}

    bool IsBound() const noexcept {return Bound.Valid();}

private:
    GhostSetupStamp Bound{};
    bool EverBound=false;
};

// Invalidate both the partial candidate and old best when authority
// disappears or the setup changes. A genuine zero-clock lap boundary
// may immediately start a new candidate; never splice into a moving lap.
inline GhostSetupTransition RefreshGhostSetup(GhostSetupGuard& guard,
    BestLapGhost& ghost,GhostSetupStamp stamp,bool atLapBoundary) noexcept
{
    const auto transition=guard.Observe(stamp);
    switch(transition)
    {
    case GhostSetupTransition::Rebound:
    case GhostSetupTransition::Lost:
        ghost.ResetRun();
        if(!atLapBoundary || !stamp.Valid())ghost.InvalidateCurrentLap();
        break;
    case GhostSetupTransition::Unavailable:
        ghost.InvalidateCurrentLap();
        break;
    case GhostSetupTransition::InitialBinding:
        // A first valid binding can recover before the first gate clock
        // advances. A mid-lap binding must wait for an ordered boundary.
        if(atLapBoundary)ghost.StartLap();
        else ghost.InvalidateCurrentLap();
        break;
    default:break;
    }
    return transition;
}
} // namespace latebrake
