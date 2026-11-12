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
enum class PitCrewPhase : std::uint8_t
{
    Unprepared, Reserved, WaitingForCar, StationaryVerified,
    JackUp, RemoveWheels, FitWheels, Fueling, Repairing,
    JackDown, SafetyInspection, ReadyForRelease, Released,
    Aborted
};
enum class PitCrewFault : std::uint8_t
{
    None, MissingRealPit, LostRoster, CarNotStationary,
    WrongServiceBox, DriverBrakeReleased, UnsafeCrewArea,
    MissingJacks, MissingWheelCrew, FuelCapacityExceeded,
    InsufficientElapsedTime, IncorrectServiceOrder,
    BrakeOrTireNotVerified, StaleEngineFrame
};
enum class PitCrewTask : std::uint8_t
{
    Lift, RemoveFrontLeft, RemoveFrontRight, RemoveRearLeft,
    RemoveRearRight, InstallFrontLeft, InstallFrontRight,
    InstallRearLeft, InstallRearRight, Refuel, RepairBody,
    RepairSuspension, InspectWheelTorque, ReleaseJack, Count
};
inline constexpr std::size_t PitCrewTaskCapacity=
    static_cast<std::size_t>(PitCrewTask::Count);
struct PitCrewServiceParameters final
{
    double MaximumPitSpeedCmS=75.0;
    double MinimumBrakeFraction=0.65;
    double MaximumSteeringFraction=0.25;
    double MaximumBoxOffsetCm=65.0;
    double MaximumWheelAlignmentErrorRadians=0.15;
    double RequiredCollisionClearanceCm=180.0;
    double MaximumHydraulicJackTravelCm=45.0;
    double MinimumJackSupportLoadN=2000.0;
    double MaximumJackSupportLoadN=18000.0;
    double MinimumWheelTorqueNm=80.0;
    double MaximumWheelTorqueNm=180.0;
    double TargetWheelTorqueNm=120.0;
    double MinimumTirePressureKpa=150.0;
    double MaximumTirePressureKpa=300.0;
    double MaximumFuelTankLiters=115.0;
    double MaximumFuelFlowLitersPerSecond=3.5;
    double FuelSpillThresholdLiters=0.25;
    double MaximumFuelRequestedLiters=100.0;
    double MaximumFuelDisagreementLiters=1.0;
    double MaximumRepairFractionPerSecond=0.15;
    double MaximumRepairableDamageFraction=0.70;
    double MinimumInitialReservationSeconds=0.2;
    double MaximumReservationSeconds=180.0;
    double MinimumLiftSeconds=0.8;
    double MinimumWheelRemovalSeconds=0.5;
    double MinimumWheelInstallationSeconds=0.6;
    double MinimumWheelTorqueInspectionSeconds=0.5;
    double MinimumJackLoweringSeconds=0.6;
    double MinimumFinalInspectionSeconds=0.5;
    double MinimumPitServiceSeconds=2.5;
    double MaximumPitServiceSeconds=150.0;
    double MaximumSourceClockSkewSeconds=0.05;
    double MaximumFrameSeconds=0.10;
    double MinimumSensorConfidence=0.8;
    double MinimumWheelQualifiedConfidence=0.75;
    double MaximumDamageAfterServiceFraction=0.95;
    bool RequireOriginalPitBox=true;
    bool RequireOfficialBoxReservation=true;
    bool RequirePhysicalWheelRemoval=true;
    bool RequireActualFuelSensor=true;
    bool RequireWheelTorqueInspection=true;
    bool DoNotInventServicedTires=true;
    bool DoNotIssueUnrealComponentChanges=true;
};
struct PitCrewWheelObservation final
{
    WheelObservation Wheel{};
    double MeasuredWheelBoltTorqueNm=0.0;
    double MeasuredTirePressureKpa=200.0;
    double WheelRemovalClockSeconds=0.0;
    double WheelInstallationClockSeconds=0.0;
    double CrewSensorConfidence=1.0;
    std::uint64_t WheelComponentToken=0;
    bool ActualWheelRemoved=false;
    bool ActualNewWheelInstalled=false;
    bool WheelTorqueVerified=false;
    bool OriginalChaosWheelComponent=false;
};
struct PitCrewServiceFrame final
{
    RaceIdentity Identity{};
    std::array<PitCrewWheelObservation,PhysicalWheelCount> Wheels{};
    std::array<bool,PitCrewTaskCapacity> CrewTaskAcknowledged{};
    double PhysicsClockSeconds=0.0;
    double DeltaSeconds=0.0;
    double CurrentVehicleSpeedCmS=0.0;
    double ActualBrakeFraction=0.0;
    double ActualSteeringFraction=0.0;
    double PhysicalBoxOffsetCm=0.0;
    double BoxHeadingErrorRadians=0.0;
    double JackHeightCm=0.0;
    double MeasuredJackSupportLoadN=0.0;
    double CrewAreaClearanceCm=0.0;
    double MeasuredFuelTankLiters=0.0;
    double RequestedFuelLiters=0.0;
    double MeasuredFuelFlowLitersSecond=0.0;
    double CurrentBodyDamageFraction=0.0;
    double CurrentSuspensionDamageFraction=0.0;
    double SensorConfidence=1.0;
    std::uint64_t PitBoxComponentToken=0;
    std::uint64_t ReservationToken=0;
    bool OriginalPitFacilityVerified=false;
    bool ReservationAuthorizedByRaceDirector=false;
    bool OriginalVehicleStationary=false;
    bool EngineVehicleBodyQualified=false;
    bool PitLaneCollisionEnabled=false;
    bool RaceSessionActive=false;
    bool EngineFuelProbeQualified=false;
    bool ActualServiceCrewPresent=false;
    bool PitReleaseAuthorized=false;
};
struct PitCrewTaskReview final
{
    PitCrewTask Task=PitCrewTask::Lift;
    bool PhysicallyObserved=false;
    bool RequiredForService=false;
    bool CanStart=false;
    bool Complete=false;
    double MinimumElapsedSeconds=0.0;
    double ExpectedRemainingSeconds=0.0;
    double MeasuredElapsedSeconds=0.0;
    double CompletionConfidence=0.0;
    bool DependenciesSatisfied=false;
};
struct PitCrewServiceReview final
{
    RaceIdentity Identity{};
    std::array<PitCrewTaskReview,PitCrewTaskCapacity> Tasks{};
    PitCrewPhase Phase=PitCrewPhase::Unprepared;
    PitCrewFault Fault=PitCrewFault::MissingRealPit;
    std::uint64_t ReservationToken=0;
    double TotalElapsedSeconds=0.0;
    double MinimumRemainingSeconds=0.0;
    double CurrentFuelLiters=0.0;
    double RemainingFuelRequestLiters=0.0;
    double DeliveredFuelLiters=0.0;
    double MaximumReportedWheelTorqueNm=0.0;
    double MinimumReportedWheelTorqueNm=0.0;
    double MinimumCrewConfidence=0.0;
    double ActualJackLoadN=0.0;
    double EstimatedFuelDurationSeconds=0.0;
    double MaximumWheelServiceDurationSeconds=0.0;
    double ServiceReadinessFraction=0.0;
    int TasksRequired=0;
    int TasksPhysicallyComplete=0;
    int WheelsActuallyReplaced=0;
    int WheelsTorqueQualified=0;
    bool PitBoxAccepted=false;
    bool DriverBrakeHolding=false;
    bool PhysicalReleaseClear=false;
    bool ServiceCanProceed=false;
    bool ActualUnrealVehicleModified=false;
    bool OfficialRaceResultModified=false;
    bool Valid=false;
};
inline bool ValidPitCrewParameters(
    PitCrewServiceParameters p) noexcept
{
    return Finite(p.MaximumPitSpeedCmS) &&
        p.MaximumPitSpeedCmS>0.0 &&
        Finite(p.MinimumBrakeFraction) &&
        p.MinimumBrakeFraction>0.0 &&
        p.MinimumBrakeFraction<=1.0 &&
        Finite(p.MaximumSteeringFraction) &&
        p.MaximumSteeringFraction>=0.0 &&
        p.MaximumSteeringFraction<=1.0 &&
        Finite(p.MaximumBoxOffsetCm) &&
        p.MaximumBoxOffsetCm>0.0 &&
        Finite(p.MaximumWheelAlignmentErrorRadians) &&
        p.MaximumWheelAlignmentErrorRadians>0.0 &&
        Finite(p.RequiredCollisionClearanceCm) &&
        p.RequiredCollisionClearanceCm>=0.0 &&
        Finite(p.MaximumHydraulicJackTravelCm) &&
        p.MaximumHydraulicJackTravelCm>0.0 &&
        Finite(p.MinimumJackSupportLoadN) &&
        p.MinimumJackSupportLoadN>0.0 &&
        Finite(p.MaximumJackSupportLoadN) &&
        p.MaximumJackSupportLoadN>p.MinimumJackSupportLoadN &&
        Finite(p.MinimumWheelTorqueNm) &&
        p.MinimumWheelTorqueNm>0.0 &&
        Finite(p.MaximumWheelTorqueNm) &&
        p.MaximumWheelTorqueNm>p.MinimumWheelTorqueNm &&
        Finite(p.TargetWheelTorqueNm) &&
        p.TargetWheelTorqueNm>=p.MinimumWheelTorqueNm &&
        p.TargetWheelTorqueNm<=p.MaximumWheelTorqueNm &&
        Finite(p.MinimumTirePressureKpa) &&
        p.MinimumTirePressureKpa>0.0 &&
        Finite(p.MaximumTirePressureKpa) &&
        p.MaximumTirePressureKpa>p.MinimumTirePressureKpa &&
        Finite(p.MaximumFuelTankLiters) &&
        p.MaximumFuelTankLiters>0.0 &&
        Finite(p.MaximumFuelFlowLitersPerSecond) &&
        p.MaximumFuelFlowLitersPerSecond>0.0 &&
        Finite(p.FuelSpillThresholdLiters) &&
        p.FuelSpillThresholdLiters>=0.0 &&
        Finite(p.MaximumFuelRequestedLiters) &&
        p.MaximumFuelRequestedLiters>0.0 &&
        Finite(p.MaximumFuelDisagreementLiters) &&
        p.MaximumFuelDisagreementLiters>=0.0 &&
        Finite(p.MaximumRepairFractionPerSecond) &&
        p.MaximumRepairFractionPerSecond>=0.0 &&
        Finite(p.MaximumRepairableDamageFraction) &&
        p.MaximumRepairableDamageFraction>=0.0 &&
        p.MaximumRepairableDamageFraction<=1.0 &&
        Finite(p.MinimumInitialReservationSeconds) &&
        p.MinimumInitialReservationSeconds>=0.0 &&
        Finite(p.MaximumReservationSeconds) &&
        p.MaximumReservationSeconds>0.0 &&
        Finite(p.MinimumLiftSeconds) &&
        p.MinimumLiftSeconds>0.0 &&
        Finite(p.MinimumWheelRemovalSeconds) &&
        p.MinimumWheelRemovalSeconds>0.0 &&
        Finite(p.MinimumWheelInstallationSeconds) &&
        p.MinimumWheelInstallationSeconds>0.0 &&
        Finite(p.MinimumWheelTorqueInspectionSeconds) &&
        p.MinimumWheelTorqueInspectionSeconds>0.0 &&
        Finite(p.MinimumJackLoweringSeconds) &&
        p.MinimumJackLoweringSeconds>0.0 &&
        Finite(p.MinimumFinalInspectionSeconds) &&
        p.MinimumFinalInspectionSeconds>0.0 &&
        Finite(p.MinimumPitServiceSeconds) &&
        p.MinimumPitServiceSeconds>0.0 &&
        Finite(p.MaximumPitServiceSeconds) &&
        p.MaximumPitServiceSeconds>p.MinimumPitServiceSeconds &&
        Finite(p.MaximumSourceClockSkewSeconds) &&
        p.MaximumSourceClockSkewSeconds>=0.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MinimumSensorConfidence) &&
        p.MinimumSensorConfidence>0.0 &&
        p.MinimumSensorConfidence<=1.0 &&
        Finite(p.MinimumWheelQualifiedConfidence) &&
        p.MinimumWheelQualifiedConfidence>0.0 &&
        p.MinimumWheelQualifiedConfidence<=1.0 &&
        Finite(p.MaximumDamageAfterServiceFraction) &&
        p.MaximumDamageAfterServiceFraction>0.0 &&
        p.MaximumDamageAfterServiceFraction<=1.0;
}
inline bool ValidPitCrewWheel(
    PitCrewWheelObservation w,PitCrewServiceParameters p) noexcept
{
    return ValidWheelObservation(w.Wheel) &&
        Finite(w.MeasuredWheelBoltTorqueNm) &&
        w.MeasuredWheelBoltTorqueNm>=0.0 &&
        w.MeasuredWheelBoltTorqueNm<=p.MaximumWheelTorqueNm*2.0 &&
        Finite(w.MeasuredTirePressureKpa) &&
        w.MeasuredTirePressureKpa>=0.0 &&
        w.MeasuredTirePressureKpa<=p.MaximumTirePressureKpa*1.5 &&
        Finite(w.WheelRemovalClockSeconds) &&
        w.WheelRemovalClockSeconds>=0.0 &&
        Finite(w.WheelInstallationClockSeconds) &&
        w.WheelInstallationClockSeconds>=0.0 &&
        Finite(w.CrewSensorConfidence) &&
        w.CrewSensorConfidence>=p.MinimumWheelQualifiedConfidence &&
        w.CrewSensorConfidence<=1.0 &&
        w.WheelComponentToken!=0 &&
        (!p.RequirePhysicalWheelRemoval ||
         w.OriginalChaosWheelComponent);
}
// A crew checkbox is not evidence that a real Chaos wheel has been
// removed and a different one fitted. Require distinct physical timestamps,
// original wheel ownership, fitment latency and measured tire condition.
inline bool PitCrewWheelPhysicallyServiced(
    PitCrewWheelObservation wheel,
    double currentEngineClockSeconds,
    PitCrewServiceParameters p) noexcept
{
    return ValidPitCrewWheel(wheel,p) &&
        wheel.ActualWheelRemoved &&
        wheel.ActualNewWheelInstalled &&
        wheel.WheelRemovalClockSeconds>0.0 &&
        wheel.WheelInstallationClockSeconds>=
            wheel.WheelRemovalClockSeconds+
            p.MinimumWheelRemovalSeconds &&
        currentEngineClockSeconds>=
            wheel.WheelInstallationClockSeconds+
            p.MinimumWheelInstallationSeconds &&
        wheel.MeasuredTirePressureKpa>=p.MinimumTirePressureKpa &&
        wheel.MeasuredTirePressureKpa<=p.MaximumTirePressureKpa;
}
inline bool PitCrewWheelTorquePhysicallyVerified(
    PitCrewWheelObservation wheel,
    double currentEngineClockSeconds,
    PitCrewServiceParameters p) noexcept
{
    return PitCrewWheelPhysicallyServiced(
        wheel,currentEngineClockSeconds,p) &&
        wheel.WheelTorqueVerified &&
        wheel.MeasuredWheelBoltTorqueNm>=p.MinimumWheelTorqueNm &&
        wheel.MeasuredWheelBoltTorqueNm<=p.MaximumWheelTorqueNm &&
        currentEngineClockSeconds>=
            wheel.WheelInstallationClockSeconds+
            p.MinimumWheelInstallationSeconds+
            p.MinimumWheelTorqueInspectionSeconds;
}
inline bool ValidPitCrewServiceFrame(
    const PitCrewServiceFrame& f,
    PitCrewServiceParameters p) noexcept
{
    if (!ValidPitCrewParameters(p) ||
        !PhysicalActorIdentityValid(f.Identity) ||
        f.Identity.Frame==0 ||
        !f.ActualServiceCrewPresent ||
        !f.EngineVehicleBodyQualified ||
        !f.PitLaneCollisionEnabled ||
        !f.RaceSessionActive ||
        (p.RequireOriginalPitBox &&
         !f.OriginalPitFacilityVerified) ||
        (p.RequireOfficialBoxReservation &&
         !f.ReservationAuthorizedByRaceDirector) ||
        (p.RequireActualFuelSensor &&
         !f.EngineFuelProbeQualified) ||
        !Finite(f.PhysicsClockSeconds) ||
        f.PhysicsClockSeconds<0.0 ||
        !Finite(f.DeltaSeconds) ||
        f.DeltaSeconds<=0.0 ||
        f.DeltaSeconds>p.MaximumFrameSeconds ||
        !Finite(f.CurrentVehicleSpeedCmS) ||
        f.CurrentVehicleSpeedCmS<0.0 ||
        f.CurrentVehicleSpeedCmS>MaximumPhysicalSpeedCmS ||
        !Finite(f.ActualBrakeFraction) ||
        f.ActualBrakeFraction<0.0 ||
        f.ActualBrakeFraction>1.0 ||
        !Finite(f.ActualSteeringFraction) ||
        std::abs(f.ActualSteeringFraction)>1.0 ||
        !Finite(f.PhysicalBoxOffsetCm) ||
        !Finite(f.BoxHeadingErrorRadians) ||
        !Finite(f.JackHeightCm) ||
        f.JackHeightCm<0.0 ||
        f.JackHeightCm>p.MaximumHydraulicJackTravelCm ||
        !Finite(f.MeasuredJackSupportLoadN) ||
        f.MeasuredJackSupportLoadN<0.0 ||
        f.MeasuredJackSupportLoadN>p.MaximumJackSupportLoadN ||
        !Finite(f.CrewAreaClearanceCm) ||
        f.CrewAreaClearanceCm<0.0 ||
        !Finite(f.MeasuredFuelTankLiters) ||
        f.MeasuredFuelTankLiters<0.0 ||
        f.MeasuredFuelTankLiters>p.MaximumFuelTankLiters ||
        !Finite(f.RequestedFuelLiters) ||
        f.RequestedFuelLiters<0.0 ||
        f.RequestedFuelLiters>p.MaximumFuelRequestedLiters ||
        !Finite(f.MeasuredFuelFlowLitersSecond) ||
        f.MeasuredFuelFlowLitersSecond<0.0 ||
        f.MeasuredFuelFlowLitersSecond>
            p.MaximumFuelFlowLitersPerSecond ||
        !Finite(f.CurrentBodyDamageFraction) ||
        f.CurrentBodyDamageFraction<0.0 ||
        f.CurrentBodyDamageFraction>1.0 ||
        !Finite(f.CurrentSuspensionDamageFraction) ||
        f.CurrentSuspensionDamageFraction<0.0 ||
        f.CurrentSuspensionDamageFraction>1.0 ||
        !Finite(f.SensorConfidence) ||
        f.SensorConfidence<p.MinimumSensorConfidence ||
        f.SensorConfidence>1.0 ||
        f.PitBoxComponentToken==0 ||
        f.ReservationToken==0)
        return false;
    for(const auto& wheel:f.Wheels)
    {
        if (!ValidPitCrewWheel(wheel,p) ||
            (wheel.ActualWheelRemoved &&
             (wheel.WheelRemovalClockSeconds<=0.0 ||
              wheel.WheelRemovalClockSeconds>f.PhysicsClockSeconds)) ||
            (wheel.ActualNewWheelInstalled &&
             (!wheel.ActualWheelRemoved ||
              wheel.WheelInstallationClockSeconds<
                wheel.WheelRemovalClockSeconds+
                    p.MinimumWheelRemovalSeconds ||
              wheel.WheelInstallationClockSeconds>
                f.PhysicsClockSeconds)) ||
            (wheel.WheelTorqueVerified &&
             !wheel.ActualNewWheelInstalled))
            return false;
    }
    return true;
}
inline bool PitCrewCarSafelyStationary(
    const PitCrewServiceFrame& f,
    PitCrewServiceParameters p) noexcept
{
    return ValidPitCrewServiceFrame(f,p) &&
        f.OriginalVehicleStationary &&
        f.CurrentVehicleSpeedCmS<=p.MaximumPitSpeedCmS &&
        f.ActualBrakeFraction>=p.MinimumBrakeFraction &&
        std::abs(f.ActualSteeringFraction)<=
            p.MaximumSteeringFraction &&
        std::abs(f.PhysicalBoxOffsetCm)<=
            p.MaximumBoxOffsetCm &&
        std::abs(f.BoxHeadingErrorRadians)<=
            p.MaximumWheelAlignmentErrorRadians &&
        f.CrewAreaClearanceCm>=
            p.RequiredCollisionClearanceCm;
}
inline PitCrewTaskReview ReviewPitCrewTask(
    const PitCrewServiceFrame& f,
    PitCrewTask task,
    PitCrewServiceParameters p) noexcept
{
    PitCrewTaskReview out;
    if (!ValidPitCrewServiceFrame(f,p))
        return out;
    out.Task=task;
    const auto index=static_cast<std::size_t>(task);
    out.PhysicallyObserved=f.CrewTaskAcknowledged[index];
    const int removeIndex=static_cast<int>(task)-
        static_cast<int>(PitCrewTask::RemoveFrontLeft);
    const int installIndex=static_cast<int>(task)-
        static_cast<int>(PitCrewTask::InstallFrontLeft);
    if(removeIndex>=0 && removeIndex<PhysicalWheelCount)
    {
        const auto& wheel=f.Wheels[
            static_cast<std::size_t>(removeIndex)];
        out.PhysicallyObserved=out.PhysicallyObserved &&
            wheel.ActualWheelRemoved &&
            wheel.WheelRemovalClockSeconds>0.0 &&
            wheel.WheelRemovalClockSeconds<=f.PhysicsClockSeconds;
    }
    else if(installIndex>=0 && installIndex<PhysicalWheelCount)
    {
        out.PhysicallyObserved=out.PhysicallyObserved &&
            PitCrewWheelPhysicallyServiced(
                f.Wheels[static_cast<std::size_t>(installIndex)],
                f.PhysicsClockSeconds,p);
    }
    else if(task==PitCrewTask::InspectWheelTorque)
    {
        out.PhysicallyObserved=out.PhysicallyObserved &&
            std::all_of(f.Wheels.begin(),f.Wheels.end(),
                [&](const PitCrewWheelObservation& w)
                {
                    return PitCrewWheelTorquePhysicallyVerified(
                        w,f.PhysicsClockSeconds,p);
                });
    }
    else if(task==PitCrewTask::ReleaseJack)
    {
        out.PhysicallyObserved=out.PhysicallyObserved &&
            f.JackHeightCm<=PhysicsEpsilon &&
            f.MeasuredJackSupportLoadN<=PhysicsEpsilon;
    }
    out.RequiredForService=
        task==PitCrewTask::Lift ||
        task==PitCrewTask::InspectWheelTorque ||
        task==PitCrewTask::ReleaseJack ||
        (task>=PitCrewTask::RemoveFrontLeft &&
         task<=PitCrewTask::InstallRearRight);
    out.CanStart=PitCrewCarSafelyStationary(f,p);
    out.MinimumElapsedSeconds=
        task==PitCrewTask::Lift?p.MinimumLiftSeconds:
        task>=PitCrewTask::RemoveFrontLeft &&
        task<=PitCrewTask::RemoveRearRight ?
            p.MinimumWheelRemovalSeconds:
        task>=PitCrewTask::InstallFrontLeft &&
        task<=PitCrewTask::InstallRearRight ?
            p.MinimumWheelInstallationSeconds:
        task==PitCrewTask::InspectWheelTorque ?
            p.MinimumWheelTorqueInspectionSeconds:
        task==PitCrewTask::ReleaseJack ?
            p.MinimumJackLoweringSeconds:
        task==PitCrewTask::Refuel ?
            SafeDivision(f.RequestedFuelLiters,
                         p.MaximumFuelFlowLitersPerSecond):
        task==PitCrewTask::RepairBody ||
        task==PitCrewTask::RepairSuspension ?
            SafeDivision(f.CurrentBodyDamageFraction,
                         p.MaximumRepairFractionPerSecond):0.0;
    out.MeasuredElapsedSeconds=0.0;
    out.ExpectedRemainingSeconds=out.MinimumElapsedSeconds;
    out.CompletionConfidence=f.SensorConfidence;
    out.DependenciesSatisfied=
        out.CanStart &&
        (task==PitCrewTask::Lift ||
         f.CrewTaskAcknowledged[
            static_cast<std::size_t>(PitCrewTask::Lift)]);
    out.Complete=out.PhysicallyObserved &&
        out.DependenciesSatisfied;
    return out;
}
inline PitCrewServiceReview EvaluatePhysicalPitService(
    const PitCrewServiceFrame& f,
    PitCrewServiceParameters p={}) noexcept
{
    PitCrewServiceReview out;
    if (!ValidPitCrewServiceFrame(f,p))
        return out;
    out.Identity=f.Identity;
    out.ReservationToken=f.ReservationToken;
    out.CurrentFuelLiters=f.MeasuredFuelTankLiters;
    out.RemainingFuelRequestLiters=
        f.RequestedFuelLiters;
    out.ActualJackLoadN=f.MeasuredJackSupportLoadN;
    out.MinimumCrewConfidence=f.SensorConfidence;
    out.PitBoxAccepted=true;
    out.DriverBrakeHolding=
        f.ActualBrakeFraction>=p.MinimumBrakeFraction;
    out.ServiceCanProceed=PitCrewCarSafelyStationary(f,p);
    if (!out.ServiceCanProceed)
    {
        out.Phase=PitCrewPhase::WaitingForCar;
        out.Fault=PitCrewFault::CarNotStationary;
        out.Valid=true;
        return out;
    }
    out.Phase=PitCrewPhase::StationaryVerified;
    out.MinimumReportedWheelTorqueNm=
        std::numeric_limits<double>::infinity();
    for(std::size_t i=0;i<PhysicalWheelCount;++i)
    {
        const auto& wheel=f.Wheels[i];
        if (PitCrewWheelPhysicallyServiced(
                wheel,f.PhysicsClockSeconds,p))
            ++out.WheelsActuallyReplaced;
        if (PitCrewWheelTorquePhysicallyVerified(
                wheel,f.PhysicsClockSeconds,p))
            ++out.WheelsTorqueQualified;
        out.MaximumReportedWheelTorqueNm=std::max(
            out.MaximumReportedWheelTorqueNm,
            wheel.MeasuredWheelBoltTorqueNm);
        out.MinimumReportedWheelTorqueNm=std::min(
            out.MinimumReportedWheelTorqueNm,
            wheel.MeasuredWheelBoltTorqueNm);
        out.MinimumCrewConfidence=std::min(
            out.MinimumCrewConfidence,
            wheel.CrewSensorConfidence);
    }
    for(std::size_t i=0;i<PitCrewTaskCapacity;++i)
    {
        const auto task=static_cast<PitCrewTask>(i);
        const PitCrewTaskReview review=
            ReviewPitCrewTask(f,task,p);
        out.Tasks[i]=review;
        if (review.RequiredForService)
            ++out.TasksRequired;
        if (review.RequiredForService && review.Complete)
            ++out.TasksPhysicallyComplete;
        if (review.RequiredForService &&
            !review.Complete)
            out.MinimumRemainingSeconds+=
                review.ExpectedRemainingSeconds;
    }
    out.EstimatedFuelDurationSeconds=SafeDivision(
        out.RemainingFuelRequestLiters,
        p.MaximumFuelFlowLitersPerSecond);
    for(const auto& task:out.Tasks)
        if (task.Task>=PitCrewTask::RemoveFrontLeft &&
            task.Task<=PitCrewTask::InstallRearRight)
            out.MaximumWheelServiceDurationSeconds=
                std::max(
                    out.MaximumWheelServiceDurationSeconds,
                    task.ExpectedRemainingSeconds);
    out.ServiceReadinessFraction=Clamp01(SafeDivision(
        static_cast<double>(out.TasksPhysicallyComplete),
        static_cast<double>(out.TasksRequired)));
    const bool liftAcknowledged=f.CrewTaskAcknowledged[
        static_cast<std::size_t>(PitCrewTask::Lift)];
    const bool jackCurrentlySupporting=
        f.JackHeightCm>
            p.MaximumHydraulicJackTravelCm*0.25 &&
        f.MeasuredJackSupportLoadN>=
            p.MinimumJackSupportLoadN;
    const bool jackPhysicallyLowered=
        f.CrewTaskAcknowledged[
            static_cast<std::size_t>(PitCrewTask::ReleaseJack)] &&
        f.JackHeightCm<=PhysicsEpsilon &&
        f.MeasuredJackSupportLoadN<=PhysicsEpsilon;
    const bool wheelsReplaced=
        out.WheelsActuallyReplaced==PhysicalWheelCount;
    const bool torqueCertified=
        out.WheelsTorqueQualified==PhysicalWheelCount;
    const bool ready=liftAcknowledged &&
        jackPhysicallyLowered &&
        wheelsReplaced && torqueCertified &&
        out.TasksPhysicallyComplete==out.TasksRequired;
    if (ready)
    {
        // A single frame proves component state, NOT the chronology of
        // lifting, removal, fitment, torque and safe lowering. Only the
        // stateful supervisor may certify physical release clearance.
        out.Phase=PitCrewPhase::SafetyInspection;
        out.Fault=PitCrewFault::None;
        out.PhysicalReleaseClear=false;
    }
    else if(!liftAcknowledged ||
            (!jackCurrentlySupporting &&
             !jackPhysicallyLowered))
    {
        out.Phase=PitCrewPhase::JackUp;
        out.Fault=PitCrewFault::MissingJacks;
    }
    else if(!wheelsReplaced)
    {
        out.Phase=PitCrewPhase::FitWheels;
        out.Fault=PitCrewFault::MissingWheelCrew;
    }
    else if(!torqueCertified)
    {
        out.Phase=PitCrewPhase::SafetyInspection;
        out.Fault=PitCrewFault::BrakeOrTireNotVerified;
    }
    out.ActualUnrealVehicleModified=false;
    out.OfficialRaceResultModified=false;
    out.Valid=Finite(out.MinimumCrewConfidence) &&
        Finite(out.MinimumRemainingSeconds) &&
        Finite(out.ServiceReadinessFraction);
    return out;
}
// Pit clearance is a stateful claim about real wheel, jack and brake
// observations across SEPARATE engine frames. A single frame containing
// every checked task can never authorize an actual wheel-service release.
class PhysicalPitCrewSupervisor final
{
public:
    bool Configure(PitCrewServiceParameters p) noexcept
    {
        if (!ValidPitCrewParameters(p))return false;
        Settings=p;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Source={};
        ReservationToken=0;
        Qualified=0;
        Rejected=0;
        WheelServiceFrames=0;
        UnsafeReleaseFrames=0;
        CompletedServiceObservations=0;
        LastClockSeconds=0.0;
        HasFrame=false;
        ResetSequence();
    }
    PitCrewServiceReview Step(
        const PitCrewServiceFrame& sample) noexcept
    {
        auto result=EvaluatePhysicalPitService(sample,Settings);
        const bool sameEpoch=HasFrame &&
            Source.Session==sample.Identity.Session &&
            Source.SessionRevision==
                sample.Identity.SessionRevision &&
            Source.Actor==sample.Identity.Actor;
        if (!result.Valid ||
            (sameEpoch &&
             (sample.Identity.Frame<=Source.Frame ||
              sample.PhysicsClockSeconds<=LastClockSeconds ||
              ReservationToken!=sample.ReservationToken)))
        {
            ++Rejected;
            Last={};
            ResetSequence();
            return {};
        }
        if (!sameEpoch)
            ResetSequence();
        result.PhysicalReleaseClear=false;
        if (!result.ServiceCanProceed)
        {
            // A rolling chassis, released brake or intruding crew-area
            // hazard voids prior wheel-service evidence immediately.
            ResetSequence();
            result.Phase=PitCrewPhase::WaitingForCar;
            result.Fault=PitCrewFault::CarNotStationary;
        }
        else if (!SequenceStarted)
        {
            if(sample.JackHeightCm<=PhysicsEpsilon &&
               sample.MeasuredJackSupportLoadN<=PhysicsEpsilon &&
               !sample.CrewTaskAcknowledged[
                    static_cast<std::size_t>(PitCrewTask::Lift)])
            {
                SequenceStarted=true;
                ServiceStartClockSeconds=sample.PhysicsClockSeconds;
                InitialFuelLiters=sample.MeasuredFuelTankLiters;
                RequestedFuelLiters=sample.RequestedFuelLiters;
                EvidencePhase=PitCrewPhase::StationaryVerified;
                result.Fault=PitCrewFault::None;
            }
            else
            {
                result.Fault=PitCrewFault::IncorrectServiceOrder;
            }
            result.Phase=EvidencePhase;
        }
        else
        {
            AdvanceAuthenticatedService(sample,result);
        }
        if (SequenceStarted)
            result.TotalElapsedSeconds=std::max(
                0.0,sample.PhysicsClockSeconds-
                    ServiceStartClockSeconds);
        result.Valid=result.Valid &&
            Finite(result.TotalElapsedSeconds);
        result.ActualUnrealVehicleModified=false;
        result.OfficialRaceResultModified=false;
        Source=sample.Identity;
        ReservationToken=sample.ReservationToken;
        LastClockSeconds=sample.PhysicsClockSeconds;
        HasFrame=true;
        ++Qualified;
        WheelServiceFrames+=
            result.WheelsActuallyReplaced>0?1U:0U;
        UnsafeReleaseFrames+=
            sample.PitReleaseAuthorized &&
            !result.PhysicalReleaseClear?1U:0U;
        if(result.PhysicalReleaseClear)
            ++CompletedServiceObservations;
        Last=result;
        return Last;
    }
    PitCrewServiceReview Snapshot() const noexcept{return Last;}
    std::uint64_t Frames() const noexcept{return Qualified;}
    std::uint64_t Rejections() const noexcept{return Rejected;}
    std::uint64_t WheelServiceObservations() const noexcept
    {return WheelServiceFrames;}
    std::uint64_t UnsafeReleaseAttempts() const noexcept
    {return UnsafeReleaseFrames;}
    std::uint64_t CertifiedReleaseFrames() const noexcept
    {return CompletedServiceObservations;}
    PitCrewPhase CurrentPhase() const noexcept
    {return EvidencePhase;}
private:
    void ResetSequence() noexcept
    {
        SequenceStarted=false;
        EvidencePhase=PitCrewPhase::Unprepared;
        ServiceStartClockSeconds=0.0;
        JackRaisedClockSeconds=0.0;
        WheelsRemovedClockSeconds=0.0;
        WheelsInstalledClockSeconds=0.0;
        WheelTorquesClockSeconds=0.0;
        JackLoweredClockSeconds=0.0;
        InitialFuelLiters=0.0;
        RequestedFuelLiters=0.0;
    }
    bool AllWheelsRemoved(
        const PitCrewServiceFrame& frame) const noexcept
    {
        for(int i=0;i<PhysicalWheelCount;++i)
        {
            const auto& wheel=frame.Wheels[
                static_cast<std::size_t>(i)];
            if(!wheel.ActualWheelRemoved ||
               wheel.WheelRemovalClockSeconds<
                    JackRaisedClockSeconds ||
               wheel.WheelRemovalClockSeconds>
                    frame.PhysicsClockSeconds ||
               !frame.CrewTaskAcknowledged[
                   static_cast<std::size_t>(
                       static_cast<int>(
                           PitCrewTask::RemoveFrontLeft)+i)])
                return false;
        }
        return true;
    }
    bool AllWheelsInstalled(
        const PitCrewServiceFrame& frame) const noexcept
    {
        for(int i=0;i<PhysicalWheelCount;++i)
        {
            const auto& wheel=frame.Wheels[
                static_cast<std::size_t>(i)];
            if(!PitCrewWheelPhysicallyServiced(
                   wheel,frame.PhysicsClockSeconds,Settings) ||
               wheel.WheelInstallationClockSeconds<
                    WheelsRemovedClockSeconds ||
               !frame.CrewTaskAcknowledged[
                   static_cast<std::size_t>(
                       static_cast<int>(
                           PitCrewTask::InstallFrontLeft)+i)])
                return false;
        }
        return true;
    }
    bool AllWheelTorqueInspections(
        const PitCrewServiceFrame& frame) const noexcept
    {
        return frame.CrewTaskAcknowledged[
            static_cast<std::size_t>(
                PitCrewTask::InspectWheelTorque)] &&
            std::all_of(frame.Wheels.begin(),frame.Wheels.end(),
                [&](const PitCrewWheelObservation& wheel)
                {
                    return PitCrewWheelTorquePhysicallyVerified(
                        wheel,frame.PhysicsClockSeconds,Settings);
                });
    }
    void AdvanceAuthenticatedService(
        const PitCrewServiceFrame& frame,
        PitCrewServiceReview& out) noexcept
    {
        const double now=frame.PhysicsClockSeconds;
        const bool jackRaised=
            frame.CrewTaskAcknowledged[
                static_cast<std::size_t>(PitCrewTask::Lift)] &&
            frame.JackHeightCm>
                Settings.MaximumHydraulicJackTravelCm*0.25 &&
            frame.MeasuredJackSupportLoadN>=
                Settings.MinimumJackSupportLoadN;
        const bool jackLowered=
            frame.CrewTaskAcknowledged[
                static_cast<std::size_t>(
                    PitCrewTask::ReleaseJack)] &&
            frame.JackHeightCm<=PhysicsEpsilon &&
            frame.MeasuredJackSupportLoadN<=PhysicsEpsilon;
        switch(EvidencePhase)
        {
        case PitCrewPhase::StationaryVerified:
            if(jackRaised &&
               now-ServiceStartClockSeconds>=
                   Settings.MinimumInitialReservationSeconds+
                   Settings.MinimumLiftSeconds)
            {
                JackRaisedClockSeconds=now;
                EvidencePhase=PitCrewPhase::JackUp;
            }
            break;
        case PitCrewPhase::JackUp:
            if(jackRaised &&
               AllWheelsRemoved(frame) &&
               now-JackRaisedClockSeconds>=
                   Settings.MinimumWheelRemovalSeconds)
            {
                WheelsRemovedClockSeconds=now;
                EvidencePhase=PitCrewPhase::RemoveWheels;
            }
            break;
        case PitCrewPhase::RemoveWheels:
            if(jackRaised &&
               AllWheelsInstalled(frame) &&
               now-WheelsRemovedClockSeconds>=
                   Settings.MinimumWheelInstallationSeconds)
            {
                WheelsInstalledClockSeconds=now;
                EvidencePhase=PitCrewPhase::FitWheels;
            }
            break;
        case PitCrewPhase::FitWheels:
            if(jackRaised &&
               AllWheelTorqueInspections(frame) &&
               now-WheelsInstalledClockSeconds>=
                   Settings.MinimumWheelTorqueInspectionSeconds)
            {
                WheelTorquesClockSeconds=now;
                EvidencePhase=PitCrewPhase::SafetyInspection;
            }
            break;
        case PitCrewPhase::SafetyInspection:
            if(jackLowered &&
               now-WheelTorquesClockSeconds>=
                   Settings.MinimumJackLoweringSeconds)
            {
                JackLoweredClockSeconds=now;
                EvidencePhase=PitCrewPhase::JackDown;
            }
            break;
        case PitCrewPhase::JackDown:
        {
            const bool fuelPhysicallyQualified=
                RequestedFuelLiters<=0.0 ||
                (frame.EngineFuelProbeQualified &&
                 frame.CrewTaskAcknowledged[
                     static_cast<std::size_t>(
                         PitCrewTask::Refuel)] &&
                 frame.MeasuredFuelTankLiters+
                     Settings.MaximumFuelDisagreementLiters>=
                        InitialFuelLiters+RequestedFuelLiters);
            const bool elapsedQualified=
                now-ServiceStartClockSeconds>=
                    Settings.MinimumPitServiceSeconds &&
                now-JackLoweredClockSeconds>=
                    Settings.MinimumFinalInspectionSeconds;
            if(frame.PitReleaseAuthorized &&
               jackLowered &&
               AllWheelsInstalled(frame) &&
               AllWheelTorqueInspections(frame) &&
               out.TasksPhysicallyComplete==out.TasksRequired &&
               fuelPhysicallyQualified &&
               elapsedQualified)
            {
                EvidencePhase=PitCrewPhase::ReadyForRelease;
                out.PhysicalReleaseClear=true;
            }
            else if(frame.PitReleaseAuthorized)
            {
                out.Fault=!elapsedQualified ?
                    PitCrewFault::InsufficientElapsedTime:
                    PitCrewFault::IncorrectServiceOrder;
            }
            break;
        }
        case PitCrewPhase::ReadyForRelease:
            // The original reservation and contact/provenance checks
            // still run every frame; never issue an actual release
            // command or official pit/lap timing change here.
            out.PhysicalReleaseClear=
                frame.PitReleaseAuthorized &&
                jackLowered &&
                AllWheelsInstalled(frame) &&
                AllWheelTorqueInspections(frame);
            break;
        default:
            break;
        }
        out.Phase=EvidencePhase;
        if(frame.PitReleaseAuthorized &&
           !out.PhysicalReleaseClear &&
           out.Fault==PitCrewFault::None)
            out.Fault=PitCrewFault::IncorrectServiceOrder;
    }
    PitCrewServiceParameters Settings{};
    PitCrewServiceReview Last{};
    RaceIdentity Source{};
    std::uint64_t ReservationToken=0;
    std::uint64_t Qualified=0;
    std::uint64_t Rejected=0;
    std::uint64_t WheelServiceFrames=0;
    std::uint64_t UnsafeReleaseFrames=0;
    std::uint64_t CompletedServiceObservations=0;
    double LastClockSeconds=0.0;
    double ServiceStartClockSeconds=0.0;
    double JackRaisedClockSeconds=0.0;
    double WheelsRemovedClockSeconds=0.0;
    double WheelsInstalledClockSeconds=0.0;
    double WheelTorquesClockSeconds=0.0;
    double JackLoweredClockSeconds=0.0;
    double InitialFuelLiters=0.0;
    double RequestedFuelLiters=0.0;
    PitCrewPhase EvidencePhase=PitCrewPhase::Unprepared;
    bool SequenceStarted=false;
    bool HasFrame=false;
};
} // namespace latebrake::racecraft
