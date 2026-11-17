#pragma once

#include "AI/RivalFleetCollisionForecast.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake
{
// Stateful six-car traffic brake coordinator. Every invocation is tied to
// the actual current physical actor roster, not elapsed game clock, UI,
// SaveGame or a guessed actor. This adds a longer-horizon, smooth-release
// *restriction* after the original short-horizon traffic policy.
//
// The coordinator never steers, adds throttle, changes rank/reward/lap,
// manipulates Unreal physics, or makes a future collision-free guarantee.
inline constexpr double FleetRiskMaximumFrameSeconds = 0.10;
inline constexpr double FleetRiskEmergencyHoldSeconds = 0.38;
inline constexpr double FleetRiskRosterChangeHoldSeconds = 0.45;
inline constexpr double FleetRiskThrottleRecoveryPerSecond = 0.95;

struct FleetRiskSupervision final
{
    PilotCommand Command{};
    FleetRiskReport Forecast{};
    bool Qualified = false;
    bool HoldingBrake = false;
    bool RosterChanged = false;
    double RemainingHoldSeconds = 0.0;
    double ThrottleCeiling = 0.0;
};

// Every epoch/recovery/reset must invalidate prior speed and traffic
// assumptions. Return a complete read-only snapshot for tests and future
// debug telemetry; it does NOT participate in player race authority.
class RivalFleetRiskSupervisor final
{
public:
    void Reset() noexcept
    {
        Roster.fill(0);
        HasRoster = false;
        RemainingHold = 0.0;
        ThrottleLimit = 1.0;
        FramesObserved = 0;
        HazardFrames = 0;
        LastThreatSlot = -1;
    }

    bool HasPhysicalRoster() const noexcept { return HasRoster; }
    double HoldSecondsRemaining() const noexcept { return RemainingHold; }
    double CurrentThrottleLimit() const noexcept { return ThrottleLimit; }
    std::uint64_t QualifiedFrames() const noexcept { return FramesObserved; }
    std::uint64_t EmergencyFrames() const noexcept { return HazardFrames; }
    int LastThreat() const noexcept { return LastThreatSlot; }

    FleetRiskSupervision Apply(
        PilotCommand planned,
        const RivalFleetTrafficSample& physical,
        double deltaSeconds) noexcept
    {
        // A temporary renderer/engine hitch, invalid route command or
        // missing actual actor cannot authorize throttle from old state.
        if (!std::isfinite(deltaSeconds) ||
            deltaSeconds <= 0.0 ||
            deltaSeconds > FleetRiskMaximumFrameSeconds ||
            !std::isfinite(planned.Throttle) ||
            !std::isfinite(planned.Steering) ||
            planned.Throttle < 0.0 || planned.Throttle > 1.0 ||
            planned.Steering < -1.0 || planned.Steering > 1.0)
        {
            Reset();
            return {};
        }

        const FleetRiskReport forecast = ForecastSixCarRisk(physical);
        if (!forecast.Qualified())
        {
            Reset();
            return {{}, forecast, false, true, false, 0.0, 0.0};
        }

        std::array<std::uint64_t, StartGridCapacity> observed{};
        if (!ValidateSixCarFleet(physical, observed))
        {
            Reset();
            return {};
        }

        const bool rosterChanged = HasRoster && observed != Roster;
        if (rosterChanged)
        {
            // A replaced/destroyed and respawned Chaos body cannot borrow
            // the previous car's clear road; one verified stable epoch is
            // required before allowing renewed propulsion.
            Reset();
            Roster = observed;
            HasRoster = true;
            RemainingHold = FleetRiskRosterChangeHoldSeconds;
            ThrottleLimit = 0.0;
            ++FramesObserved;
            LastThreatSlot = forecast.LimitingSlot;
            PilotCommand held = planned;
            held.Throttle = 0.0;
            held.Brake = true;
            return {held, forecast, true, true, true,
                    RemainingHold, ThrottleLimit};
        }

        if (!HasRoster)
        {
            Roster = observed;
            HasRoster = true;
            RemainingHold = 0.0;
            ThrottleLimit = 1.0;
        }
        ++FramesObserved;

        // The route's already-authorized arrival is never fabricated by
        // traffic and does not preserve an old hazard past recovery.
        if (planned.Arrived)
        {
            Reset();
            return {planned, forecast, true, false, false, 0.0, 1.0};
        }

        if (forecast.Emergency())
        {
            ++HazardFrames;
            RemainingHold = FleetRiskEmergencyHoldSeconds;
            ThrottleLimit = 0.0;
            LastThreatSlot = forecast.LimitingSlot;
            planned.Throttle = 0.0;
            planned.Brake = true;
            return {planned, forecast, true, true, false,
                    RemainingHold, ThrottleLimit};
        }

        if (RemainingHold > 0.0)
        {
            // Once there is no current hazard, pay down the hysteresis
            // using only finite frame time, never wall clock or tick count.
            RemainingHold =
                std::max(0.0, RemainingHold - deltaSeconds);
            if (RemainingHold > 0.0)
            {
                ThrottleLimit = 0.0;
                planned.Throttle = 0.0;
                planned.Brake = true;
                return {planned, forecast, true, true, false,
                        RemainingHold, ThrottleLimit};
            }
        }

        // Release motor demand smoothly after an emergency; an unrelated
        // route/heat/grip/short-horizon policy may restrict it further.
        // Reducing the throttle cap is instantaneous, increasing it obeys
        // a bounded per-second slope. Never increase planned.Throttle.
        const double target = std::clamp(
            forecast.ThrottleCeiling, 0.0, 1.0);
        if (target < ThrottleLimit)
        {
            ThrottleLimit = target;
        }
        else
        {
            ThrottleLimit = std::min(
                target, ThrottleLimit +
                FleetRiskThrottleRecoveryPerSecond * deltaSeconds);
        }
        planned.Throttle = std::min(planned.Throttle, ThrottleLimit);
        if (planned.Brake)
        {
            planned.Throttle = 0.0;
        }
        return {planned, forecast, true, false, false,
                RemainingHold, ThrottleLimit};
    }

private:
    std::array<std::uint64_t, StartGridCapacity> Roster{};
    bool HasRoster = false;
    double RemainingHold = 0.0;
    double ThrottleLimit = 1.0;
    std::uint64_t FramesObserved = 0;
    std::uint64_t HazardFrames = 0;
    int LastThreatSlot = -1;
};

} // namespace latebrake
