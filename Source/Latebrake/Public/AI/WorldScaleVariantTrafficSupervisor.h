#pragma once

#include "AI/WorldScaleVariantFleetContinuityAudit.h"
#include "AI/WorldScaleVariantRivalPilot.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Portable SOURCE-ONLY measured-six-car following brake arbitration.
// It never creates synthetic actors, a passing command, collision
// immunity, official flags/penalties, or any actual Chaos vehicle force.
// The native owner must independently ensure all six genuine physical
// bodies are in the SAME physics snapshot before using this advisory.
inline constexpr double WorldScaleVariantTrafficMaxVehicleSpeedCmps =
    WorldScaleVariantFleetMaximumSpeedCmps;

struct WorldScaleVariantTrafficSettings final
{
    double VehicleHalfLengthCm=205.0;
    double VehicleHalfWidthCm=95.0;
    double MinimumChassisClearanceCm=75.0;
    double LaneConflictLateralCm=280.0;
    double MaximumRelevantForwardGapCm=3500.0;
    double FollowingThrottleGapCm=2200.0;
    double ReactionSeconds=0.27;
    double AvailableBrakingCmps2=520.0;
    double CautiousThrottleCeiling=0.15;
    bool RequireAllSixRealActors=true;
    bool NeverIncreasePilotThrottle=true;
    bool NeverReleaseOriginalPilotBrake=true;
    bool NoSteeringOrCollisionAuthority=true;
};

struct WorldScaleVariantTrafficReceipt final
{
    bool Valid=false;
    bool EmergencyBrake=false;
    bool FollowingThrottleRestricted=false;
    bool FullPhysicalRosterQualified=false;
    bool NoLiveBrakeActuatorAuthority=true;
    int SelfSlot=-1;
    int LimitingOtherSlot=-1;
    double ClosestRelevantGapCm=0.0;
    double MaximumApproachSpeedCmps=0.0;
    double ConservativeStopDistanceCm=0.0;
    double ThrottleCeiling=0.0;
    WorldScalePilotDecision Decision{};
    std::uint64_t SourceFrameOrdinal=0;
    std::uint64_t SourceSessionToken=0;
    WorldScaleVariantId SourceTrack=WorldScaleVariantId::OriginalLoop;
};

inline bool ValidWorldScaleVariantTrafficSettings(
    WorldScaleVariantTrafficSettings p) noexcept
{
    return std::isfinite(p.VehicleHalfLengthCm) &&
        p.VehicleHalfLengthCm>=150.0 &&
        p.VehicleHalfLengthCm<=240.0 &&
        std::isfinite(p.VehicleHalfWidthCm) &&
        p.VehicleHalfWidthCm>=70.0 &&
        p.VehicleHalfWidthCm<=130.0 &&
        std::isfinite(p.MinimumChassisClearanceCm) &&
        p.MinimumChassisClearanceCm>=40.0 &&
        p.MinimumChassisClearanceCm<=150.0 &&
        std::isfinite(p.LaneConflictLateralCm) &&
        p.LaneConflictLateralCm>=
            2.0*p.VehicleHalfWidthCm &&
        p.LaneConflictLateralCm<=500.0 &&
        std::isfinite(p.MaximumRelevantForwardGapCm) &&
        p.MaximumRelevantForwardGapCm>=1500.0 &&
        p.MaximumRelevantForwardGapCm<=8000.0 &&
        std::isfinite(p.FollowingThrottleGapCm) &&
        p.FollowingThrottleGapCm>=
            2.0*p.VehicleHalfLengthCm+
                p.MinimumChassisClearanceCm &&
        p.FollowingThrottleGapCm<=
            p.MaximumRelevantForwardGapCm &&
        std::isfinite(p.ReactionSeconds) &&
        p.ReactionSeconds>=0.1 &&
        p.ReactionSeconds<=0.5 &&
        std::isfinite(p.AvailableBrakingCmps2) &&
        p.AvailableBrakingCmps2>=200.0 &&
        p.AvailableBrakingCmps2<=800.0 &&
        std::isfinite(p.CautiousThrottleCeiling) &&
        p.CautiousThrottleCeiling>=0.0 &&
        p.CautiousThrottleCeiling<=0.30 &&
        p.RequireAllSixRealActors &&
        p.NeverIncreasePilotThrottle &&
        p.NeverReleaseOriginalPilotBrake &&
        p.NoSteeringOrCollisionAuthority;
}

