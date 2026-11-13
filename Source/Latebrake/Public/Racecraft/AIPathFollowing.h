#pragma once
#include "Racecraft/CornerSpeedPlanner.h"
#include "Racecraft/SteeringKinematics.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
struct AIPathSettings final
{
    double LookaheadBaseCm=360.0;
    double LookaheadSpeedSeconds=0.45;
    double LookaheadMaximumCm=2400.0;
    double MaximumSteeringRadians=0.55;
    double MaximumSteeringInput=1.0;
    double MinimumForwardSpeedCmS=80.0;
    double MaximumForwardSpeedCmS=6000.0;
    double LateralErrorP=0.003;
    double LateralErrorD=0.00015;
    double HeadingErrorP=1.5;
    double YawRateDamping=0.17;
    double ForwardCurvatureGain=1.0;
    double FeedForwardSteeringGain=1.0;
    double ThrottleSpeedErrorGain=0.001;
    double BrakeSpeedErrorGain=0.0016;
    double MaximumThrottle=0.88;
    double MinimumThrottle=0.0;
    double MinimumBrakeOnOverspeed=0.12;
    double MaximumBrake=1.0;
    double Aggression=0.65;
    double CornerConfidenceScale=0.75;
    double MinimumPhysicalGripG=0.25;
    double MaxCrossTrackCm=800.0;
    double ExcessCrossTrackEmergencyCm=1000.0;
    double MaximumHeadingErrorRadians=1.2;
    double MaximumYawRateRadS=6.0;
    double MaximumFrameSeconds=0.1;
    double SteeringRisePerSecond=3.0;
    double SteeringFallPerSecond=4.5;
    double ThrottleRecoveryPerSecond=1.4;
    double BrakeReleasePerSecond=2.4;
    double CornerBrakingMarginCm=120.0;
    double MinimumSafeRoadMarginCm=180.0;
    bool RequireFullPhysicalCar=true;
    bool EmergencyStopForOppositeDirection=true;
    bool RestrictThrottleOnOffPavement=true;
    bool NoDriverControlAuthority=true;
};

struct AIPathInput final
{
    PhysicalVehicleFrame Car{};
    SmoothLineGeometry Course{};
    SpeedPlanResult SpeedPlan{};
    double DeltaSeconds=0.0;
    double TrafficThrottleCeiling=1.0;
    double TrafficBrakeFloor=0.0;
    double AvailableGripG=1.0;
    double RouteConfidence=1.0;
    bool OriginalEnginePhysicsQualified=false;
    bool ActualRivalActor=true;
    bool EngineRaceRunning=false;
};

struct AIPathOutput final
{
    double ProposedThrottle=0.0;
    double ProposedBrake=1.0;
    double ProposedSteering=0.0;
    double TargetSpeedCmS=0.0;
    double LookaheadCm=0.0;
    double TargetHeadingRadians=0.0;
    double HeadingErrorRadians=0.0;
    double LateralErrorCm=0.0;
    double LateralErrorRateCmS=0.0;
    double TargetCurvaturePerCm=0.0;
    double PredictedYawRateRadiansSecond=0.0;
    double MeasuredYawRateRadiansSecond=0.0;
    double SpeedErrorCmS=0.0;
    double RoadMarginCm=0.0;
    double AppliedThrottleCeiling=0.0;
    double AppliedBrakeFloor=0.0;
    double TargetDistanceCm=0.0;
    double SteeringFeedForward=0.0;
    double SteeringFeedback=0.0;
    double BrakingDistanceCm=0.0;
    bool WrongDirection=false;
    bool OffOriginalPavement=false;
    bool CrossTrackEmergency=false;
    bool SpeedPlanMissing=false;
    bool NeedsBraking=false;
    bool HoldingStop=false;
    bool MotorNeverRaised=true;
    bool HasUnrealControlAuthority=false;
    bool Valid=false;
};

