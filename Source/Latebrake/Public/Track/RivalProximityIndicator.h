#pragma once

#include "Track/RaceMiniMap.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Advisory HUD state only. A player's checkpoint order, race clock,
// collision response and awards must never depend on this warning.
//
// The warning requires continuous, real physics-backed player + rival
// position samples for 0.6s within 15m DIRECT planar separation.
// Once visible, it stays visible through 20m to avoid flickering.
// Game mode, pause, focus loss, teleport and pawn replacement must reset it.
class RivalProximityIndicator final
{
public:
    void Reset() noexcept
    {
        NearSeconds = 0.0;
        Active = false;
    }

    bool Update(const RaceMiniMapSnapshot& radar,
                double deltaSeconds, bool raceEligible) noexcept
    {
        if (!raceEligible ||
            !std::isfinite(deltaSeconds) ||
            deltaSeconds <= 0.0 || deltaSeconds > 0.25 ||
            !radar.Player.Position.Valid ||
            !radar.Rival.Valid ||
            !radar.DirectRivalSeparationValid ||
            !std::isfinite(radar.DirectRivalSeparationMeters) ||
            radar.DirectRivalSeparationMeters < 0.0)
        {
            Reset();
            return false;
        }

        constexpr double EnterMeters = 15.0;
        constexpr double ExitMeters = 20.0;
        constexpr double DebounceSeconds = 0.6;

        if (Active)
        {
            if (radar.DirectRivalSeparationMeters > ExitMeters)
            {
                Reset();
            }
            return Active;
        }
        if (radar.DirectRivalSeparationMeters > EnterMeters)
        {
            NearSeconds = 0.0;
            return false;
        }
        NearSeconds = std::min(DebounceSeconds,
                               NearSeconds + deltaSeconds);
        Active = NearSeconds >= DebounceSeconds;
        return Active;
    }

    bool IsActive() const noexcept { return Active; }
    double GetNearSeconds() const noexcept { return NearSeconds; }

private:
    double NearSeconds = 0.0;
    bool Active = false;
};
} // namespace latebrake
