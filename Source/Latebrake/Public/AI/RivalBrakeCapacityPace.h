#pragma once

#include "AI/RivalPilot.h"
#include "Vehicles/ServiceBrakeCapability.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace latebrake
{
struct RivalBrakeCapacitySample final
{
    double DeltaSeconds = 0.0;
    std::uint64_t TeleportRevision = 0;
    bool PhysicsQualified = false;
    AcceptedBrakeCapability Capability{};
};

struct RivalBrakeCapacityResult final
{
    PilotCommand Command{};
    bool Valid = false;
    bool CapacityAvailable = false;
    double DecelerationCeilingCmPerSecondSquared = 0.0;
    std::uint64_t SourceSequence = 0;
};

// Loss of estimated braking acts immediately. Recovery of a larger
// estimate is rate-limited, so a crest landing or one good road frame
// cannot immediately restore an optimistic high-speed corner forecast.
class RivalBrakeCapacityPace final
{
public:
    static constexpr double RecoveryCmPerSecondCubed = 360.0;

    void Reset() noexcept
    {
        HasState = false;
        Ceiling = 0.0;
    }

    RivalBrakeCapacityResult Apply(PilotCommand command,
                                  const RivalBrakeCapacitySample& s) noexcept
    {
        if (!std::isfinite(command.Throttle) || command.Throttle < 0.0 ||
            command.Throttle > 1.0 || !std::isfinite(command.Steering) ||
            std::abs(command.Steering) > 1.0)
        {
            Reset();
            return {};
        }
        const auto reject = [&]() noexcept
        {
            Reset();
            command.Throttle = 0.0;
            command.Brake = true;
            return RivalBrakeCapacityResult{command, false, false, 0.0, 0};
        };
        const auto& c = s.Capability;
        if (!s.PhysicsQualified || !c.Valid || c.Sequence == 0 ||
            !std::isfinite(s.DeltaSeconds) || s.DeltaSeconds <= 0.0 ||
            s.DeltaSeconds > SuspensionMaximumFrameSeconds ||
            !std::isfinite(c.FrameSeconds) || c.FrameSeconds <= 0.0 ||
            c.FrameSeconds > SuspensionMaximumFrameSeconds ||
            c.TeleportRevision != s.TeleportRevision ||
            !ServiceBrakeCapabilityLedger::IsConsistent(c.Estimate) ||
            c.Sequence <= LastSequence) return reject();
        const double capacity = c.Estimate.PlanningDecelerationCmPerSecondSquared;
        if (!HasState)
            Ceiling = capacity * 0.5;
        else
            Ceiling = std::min(capacity, Ceiling +
                RecoveryCmPerSecondCubed * std::min(s.DeltaSeconds, c.FrameSeconds));
        LastSequence = c.Sequence;
        HasState = true;
        const bool available = c.Estimate.Supported && Ceiling > 1.0e-6;
        if (!available)
        {
            command.Throttle = 0.0;
            command.Brake = true;
        }
        return {command, true, available, Ceiling, LastSequence};
    }

private:
    bool HasState = false;
    std::uint64_t LastSequence = 0;
    double Ceiling = 0.0;
};
} // namespace latebrake
