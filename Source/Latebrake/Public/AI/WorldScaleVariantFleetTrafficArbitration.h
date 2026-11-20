#pragma once

#include "AI/WorldScaleVariantTrafficSupervisor.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// A source-only SIX-CAR atomic arbitration receipt.
// No car-specific proposal is qualified until ALL six upstream
// real-source pilot/fleet receipts agree on the same physics frame.
// Does not command physical brakes, create UE actors or change
// official race classification. The owner of an actual Unreal
// vehicle must independently arbitrate physical brake authority.
struct WorldScaleVariantFleetTrafficReadout final
{
    std::array<WorldScaleVariantTrafficReceipt,
               WorldScaleVariantFleetCars> Car{};
    WorldScaleVariantId SourceTrack=WorldScaleVariantId::OriginalLoop;
    std::uint64_t SourceSessionToken=0;
    std::uint64_t SourceFrameOrdinal=0;
    unsigned EmergencyBrakeCount=0;
    unsigned RestrictedThrottleCount=0;
    bool AllSixPilotsPhysicallyQualified=false;
    bool NoOfficialLapCredit=true;
    bool NoLiveActuatorAuthority=true;
    bool Valid=false;
};

inline WorldScaleVariantFleetTrafficReadout
    RejectWorldScaleVariantFleetTraffic() noexcept
{
    WorldScaleVariantFleetTrafficReadout fail{};
    for(auto& car:fail.Car)
    {
        car.Decision=WorldScaleVariantTrafficSafeStop();
        car.NoLiveBrakeActuatorAuthority=true;
    }
    return fail;
}

inline WorldScaleVariantFleetTrafficReadout
    ApplyWorldScaleVariantFleetTraffic(
    const std::array<WorldScaleVariantPilotReadout,
                     WorldScaleVariantFleetCars>& pilots,
    const WorldScaleVariantFleetReadout& fleet,
    WorldScaleVariantTrafficSettings settings={}) noexcept
{
    auto result=RejectWorldScaleVariantFleetTraffic();
    if(!ValidWorldScaleVariantTrafficSettings(settings) ||
       !fleet.Valid ||
       !fleet.MotionContinuous ||
       fleet.AcceptedContinuousFrames==0 ||
       !fleet.NoOfficialLapCredit ||
       !fleet.DoesNotControlVehicles ||
       !fleet.AdvisoryOnly ||
       fleet.SourceSessionToken==0 ||
       fleet.SourceFrameOrdinal==0)
    {
        return result;
    }

    // Preflight all six sources BEFORE computing any individual
    // decision. A mixed-frame roster does not expose even one
    // "valid" row of partial traffic advice.
    for(unsigned slot=0;slot<WorldScaleVariantFleetCars;++slot)
    {
        const auto& pilot=pilots[slot];
        const auto& actor=fleet.CarsBySlot[slot];
        if(!pilot.Valid ||
           !pilot.Decision.Valid ||
           !pilot.OriginalPhysicalRoadVerified ||
           pilot.HasLiveActuatorAuthority ||
           pilot.OfficialLapCredit ||
           pilot.SourceActor==0 ||
           pilot.SourceActor!=actor.ActorToken ||
           pilot.SourceSessionToken!=fleet.SourceSessionToken ||
           pilot.SourceFrame!=fleet.SourceFrameOrdinal ||
           pilot.SourceTrack!=fleet.TrackId ||
           actor.Slot!=static_cast<int>(slot) ||
           !actor.WithinOriginalRoad)
        {
            return result;
        }
    }

    for(unsigned slot=0;slot<WorldScaleVariantFleetCars;++slot)
    {
        const auto proposal=ApplyWorldScaleVariantTrafficSupervisor(
            pilots[slot],fleet,static_cast<int>(slot),settings);
        if(!proposal.Valid ||
           proposal.SelfSlot!=static_cast<int>(slot) ||
           proposal.SourceFrameOrdinal!=fleet.SourceFrameOrdinal ||
           proposal.SourceSessionToken!=fleet.SourceSessionToken ||
           proposal.SourceTrack!=fleet.TrackId ||
           !proposal.NoLiveBrakeActuatorAuthority)
        {
            return RejectWorldScaleVariantFleetTraffic();
        }
        result.Car[slot]=proposal;
        if(proposal.EmergencyBrake)
            ++result.EmergencyBrakeCount;
        if(proposal.FollowingThrottleRestricted)
            ++result.RestrictedThrottleCount;
    }

    result.SourceTrack=fleet.TrackId;
    result.SourceSessionToken=fleet.SourceSessionToken;
    result.SourceFrameOrdinal=fleet.SourceFrameOrdinal;
    result.AllSixPilotsPhysicallyQualified=true;
    result.Valid=true;
    return result;
}
} // namespace latebrake