inline bool ValidAIPathSettings(AIPathSettings s) noexcept
{
    return Finite(s.LookaheadBaseCm) &&
        s.LookaheadBaseCm>0.0 &&
        Finite(s.LookaheadSpeedSeconds) &&
        s.LookaheadSpeedSeconds>0.0 &&
        Finite(s.LookaheadMaximumCm) &&
        s.LookaheadMaximumCm>s.LookaheadBaseCm &&
        Finite(s.MaximumSteeringRadians) &&
        s.MaximumSteeringRadians>0.0 &&
        Finite(s.MaximumSteeringInput) &&
        s.MaximumSteeringInput>0.0 &&
        s.MaximumSteeringInput<=1.0 &&
        Finite(s.MinimumForwardSpeedCmS) &&
        s.MinimumForwardSpeedCmS>=0.0 &&
        Finite(s.MaximumForwardSpeedCmS) &&
        s.MaximumForwardSpeedCmS>s.MinimumForwardSpeedCmS &&
        Finite(s.LateralErrorP) &&
        s.LateralErrorP>=0.0 &&
        Finite(s.LateralErrorD) &&
        s.LateralErrorD>=0.0 &&
        Finite(s.HeadingErrorP) &&
        s.HeadingErrorP>=0.0 &&
        Finite(s.YawRateDamping) &&
        s.YawRateDamping>=0.0 &&
        Finite(s.ThrottleSpeedErrorGain) &&
        s.ThrottleSpeedErrorGain>0.0 &&
        Finite(s.BrakeSpeedErrorGain) &&
        s.BrakeSpeedErrorGain>0.0 &&
        Finite(s.MaximumThrottle) &&
        s.MaximumThrottle>0.0 &&
        s.MaximumThrottle<=1.0 &&
        Finite(s.MaximumBrake) &&
        s.MaximumBrake>0.0 &&
        s.MaximumBrake<=1.0 &&
        Finite(s.Aggression) &&
        s.Aggression>=0.0 &&
        s.Aggression<=1.0 &&
        Finite(s.MinimumPhysicalGripG) &&
        s.MinimumPhysicalGripG>0.0 &&
        Finite(s.MaxCrossTrackCm) &&
        s.MaxCrossTrackCm>0.0 &&
        Finite(s.ExcessCrossTrackEmergencyCm) &&
        s.ExcessCrossTrackEmergencyCm>=s.MaxCrossTrackCm &&
        Finite(s.MaximumHeadingErrorRadians) &&
        s.MaximumHeadingErrorRadians>0.0 &&
        Finite(s.SteeringRisePerSecond) &&
        s.SteeringRisePerSecond>0.0 &&
        Finite(s.SteeringFallPerSecond) &&
        s.SteeringFallPerSecond>0.0 &&
        Finite(s.ThrottleRecoveryPerSecond) &&
        s.ThrottleRecoveryPerSecond>0.0 &&
        Finite(s.BrakeReleasePerSecond) &&
        s.BrakeReleasePerSecond>0.0;
}

inline bool ValidAIPathInput(AIPathInput input,
                             AIPathSettings s) noexcept
{
    return input.OriginalEnginePhysicsQualified &&
        input.ActualRivalActor &&
        input.EngineRaceRunning &&
        ValidateVehicleFrame(input.Car,
            s.RequireFullPhysicalCar)==SnapshotFault::None &&
        input.Course.Valid &&
        input.SpeedPlan.Valid &&
        ValidFrameDelta(input.DeltaSeconds) &&
        Finite(input.TrafficThrottleCeiling) &&
        Finite(input.TrafficBrakeFloor) &&
        Finite(input.AvailableGripG) &&
        Finite(input.RouteConfidence) &&
        input.TrafficThrottleCeiling>=0.0 &&
        input.TrafficThrottleCeiling<=1.0 &&
        input.TrafficBrakeFloor>=0.0 &&
        input.TrafficBrakeFloor<=1.0 &&
        input.AvailableGripG>=s.MinimumPhysicalGripG &&
        input.AvailableGripG<=4.0 &&
        input.RouteConfidence>=0.0 &&
        input.RouteConfidence<=1.0 &&
        input.Car.Identity.Slot>0 &&
        input.Car.Identity.Slot<PhysicalFleetCapacity &&
        !input.Car.Controls.HumanControlled;
}

inline double AIPathSteeringFeedForward(
    double curvaturePerCm,double wheelbaseCm,
    double maximumAngleRadians) noexcept
{
    if (!Finite(curvaturePerCm) ||
        !Finite(wheelbaseCm) ||
        !Finite(maximumAngleRadians) ||
        wheelbaseCm<=0.0 || maximumAngleRadians<=0.0)
        return 0.0;
    return std::clamp(
        std::atan(curvaturePerCm*wheelbaseCm)/
            maximumAngleRadians,-1.0,1.0);
}

