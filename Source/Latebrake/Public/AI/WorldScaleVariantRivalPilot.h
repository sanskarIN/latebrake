#pragma once

#include "AI/WorldScaleCandidateRivalPilot.h"
#include "Track/WorldScaleVariantRacePace.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake
{
// One measured physical rival / one future original circuit source.
// This is only a validated control ADVISORY, never an engine actuator,
// fake AI actor, six-car session clock, checkpoint or qualifying race.
inline constexpr double WorldScaleVariantPilotClockSkewSeconds=0.02;
inline constexpr double WorldScaleVariantPilotVelocityMismatchCmps=50.0;
inline constexpr double WorldScaleVariantPilotMotionSlackCm=10.0;

struct WorldScaleVariantPilotObservation final
{
    WorldScalePilotSample Physical{};
    CircuitPoint MeasuredWorldVelocityCmps{};
    std::uint64_t SessionToken=0;
    std::uint64_t ActorToken=0;
    std::uint64_t PhysicsFrame=0;
    double PhysicsClockSeconds=0.0;
    bool AuthenticatedActorObservation=false;
    bool OriginalRoadContactVerified=false;
};

struct WorldScaleVariantPilotReadout final
{
    bool Valid=false;
    WorldScaleVariantId SourceTrack=WorldScaleVariantId::OriginalLoop;
    WorldScalePilotDecision Decision{};
    std::uint64_t SourceActor=0;
    std::uint64_t SourceSessionToken=0;
    std::uint64_t SourceFrame=0;
    double WorldMotionResidualCm=0.0;
    double SignedObservedRouteAdvanceCm=0.0;
    double ObservedCrossTrackCm=0.0;
    bool OriginalPhysicalRoadVerified=false;
    bool HasLiveActuatorAuthority=false;
    bool OfficialLapCredit=false;
};

class WorldScaleVariantRivalPilot final
{
public:
    void Reset() noexcept
    {
        LastPlan=nullptr;
        LastSession=0;
        LastActor=0;
        LastFrame=0;
        LastClock=0.0;
        LastPosition={};
        LastVelocity={};
        LastStation=0.0;
        HavePrevious=false;
    }

    WorldScaleVariantPilotReadout Step(
        const WorldScaleVariantPacePlan& plan,
        const WorldScaleVariantPilotObservation& observation) noexcept
    {
        const auto& sample=observation.Physical;
        if(!plan.Valid ||
           !ValidWorldScaleVariantRecipe(plan.Track) ||
           !sample.PhysicsReady || !sample.RaceGreen ||
           !observation.AuthenticatedActorObservation ||
           !observation.OriginalRoadContactVerified ||
           observation.SessionToken==0 ||
           observation.ActorToken==0 ||
           observation.PhysicsFrame==0 ||
           !std::isfinite(observation.PhysicsClockSeconds) ||
           observation.PhysicsClockSeconds<0.0 ||
           !std::isfinite(sample.Position.X) ||
           !std::isfinite(sample.Position.Y) ||
           std::abs(sample.Position.X)>1000000.0 ||
           std::abs(sample.Position.Y)>1000000.0 ||
           !std::isfinite(sample.HeadingRadians) ||
           std::abs(sample.HeadingRadians)>1000000.0 ||
           !std::isfinite(sample.FlatSpeedCmps) ||
           sample.FlatSpeedCmps<0.0 ||
           sample.FlatSpeedCmps>WorldScalePilotMaxObservedSpeedCmps ||
           !std::isfinite(sample.ForwardSpeedCmps) ||
           std::abs(sample.ForwardSpeedCmps)>
               sample.FlatSpeedCmps+0.1 ||
           !std::isfinite(sample.DeltaSeconds) ||
           sample.DeltaSeconds<=0.0 ||
           sample.DeltaSeconds>WorldScalePilotMaxFrameSeconds ||
           !std::isfinite(observation.MeasuredWorldVelocityCmps.X) ||
           !std::isfinite(observation.MeasuredWorldVelocityCmps.Y))
        {
            Reset();
            return {};
        }
        const double vx=observation.MeasuredWorldVelocityCmps.X;
        const double vy=observation.MeasuredWorldVelocityCmps.Y;
        const double speed=std::hypot(vx,vy);
        const double bodyForward=vx*std::cos(sample.HeadingRadians)+
            vy*std::sin(sample.HeadingRadians);
        const double speedTolerance=std::max(
            WorldScaleVariantPilotVelocityMismatchCmps,
            sample.FlatSpeedCmps*0.02);
        if(!std::isfinite(speed) ||
           !std::isfinite(bodyForward) ||
           std::abs(speed-sample.FlatSpeedCmps)>speedTolerance ||
           std::abs(bodyForward-sample.ForwardSpeedCmps)>
                speedTolerance)
        {
            Reset();
            return {};
        }

        const auto road=ProjectWorldScaleVariantPace(
            plan,sample.Position);
        if(!road.Valid || !road.InsidePhysicalCandidateRoad ||
           !std::isfinite(road.SignedLateralCm) ||
           !std::isfinite(road.CentrelineSeparationCm) ||
           road.CentrelineSeparationCm>WorldScaleVariantRoadHalfWidthCm)
        {
            Reset();
            return {};
        }
        const bool newSession=LastSession!=observation.SessionToken;
        if(newSession)
        {
            Reset();
            LastSession=observation.SessionToken;
            LastActor=observation.ActorToken;
            LastPlan=&plan;
        }
        else if(LastPlan!=&plan ||
                LastActor!=observation.ActorToken)
        {
            Reset();
            return {};
        }

        double residual=0.0;
        double progressAdvance=0.0;
        if(HavePrevious)
        {
            if(LastFrame==std::numeric_limits<std::uint64_t>::max() ||
               observation.PhysicsFrame!=LastFrame+1u ||
               observation.PhysicsClockSeconds<=LastClock ||
               std::abs(observation.PhysicsClockSeconds-
                        LastClock-sample.DeltaSeconds)>
                   WorldScaleVariantPilotClockSkewSeconds)
            {
                Reset();
                return {};
            }
            const double expectX=LastPosition.X+
                sample.DeltaSeconds*0.5*(vx+LastVelocity.X);
            const double expectY=LastPosition.Y+
                sample.DeltaSeconds*0.5*(vy+LastVelocity.Y);
            residual=std::hypot(
                sample.Position.X-expectX,
                sample.Position.Y-expectY);
            const double expectedTravel=sample.DeltaSeconds*
                0.5*(speed+std::hypot(
                    LastVelocity.X,LastVelocity.Y));
            const double slack=std::max(
                WorldScaleVariantPilotMotionSlackCm,
                expectedTravel*0.12);
            progressAdvance=std::remainder(
                road.LapDistanceCm-LastStation,
                plan.Track.LapLengthCm);
            if(!std::isfinite(residual) ||
               !std::isfinite(progressAdvance) ||
               residual>slack ||
               std::abs(progressAdvance)>
                   1.7*expectedTravel+plan.StepCm ||
               std::abs(progressAdvance)>
                   plan.Track.LapLengthCm*0.25)
            {
                Reset();
                return {};
            }
        }

        const double lookahead=std::clamp(
            450.0+0.22*sample.FlatSpeedCmps,
            WorldScalePilotMinLookaheadCm,
            WorldScalePilotMaxLookaheadCm);
        const auto target=SampleWorldScaleVariantPace(
            plan,road.LapDistanceCm+lookahead);
        if(!target.Valid)
        {
            Reset();
            return {};
        }
        const double tx=target.Pose.Position.X-sample.Position.X;
        const double ty=target.Pose.Position.Y-sample.Position.Y;
        if(!std::isfinite(tx) || !std::isfinite(ty) ||
           std::hypot(tx,ty)<1.0)
        {
            Reset();
            return {};
        }
        const double desiredHeading=std::atan2(ty,tx);
        const double error=std::remainder(
            desiredHeading-sample.HeadingRadians,
            2.0*WorldScalePilotRadiansPi);
        const double steer=std::clamp(error/0.65,-1.0,1.0);
        const double limit=std::min(
            road.SuggestedSpeedCmps,target.TargetSpeedCmps);
        if(!std::isfinite(error) ||
           !std::isfinite(steer) ||
           !std::isfinite(limit) ||
           limit<WorldScalePaceMinSpeedCmps ||
           limit>WorldScalePaceMaxSpeedCmps)
        {
            Reset();
            return {};
        }
        const bool reverse=sample.ForwardSpeedCmps< -50.0;
        const bool badlyAligned=std::abs(error)>2.2 ||
            (std::abs(error)>0.8 && sample.FlatSpeedCmps>1300.0);
        const bool brake=reverse || badlyAligned ||
            sample.FlatSpeedCmps>limit+120.0;
        const double throttle=(!brake &&
            sample.FlatSpeedCmps+120.0<limit)?0.85:0.0;
        const WorldScalePilotDecision decision{
            true,{throttle,steer,brake,false},
            road.LapDistanceCm,road.SignedLateralCm,
            lookahead,limit,error
        };

        LastPlan=&plan;
        LastSession=observation.SessionToken;
        LastActor=observation.ActorToken;
        LastFrame=observation.PhysicsFrame;
        LastClock=observation.PhysicsClockSeconds;
        LastPosition=sample.Position;
        LastVelocity=observation.MeasuredWorldVelocityCmps;
        LastStation=road.LapDistanceCm;
        HavePrevious=true;
        return {
            true,plan.Track.Id,decision,
            observation.ActorToken,
            observation.SessionToken,observation.PhysicsFrame,
            residual,progressAdvance,road.SignedLateralCm,
            true,false,false
        };
    }

private:
    const WorldScaleVariantPacePlan* LastPlan=nullptr;
    std::uint64_t LastSession=0;
    std::uint64_t LastActor=0;
    std::uint64_t LastFrame=0;
    double LastClock=0.0;
    CircuitPoint LastPosition{};
    CircuitPoint LastVelocity{};
    double LastStation=0.0;
    bool HavePrevious=false;
};
} // namespace latebrake
