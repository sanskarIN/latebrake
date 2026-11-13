#pragma once
#include "Racecraft/CollisionImpulseModel.h"
#include "Racecraft/VehicleAerodynamics.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake::racecraft
{
enum class DamageZone : std::uint8_t
{
    FrontLeft, FrontRight, RearLeft, RearRight,
    FrontBumper, RearBumper, EngineBay, Gearbox,
    SteeringRack, Underbody, FuelSystem, AeroFront,
    AeroRear, CoolingSystem, Windshield, Count
};
inline constexpr int DamageZoneCount=static_cast<int>(
    DamageZone::Count);
struct DamageParameters final
{
    double MinimumEnergyToDeformJ=800.0;
    double MaximumSingleImpactEnergyJ=1.0e7;
    double DamageScalingEnergyJ=35000.0;
    double EnginePowerLossPerDamage=0.65;
    double GearboxEfficiencyLossPerDamage=0.50;
    double SteeringRackRangeLossPerDamage=0.35;
    double TireGripLossPerSuspensionDamage=0.25;
    double BrakeEffectivenessLossPerDamage=0.45;
    double CoolingLossPerDamage=0.60;
    double AeroFrontDownforceLossPerDamage=0.78;
    double AeroRearDownforceLossPerDamage=0.74;
    double AeroDragIncreasePerDamage=0.32;
    double FuelConsumptionIncreasePerDamage=0.40;
    double MaximumDamageIncrementPerEvent=0.45;
    double MaximumLegalRepairFractionPerPit=0.80;
    double MinimumComponentIntegrity=0.02;
    double MaximumScrapeForceN=40000.0;
    double ScrapeDamagePerJoule=0.000000002;
    double MaximumFrameSeconds=0.1;
    bool RequireActualContactReceipt=true;
    bool RequirePhysicalRepairReceipt=true;
    bool NeverApplyToLiveVehicle=true;
};
struct VehicleDamageObservation final
{
    RaceIdentity Identity{};
    DamageZone PrimaryZone=DamageZone::FrontBumper;
    ImpactMaterial ContactMaterial=ImpactMaterial::VehicleBody;
    double MeasuredImpactEnergyJ=0.0;
    double MeasuredRelativeSpeedCmS=0.0;
    double MeasuredNormalImpulseNs=0.0;
    double SurfaceScrapeEnergyJ=0.0;
    double MeasuredCurrentZoneIntegrity=1.0;
    double DeltaSeconds=0.0;
    double EngineRaceSeconds=0.0;
    bool OriginalChaosContactVerified=false;
    bool ActualPhysicalCar=true;
    bool OriginalWallOrCarBody=true;
    bool RepairReceipt=false;
    bool ActualPitServiceAuthorized=false;
    bool ChassisAtPhysicalPit=false;
};
struct DamageZoneStatus final
{
    DamageZone Zone=DamageZone::FrontBumper;
    double Integrity=1.0;
    double TotalAbsorbedEnergyJ=0.0;
    double TotalScrapeEnergyJ=0.0;
    double LastImpactEnergyJ=0.0;
    double LastImpactClockSeconds=0.0;
    int VerifiedImpacts=0;
    bool Critical=false;
    bool PhysicallyObserved=false;
};
struct VehicleDamageReport final
{
    std::array<DamageZoneStatus,DamageZoneCount> Zones{};
    double OverallIntegrityFraction=1.0;
    double DrivePowerCeiling=1.0;
    double BrakeStrengthCeiling=1.0;
    double SteeringRangeCeiling=1.0;
    double LateralGripCeiling=1.0;
    double AeroDownforceCeiling=1.0;
    double FuelEfficiencyMultiplier=1.0;
    double CoolingCapacityFraction=1.0;
    double EstimatedTopSpeedCeiling=MaximumPhysicalSpeedCmS;
    double ChassisDamageFraction=0.0;
    double MechanicalDamageFraction=0.0;
    double TotalImpactEnergyJ=0.0;
    double MaximumImpactEnergyJ=0.0;
    int DamagedZones=0;
    int CriticalZones=0;
    int VerifiedImpacts=0;
    int VerifiedPitRepairs=0;
    int RejectedEvents=0;
    bool UnsafeToContinue=false;
    bool MechanicalConsequencesHypothetical=true;
    bool ChangedActualVehiclePhysics=false;
    bool Valid=false;
};
inline bool ValidDamageParameters(DamageParameters p) noexcept
{
    return Finite(p.MinimumEnergyToDeformJ) &&
        p.MinimumEnergyToDeformJ>=0.0 &&
        Finite(p.MaximumSingleImpactEnergyJ) &&
        p.MaximumSingleImpactEnergyJ>
            p.MinimumEnergyToDeformJ &&
        Finite(p.DamageScalingEnergyJ) &&
        p.DamageScalingEnergyJ>0.0 &&
        Finite(p.EnginePowerLossPerDamage) &&
        p.EnginePowerLossPerDamage>=0.0 &&
        p.EnginePowerLossPerDamage<=1.0 &&
        Finite(p.GearboxEfficiencyLossPerDamage) &&
        p.GearboxEfficiencyLossPerDamage>=0.0 &&
        p.GearboxEfficiencyLossPerDamage<=1.0 &&
        Finite(p.SteeringRackRangeLossPerDamage) &&
        p.SteeringRackRangeLossPerDamage>=0.0 &&
        p.SteeringRackRangeLossPerDamage<=1.0 &&
        Finite(p.TireGripLossPerSuspensionDamage) &&
        p.TireGripLossPerSuspensionDamage>=0.0 &&
        p.TireGripLossPerSuspensionDamage<=1.0 &&
        Finite(p.BrakeEffectivenessLossPerDamage) &&
        p.BrakeEffectivenessLossPerDamage>=0.0 &&
        p.BrakeEffectivenessLossPerDamage<=1.0 &&
        Finite(p.CoolingLossPerDamage) &&
        p.CoolingLossPerDamage>=0.0 &&
        p.CoolingLossPerDamage<=1.0 &&
        Finite(p.MaximumDamageIncrementPerEvent) &&
        p.MaximumDamageIncrementPerEvent>0.0 &&
        p.MaximumDamageIncrementPerEvent<=1.0 &&
        Finite(p.MaximumLegalRepairFractionPerPit) &&
        p.MaximumLegalRepairFractionPerPit>=0.0 &&
        p.MaximumLegalRepairFractionPerPit<=1.0 &&
        Finite(p.MinimumComponentIntegrity) &&
        p.MinimumComponentIntegrity>=0.0 &&
        p.MinimumComponentIntegrity<=1.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaxSimulationFrameSeconds;
}
inline bool ValidDamageObservation(
    VehicleDamageObservation e,
    DamageParameters p) noexcept
{
    return PhysicalActorIdentityValid(e.Identity) &&
        e.ActualPhysicalCar &&
        e.OriginalWallOrCarBody &&
        (e.OriginalChaosContactVerified ||
         (e.RepairReceipt &&
          e.ActualPitServiceAuthorized)) &&
        // DamageZone's uint8_t backing is nonnegative by definition.
        // Still reject a deliberately cast unknown value >= Count.
        static_cast<int>(e.PrimaryZone)<DamageZoneCount &&
        ValidFrameDelta(e.DeltaSeconds) &&
        e.DeltaSeconds<=p.MaximumFrameSeconds &&
        Finite(e.MeasuredImpactEnergyJ) &&
        e.MeasuredImpactEnergyJ>=0.0 &&
        e.MeasuredImpactEnergyJ<=
            p.MaximumSingleImpactEnergyJ &&
        Finite(e.MeasuredRelativeSpeedCmS) &&
        e.MeasuredRelativeSpeedCmS>=0.0 &&
        Finite(e.MeasuredNormalImpulseNs) &&
        e.MeasuredNormalImpulseNs>=0.0 &&
        Finite(e.SurfaceScrapeEnergyJ) &&
        e.SurfaceScrapeEnergyJ>=0.0 &&
        Finite(e.MeasuredCurrentZoneIntegrity) &&
        e.MeasuredCurrentZoneIntegrity>=0.0 &&
        e.MeasuredCurrentZoneIntegrity<=1.0 &&
        Finite(e.EngineRaceSeconds) &&
        e.EngineRaceSeconds>=0.0;
}
inline double DamageSensitivityForZone(DamageZone zone) noexcept
{
    switch(zone)
    {
    case DamageZone::FrontLeft:
    case DamageZone::FrontRight:
    case DamageZone::RearLeft:
    case DamageZone::RearRight: return 1.4;
    case DamageZone::EngineBay:return 1.0;
    case DamageZone::SteeringRack:return 1.2;
    case DamageZone::Gearbox:return 1.15;
    case DamageZone::CoolingSystem:return 0.9;
    case DamageZone::FuelSystem:return 1.1;
    case DamageZone::AeroFront:
    case DamageZone::AeroRear:return 1.5;
    case DamageZone::Underbody:return 1.1;
    default:return 0.75;
    }
}
inline double DamageIncrementFromEnergy(
    double impactEnergyJ,DamageZone zone,
    DamageParameters p) noexcept
{
    if (!Finite(impactEnergyJ) ||
        impactEnergyJ<=p.MinimumEnergyToDeformJ)
        return 0.0;
    const double excess=impactEnergyJ-
        p.MinimumEnergyToDeformJ;
    return std::clamp(
        (1.0-std::exp(-excess/p.DamageScalingEnergyJ))*
        DamageSensitivityForZone(zone),
        0.0,p.MaximumDamageIncrementPerEvent);
}
inline VehicleDamageReport InitialVehicleDamage() noexcept
{
    VehicleDamageReport report;
    for (int i=0;i<DamageZoneCount;++i)
    {
        report.Zones[static_cast<std::size_t>(i)].Zone=
            static_cast<DamageZone>(i);
    }
    report.Valid=true;
    return report;
}
inline double DamageForZone(
    const VehicleDamageReport& report,
    DamageZone zone) noexcept
{
    return 1.0-report.Zones[
        static_cast<std::size_t>(zone)].Integrity;
}
inline void RecomputeMechanicalDamage(
    VehicleDamageReport& result,
    DamageParameters p) noexcept
{
    result.DamagedZones=0;
    result.CriticalZones=0;
    double sum=0.0;
    for (const DamageZoneStatus& zone:result.Zones)
    {
        const double damage=1.0-zone.Integrity;
        sum+=damage;
        result.DamagedZones+=damage>0.005?1:0;
        result.CriticalZones+=
            zone.Integrity<p.MinimumComponentIntegrity?1:0;
    }
    result.OverallIntegrityFraction=
        1.0-sum/static_cast<double>(DamageZoneCount);
    const double motor=DamageForZone(
        result,DamageZone::EngineBay);
    const double cooling=DamageForZone(
        result,DamageZone::CoolingSystem);
    const double gearbox=DamageForZone(
        result,DamageZone::Gearbox);
    const double steering=DamageForZone(
        result,DamageZone::SteeringRack);
    const double frontSusp=std::max(
        DamageForZone(result,DamageZone::FrontLeft),
        DamageForZone(result,DamageZone::FrontRight));
    const double rearSusp=std::max(
        DamageForZone(result,DamageZone::RearLeft),
        DamageForZone(result,DamageZone::RearRight));
    const double body=DamageForZone(
        result,DamageZone::Underbody);
    const double aero=std::max(
        DamageForZone(result,DamageZone::AeroFront),
        DamageForZone(result,DamageZone::AeroRear));
    const double brakeDamage=std::max(frontSusp,rearSusp);
    result.DrivePowerCeiling=Clamp01(
        (1.0-p.EnginePowerLossPerDamage*motor)*
        (1.0-p.GearboxEfficiencyLossPerDamage*gearbox));
    result.BrakeStrengthCeiling=Clamp01(
        1.0-p.BrakeEffectivenessLossPerDamage*brakeDamage);
    result.SteeringRangeCeiling=Clamp01(
        1.0-p.SteeringRackRangeLossPerDamage*steering);
    result.LateralGripCeiling=Clamp01(
        1.0-p.TireGripLossPerSuspensionDamage*
        std::max(frontSusp,rearSusp));
    result.AeroDownforceCeiling=Clamp01(
        1.0-std::max(
            p.AeroFrontDownforceLossPerDamage*
                DamageForZone(result,DamageZone::AeroFront),
            p.AeroRearDownforceLossPerDamage*
                DamageForZone(result,DamageZone::AeroRear)));
    result.FuelEfficiencyMultiplier=1.0+
        p.FuelConsumptionIncreasePerDamage*
        (motor+body)*0.5;
    result.CoolingCapacityFraction=Clamp01(
        1.0-p.CoolingLossPerDamage*cooling);
    result.EstimatedTopSpeedCeiling=
        MaximumPhysicalSpeedCmS*
        std::pow(std::max(0.05,
            result.DrivePowerCeiling),1.0/3.0);
    result.ChassisDamageFraction=
        (frontSusp+rearSusp+body+aero)*0.25;
    result.MechanicalDamageFraction=
        (motor+cooling+gearbox+steering)*0.25;
    result.UnsafeToContinue=
        result.CriticalZones>0 ||
        result.SteeringRangeCeiling<0.25 ||
        result.BrakeStrengthCeiling<0.25;
    result.MechanicalConsequencesHypothetical=true;
    result.ChangedActualVehiclePhysics=false;
}
inline VehicleDamageReport ApplyObservedDamage(
    VehicleDamageReport prior,
    VehicleDamageObservation sample,
    DamageParameters params={}) noexcept
{
    if (!prior.Valid ||
        !ValidDamageParameters(params) ||
        !ValidDamageObservation(sample,params))
    {
        ++prior.RejectedEvents;
        return prior;
    }
    DamageZoneStatus& zone=prior.Zones[
        static_cast<std::size_t>(sample.PrimaryZone)];
    if (sample.RepairReceipt)
    {
        if (!sample.ActualPitServiceAuthorized ||
            !sample.ChassisAtPhysicalPit)
        {
            ++prior.RejectedEvents;
            return prior;
        }
        zone.Integrity=std::min(1.0,
            zone.Integrity+
            params.MaximumLegalRepairFractionPerPit);
        ++prior.VerifiedPitRepairs;
    }
    else
    {
        const double estimate=DamageIncrementFromEnergy(
            sample.MeasuredImpactEnergyJ,
            sample.PrimaryZone,params);
        const double scrape=std::clamp(
            sample.SurfaceScrapeEnergyJ*
            params.ScrapeDamagePerJoule,0.0,0.1);
        zone.Integrity=std::max(
            0.0,zone.Integrity-estimate-scrape);
        zone.TotalAbsorbedEnergyJ+=
            sample.MeasuredImpactEnergyJ;
        zone.TotalScrapeEnergyJ+=
            sample.SurfaceScrapeEnergyJ;
        zone.LastImpactEnergyJ=
            sample.MeasuredImpactEnergyJ;
        zone.LastImpactClockSeconds=
            sample.EngineRaceSeconds;
        ++zone.VerifiedImpacts;
        ++prior.VerifiedImpacts;
        prior.TotalImpactEnergyJ+=
            sample.MeasuredImpactEnergyJ;
        prior.MaximumImpactEnergyJ=std::max(
            prior.MaximumImpactEnergyJ,
            sample.MeasuredImpactEnergyJ);
    }
    zone.Critical=
        zone.Integrity<params.MinimumComponentIntegrity;
    zone.PhysicallyObserved=true;
    RecomputeMechanicalDamage(prior,params);
    return prior;
}
class PhysicalDamageLedger final
{
public:
    bool Configure(DamageParameters params) noexcept
    {
        if (!ValidDamageParameters(params))return false;
        Params=params;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Report=InitialVehicleDamage();
        Session=0;
        Actor=0;
        LastClockSeconds=0.0;
        HasEpoch=false;
        IgnoredEvents=0;
    }
    bool Observe(VehicleDamageObservation event) noexcept
    {
        if (!ValidDamageObservation(event,Params))
        {
            ++IgnoredEvents;
            return false;
        }
        if (HasEpoch &&
            (event.Identity.Session!=Session ||
             event.Identity.Actor!=Actor ||
             event.EngineRaceSeconds<LastClockSeconds))
        {
            ++IgnoredEvents;
            return false;
        }
        HasEpoch=true;
        Session=event.Identity.Session;
        Actor=event.Identity.Actor;
        LastClockSeconds=event.EngineRaceSeconds;
        const int before=Report.VerifiedImpacts+
            Report.VerifiedPitRepairs;
        Report=ApplyObservedDamage(Report,event,Params);
        return Report.VerifiedImpacts+
            Report.VerifiedPitRepairs>before;
    }
    VehicleDamageReport Snapshot() const noexcept{return Report;}
    std::uint64_t RejectedEvents() const noexcept
    {return IgnoredEvents;}
private:
    DamageParameters Params{};
    VehicleDamageReport Report=InitialVehicleDamage();
    std::uint64_t Session=0;
    std::uint64_t Actor=0;
    std::uint64_t IgnoredEvents=0;
    double LastClockSeconds=0.0;
    bool HasEpoch=false;
};
} // namespace latebrake::racecraft