inline WorldScalePilotDecision WorldScaleVariantTrafficSafeStop() noexcept
{
    WorldScalePilotDecision stopped{};
    stopped.Command={0.0,0.0,true,false};
    return stopped;
}

// Read-only and stateless: a future native adapter MUST call the
// original safety/ABS/surface/Collision supervisor afterward as well.
inline WorldScaleVariantTrafficReceipt
    ApplyWorldScaleVariantTrafficSupervisor(
    const WorldScaleVariantPilotReadout& pilot,
    const WorldScaleVariantFleetReadout& fleet,
    int selfSlot,
    WorldScaleVariantTrafficSettings settings = {}) noexcept
{
    WorldScaleVariantTrafficReceipt result{};
    result.Decision=WorldScaleVariantTrafficSafeStop();
    if(!ValidWorldScaleVariantTrafficSettings(settings) ||
       !pilot.Valid || !pilot.Decision.Valid ||
       !fleet.Valid || !fleet.MotionContinuous ||
       fleet.AcceptedContinuousFrames==0 ||
       !fleet.NoOfficialLapCredit ||
       !fleet.DoesNotControlVehicles ||
       !fleet.AdvisoryOnly ||
       !pilot.OriginalPhysicalRoadVerified ||
       pilot.HasLiveActuatorAuthority ||
       pilot.OfficialLapCredit ||
       selfSlot<0 ||
       selfSlot>=static_cast<int>(WorldScaleVariantFleetCars) ||
       pilot.SourceActor==0 ||
       pilot.SourceSessionToken==0 ||
       pilot.SourceFrame==0 ||
       pilot.SourceTrack!=fleet.TrackId ||
       pilot.SourceSessionToken!=fleet.SourceSessionToken ||
       pilot.SourceFrame!=fleet.SourceFrameOrdinal ||
       fleet.CarsBySlot[static_cast<unsigned>(selfSlot)]
           .ActorToken!=pilot.SourceActor ||
       !std::isfinite(pilot.Decision.Command.Throttle) ||
       !std::isfinite(pilot.Decision.Command.Steering) ||
       pilot.Decision.Command.Throttle<0.0 ||
       pilot.Decision.Command.Throttle>1.0 ||
       pilot.Decision.Command.Steering<-1.0 ||
       pilot.Decision.Command.Steering>1.0)
        return result;

    // Standings use signed unwrapped distance. Physical collisions do
    // not: a lapped rival is still on the same asphalt. Resolve every
    // forward gap modulo this circuit's own authored lap length,
    // including signed-revolution differences across the start line.
    // Never use the original fixed 1.7km length on other variants.
    const auto track=WorldScaleVariantAt(
        static_cast<unsigned>(fleet.TrackId));
    if(!ValidWorldScaleVariantRecipe(track))
        return result;
    const auto& self=fleet.CarsBySlot[
        static_cast<unsigned>(selfSlot)];
    std::array<std::uint64_t,WorldScaleVariantFleetCars> seen{};
    for(unsigned slot=0;slot<WorldScaleVariantFleetCars;++slot)
    {
        const auto& body=fleet.CarsBySlot[slot];
        if(body.Slot!=static_cast<int>(slot) ||
           body.ActorToken==0 ||
           body.SourceLapStationCm<0.0 ||
           !body.WithinOriginalRoad ||
           !std::isfinite(body.UnwrappedProjectedStationCm) ||
           !std::isfinite(body.SignedLateralOffsetCm) ||
           !std::isfinite(body.SignedRouteSpeedCmps) ||
           !std::isfinite(body.MeasuredSpeedCmps) ||
           body.MeasuredSpeedCmps<0.0 ||
           body.MeasuredSpeedCmps>
               WorldScaleVariantTrafficMaxVehicleSpeedCmps)
            return result;
        for(unsigned earlier=0;earlier<slot;++earlier)
        {
            if(seen[earlier]==body.ActorToken)
                return result;
        }
        seen[slot]=body.ActorToken;
    }
    if(!std::isfinite(self.SignedRouteSpeedCmps) ||
       self.SignedRouteSpeedCmps<
           -WorldScaleVariantTrafficMaxVehicleSpeedCmps ||
       self.SignedRouteSpeedCmps>
           WorldScaleVariantTrafficMaxVehicleSpeedCmps)
        return result;

    double closest=settings.MaximumRelevantForwardGapCm;
    double closing=0.0;
    double requiredStop=0.0;
    bool emergency=false;
    bool following=false;
    int limiting=-1;
    for(unsigned slot=0;slot<WorldScaleVariantFleetCars;++slot)
    {
        if(static_cast<int>(slot)==selfSlot)continue;
        const auto& other=fleet.CarsBySlot[slot];
        const double signedGap=
            other.UnwrappedProjectedStationCm-
            self.UnwrappedProjectedStationCm;
        const double gap=WorldScaleVariantWrapDistance(
            track,signedGap);
        if(!std::isfinite(gap))return result;
        if(gap>settings.MaximumRelevantForwardGapCm)
            continue;
        const double lateral=std::abs(
            other.SignedLateralOffsetCm-
            self.SignedLateralOffsetCm);
        if(!std::isfinite(lateral) ||
           lateral>settings.LaneConflictLateralCm)
            continue;
        const double relative=std::max(
            0.0,self.SignedRouteSpeedCmps-
                other.SignedRouteSpeedCmps);
        const double stop=2.0*settings.VehicleHalfLengthCm+
            settings.MinimumChassisClearanceCm+
            settings.ReactionSeconds*
                std::max(0.0,self.SignedRouteSpeedCmps)+
            relative*relative/
                (2.0*settings.AvailableBrakingCmps2);
        if(!std::isfinite(stop) || !std::isfinite(relative))
            return result;
        const bool contactThreat=gap<=stop;
        const bool approach=gap<=
            settings.FollowingThrottleGapCm;
        if(contactThreat || approach)
        {
            if(limiting<0 || gap<closest)
            {
                closest=gap;
                closing=relative;
                requiredStop=stop;
                limiting=static_cast<int>(slot);
            }
        }
        emergency=emergency || contactThreat;
        following=following || approach;
    }

    WorldScalePilotDecision decision=pilot.Decision;
    double throttleCeiling=pilot.Decision.Command.Throttle;
    if(emergency)
    {
        decision.Command.Brake=true;
        decision.Command.Throttle=0.0;
        throttleCeiling=0.0;
    }
    else if(following)
    {
        throttleCeiling=std::min(throttleCeiling,
            settings.CautiousThrottleCeiling);
        decision.Command.Throttle=throttleCeiling;
    }
    if(pilot.Decision.Command.Brake)
    {
        decision.Command.Brake=true;
        decision.Command.Throttle=0.0;
        throttleCeiling=0.0;
    }
    if(decision.Command.Throttle>
           pilot.Decision.Command.Throttle ||
       decision.Command.Steering!=
           pilot.Decision.Command.Steering ||
       (!pilot.Decision.Command.Brake &&
         decision.Command.Brake &&
         decision.Command.Throttle!=0.0) ||
       (pilot.Decision.Command.Brake &&
         !decision.Command.Brake))
        return result;

    result.Valid=true;
    result.EmergencyBrake=emergency;
    result.FollowingThrottleRestricted=
        following && !emergency;
    result.FullPhysicalRosterQualified=true;
    result.SelfSlot=selfSlot;
    result.LimitingOtherSlot=limiting;
    result.ClosestRelevantGapCm=closest;
    result.MaximumApproachSpeedCmps=closing;
    result.ConservativeStopDistanceCm=requiredStop;
    result.ThrottleCeiling=throttleCeiling;
    result.Decision=decision;
    result.SourceFrameOrdinal=fleet.SourceFrameOrdinal;
    result.SourceSessionToken=fleet.SourceSessionToken;
    result.SourceTrack=fleet.TrackId;
    return result;
}
} // namespace latebrake
