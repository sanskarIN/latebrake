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
inline constexpr int PhysicalIncidentContactCapacity=8;
enum class IncidentSourceClass : std::uint8_t
{
    Unknown, WallImpact, RivalContact, RoadKerb,
    LossOfGrip, SuspensionBottoming, DebrisImpact,
    SpinWithoutImpact
};
enum class IncidentReviewFault : std::uint8_t
{
    None, MissingPhysics, MissingContactProvenance,
    DuplicateContact, UnphysicalImpulse, WrongSession,
    StaleFrame, ContactOutsideMap, ImpactEnergyMismatch,
    VehicleNotSimulating, RaceNotRunning
};
struct IncidentReviewParameters final
{
    double MinimumContactImpulseNs=80.0;
    double MaximumContactImpulseNs=80000.0;
    double MaximumNormalImpulseNs=75000.0;
    double MaximumTangentialImpulseNs=25000.0;
    double MinimumContactSeparationCm=0.0;
    double MaximumContactSeparationCm=40.0;
    double MaximumContactPointRadiusCm=1000.0;
    double MaximumVehicleMassKg=8500.0;
    double MinimumVehicleMassKg=300.0;
    double MaximumVehicleSpeedCmS=MaximumPhysicalSpeedCmS;
    double MaximumPostImpactSpeedCmS=MaximumPhysicalSpeedCmS;
    double MaximumAccelerationCmS2=12000.0;
    double MaximumAngularVelocityRadiansSecond=12.0;
    double MaximumChassisDamageFraction=1.0;
    double MaximumWheelDamageFraction=1.0;
    double MinimumRaceClockSeconds=0.0;
    double MaximumRaceClockSeconds=86400.0;
    double MaximumFrameSeconds=0.10;
    double MaximumContactClockSkewSeconds=0.04;
    double MinimumSensorConfidence=0.75;
    double MaximumMomentumResidualNs=30000.0;
    double MaximumEnergyMismatchFraction=0.70;
    double MaximumKineticEnergyJ=90000000.0;
    double MaximumPhysicalDamageEnergyJ=250000.0;
    double ContactSeverityLowNs=1200.0;
    double ContactSeverityMediumNs=6000.0;
    double ContactSeverityHighNs=18000.0;
    double MinimumRoadNormalZ=0.35;
    double TrackEdgeIncidentCm=350.0;
    double MinimumWheelContactCount=2.0;
    double MaximumSteeringInput=1.0;
    double BrakePedalForEmergency=1.0;
    double ThrottleCeilingAfterImpact=0.10;
    double MinimumObservedSpeedDropCmS=50.0;
    double MaximumDamageWithoutImpactEnergy=0.2;
    double MinimumGripG=0.15;
    double MaximumGripG=2.5;
    double MaximumContactConfidencePenalty=0.35;
    bool RequireOriginalCollisionComponents=true;
    bool RequireActualChaosCollisionEvents=true;
    bool RequireSameEngineFrame=true;
    bool NeverWriteRacePenalty=true;
    bool NeverCreateVirtualContact=true;
};
struct PhysicalIncidentContact final
{
    RaceIdentity Identity{};
    std::uint64_t ContactEventToken=0;
    std::uint64_t OtherActorToken=0;
    std::uint64_t OriginalCollisionComponent=0;
    Vec3 ContactWorldCm{};
    Vec3 ContactNormal{};
    Vec3 ImpulseNSeconds{};
    Vec3 RelativeContactVelocityCmS{};
    double ContactPenetrationCm=0.0;
    double PhysicsClockSeconds=0.0;
    double SensorConfidence=1.0;
    ContactSurface Material=ContactSurface::Unknown;
    IncidentSourceClass Class=IncidentSourceClass::Unknown;
    bool OriginalCollisionBodyObserved=false;
    bool ActualChaosContactReported=false;
    bool OriginalCircuitProven=false;
    bool OpponentPhysicsVerified=false;
};
struct PhysicalIncidentFrame final
{
    RaceIdentity Identity{};
    std::array<PhysicalIncidentContact,PhysicalIncidentContactCapacity> Contacts{};
    int ContactCount=0;
    Vec3 PreImpactVelocityCmS{};
    Vec3 PostImpactVelocityCmS{};
    Vec3 MeasuredAngularVelocityRadS{};
    double MeasuredVehicleMassKg=1300.0;
    double MeasuredChassisDamageFraction=0.0;
    double MeasuredSuspensionDamageFraction=0.0;
    double MeasuredWheelDamageFraction=0.0;
    double MeasuredGripG=1.0;
    double MeasuredWheelContactCount=4.0;
    double MeasuredLateralTrackOffsetCm=0.0;
    double PhysicsClockSeconds=0.0;
    double DeltaSeconds=0.0;
    double DriverThrottle=0.0;
    double DriverBrake=0.0;
    double DriverSteering=0.0;
    bool RaceRunning=false;
    bool OriginalCircuitVerified=false;
    bool ChaosVehicleBodySimulating=false;
    bool ChassisDamageSensorQualified=false;
    bool WheelContactSensorsQualified=false;
};
struct PhysicalIncidentContactReview final
{
    IncidentSourceClass Class=IncidentSourceClass::Unknown;
    std::uint64_t ContactEventToken=0;
    std::uint64_t OtherActorToken=0;
    double NormalImpulseNs=0.0;
    double TangentialImpulseNs=0.0;
    double TotalImpulseNs=0.0;
    double RelativeClosingVelocityCmS=0.0;
    double ContactEnergyJ=0.0;
    double EstimatedAccelerationG=0.0;
    double NormalAlignmentFraction=0.0;
    double MeasuredConfidence=0.0;
    double CollisionAngleRadians=0.0;
    double ContactPointLeverCm=0.0;
    double EstimatedYawImpulseNmSeconds=0.0;
    double RelativeKineticEnergyJ=0.0;
    double EstimatedDamageSeverity=0.0;
    double ContactPenetrationCm=0.0;
    bool RivalVerified=false;
    bool BarrierVerified=false;
    bool SupportsRaceMarshalReview=false;
    bool CanTrustAsPhysicalContact=false;
    bool Valid=false;
};
struct PhysicalIncidentReview final
{
    RaceIdentity Identity{};
    std::array<PhysicalIncidentContactReview,
               PhysicalIncidentContactCapacity> Contacts{};
    IncidentReviewFault Fault=IncidentReviewFault::MissingPhysics;
    double TotalNormalImpulseNs=0.0;
    double TotalTangentialImpulseNs=0.0;
    double TotalImpulseNs=0.0;
    double PeakContactImpulseNs=0.0;
    double TotalContactEnergyJ=0.0;
    double MeasuredKineticEnergyBeforeJ=0.0;
    double MeasuredKineticEnergyAfterJ=0.0;
    double ActualKineticEnergyLossJ=0.0;
    double MomentumResidualNs=0.0;
    double MaximumEstimatedImpactG=0.0;
    double MaximumDamageSeverity=0.0;
    double MeasuredDamageFraction=0.0;
    double MeanContactConfidence=0.0;
    double RecommendedThrottleCeiling=1.0;
    double RecommendedBrakeFloor=0.0;
    double RecommendedSteeringCeiling=1.0;
    int VerifiedPhysicalContacts=0;
    int VerifiedRivalContacts=0;
    int OriginalBarrierContacts=0;
    int SevereContactCount=0;
    bool PhysicalIncidentDetected=false;
    bool PitInspectionRecommended=false;
    bool ImmediateStopRecommended=false;
    bool NeverIssuedChaosImpulse=true;
    bool OfficialPenaltyUntouched=true;
    bool Valid=false;
};
inline bool ValidIncidentReviewParameters(
    IncidentReviewParameters p) noexcept
{
    return Finite(p.MinimumContactImpulseNs) &&
        p.MinimumContactImpulseNs>=0.0 &&
        Finite(p.MaximumContactImpulseNs) &&
        p.MaximumContactImpulseNs>p.MinimumContactImpulseNs &&
        Finite(p.MaximumNormalImpulseNs) &&
        p.MaximumNormalImpulseNs>0.0 &&
        Finite(p.MaximumTangentialImpulseNs) &&
        p.MaximumTangentialImpulseNs>0.0 &&
        Finite(p.MinimumContactSeparationCm) &&
        p.MinimumContactSeparationCm>=0.0 &&
        Finite(p.MaximumContactSeparationCm) &&
        p.MaximumContactSeparationCm>=p.MinimumContactSeparationCm &&
        Finite(p.MaximumContactPointRadiusCm) &&
        p.MaximumContactPointRadiusCm>0.0 &&
        Finite(p.MaximumVehicleMassKg) &&
        p.MaximumVehicleMassKg>0.0 &&
        Finite(p.MinimumVehicleMassKg) &&
        p.MinimumVehicleMassKg>0.0 &&
        p.MinimumVehicleMassKg<p.MaximumVehicleMassKg &&
        Finite(p.MaximumVehicleSpeedCmS) &&
        p.MaximumVehicleSpeedCmS>0.0 &&
        Finite(p.MaximumPostImpactSpeedCmS) &&
        p.MaximumPostImpactSpeedCmS>0.0 &&
        Finite(p.MaximumAccelerationCmS2) &&
        p.MaximumAccelerationCmS2>0.0 &&
        Finite(p.MaximumAngularVelocityRadiansSecond) &&
        p.MaximumAngularVelocityRadiansSecond>0.0 &&
        Finite(p.MaximumChassisDamageFraction) &&
        p.MaximumChassisDamageFraction>0.0 &&
        Finite(p.MaximumWheelDamageFraction) &&
        p.MaximumWheelDamageFraction>0.0 &&
        Finite(p.MaximumRaceClockSeconds) &&
        p.MaximumRaceClockSeconds>0.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MaximumContactClockSkewSeconds) &&
        p.MaximumContactClockSkewSeconds>=0.0 &&
        Finite(p.MinimumSensorConfidence) &&
        p.MinimumSensorConfidence>0.0 &&
        p.MinimumSensorConfidence<=1.0 &&
        Finite(p.MaximumMomentumResidualNs) &&
        p.MaximumMomentumResidualNs>0.0 &&
        Finite(p.MaximumEnergyMismatchFraction) &&
        p.MaximumEnergyMismatchFraction>0.0 &&
        Finite(p.MaximumKineticEnergyJ) &&
        p.MaximumKineticEnergyJ>0.0 &&
        Finite(p.MaximumPhysicalDamageEnergyJ) &&
        p.MaximumPhysicalDamageEnergyJ>0.0 &&
        Finite(p.ContactSeverityLowNs) &&
        p.ContactSeverityLowNs>0.0 &&
        Finite(p.ContactSeverityMediumNs) &&
        p.ContactSeverityMediumNs>p.ContactSeverityLowNs &&
        Finite(p.ContactSeverityHighNs) &&
        p.ContactSeverityHighNs>p.ContactSeverityMediumNs &&
        Finite(p.MinimumRoadNormalZ) &&
        p.MinimumRoadNormalZ>0.0 &&
        Finite(p.TrackEdgeIncidentCm) &&
        p.TrackEdgeIncidentCm>=0.0 &&
        Finite(p.MinimumWheelContactCount) &&
        p.MinimumWheelContactCount>=0.0 &&
        Finite(p.MaximumSteeringInput) &&
        p.MaximumSteeringInput>0.0 &&
        Finite(p.BrakePedalForEmergency) &&
        p.BrakePedalForEmergency>0.0 &&
        p.BrakePedalForEmergency<=1.0 &&
        Finite(p.ThrottleCeilingAfterImpact) &&
        p.ThrottleCeilingAfterImpact>=0.0 &&
        p.ThrottleCeilingAfterImpact<=1.0 &&
        Finite(p.MinimumObservedSpeedDropCmS) &&
        p.MinimumObservedSpeedDropCmS>=0.0 &&
        Finite(p.MaximumDamageWithoutImpactEnergy) &&
        p.MaximumDamageWithoutImpactEnergy>=0.0 &&
        Finite(p.MinimumGripG) &&
        p.MinimumGripG>0.0 &&
        Finite(p.MaximumGripG) &&
        p.MaximumGripG>p.MinimumGripG &&
        Finite(p.MaximumContactConfidencePenalty) &&
        p.MaximumContactConfidencePenalty>=0.0 &&
        p.MaximumContactConfidencePenalty<=1.0;
}
inline bool ValidIncidentContact(
    PhysicalIncidentContact c,
    IncidentReviewParameters p) noexcept
{
    return PhysicalActorIdentityValid(c.Identity) &&
        c.Identity.Frame>0 &&
        c.ContactEventToken!=0 &&
        c.OriginalCollisionComponent!=0 &&
        (!p.RequireOriginalCollisionComponents ||
         (c.OriginalCollisionBodyObserved &&
          c.OriginalCircuitProven)) &&
        (!p.RequireActualChaosCollisionEvents ||
         c.ActualChaosContactReported) &&
        c.Class!=IncidentSourceClass::Unknown &&
        Finite(c.ContactWorldCm) &&
        Finite(c.ContactNormal) &&
        Length(c.ContactNormal)>0.9 &&
        Length(c.ContactNormal)<1.1 &&
        Finite(c.ImpulseNSeconds) &&
        Length(c.ImpulseNSeconds)<=p.MaximumContactImpulseNs &&
        Finite(c.RelativeContactVelocityCmS) &&
        Length(c.RelativeContactVelocityCmS)<=
            p.MaximumVehicleSpeedCmS*2.0 &&
        Finite(c.ContactPenetrationCm) &&
        c.ContactPenetrationCm>=p.MinimumContactSeparationCm &&
        c.ContactPenetrationCm<=p.MaximumContactSeparationCm &&
        Finite(c.PhysicsClockSeconds) &&
        c.PhysicsClockSeconds>=0.0 &&
        Finite(c.SensorConfidence) &&
        c.SensorConfidence>=p.MinimumSensorConfidence &&
        c.SensorConfidence<=1.0;
}
inline bool ValidIncidentFrame(
    const PhysicalIncidentFrame& f,
    IncidentReviewParameters p) noexcept
{
    if (!ValidIncidentReviewParameters(p) ||
        !PhysicalActorIdentityValid(f.Identity) ||
        f.Identity.Frame==0 ||
        !f.RaceRunning ||
        !f.OriginalCircuitVerified ||
        !f.ChaosVehicleBodySimulating ||
        !f.ChassisDamageSensorQualified ||
        !f.WheelContactSensorsQualified ||
        f.ContactCount<0 ||
        f.ContactCount>PhysicalIncidentContactCapacity ||
        !Finite(f.PreImpactVelocityCmS) ||
        Length(f.PreImpactVelocityCmS)>
            p.MaximumVehicleSpeedCmS ||
        !Finite(f.PostImpactVelocityCmS) ||
        Length(f.PostImpactVelocityCmS)>
            p.MaximumPostImpactSpeedCmS ||
        !Finite(f.MeasuredAngularVelocityRadS) ||
        Length(f.MeasuredAngularVelocityRadS)>
            p.MaximumAngularVelocityRadiansSecond ||
        !Finite(f.MeasuredVehicleMassKg) ||
        f.MeasuredVehicleMassKg<p.MinimumVehicleMassKg ||
        f.MeasuredVehicleMassKg>p.MaximumVehicleMassKg ||
        !Finite(f.MeasuredChassisDamageFraction) ||
        f.MeasuredChassisDamageFraction<0.0 ||
        f.MeasuredChassisDamageFraction>
            p.MaximumChassisDamageFraction ||
        !Finite(f.MeasuredSuspensionDamageFraction) ||
        f.MeasuredSuspensionDamageFraction<0.0 ||
        f.MeasuredSuspensionDamageFraction>1.0 ||
        !Finite(f.MeasuredWheelDamageFraction) ||
        f.MeasuredWheelDamageFraction<0.0 ||
        f.MeasuredWheelDamageFraction>
            p.MaximumWheelDamageFraction ||
        !Finite(f.MeasuredGripG) ||
        f.MeasuredGripG<p.MinimumGripG ||
        f.MeasuredGripG>p.MaximumGripG ||
        !Finite(f.MeasuredWheelContactCount) ||
        f.MeasuredWheelContactCount<
            p.MinimumWheelContactCount ||
        f.MeasuredWheelContactCount>4.0 ||
        !Finite(f.MeasuredLateralTrackOffsetCm) ||
        !Finite(f.PhysicsClockSeconds) ||
        f.PhysicsClockSeconds<p.MinimumRaceClockSeconds ||
        f.PhysicsClockSeconds>p.MaximumRaceClockSeconds ||
        !Finite(f.DeltaSeconds) ||
        f.DeltaSeconds<=0.0 ||
        f.DeltaSeconds>p.MaximumFrameSeconds ||
        !Finite(f.DriverThrottle) ||
        f.DriverThrottle<0.0 ||
        f.DriverThrottle>1.0 ||
        !Finite(f.DriverBrake) ||
        f.DriverBrake<0.0 ||
        f.DriverBrake>1.0 ||
        !Finite(f.DriverSteering) ||
        std::abs(f.DriverSteering)>p.MaximumSteeringInput)
        return false;
    for(int i=0;i<f.ContactCount;++i)
    {
        const auto& contact=f.Contacts[
            static_cast<std::size_t>(i)];
        if(!ValidIncidentContact(contact,p) ||
           contact.Identity.Session!=f.Identity.Session ||
           contact.Identity.SessionRevision!=
               f.Identity.SessionRevision ||
           contact.Identity.Actor!=f.Identity.Actor ||
           contact.Identity.Frame!=f.Identity.Frame ||
           std::abs(contact.PhysicsClockSeconds-
                    f.PhysicsClockSeconds)>
                p.MaximumContactClockSkewSeconds)
            return false;
        for(int j=0;j<i;++j)
            if(f.Contacts[static_cast<std::size_t>(j)].
                ContactEventToken==contact.ContactEventToken)
                return false;
    }
    return true;
}
inline PhysicalIncidentContactReview ReviewPhysicalImpact(
    PhysicalIncidentContact contact,
    double measuredMassKg,
    IncidentReviewParameters p) noexcept
{
    PhysicalIncidentContactReview out;
    if(!ValidIncidentContact(contact,p) ||
       !Finite(measuredMassKg) ||
       measuredMassKg<p.MinimumVehicleMassKg ||
       measuredMassKg>p.MaximumVehicleMassKg)
        return out;
    const Vec3 n=contact.ContactNormal;
    const Vec3 impulse=contact.ImpulseNSeconds;
    out.ContactEventToken=contact.ContactEventToken;
    out.OtherActorToken=contact.OtherActorToken;
    out.Class=contact.Class;
    out.NormalImpulseNs=std::abs(Dot(impulse,n));
    out.TotalImpulseNs=Length(impulse);
    out.TangentialImpulseNs=std::sqrt(std::max(
        0.0,Square(out.TotalImpulseNs)-
            Square(out.NormalImpulseNs)));
    out.RelativeClosingVelocityCmS=std::max(
        0.0,-Dot(contact.RelativeContactVelocityCmS,n));
    out.ContactEnergyJ=out.TotalImpulseNs*
        out.RelativeClosingVelocityCmS/100.0*0.5;
    out.EstimatedAccelerationG=SafeDivision(
        out.TotalImpulseNs/measuredMassKg,
        p.MaximumFrameSeconds*GravityCmS2/100.0);
    out.NormalAlignmentFraction=SafeDivision(
        out.NormalImpulseNs,
        std::max(PhysicsEpsilon,out.TotalImpulseNs));
    out.MeasuredConfidence=contact.SensorConfidence;
    out.CollisionAngleRadians=std::acos(
        std::clamp(out.NormalAlignmentFraction,-1.0,1.0));
    out.ContactPointLeverCm=std::hypot(
        contact.ContactWorldCm.X,
        contact.ContactWorldCm.Y);
    out.EstimatedYawImpulseNmSeconds=SafeDivision(
        out.TangentialImpulseNs*out.ContactPointLeverCm,100.0);
    out.RelativeKineticEnergyJ=0.5*measuredMassKg*
        Square(Length(
            contact.RelativeContactVelocityCmS)/100.0);
    out.EstimatedDamageSeverity=Clamp01(SafeDivision(
        out.ContactEnergyJ,
        p.MaximumPhysicalDamageEnergyJ));
    out.ContactPenetrationCm=contact.ContactPenetrationCm;
    out.RivalVerified=contact.Class==
        IncidentSourceClass::RivalContact &&
        contact.OpponentPhysicsVerified;
    out.BarrierVerified=contact.Class==
        IncidentSourceClass::WallImpact &&
        contact.OriginalCircuitProven;
    out.SupportsRaceMarshalReview=
        out.RivalVerified || out.BarrierVerified;
    out.CanTrustAsPhysicalContact=
        out.TotalImpulseNs>=p.MinimumContactImpulseNs &&
        contact.ActualChaosContactReported;
    out.Valid=Finite(out.TotalImpulseNs) &&
        Finite(out.TangentialImpulseNs) &&
        Finite(out.ContactEnergyJ) &&
        Finite(out.EstimatedDamageSeverity);
    return out;
}
inline PhysicalIncidentReview ReviewPhysicalIncident(
    const PhysicalIncidentFrame& f,
    IncidentReviewParameters p={}) noexcept
{
    PhysicalIncidentReview out;
    if(!ValidIncidentFrame(f,p))
        return out;
    out.Identity=f.Identity;
    out.Fault=IncidentReviewFault::None;
    out.MeasuredDamageFraction=std::max({
        f.MeasuredChassisDamageFraction,
        f.MeasuredSuspensionDamageFraction,
        f.MeasuredWheelDamageFraction});
    out.MeasuredKineticEnergyBeforeJ=
        0.5*f.MeasuredVehicleMassKg*
        Square(Length(f.PreImpactVelocityCmS)/100.0);
    out.MeasuredKineticEnergyAfterJ=
        0.5*f.MeasuredVehicleMassKg*
        Square(Length(f.PostImpactVelocityCmS)/100.0);
    out.ActualKineticEnergyLossJ=std::max(
        0.0,out.MeasuredKineticEnergyBeforeJ-
            out.MeasuredKineticEnergyAfterJ);
    out.MeanContactConfidence=f.ContactCount>0?0.0:1.0;
    for(int i=0;i<f.ContactCount;++i)
    {
        const auto& contact=f.Contacts[
            static_cast<std::size_t>(i)];
        const auto review=ReviewPhysicalImpact(
            contact,f.MeasuredVehicleMassKg,p);
        if(!review.Valid)return {};
        out.Contacts[static_cast<std::size_t>(i)]=review;
        out.TotalNormalImpulseNs+=
            review.NormalImpulseNs;
        out.TotalTangentialImpulseNs+=
            review.TangentialImpulseNs;
        out.TotalImpulseNs+=review.TotalImpulseNs;
        out.TotalContactEnergyJ+=review.ContactEnergyJ;
        out.PeakContactImpulseNs=std::max(
            out.PeakContactImpulseNs,review.TotalImpulseNs);
        out.MaximumEstimatedImpactG=std::max(
            out.MaximumEstimatedImpactG,
            review.EstimatedAccelerationG);
        out.MaximumDamageSeverity=std::max(
            out.MaximumDamageSeverity,
            review.EstimatedDamageSeverity);
        out.MeanContactConfidence+=
            review.MeasuredConfidence;
        out.VerifiedPhysicalContacts+=
            review.CanTrustAsPhysicalContact?1:0;
        out.VerifiedRivalContacts+=review.RivalVerified?1:0;
        out.OriginalBarrierContacts+=review.BarrierVerified?1:0;
        out.SevereContactCount+=
            review.TotalImpulseNs>=p.ContactSeverityHighNs?1:0;
    }
    if(f.ContactCount>0)
        out.MeanContactConfidence/=f.ContactCount;
    out.PhysicalIncidentDetected=
        out.VerifiedPhysicalContacts>0;
    out.MomentumResidualNs=std::abs(
        f.MeasuredVehicleMassKg*
            Length(f.PostImpactVelocityCmS-
                   f.PreImpactVelocityCmS)/100.0-
        out.TotalImpulseNs);
    out.PitInspectionRecommended=
        out.SevereContactCount>0 ||
        f.MeasuredSuspensionDamageFraction>0.3 ||
        f.MeasuredWheelDamageFraction>0.3;
    out.ImmediateStopRecommended=
        out.SevereContactCount>0 ||
        f.MeasuredWheelContactCount<3.0 ||
        out.MeasuredDamageFraction>0.75;
    out.RecommendedThrottleCeiling=
        out.ImmediateStopRecommended ?
            0.0 :
        out.PitInspectionRecommended ?
            p.ThrottleCeilingAfterImpact:1.0;
    out.RecommendedBrakeFloor=
        out.ImmediateStopRecommended ?
            p.BrakePedalForEmergency:0.0;
    out.RecommendedSteeringCeiling=
        out.ImmediateStopRecommended ?0.25:1.0;
    out.NeverIssuedChaosImpulse=true;
    out.OfficialPenaltyUntouched=true;
    out.Valid=Finite(out.MomentumResidualNs) &&
        Finite(out.TotalContactEnergyJ) &&
        Finite(out.RecommendedBrakeFloor);
    return out;
}
class PhysicalIncidentReviewSupervisor final
{
public:
    bool Configure(IncidentReviewParameters p) noexcept
    {
        if(!ValidIncidentReviewParameters(p))return false;
        Settings=p;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Source={};
        HasFrame=false;
        Accepted=0;
        Rejected=0;
        IncidentFrames=0;
        SevereIncidentFrames=0;
        OriginalBarrierFrames=0;
        PeakObservedImpulseNs=0.0;
        TotalEstimatedContactEnergyJ=0.0;
    }
    PhysicalIncidentReview Step(
        const PhysicalIncidentFrame& s) noexcept
    {
        const auto proposed=ReviewPhysicalIncident(s,Settings);
        const bool sameEpoch=HasFrame &&
            Source.Session==s.Identity.Session &&
            Source.SessionRevision==s.Identity.SessionRevision &&
            Source.Actor==s.Identity.Actor;
        if(!proposed.Valid ||
           (sameEpoch && s.Identity.Frame<=Source.Frame))
        {
            ++Rejected;
            Last={};
            return {};
        }
        Source=s.Identity;
        HasFrame=true;
        Last=proposed;
        ++Accepted;
        IncidentFrames+=proposed.PhysicalIncidentDetected?1U:0U;
        SevereIncidentFrames+=
            proposed.SevereContactCount>0?1U:0U;
        OriginalBarrierFrames+=
            proposed.OriginalBarrierContacts>0?1U:0U;
        PeakObservedImpulseNs=std::max(
            PeakObservedImpulseNs,proposed.PeakContactImpulseNs);
        TotalEstimatedContactEnergyJ+=
            proposed.TotalContactEnergyJ;
        return Last;
    }
    PhysicalIncidentReview Snapshot() const noexcept{return Last;}
    std::uint64_t Frames() const noexcept{return Accepted;}
    std::uint64_t Rejections() const noexcept{return Rejected;}
    std::uint64_t IncidentSamples() const noexcept
    {return IncidentFrames;}
    std::uint64_t SevereIncidentSamples() const noexcept
    {return SevereIncidentFrames;}
    std::uint64_t BarrierIncidentSamples() const noexcept
    {return OriginalBarrierFrames;}
    double MaximumPhysicalImpulseNs() const noexcept
    {return PeakObservedImpulseNs;}
    double TotalEstimatedContactEnergyJoules() const noexcept
    {return TotalEstimatedContactEnergyJ;}
private:
    IncidentReviewParameters Settings{};
    PhysicalIncidentReview Last{};
    RaceIdentity Source{};
    std::uint64_t Accepted=0;
    std::uint64_t Rejected=0;
    std::uint64_t IncidentFrames=0;
    std::uint64_t SevereIncidentFrames=0;
    std::uint64_t OriginalBarrierFrames=0;
    double PeakObservedImpulseNs=0.0;
    double TotalEstimatedContactEnergyJ=0.0;
    bool HasFrame=false;
};
} // namespace latebrake::racecraft
