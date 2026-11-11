#pragma once
#include "Racecraft/RacecraftFrames.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
// A road surface sampling interface for measured Unreal collision geometry.
// This never creates terrain. Every point must originate from a current
// original-map trace and retain the same engine session provenance.
inline constexpr int RoadVerticalProfileCapacity=24;
struct RoadVerticalProfileParameters final
{
    double MaximumForwardRangeCm=5000.0;
    double MaximumSampleSeparationCm=400.0;
    double MinimumSampleSeparationCm=5.0;
    double MaximumHeightCm=5000.0;
    double MaximumGrade=0.70;
    double MaximumCrossSlope=0.55;
    double MaximumVerticalSpeedCmS=800.0;
    double MaximumRoughnessCm=55.0;
    double MaximumBumpCm=35.0;
    double MaximumDropCm=30.0;
    double MaximumRideSpeedCmS=16000.0;
    double MaximumSensorClockSkewSeconds=0.05;
    double MaximumFrameSeconds=0.10;
    double WheelbaseCm=270.0;
    double FrontTrackCm=160.0;
    double RearTrackCm=158.0;
    double MinimumClearanceCm=7.0;
    double MaximumClearanceCm=35.0;
    double SpringTravelCm=25.0;
    double MotionAmplification=1.0;
    double FrontHeightWeight=0.53;
    double MaximumCurvature=0.025;
    double EarlyBumpWindowSeconds=0.35;
    double MinimumSensorConfidence=0.70;
    bool RequireOriginalMap=true;
    bool RequireAllWheelTraces=true;
    bool RequireCollisionBearingGeometry=true;
};
struct RoadVerticalSample final
{
    double ForwardDistanceCm=0.0;
    double CenterHeightCm=0.0;
    double LeftHeightCm=0.0;
    double RightHeightCm=0.0;
    double RoadGradeRadians=0.0;
    double RoadBankRadians=0.0;
    double LeftRoughnessCm=0.0;
    double RightRoughnessCm=0.0;
    double TraceClockSeconds=0.0;
    double Confidence=1.0;
    std::uint64_t SourceComponent=0;
    ContactSurface Surface=ContactSurface::Unknown;
    bool OriginalCollisionHit=false;
    bool RoadContactVerified=false;
    bool LeftWheelTraceVerified=false;
    bool RightWheelTraceVerified=false;
};
struct RoadVerticalObservation final
{
    RaceIdentity Identity{};
    std::array<RoadVerticalSample,RoadVerticalProfileCapacity> Samples{};
    int Count=0;
    double PhysicsClockSeconds=0.0;
    double ChassisForwardSpeedCmS=0.0;
    double ChassisVerticalSpeedCmS=0.0;
    double CurrentRideHeightCm=12.0;
    double CurrentPitchRadians=0.0;
    double CurrentRollRadians=0.0;
    double DeltaSeconds=0.0;
    bool ActualChaosBodyReady=false;
    bool OriginalCircuitKnown=false;
    bool PhysicsTracesFresh=false;
};
struct VerticalProfileKnot final
{
    double ForwardDistanceCm=0.0;
    double CenterHeightCm=0.0;
    double LeftHeightCm=0.0;
    double RightHeightCm=0.0;
    double LongitudinalSlope=0.0;
    double LateralSlope=0.0;
    double ElevationChangeCm=0.0;
    double BumpHeightCm=0.0;
    double DropHeightCm=0.0;
    double RoughnessCm=0.0;
    double SurfaceNormalTiltRadians=0.0;
    double EstimatedArrivalSeconds=0.0;
    double WheelVerticalSpeedCmS=0.0;
    double SpringCompressionNeededCm=0.0;
    double ClearanceReserveCm=0.0;
    double BumpSeverity=0.0;
    double VerticalImpactIndex=0.0;
    double PitchRateDemandRadS=0.0;
    double RollRateDemandRadS=0.0;
    bool BumpStopExpected=false;
    bool BottomOutExpected=false;
    bool SurfaceQualified=false;
};
struct RoadVerticalProfile final
{
    RaceIdentity Identity{};
    std::array<VerticalProfileKnot,RoadVerticalProfileCapacity> Knots{};
    int Count=0;
    int FirstBumpStopKnot=-1;
    int FirstBottomOutKnot=-1;
    double PeakRiseCm=0.0;
    double PeakDropCm=0.0;
    double PeakRoughnessCm=0.0;
    double PeakWheelVerticalSpeedCmS=0.0;
    double PeakImpactIndex=0.0;
    double MeanGradeRadians=0.0;
    double MeanBankRadians=0.0;
    double MaximumEstimatedPitchRateRadS=0.0;
    double MaximumEstimatedRollRateRadS=0.0;
    double NearestBumpCm=std::numeric_limits<double>::infinity();
    double NearestDropCm=std::numeric_limits<double>::infinity();
    double NearestBottomOutCm=std::numeric_limits<double>::infinity();
    double RecommendedSpeedCeilingCmS=0.0;
    double MinimumConfidence=0.0;
    double TotalVerifiedDistanceCm=0.0;
    int VerifiedTraces=0;
    bool WheelsMayLoseContact=false;
    bool OriginalRoadOnly=true;
    bool ActualChaosActuation=false;
    bool Valid=false;
};
inline bool ValidRoadVerticalParameters(
    RoadVerticalProfileParameters p) noexcept
{
    return Finite(p.MaximumForwardRangeCm) &&
        p.MaximumForwardRangeCm>100.0 &&
        Finite(p.MaximumSampleSeparationCm) &&
        p.MaximumSampleSeparationCm>0.0 &&
        Finite(p.MinimumSampleSeparationCm) &&
        p.MinimumSampleSeparationCm>0.0 &&
        p.MinimumSampleSeparationCm<p.MaximumSampleSeparationCm &&
        Finite(p.MaximumHeightCm) && p.MaximumHeightCm>0.0 &&
        Finite(p.MaximumGrade) && p.MaximumGrade>0.0 &&
        Finite(p.MaximumCrossSlope) && p.MaximumCrossSlope>0.0 &&
        Finite(p.MaximumVerticalSpeedCmS) &&
        p.MaximumVerticalSpeedCmS>0.0 &&
        Finite(p.MaximumRoughnessCm) && p.MaximumRoughnessCm>0.0 &&
        Finite(p.MaximumBumpCm) && p.MaximumBumpCm>0.0 &&
        Finite(p.MaximumDropCm) && p.MaximumDropCm>0.0 &&
        Finite(p.MaximumRideSpeedCmS) &&
        p.MaximumRideSpeedCmS>0.0 &&
        Finite(p.MaximumSensorClockSkewSeconds) &&
        p.MaximumSensorClockSkewSeconds>=0.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.WheelbaseCm) && p.WheelbaseCm>100.0 &&
        Finite(p.FrontTrackCm) && p.FrontTrackCm>80.0 &&
        Finite(p.RearTrackCm) && p.RearTrackCm>80.0 &&
        Finite(p.MinimumClearanceCm) && p.MinimumClearanceCm>0.0 &&
        Finite(p.MaximumClearanceCm) &&
        p.MaximumClearanceCm>p.MinimumClearanceCm &&
        Finite(p.SpringTravelCm) && p.SpringTravelCm>0.0 &&
        Finite(p.MotionAmplification) && p.MotionAmplification>0.0 &&
        Finite(p.FrontHeightWeight) &&
        p.FrontHeightWeight>0.0 && p.FrontHeightWeight<1.0 &&
        Finite(p.MaximumCurvature) && p.MaximumCurvature>0.0 &&
        Finite(p.EarlyBumpWindowSeconds) &&
        p.EarlyBumpWindowSeconds>0.0 &&
        Finite(p.MinimumSensorConfidence) &&
        p.MinimumSensorConfidence>0.0 &&
        p.MinimumSensorConfidence<=1.0;
}
inline bool ValidRoadVerticalSample(
    RoadVerticalSample s, RoadVerticalProfileParameters p) noexcept
{
    return (!p.RequireOriginalMap || s.OriginalCollisionHit) &&
        (!p.RequireCollisionBearingGeometry ||
         s.RoadContactVerified) &&
        (!p.RequireAllWheelTraces ||
         (s.LeftWheelTraceVerified && s.RightWheelTraceVerified)) &&
        s.SourceComponent!=0 &&
        Finite(s.ForwardDistanceCm) &&
        s.ForwardDistanceCm>=0.0 &&
        s.ForwardDistanceCm<=p.MaximumForwardRangeCm &&
        Finite(s.CenterHeightCm) &&
        std::abs(s.CenterHeightCm)<=p.MaximumHeightCm &&
        Finite(s.LeftHeightCm) &&
        std::abs(s.LeftHeightCm)<=p.MaximumHeightCm &&
        Finite(s.RightHeightCm) &&
        std::abs(s.RightHeightCm)<=p.MaximumHeightCm &&
        Finite(s.RoadGradeRadians) &&
        std::abs(s.RoadGradeRadians)<=p.MaximumGrade &&
        Finite(s.RoadBankRadians) &&
        std::abs(s.RoadBankRadians)<=p.MaximumCrossSlope &&
        Finite(s.LeftRoughnessCm) &&
        s.LeftRoughnessCm>=0.0 &&
        s.LeftRoughnessCm<=p.MaximumRoughnessCm &&
        Finite(s.RightRoughnessCm) &&
        s.RightRoughnessCm>=0.0 &&
        s.RightRoughnessCm<=p.MaximumRoughnessCm &&
        Finite(s.TraceClockSeconds) &&
        s.TraceClockSeconds>=0.0 &&
        Finite(s.Confidence) &&
        s.Confidence>=p.MinimumSensorConfidence &&
        s.Confidence<=1.0 &&
        (s.Surface==ContactSurface::Asphalt ||
         s.Surface==ContactSurface::WetAsphalt ||
         s.Surface==ContactSurface::PaintedKerb);
}
inline bool ValidRoadVerticalObservation(
    const RoadVerticalObservation& s,
    RoadVerticalProfileParameters p) noexcept
{
    if (!ValidRoadVerticalParameters(p) ||
        !PhysicalActorIdentityValid(s.Identity) ||
        s.Identity.Frame==0 ||
        !s.ActualChaosBodyReady ||
        !s.OriginalCircuitKnown ||
        !s.PhysicsTracesFresh ||
        s.Count<2 || s.Count>RoadVerticalProfileCapacity ||
        !Finite(s.PhysicsClockSeconds) ||
        s.PhysicsClockSeconds<0.0 ||
        !Finite(s.ChassisForwardSpeedCmS) ||
        s.ChassisForwardSpeedCmS<0.0 ||
        s.ChassisForwardSpeedCmS>p.MaximumRideSpeedCmS ||
        !Finite(s.ChassisVerticalSpeedCmS) ||
        std::abs(s.ChassisVerticalSpeedCmS)>
            p.MaximumVerticalSpeedCmS ||
        !Finite(s.CurrentRideHeightCm) ||
        s.CurrentRideHeightCm<p.MinimumClearanceCm ||
        s.CurrentRideHeightCm>p.MaximumClearanceCm ||
        !Finite(s.CurrentPitchRadians) ||
        std::abs(s.CurrentPitchRadians)>p.MaximumGrade ||
        !Finite(s.CurrentRollRadians) ||
        std::abs(s.CurrentRollRadians)>p.MaximumCrossSlope ||
        !Finite(s.DeltaSeconds) ||
        s.DeltaSeconds<=0.0 ||
        s.DeltaSeconds>p.MaximumFrameSeconds)
        return false;
    double previousDistance=-p.MinimumSampleSeparationCm;
    for(int i=0;i<s.Count;++i)
    {
        const RoadVerticalSample& sample=
            s.Samples[static_cast<std::size_t>(i)];
        if (!ValidRoadVerticalSample(sample,p) ||
            std::abs(sample.TraceClockSeconds-
                     s.PhysicsClockSeconds)>
                p.MaximumSensorClockSkewSeconds ||
            sample.ForwardDistanceCm-previousDistance<
                p.MinimumSampleSeparationCm ||
            (i>0 &&
             sample.ForwardDistanceCm-previousDistance>
                p.MaximumSampleSeparationCm))
            return false;
        previousDistance=sample.ForwardDistanceCm;
    }
    return s.Samples[0].ForwardDistanceCm<
        p.MaximumSampleSeparationCm;
}
inline VerticalProfileKnot EvaluateVerticalKnot(
    const RoadVerticalSample& previous,
    const RoadVerticalSample& current,
    double forwardSpeed,
    double chassisHeight,
    RoadVerticalProfileParameters p) noexcept
{
    VerticalProfileKnot result;
    if (!ValidRoadVerticalParameters(p) ||
        !ValidRoadVerticalSample(previous,p) ||
        !ValidRoadVerticalSample(current,p) ||
        current.ForwardDistanceCm<=previous.ForwardDistanceCm)
        return result;
    const double distance=
        current.ForwardDistanceCm-previous.ForwardDistanceCm;
    const double rise=
        current.CenterHeightCm-previous.CenterHeightCm;
    const double leftRise=
        current.LeftHeightCm-previous.LeftHeightCm;
    const double rightRise=
        current.RightHeightCm-previous.RightHeightCm;
    const double bankDifference=rightRise-leftRise;
    const double travelTime=
        SafeDivision(distance,std::max(100.0,forwardSpeed));
    result.ForwardDistanceCm=current.ForwardDistanceCm;
    result.CenterHeightCm=current.CenterHeightCm;
    result.LeftHeightCm=current.LeftHeightCm;
    result.RightHeightCm=current.RightHeightCm;
    result.LongitudinalSlope=SafeDivision(rise,distance);
    result.LateralSlope=SafeDivision(
        current.RightHeightCm-current.LeftHeightCm,
        p.FrontTrackCm);
    result.ElevationChangeCm=rise;
    result.BumpHeightCm=std::max(0.0,rise);
    result.DropHeightCm=std::max(0.0,-rise);
    result.RoughnessCm=std::max(
        current.LeftRoughnessCm,current.RightRoughnessCm);
    result.SurfaceNormalTiltRadians=std::hypot(
        std::atan(result.LongitudinalSlope),
        std::atan(result.LateralSlope));
    result.EstimatedArrivalSeconds=
        SafeDivision(current.ForwardDistanceCm,
                     std::max(100.0,forwardSpeed));
    result.WheelVerticalSpeedCmS=std::max(
        std::abs(leftRise),std::abs(rightRise))/
        std::max(PhysicsEpsilon,travelTime);
    result.SpringCompressionNeededCm=
        std::max(0.0,std::max(leftRise,rightRise))*
        p.MotionAmplification;
    result.ClearanceReserveCm=chassisHeight-
        result.SpringCompressionNeededCm;
    result.BumpSeverity=Clamp01(
        SafeDivision(result.SpringCompressionNeededCm,
                     p.SpringTravelCm));
    result.VerticalImpactIndex=Clamp01(
        SafeDivision(result.WheelVerticalSpeedCmS,
                     p.MaximumVerticalSpeedCmS)*
        (0.5+result.BumpSeverity));
    result.PitchRateDemandRadS=SafeDivision(
        std::atan(result.LongitudinalSlope)-
            previous.RoadGradeRadians,
        std::max(PhysicsEpsilon,travelTime));
    result.RollRateDemandRadS=SafeDivision(
        std::atan(result.LateralSlope)-
            previous.RoadBankRadians,
        std::max(PhysicsEpsilon,travelTime));
    result.BumpStopExpected=
        result.SpringCompressionNeededCm>=
            p.SpringTravelCm*0.8;
    result.BottomOutExpected=
        result.ClearanceReserveCm<
            p.MinimumClearanceCm;
    result.SurfaceQualified=
        Finite(result.WheelVerticalSpeedCmS) &&
        Finite(result.PitchRateDemandRadS) &&
        Finite(result.RollRateDemandRadS) &&
        Finite(result.ClearanceReserveCm);
    return result;
}
inline RoadVerticalProfile ForecastRoadVerticalProfile(
    const RoadVerticalObservation& observation,
    RoadVerticalProfileParameters p={}) noexcept
{
    RoadVerticalProfile out;
    if (!ValidRoadVerticalObservation(observation,p))
        return out;
    out.Identity=observation.Identity;
    out.Count=observation.Count;
    out.MinimumConfidence=1.0;
    out.RecommendedSpeedCeilingCmS=
        p.MaximumRideSpeedCmS;
    out.Knots[0].ForwardDistanceCm=
        observation.Samples[0].ForwardDistanceCm;
    out.Knots[0].CenterHeightCm=
        observation.Samples[0].CenterHeightCm;
    out.Knots[0].SurfaceQualified=true;
    out.VerifiedTraces=1;
    const double currentSpeed=
        observation.ChassisForwardSpeedCmS;
    for(int i=1;i<observation.Count;++i)
    {
        const auto& earlier=observation.Samples[
            static_cast<std::size_t>(i-1)];
        const auto& now=observation.Samples[
            static_cast<std::size_t>(i)];
        const VerticalProfileKnot knot=EvaluateVerticalKnot(
            earlier,now,currentSpeed,
            observation.CurrentRideHeightCm,p);
        if (!knot.SurfaceQualified)
            return {};
        out.Knots[static_cast<std::size_t>(i)]=knot;
        ++out.VerifiedTraces;
        out.TotalVerifiedDistanceCm+=
            now.ForwardDistanceCm-earlier.ForwardDistanceCm;
        out.MinimumConfidence=std::min(
            out.MinimumConfidence,now.Confidence);
        out.PeakRiseCm=std::max(
            out.PeakRiseCm,knot.BumpHeightCm);
        out.PeakDropCm=std::max(
            out.PeakDropCm,knot.DropHeightCm);
        out.PeakRoughnessCm=std::max(
            out.PeakRoughnessCm,knot.RoughnessCm);
        out.PeakWheelVerticalSpeedCmS=std::max(
            out.PeakWheelVerticalSpeedCmS,
            knot.WheelVerticalSpeedCmS);
        out.PeakImpactIndex=std::max(
            out.PeakImpactIndex,knot.VerticalImpactIndex);
        out.MaximumEstimatedPitchRateRadS=std::max(
            out.MaximumEstimatedPitchRateRadS,
            std::abs(knot.PitchRateDemandRadS));
        out.MaximumEstimatedRollRateRadS=std::max(
            out.MaximumEstimatedRollRateRadS,
            std::abs(knot.RollRateDemandRadS));
        out.MeanGradeRadians+=now.RoadGradeRadians;
        out.MeanBankRadians+=now.RoadBankRadians;
        if (knot.BumpHeightCm>PhysicsEpsilon)
            out.NearestBumpCm=std::min(
                out.NearestBumpCm,knot.ForwardDistanceCm);
        if (knot.DropHeightCm>PhysicsEpsilon)
            out.NearestDropCm=std::min(
                out.NearestDropCm,knot.ForwardDistanceCm);
        if (knot.BumpStopExpected &&
            out.FirstBumpStopKnot<0)
            out.FirstBumpStopKnot=i;
        if (knot.BottomOutExpected)
        {
            if (out.FirstBottomOutKnot<0)
                out.FirstBottomOutKnot=i;
            out.NearestBottomOutCm=std::min(
                out.NearestBottomOutCm,
                knot.ForwardDistanceCm);
        }
        if (knot.WheelVerticalSpeedCmS>
            p.MaximumVerticalSpeedCmS*0.5)
            out.WheelsMayLoseContact=true;
        const double allowedSpeed=std::min(
            p.MaximumRideSpeedCmS,
            SafeDivision(
                p.MaximumVerticalSpeedCmS*
                    (now.ForwardDistanceCm-
                     earlier.ForwardDistanceCm),
                std::max(
                    std::abs(now.LeftHeightCm-earlier.LeftHeightCm),
                    std::abs(now.RightHeightCm-earlier.RightHeightCm)),
                p.MaximumRideSpeedCmS));
        if (knot.BumpStopExpected ||
            knot.BottomOutExpected)
            out.RecommendedSpeedCeilingCmS=std::min(
                out.RecommendedSpeedCeilingCmS,
                std::max(0.0,allowedSpeed));
    }
    out.MeanGradeRadians/=observation.Count-1;
    out.MeanBankRadians/=observation.Count-1;
    out.ActualChaosActuation=false;
    out.Valid=out.VerifiedTraces==out.Count &&
        Finite(out.TotalVerifiedDistanceCm) &&
        Finite(out.RecommendedSpeedCeilingCmS) &&
        Finite(out.MinimumConfidence) &&
        Finite(out.PeakImpactIndex);
    return out;
}
class RoadVerticalProfileSupervisor final
{
public:
    bool Configure(RoadVerticalProfileParameters p) noexcept
    {
        if (!ValidRoadVerticalParameters(p))
            return false;
        Settings=p;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Session=0;
        Revision=0;
        Actor=0;
        Frame=0;
        Accepted=0;
        Rejected=0;
        BumpStopFrames=0;
        BottomOutFrames=0;
        PeakBump=0.0;
        PeakImpact=0.0;
    }
    RoadVerticalProfile Step(
        const RoadVerticalObservation& sample) noexcept
    {
        const RoadVerticalProfile target=
            ForecastRoadVerticalProfile(sample,Settings);
        const bool sameEpoch=Session!=0 &&
            Session==sample.Identity.Session &&
            Revision==sample.Identity.SessionRevision &&
            Actor==sample.Identity.Actor;
        if (!target.Valid ||
            (sameEpoch && sample.Identity.Frame<=Frame))
        {
            ++Rejected;
            Last={};
            return {};
        }
        Session=sample.Identity.Session;
        Revision=sample.Identity.SessionRevision;
        Actor=sample.Identity.Actor;
        Frame=sample.Identity.Frame;
        ++Accepted;
        if (target.FirstBumpStopKnot>=0)
            ++BumpStopFrames;
        if (target.FirstBottomOutKnot>=0)
            ++BottomOutFrames;
        PeakBump=std::max(PeakBump,target.PeakRiseCm);
        PeakImpact=std::max(
            PeakImpact,target.PeakImpactIndex);
        Last=target;
        return Last;
    }
    RoadVerticalProfile Snapshot() const noexcept
    {return Last;}
    std::uint64_t Frames() const noexcept{return Accepted;}
    std::uint64_t Rejections() const noexcept{return Rejected;}
    std::uint64_t BumpEvents() const noexcept
    {return BumpStopFrames;}
    std::uint64_t BottomOutEvents() const noexcept
    {return BottomOutFrames;}
    double MaximumMeasuredBumpCm() const noexcept
    {return PeakBump;}
    double MaximumImpactIndex() const noexcept
    {return PeakImpact;}
private:
    RoadVerticalProfileParameters Settings{};
    RoadVerticalProfile Last{};
    std::uint64_t Session=0;
    std::uint64_t Actor=0;
    std::uint64_t Frame=0;
    std::uint64_t Accepted=0;
    std::uint64_t Rejected=0;
    std::uint64_t BumpStopFrames=0;
    std::uint64_t BottomOutFrames=0;
    std::uint32_t Revision=0;
    double PeakBump=0.0;
    double PeakImpact=0.0;
};
} // namespace latebrake::racecraft