inline AIPathOutput PlanPhysicalRivalPath(
    const AIPathInput& input,
    AIPathSettings s={}) noexcept
{
    AIPathOutput out;
    if (!ValidAIPathSettings(s) ||
        !ValidAIPathInput(input,s))
        return out;
    const Vec2 velocity{
        input.Car.Chassis.VelocityCmS.X,
        input.Car.Chassis.VelocityCmS.Y
    };
    const Vec2 position=
        input.Car.Chassis.PlanarPose.Position;
    const double yaw=
        input.Car.Chassis.PlanarPose.HeadingRadians;
    const double actualSpeed=Length(velocity);
    const double longitudinal=ForwardSpeed(velocity,yaw);
    const auto projection=ProjectToSmoothRacingLine(
        input.Course,position,yaw,true);
    if (!projection.Qualified) return out;
    const double lookahead=std::clamp(
        s.LookaheadBaseCm+
        actualSpeed*s.LookaheadSpeedSeconds,
        s.LookaheadBaseCm,s.LookaheadMaximumCm);
    const auto target=SampleSmoothRacingLine(
        input.Course,
        projection.TrackDistanceCm+lookahead);
    if (!target.Valid) return out;
    const Vec2 direction=target.PositionCm-position;
    const double desiredHeading=AngleOf(direction);
    const double headingError=AngleDifference(
        yaw,desiredHeading);
    const double lateralError=
        projection.SignedLateralCm;
    const double lateralErrorRate=
        Cross(projection.Target.Tangent,velocity);
    const double curvatureFeedforward=
        AIPathSteeringFeedForward(
            target.CurvaturePerCm,
            input.Car.Chassis.WheelbaseCm,
            s.MaximumSteeringRadians);
    const double feedback=
        s.HeadingErrorP*headingError-
        s.LateralErrorP*lateralError-
        s.LateralErrorD*lateralErrorRate-
        s.YawRateDamping*
        input.Car.Chassis.AngularVelocityRadS.Z;
    const double commandedSteer=std::clamp(
        curvatureFeedforward*
            s.FeedForwardSteeringGain+feedback,
        -s.MaximumSteeringInput,s.MaximumSteeringInput);
    double targetSpeed=input.SpeedPlan.Nodes[0].
        FinalTargetSpeedCmS;
    for (int i=0;i<input.SpeedPlan.Count;++i)
    {
        const auto& sample=input.SpeedPlan.Nodes[
            static_cast<std::size_t>(i)];
        if (sample.HorizonDistanceCm>lookahead)
            break;
        targetSpeed=std::min(
            targetSpeed,sample.FinalTargetSpeedCmS);
    }
    targetSpeed=std::clamp(targetSpeed,0.0,
        s.MaximumForwardSpeedCmS);
    const bool wrongDirection=
        projection.MotionAligned==false ||
        longitudinal<-s.MinimumForwardSpeedCmS ||
        std::abs(headingError)>s.MaximumHeadingErrorRadians;
    const bool tooFar=
        std::abs(lateralError)>
        s.ExcessCrossTrackEmergencyCm;
    const bool offRoad=
        !projection.OnPavement ||
        !input.Car.Track.OnOriginalPavement;
    const double speedError=
        targetSpeed-actualSpeed;
    const double weatherGrip=
        Clamp01(input.AvailableGripG);
    const double routeConfidence=std::min(
        input.RouteConfidence,weatherGrip);
    const double throttle=std::clamp(
        speedError*s.ThrottleSpeedErrorGain,
        s.MinimumThrottle,s.MaximumThrottle)*
        routeConfidence;
    const double brake=std::clamp(
        -speedError*s.BrakeSpeedErrorGain,
        0.0,s.MaximumBrake);
    out.TargetSpeedCmS=targetSpeed;
    out.LookaheadCm=lookahead;
    out.TargetHeadingRadians=desiredHeading;
    out.HeadingErrorRadians=headingError;
    out.LateralErrorCm=lateralError;
    out.LateralErrorRateCmS=lateralErrorRate;
    out.TargetCurvaturePerCm=
        target.CurvaturePerCm;
    out.PredictedYawRateRadiansSecond=
        SafeDivision(actualSpeed*std::tan(
            commandedSteer*s.MaximumSteeringRadians),
            input.Car.Chassis.WheelbaseCm);
    out.MeasuredYawRateRadiansSecond=
        input.Car.Chassis.AngularVelocityRadS.Z;
    out.SpeedErrorCmS=speedError;
    out.RoadMarginCm=
        projection.SourceRoadClearanceCm;
    out.TargetDistanceCm=target.DistanceCm;
    out.SteeringFeedForward=curvatureFeedforward;
    out.SteeringFeedback=feedback;
    out.BrakingDistanceCm=StoppingEnvelope(
        actualSpeed,std::min(actualSpeed,targetSpeed),
        800.0,0.18).DistanceCm;
    out.WrongDirection=wrongDirection;
    out.CrossTrackEmergency=tooFar;
    out.OffOriginalPavement=offRoad;
    out.AppliedThrottleCeiling=
        input.TrafficThrottleCeiling;
    out.AppliedBrakeFloor=input.TrafficBrakeFloor;
    out.ProposedThrottle=std::min(
        throttle,input.TrafficThrottleCeiling);
    out.ProposedBrake=std::max(
        brake,input.TrafficBrakeFloor);
    out.ProposedSteering=commandedSteer;
    out.NeedsBraking=out.ProposedBrake>0.1;
    const bool stop=tooFar ||
        (wrongDirection &&
         s.EmergencyStopForOppositeDirection) ||
        (offRoad && s.RestrictThrottleOnOffPavement);
    out.HoldingStop=stop;
    if (stop)
    {
        out.ProposedThrottle=0.0;
        out.ProposedBrake=1.0;
    }
    if (out.ProposedBrake>0.05)
        out.ProposedThrottle=0.0;
    out.HasUnrealControlAuthority=false;
    out.MotorNeverRaised=true;
    out.Valid=Finite(out.ProposedThrottle) &&
        Finite(out.ProposedBrake) &&
        Finite(out.ProposedSteering);
    return out;
}

