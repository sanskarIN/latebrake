#pragma once
#include "Garage/PhysicalTyreStintTrend.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace latebrake {
// Original six-car, physically observed FOUR-TIRE service forecast.
// Decisions are only garage/pit preparation hints: no scheduled pit is
// actually reserved, no wheel is swapped, no career credits are spent,
// and no official game result or Chaos wheel force is ever touched.
enum class PhysicalTyreServiceUrgency : std::uint8_t {
    Unverified, None, Monitor, ServiceOpportunity, ImmediateReview
};
enum class PhysicalTyreServiceReason : std::uint8_t {
    None, TreadNearLimit, AcceleratedWear, OverheatedCarcass,
    PressureInstability, ExcessAxleImbalance, InsufficientEvidence
};
struct PhysicalTyreServiceSettings final {
    double MinimumTreadToContinueMm=1.6;
    double TargetTreadReserveMm=2.8;
    double WarningTreadDepthMm=2.6;
    double MaximumPredictedWearMmPerLap=0.8;
    double HighWearRateMmPerSecond=0.045;
    double MaximumRecommendedHorizonLaps=6.0;
    double MinimumQualifiedObservationSeconds=0.1;
    double MaximumQualifiedObservationSeconds=30.0;
    double MinimumPaceLapSeconds=15.0;
    double MaximumPaceLapSeconds=600.0;
    double MinimumMeasuredTemperatureC=0.0;
    double MaximumMeasuredTemperatureC=180.0;
    double MaximumSafeCarcassTemperatureC=130.0;
    double MaximumPressureGainKpaPerSecond=9.0;
    double MaximumAxleTemperatureSpreadC=28.0;
    double ServiceStopCostSeconds=22.0;
    double TireChangeSeconds=9.0;
    double ConservativePitEntryLossSeconds=7.0;
    double ConservativePitExitLossSeconds=8.0;
    double MaximumSafeRefuelWhileChangingSeconds=15.0;
    unsigned MinimumSourceFrames=3u;
    unsigned MaximumRemainingOfficialLaps=80u;
    bool RequireNativePitFacilityEvidence=true;
    bool RequireOriginalTyreServiceCapability=true;
    bool NeverBookPitOrApplyRealRepair=true;
    bool PreserveOfficialRaceTiming=true;
    bool NeverAwardGarageCredits=true;
};
struct PhysicalTyreServiceRaceWitness final {
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t SourceFrame=0u;
    std::uint64_t DriverActor=0u;
    std::uint64_t NativePitFacility=0u;
    double ClockSeconds=0.0;
    double OfficialLapSeconds=0.0;
    double MeasuredFuelReserveLiters=0.0;
    unsigned OfficialRemainingLaps=0u;
    bool OriginalNativePitBoxVerified=false;
    bool ActualWheelCrewAvailable=false;
    bool ActualPitSpeedLimitVerified=false;
    bool OriginalRaceClockUntouched=true;
};
struct PhysicalTyreServiceWheelForecast final {
    bool Valid=false;
    std::uint64_t NativeWheel=0u;
    unsigned WheelIndex=0u;
    double LatestTreadMm=0.0;
    double LatestCarcassC=0.0;
    double LatestPressureKpa=0.0;
    double MeasuredWearMmPerSecond=0.0;
    double PredictedWearMmPerLap=0.0;
    double LapsUntilTreadReserve=0.0;
    double EstimatedEndOfStintTreadMm=0.0;
    PhysicalTyreServiceUrgency Urgency=
        PhysicalTyreServiceUrgency::Unverified;
    PhysicalTyreServiceReason Reason=
        PhysicalTyreServiceReason::InsufficientEvidence;
    bool OriginalWheelUnmodified=true;
};
struct PhysicalTyreServicePlan final {
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t SourceFrame=0u;
    std::uint64_t DriverActor=0u;
    double SourceClockSeconds=0.0;
    unsigned OfficialRemainingLaps=0u;
    std::array<PhysicalTyreServiceWheelForecast,
        TyreSetupWheelCount> Wheels{};
    PhysicalTyreServiceUrgency FleetUrgency=
        PhysicalTyreServiceUrgency::Unverified;
    double ShortestTreadReserveLaps=0.0;
    double LongestExpectedTireServiceSeconds=0.0;
    double ConservativePitLossSeconds=0.0;
    double EarliestSourceReviewLap=0.0;
    double ProposedServiceWindowLap=0.0;
    unsigned ServiceRecommendedWheels=0u;
    unsigned HotWheels=0u;
    unsigned WornWheels=0u;
    bool PitReservationSubmitted=false;
    bool UnrealTyreForceChanged=false;
    bool CareerCreditsSpent=false;
    bool OfficialTimeUnaffected=true;
};
inline bool ValidPhysicalTyreServiceSettings(
    PhysicalTyreServiceSettings s) noexcept
{
    const auto finite=[](double x) noexcept {
        return std::isfinite(x);
    };
    return finite(s.MinimumTreadToContinueMm) &&
        s.MinimumTreadToContinueMm>=1.0 &&
        s.MinimumTreadToContinueMm<=2.0 &&
        finite(s.TargetTreadReserveMm) &&
        s.TargetTreadReserveMm>s.MinimumTreadToContinueMm &&
        s.TargetTreadReserveMm<=4.0 &&
        finite(s.WarningTreadDepthMm) &&
        s.WarningTreadDepthMm>=s.MinimumTreadToContinueMm &&
        s.WarningTreadDepthMm<=s.TargetTreadReserveMm &&
        finite(s.MaximumPredictedWearMmPerLap) &&
        s.MaximumPredictedWearMmPerLap>=0.2 &&
        s.MaximumPredictedWearMmPerLap<=2.0 &&
        finite(s.HighWearRateMmPerSecond) &&
        s.HighWearRateMmPerSecond>=0.01 &&
        s.HighWearRateMmPerSecond<=0.10 &&
        finite(s.MaximumRecommendedHorizonLaps) &&
        s.MaximumRecommendedHorizonLaps>=2.0 &&
        s.MaximumRecommendedHorizonLaps<=15.0 &&
        finite(s.MinimumQualifiedObservationSeconds) &&
        s.MinimumQualifiedObservationSeconds>=0.05 &&
        s.MinimumQualifiedObservationSeconds<=0.5 &&
        finite(s.MaximumQualifiedObservationSeconds) &&
        s.MaximumQualifiedObservationSeconds>=10.0 &&
        s.MaximumQualifiedObservationSeconds<=60.0 &&
        finite(s.MinimumPaceLapSeconds) &&
        s.MinimumPaceLapSeconds>=10.0 &&
        s.MinimumPaceLapSeconds<=30.0 &&
        finite(s.MaximumPaceLapSeconds) &&
        s.MaximumPaceLapSeconds>=300.0 &&
        s.MaximumPaceLapSeconds<=900.0 &&
        finite(s.MinimumMeasuredTemperatureC) &&
        finite(s.MaximumMeasuredTemperatureC) &&
        s.MaximumMeasuredTemperatureC>
            s.MinimumMeasuredTemperatureC &&
        finite(s.MaximumSafeCarcassTemperatureC) &&
        s.MaximumSafeCarcassTemperatureC>=100.0 &&
        s.MaximumSafeCarcassTemperatureC<=150.0 &&
        finite(s.MaximumPressureGainKpaPerSecond) &&
        s.MaximumPressureGainKpaPerSecond>=2.0 &&
        s.MaximumPressureGainKpaPerSecond<=15.0 &&
        finite(s.MaximumAxleTemperatureSpreadC) &&
        s.MaximumAxleTemperatureSpreadC>=10.0 &&
        s.MaximumAxleTemperatureSpreadC<=50.0 &&
        finite(s.ServiceStopCostSeconds) &&
        s.ServiceStopCostSeconds>=5.0 &&
        s.ServiceStopCostSeconds<=60.0 &&
        finite(s.TireChangeSeconds) &&
        s.TireChangeSeconds>=2.0 &&
        s.TireChangeSeconds<=20.0 &&
        finite(s.ConservativePitEntryLossSeconds) &&
        s.ConservativePitEntryLossSeconds>=2.0 &&
        s.ConservativePitEntryLossSeconds<=20.0 &&
        finite(s.ConservativePitExitLossSeconds) &&
        s.ConservativePitExitLossSeconds>=2.0 &&
        s.ConservativePitExitLossSeconds<=20.0 &&
        finite(s.MaximumSafeRefuelWhileChangingSeconds) &&
        s.MaximumSafeRefuelWhileChangingSeconds>=5.0 &&
        s.MaximumSafeRefuelWhileChangingSeconds<=20.0 &&
        s.MinimumSourceFrames>=2u &&
        s.MinimumSourceFrames<=20u &&
        s.MaximumRemainingOfficialLaps>=10u &&
        s.MaximumRemainingOfficialLaps<=100u &&
        s.RequireNativePitFacilityEvidence &&
        s.RequireOriginalTyreServiceCapability &&
        s.NeverBookPitOrApplyRealRepair &&
        s.PreserveOfficialRaceTiming &&
        s.NeverAwardGarageCredits;
}
inline bool ValidPhysicalTyreServiceRaceWitness(
    PhysicalTyreServiceRaceWitness witness,
    const PhysicalTyreTrendSummary& source,
    PhysicalTyreServiceSettings settings) noexcept
{
    if(!source.Valid ||
       source.Session==0u ||
       source.Revision==0u ||
       source.OwnerActor==0u ||
       witness.Session!=source.Session ||
       witness.Revision!=source.Revision ||
       witness.DriverActor!=source.OwnerActor ||
       witness.SourceFrame!=source.LastFrame ||
       witness.NativePitFacility==0u ||
       !witness.OriginalNativePitBoxVerified ||
       !witness.ActualWheelCrewAvailable ||
       !witness.ActualPitSpeedLimitVerified ||
       !witness.OriginalRaceClockUntouched ||
       !std::isfinite(witness.ClockSeconds) ||
       !std::isfinite(witness.OfficialLapSeconds) ||
       !std::isfinite(witness.MeasuredFuelReserveLiters) ||
       witness.OfficialLapSeconds<settings.MinimumPaceLapSeconds ||
       witness.OfficialLapSeconds>settings.MaximumPaceLapSeconds ||
       witness.OfficialRemainingLaps>
           settings.MaximumRemainingOfficialLaps ||
       witness.ClockSeconds<=0.0 ||
       witness.MeasuredFuelReserveLiters<0.0 ||
       witness.MeasuredFuelReserveLiters>200.0 ||
       std::abs(witness.ClockSeconds-source.LastClockSeconds)>0.05)
        return false;
    return true;
}
inline PhysicalTyreServiceWheelForecast ForecastPhysicalTyreServiceWheel(
    const PhysicalTyreTrendWheel& trend,
    unsigned remainingLaps,
    double officialLapSeconds,
    PhysicalTyreServiceSettings settings={}) noexcept
{
    PhysicalTyreServiceWheelForecast out;
    if(!ValidPhysicalTyreServiceSettings(settings) ||
       !trend.Valid || trend.NativeWheel==0u ||
       !std::isfinite(officialLapSeconds) ||
       officialLapSeconds<settings.MinimumPaceLapSeconds ||
       officialLapSeconds>settings.MaximumPaceLapSeconds ||
       !std::isfinite(trend.LatestTreadMm) ||
       !std::isfinite(trend.LatestCarcassTemperatureC) ||
       !std::isfinite(trend.LatestPressureKpa) ||
       !std::isfinite(trend.TreadWearMmPerSecond) ||
       !std::isfinite(trend.PressureGainKpaPerSecond) ||
       trend.TreadWearMmPerSecond<0.0)
        return out;
    const double wearLap=std::min(
        settings.MaximumPredictedWearMmPerLap,
        trend.TreadWearMmPerSecond*officialLapSeconds);
    const double remaining=std::max(
        0.0,trend.LatestTreadMm-
            settings.TargetTreadReserveMm);
    const double reserveLaps=wearLap<=1e-9 ?
        settings.MaximumRecommendedHorizonLaps :
        remaining/wearLap;
    const double projected=std::max(
        0.0,trend.LatestTreadMm-
            wearLap*remainingLaps);
    out.Valid=true;
    out.NativeWheel=trend.NativeWheel;
    out.WheelIndex=trend.WheelIndex;
    out.LatestTreadMm=trend.LatestTreadMm;
    out.LatestCarcassC=trend.LatestCarcassTemperatureC;
    out.LatestPressureKpa=trend.LatestPressureKpa;
    out.MeasuredWearMmPerSecond=trend.TreadWearMmPerSecond;
    out.PredictedWearMmPerLap=wearLap;
    out.LapsUntilTreadReserve=std::clamp(
        reserveLaps,0.0,settings.MaximumRecommendedHorizonLaps);
    out.EstimatedEndOfStintTreadMm=projected;
    out.Urgency=PhysicalTyreServiceUrgency::None;
    out.Reason=PhysicalTyreServiceReason::None;
    if(trend.LatestTreadMm<=settings.MinimumTreadToContinueMm){
        out.Urgency=PhysicalTyreServiceUrgency::ImmediateReview;
        out.Reason=PhysicalTyreServiceReason::TreadNearLimit;
    }else if(trend.LatestCarcassTemperatureC>=
               settings.MaximumSafeCarcassTemperatureC){
        out.Urgency=PhysicalTyreServiceUrgency::ImmediateReview;
        out.Reason=PhysicalTyreServiceReason::OverheatedCarcass;
    }else if(trend.LatestTreadMm<=settings.WarningTreadDepthMm ||
             out.LapsUntilTreadReserve<1.0){
        out.Urgency=PhysicalTyreServiceUrgency::ServiceOpportunity;
        out.Reason=PhysicalTyreServiceReason::TreadNearLimit;
    }else if(trend.TreadWearMmPerSecond>=
               settings.HighWearRateMmPerSecond){
        out.Urgency=PhysicalTyreServiceUrgency::ServiceOpportunity;
        out.Reason=PhysicalTyreServiceReason::AcceleratedWear;
    }else if(trend.PressureGainKpaPerSecond>=
               settings.MaximumPressureGainKpaPerSecond){
        out.Urgency=PhysicalTyreServiceUrgency::Monitor;
        out.Reason=PhysicalTyreServiceReason::PressureInstability;
    }else if(out.LapsUntilTreadReserve<
               static_cast<double>(remainingLaps)){
        out.Urgency=PhysicalTyreServiceUrgency::Monitor;
        out.Reason=PhysicalTyreServiceReason::AcceleratedWear;
    }
    return out;
}
inline PhysicalTyreServicePlan PlanPhysicalTyreService(
    const PhysicalTyreTrendSummary& source,
    PhysicalTyreServiceRaceWitness race,
    PhysicalTyreServiceSettings settings={}) noexcept
{
    PhysicalTyreServicePlan out;
    if(!ValidPhysicalTyreServiceSettings(settings) ||
       !ValidPhysicalTyreServiceRaceWitness(race,source,settings) ||
       source.StoredSourceFrames<settings.MinimumSourceFrames ||
       source.ObservedDurationSeconds<
           settings.MinimumQualifiedObservationSeconds ||
       source.ObservedDurationSeconds>
           settings.MaximumQualifiedObservationSeconds)
        return out;
    out.Circuit=WorldScaleVariantId::OriginalLoop;
    out.Session=race.Session;
    out.SourceFrame=race.SourceFrame;
    out.DriverActor=race.DriverActor;
    out.SourceClockSeconds=race.ClockSeconds;
    out.OfficialRemainingLaps=race.OfficialRemainingLaps;
    out.ShortestTreadReserveLaps=
        settings.MaximumRecommendedHorizonLaps;
    out.FleetUrgency=PhysicalTyreServiceUrgency::None;
    for(unsigned i=0u;i<TyreSetupWheelCount;++i){
        const auto& t=source.Wheels[i];
        const auto computed=ForecastPhysicalTyreServiceWheel(
            t,race.OfficialRemainingLaps,
            race.OfficialLapSeconds,settings);
        if(!computed.Valid)return {};
        for(unsigned j=0u;j<i;++j)
            if(computed.NativeWheel==
               out.Wheels[j].NativeWheel)
                return {};
        out.Wheels[i]=computed;
        out.ShortestTreadReserveLaps=std::min(
            out.ShortestTreadReserveLaps,
            computed.LapsUntilTreadReserve);
        if(static_cast<unsigned>(computed.Urgency)>
           static_cast<unsigned>(out.FleetUrgency))
            out.FleetUrgency=computed.Urgency;
        if(computed.Urgency>=
           PhysicalTyreServiceUrgency::ServiceOpportunity)
            ++out.ServiceRecommendedWheels;
        if(computed.LatestCarcassC>=
           settings.MaximumSafeCarcassTemperatureC)
            ++out.HotWheels;
        if(computed.LatestTreadMm<=
           settings.WarningTreadDepthMm)
            ++out.WornWheels;
    }
    const double frontDifference=std::abs(
        source.Wheels[0].LatestCarcassTemperatureC-
        source.Wheels[1].LatestCarcassTemperatureC);
    const double rearDifference=std::abs(
        source.Wheels[2].LatestCarcassTemperatureC-
        source.Wheels[3].LatestCarcassTemperatureC);
    if(std::max(frontDifference,rearDifference)>
       settings.MaximumAxleTemperatureSpreadC &&
       out.FleetUrgency<
           PhysicalTyreServiceUrgency::ServiceOpportunity)
        out.FleetUrgency=PhysicalTyreServiceUrgency::Monitor;
    out.EarliestSourceReviewLap=std::max(
        0.0,out.ShortestTreadReserveLaps-1.0);
    out.ProposedServiceWindowLap=std::min(
        out.ShortestTreadReserveLaps,
        static_cast<double>(race.OfficialRemainingLaps));
    out.LongestExpectedTireServiceSeconds=
        settings.TireChangeSeconds*
        (out.ServiceRecommendedWheels>0u ? 1.0:0.0);
    out.ConservativePitLossSeconds=
        settings.ServiceStopCostSeconds+
        settings.ConservativePitEntryLossSeconds+
        settings.ConservativePitExitLossSeconds+
        out.LongestExpectedTireServiceSeconds;
    out.Valid=out.FleetUrgency!=
        PhysicalTyreServiceUrgency::Unverified &&
        std::isfinite(out.ConservativePitLossSeconds) &&
        std::isfinite(out.ShortestTreadReserveLaps) &&
        out.WornWheels<=TyreSetupWheelCount;
    return out;
}
} // namespace latebrake