class PhysicalRivalPathController final
{
public:
    bool Configure(AIPathSettings s) noexcept
    {
        if (!ValidAIPathSettings(s))return false;
        Settings=s;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        PreviousSteering=0.0;
        PreviousThrottle=0.0;
        PreviousBrake=0.0;
        Session=0;
        Actor=0;
        HasActor=false;
        Qualified=0;
        Rejected=0;
        RecoveryFrames=0;
        EmergencyFrames=0;
        PeakLateralErrorCm=0.0;
        TotalErrorCmSeconds=0.0;
    }
    AIPathOutput Step(const AIPathInput& input) noexcept
    {
        AIPathOutput proposed=
            PlanPhysicalRivalPath(input,Settings);
        if (!proposed.Valid)
        {
            ++Rejected;
            PreviousThrottle=0.0;
            PreviousBrake=1.0;
            return {};
        }
        if (HasActor &&
            (input.Car.Identity.Session!=Session ||
             input.Car.Identity.Actor!=Actor))
        {
            PreviousThrottle=0.0;
            PreviousBrake=1.0;
            PreviousSteering=0.0;
            ++RecoveryFrames;
        }
        Session=input.Car.Identity.Session;
        Actor=input.Car.Identity.Actor;
        HasActor=true;
        const double dt=input.DeltaSeconds;
        proposed.ProposedThrottle=RateLimited(
            PreviousThrottle,proposed.ProposedThrottle,
            Settings.ThrottleRecoveryPerSecond,
            1000.0,dt);
        proposed.ProposedBrake=RateLimited(
            PreviousBrake,proposed.ProposedBrake,
            1000.0,Settings.BrakeReleasePerSecond,dt);
        proposed.ProposedSteering=RateLimited(
            PreviousSteering,proposed.ProposedSteering,
            Settings.SteeringRisePerSecond,
            Settings.SteeringFallPerSecond,dt);
        if (proposed.HoldingStop)
        {
            proposed.ProposedThrottle=0.0;
            proposed.ProposedBrake=1.0;
            ++EmergencyFrames;
        }
        if (proposed.ProposedBrake>0.05)
            proposed.ProposedThrottle=0.0;
        PreviousSteering=proposed.ProposedSteering;
        PreviousThrottle=proposed.ProposedThrottle;
        PreviousBrake=proposed.ProposedBrake;
        PeakLateralErrorCm=std::max(
            PeakLateralErrorCm,std::abs(proposed.LateralErrorCm));
        TotalErrorCmSeconds+=
            std::abs(proposed.LateralErrorCm)*dt;
        ++Qualified;
        Last=proposed;
        return Last;
    }
    AIPathOutput Snapshot() const noexcept {return Last;}
    std::uint64_t QualifiedFrames() const noexcept
    {return Qualified;}
    std::uint64_t RejectedFrames() const noexcept
    {return Rejected;}
    std::uint64_t EmergencyStopFrames() const noexcept
    {return EmergencyFrames;}
    double IntegratedLateralDeviationCmSeconds() const noexcept
    {return TotalErrorCmSeconds;}
private:
    AIPathSettings Settings{};
    AIPathOutput Last{};
    std::uint64_t Session=0;
    std::uint64_t Actor=0;
    std::uint64_t Qualified=0;
    std::uint64_t Rejected=0;
    std::uint64_t RecoveryFrames=0;
    std::uint64_t EmergencyFrames=0;
    double PreviousSteering=0.0;
    double PreviousThrottle=0.0;
    double PreviousBrake=0.0;
    double PeakLateralErrorCm=0.0;
    double TotalErrorCmSeconds=0.0;
    bool HasActor=false;
};
} // namespace latebrake::racecraft
