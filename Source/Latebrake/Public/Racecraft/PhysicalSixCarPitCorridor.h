#pragma once
#include "Racing/PhysicalSixCarReplayTimeline.h"
#include "Track/WorldScaleCircuitVariants.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace latebrake {
// Original six-circuit PIT ACCESS CLEARANCE observation. This is not an
// authorized pit arrival, service request or collision avoidance controller.
// Native Unreal Chaos road contacts and six vehicle identities must be
// independently verified; no projected car can award a checkpoint or lap.
inline constexpr unsigned PitCorridorCarCount=PhysicalReplayCarCount;
inline constexpr double PitCorridorMaximumSourceLateralCm=2000.0;
enum class PitCorridorFault : std::uint8_t {
    None, InvalidPolicy, BadRoad, BadSource, MixedSession,
    WrongActor, UnqualifiedProjection, ImpossibleMotion,
    UnsafeTraffic, InvalidPitStation
};
enum class PitCorridorTraffic : std::uint8_t {
    Unknown, Vacant, WatchApproach, Blocked, YieldToCrossTraffic
};
struct PitCorridorSettings final {
    double PitLaneHalfWidthCm=450.0;
    double PitEntryBufferCm=850.0;
    double MinimumVehicleLongCm=240.0;
    double MaximumVehicleLongCm=620.0;
    double MinimumVehicleWideCm=135.0;
    double MaximumVehicleWideCm=320.0;
    double MaximumLateralCenterCm=PitCorridorMaximumSourceLateralCm;
    double MaximumMeasuredSpeedCmps=PhysicalReplayMaximumSpeedCmps;
    double MinimumTimeToConflictSeconds=0.1;
    double MaximumPredictSeconds=4.0;
    double MaximumClockSkewSeconds=0.05;
    double MaximumPhysicalPositionJumpCm=175.0;
    double MaximumSourceLateralDriftCm=250.0;
    double ServiceEntryHoldSpeedCmps=120.0;
    double SafetyLongitudinalBufferCm=350.0;
    double SafetyLateralBufferCm=80.0;
    bool NeedActualNativeRoadProjection=true;
    bool NeedRealSixCarChaosRoster=true;
    bool NeverIssuePitService=true;
    bool NeverChangeVehicleCollision=true;
    bool NeverAwardRaceCredits=true;
};
struct PitCorridorPhysicalSource final {
    std::uint64_t Actor=0u;
    std::uint64_t SourceFrame=0u;
    double ClockSeconds=0.0;
    double StationCm=0.0;
    double SignedLateralCm=0.0;
    double LongitudinalSpeedCmps=0.0;
    double MeasuredWidthCm=180.0;
    double MeasuredLengthCm=450.0;
    double SensorConfidence=0.0;
    bool RealWorldStaticContact=true;
    bool NativeRoadStationObserved=false;
    bool NativeLateralOffsetObserved=false;
    bool RealVelocityObserved=false;
    bool OriginalRaceAuthorityIntact=true;
};
struct PitCorridorCarProjection final {
    bool Valid=false;
    std::uint64_t Actor=0u;
    double WrappedStationCm=0.0;
    double SignedLateralCm=0.0;
    double DistanceToPitAheadCm=0.0;
    double NativeLongitudinalSpeedCmps=0.0;
    double HalfWidthCm=0.0;
    double HalfLengthCm=0.0;
    double SeparationFromPitCenterCm=0.0;
    double EstimatedArrivalSeconds=0.0;
    bool CouldEnterService=false;
    bool ActualPitEntryNotGranted=true;
};
struct PitCorridorTrafficFrame final {
    bool Valid=false;
    PitCorridorFault Fault=PitCorridorFault::BadSource;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t SourceFrame=0u;
    std::uint64_t DriverActor=0u;
    double SourceClockSeconds=0.0;
    double PitStationCm=0.0;
    double MinimumRivalDistanceCm=0.0;
    double MinimumPredictedConflictSeconds=0.0;
    unsigned QualifiedCars=0u;
    unsigned TrafficWarnings=0u;
    PitCorridorTraffic Traffic=PitCorridorTraffic::Unknown;
    std::array<PitCorridorCarProjection,PitCorridorCarCount> Cars{};
    bool NoNativePitServiceIssued=true;
    bool OriginalRaceTimingUnchanged=true;
};
inline bool ValidPitCorridorSettings(PitCorridorSettings p) noexcept {
    return std::isfinite(p.PitLaneHalfWidthCm) &&
        p.PitLaneHalfWidthCm>=250.0 &&
        p.PitLaneHalfWidthCm<=650.0 &&
        std::isfinite(p.PitEntryBufferCm) &&
        p.PitEntryBufferCm>=450.0 &&
        p.PitEntryBufferCm<=1300.0 &&
        std::isfinite(p.MinimumVehicleLongCm) &&
        p.MinimumVehicleLongCm>=200.0 &&
        p.MinimumVehicleLongCm<=350.0 &&
        std::isfinite(p.MaximumVehicleLongCm) &&
        p.MaximumVehicleLongCm>=500.0 &&
        p.MaximumVehicleLongCm<=800.0 &&
        std::isfinite(p.MinimumVehicleWideCm) &&
        p.MinimumVehicleWideCm>=100.0 &&
        p.MinimumVehicleWideCm<=150.0 &&
        std::isfinite(p.MaximumVehicleWideCm) &&
        p.MaximumVehicleWideCm>=300.0 &&
        p.MaximumVehicleWideCm<=450.0 &&
        std::isfinite(p.MaximumLateralCenterCm) &&
        p.MaximumLateralCenterCm>=1500.0 &&
        p.MaximumLateralCenterCm<=3000.0 &&
        std::isfinite(p.MaximumMeasuredSpeedCmps) &&
        p.MaximumMeasuredSpeedCmps>=5000.0 &&
        p.MaximumMeasuredSpeedCmps<=15000.0 &&
        std::isfinite(p.MinimumTimeToConflictSeconds) &&
        p.MinimumTimeToConflictSeconds>=0.05 &&
        p.MinimumTimeToConflictSeconds<=0.5 &&
        std::isfinite(p.MaximumPredictSeconds) &&
        p.MaximumPredictSeconds>=1.0 &&
        p.MaximumPredictSeconds<=5.0 &&
        std::isfinite(p.MaximumClockSkewSeconds) &&
        p.MaximumClockSkewSeconds>=0.0 &&
        p.MaximumClockSkewSeconds<=0.1 &&
        std::isfinite(p.MaximumPhysicalPositionJumpCm) &&
        p.MaximumPhysicalPositionJumpCm>=100.0 &&
        p.MaximumPhysicalPositionJumpCm<=300.0 &&
        std::isfinite(p.MaximumSourceLateralDriftCm) &&
        p.MaximumSourceLateralDriftCm>=100.0 &&
        p.MaximumSourceLateralDriftCm<=350.0 &&
        std::isfinite(p.ServiceEntryHoldSpeedCmps) &&
        p.ServiceEntryHoldSpeedCmps>=30.0 &&
        p.ServiceEntryHoldSpeedCmps<=150.0 &&
        std::isfinite(p.SafetyLongitudinalBufferCm) &&
        p.SafetyLongitudinalBufferCm>=250.0 &&
        p.SafetyLongitudinalBufferCm<=500.0 &&
        std::isfinite(p.SafetyLateralBufferCm) &&
        p.SafetyLateralBufferCm>=50.0 &&
        p.SafetyLateralBufferCm<=140.0 &&
        p.NeedActualNativeRoadProjection &&
        p.NeedRealSixCarChaosRoster &&
        p.NeverIssuePitService &&
        p.NeverChangeVehicleCollision &&
        p.NeverAwardRaceCredits;
}
inline bool ValidPitCorridorSource(
    const WorldScaleVariantRecipe& circuit,
    const PhysicalReplayFrame& fleet,
    const std::array<PitCorridorPhysicalSource,
        PitCorridorCarCount>& observed,
    PitCorridorSettings settings={}) noexcept
{
    if(!ValidPitCorridorSettings(settings) ||
       !ValidWorldScaleVariantRecipe(circuit) ||
       ValidatePhysicalReplayFrame(fleet)!=PhysicalReplayReject::None)
        return false;
    for(unsigned i=0u;i<PitCorridorCarCount;++i) {
        const auto& car=fleet.Cars[i];
        const auto& p=observed[i];
        if(p.Actor!=car.Actor ||
           p.SourceFrame!=fleet.SourceFrame ||
           !std::isfinite(p.ClockSeconds) ||
           std::abs(p.ClockSeconds-fleet.ActiveClockSeconds)>
               settings.MaximumClockSkewSeconds ||
           !std::isfinite(p.StationCm) ||
           !std::isfinite(p.SignedLateralCm) ||
           std::abs(p.StationCm)>1e9 ||
           std::abs(p.SignedLateralCm)>settings.MaximumLateralCenterCm ||
           !std::isfinite(p.LongitudinalSpeedCmps) ||
           std::abs(p.LongitudinalSpeedCmps)>
               settings.MaximumMeasuredSpeedCmps ||
           !std::isfinite(p.MeasuredWidthCm) ||
           p.MeasuredWidthCm<settings.MinimumVehicleWideCm ||
           p.MeasuredWidthCm>settings.MaximumVehicleWideCm ||
           !std::isfinite(p.MeasuredLengthCm) ||
           p.MeasuredLengthCm<settings.MinimumVehicleLongCm ||
           p.MeasuredLengthCm>settings.MaximumVehicleLongCm ||
           !std::isfinite(p.SensorConfidence) ||
           p.SensorConfidence<0.85 || p.SensorConfidence>1.0 ||
           !p.RealWorldStaticContact ||
           !p.NativeRoadStationObserved ||
           !p.NativeLateralOffsetObserved ||
           !p.RealVelocityObserved ||
           !p.OriginalRaceAuthorityIntact)
            return false;
    }
    return true;
}
inline double PitCorridorForwardGap(
    const WorldScaleVariantRecipe& circuit,
    double sourceStation,double targetStation) noexcept
{
    const double from=WorldScaleVariantWrapDistance(
        circuit,sourceStation);
    const double to=WorldScaleVariantWrapDistance(
        circuit,targetStation);
    if(!std::isfinite(from) ||
       !std::isfinite(to) ||
       !std::isfinite(circuit.LapLengthCm) ||
       circuit.LapLengthCm<=0.0)
        return std::numeric_limits<double>::infinity();
    double gap=to-from;
    if(gap<0.0)gap+=circuit.LapLengthCm;
    return gap;
}
inline PitCorridorCarProjection ProjectPitCorridorCar(
    const WorldScaleVariantRecipe& circuit,
    PitCorridorPhysicalSource source,
    double pitStation,PitCorridorSettings settings={}) noexcept
{
    PitCorridorCarProjection out;
    if(!ValidPitCorridorSettings(settings) ||
       !ValidWorldScaleVariantRecipe(circuit) ||
       !std::isfinite(pitStation) ||
       std::abs(pitStation)>1e9 ||
       !std::isfinite(source.StationCm) ||
       !std::isfinite(source.SignedLateralCm) ||
       !std::isfinite(source.LongitudinalSpeedCmps) ||
       source.Actor==0u)
        return out;
    const double gap=PitCorridorForwardGap(
        circuit,source.StationCm,pitStation);
    if(!std::isfinite(gap))return out;
    const double halfLength=source.MeasuredLengthCm*0.5;
    const double halfWidth=source.MeasuredWidthCm*0.5;
    const double roadDistance=std::abs(source.SignedLateralCm);
    const double lateralClearance=roadDistance-halfWidth-
        settings.PitLaneHalfWidthCm;
    const double speed=source.LongitudinalSpeedCmps;
    const double arrival=speed>1.0 ?
        std::max(0.0,gap-halfLength)/speed:
        std::numeric_limits<double>::infinity();
    out.Valid=true;
    out.Actor=source.Actor;
    out.WrappedStationCm=WorldScaleVariantWrapDistance(
        circuit,source.StationCm);
    out.SignedLateralCm=source.SignedLateralCm;
    out.DistanceToPitAheadCm=gap;
    out.NativeLongitudinalSpeedCmps=speed;
    out.HalfWidthCm=halfWidth;
    out.HalfLengthCm=halfLength;
    out.SeparationFromPitCenterCm=lateralClearance;
    out.EstimatedArrivalSeconds=arrival;
    out.CouldEnterService=gap<=
        settings.PitEntryBufferCm+halfLength &&
        roadDistance<=settings.PitLaneHalfWidthCm &&
        std::abs(speed)<=settings.ServiceEntryHoldSpeedCmps;
    return out;
}
inline PitCorridorTrafficFrame AssessSixCarPitCorridor(
    const WorldScaleVariantRecipe& circuit,
    const PhysicalReplayFrame& fleet,
    const std::array<PitCorridorPhysicalSource,
        PitCorridorCarCount>& observed,
    std::uint64_t driverActor,double pitStation,
    PitCorridorSettings settings={}) noexcept
{
    PitCorridorTrafficFrame out;
    if(!ValidPitCorridorSettings(settings)){
        out.Fault=PitCorridorFault::InvalidPolicy;
        return out;
    }
    if(!ValidWorldScaleVariantRecipe(circuit)){
        out.Fault=PitCorridorFault::BadRoad;
        return out;
    }
    if(!ValidPitCorridorSource(
        circuit,fleet,observed,settings) ||
       driverActor==0u || !std::isfinite(pitStation) ||
       std::abs(pitStation)>1e9){
        out.Fault=PitCorridorFault::BadSource;
        return out;
    }
    out.Circuit=circuit.Id;
    out.Session=fleet.Session;
    out.Revision=fleet.SessionRevision;
    out.SourceFrame=fleet.SourceFrame;
    out.DriverActor=driverActor;
    out.SourceClockSeconds=fleet.ActiveClockSeconds;
    out.PitStationCm=WorldScaleVariantWrapDistance(
        circuit,pitStation);
    out.MinimumRivalDistanceCm=
        std::numeric_limits<double>::infinity();
    out.MinimumPredictedConflictSeconds=
        std::numeric_limits<double>::infinity();
    bool haveDriver=false;
    out.Traffic=PitCorridorTraffic::Vacant;
    for(unsigned i=0u;i<PitCorridorCarCount;++i){
        const auto projection=ProjectPitCorridorCar(
            circuit,observed[i],pitStation,settings);
        if(!projection.Valid)return {};
        out.Cars[i]=projection;
        ++out.QualifiedCars;
        if(projection.Actor==driverActor){
            haveDriver=true;
            continue;
        }
        // Three separate constraints: lateral footprint, longitudinal
        // gap to the pit and bounded time-to-conflict from actual speed.
        // No global/absolute vehicle teleport is inferred.
        const double lateral=
            std::abs(projection.SignedLateralCm);
        const bool laneRelevant=lateral<=
            settings.PitLaneHalfWidthCm+
            settings.SafetyLateralBufferCm+
            projection.HalfWidthCm;
        const double forward=projection.DistanceToPitAheadCm;
        const double backward=circuit.LapLengthCm-forward;
        const double closest=std::min(forward,backward);
        if(laneRelevant){
            out.MinimumRivalDistanceCm=std::min(
                out.MinimumRivalDistanceCm,closest);
            if(closest<=settings.PitEntryBufferCm+
                settings.SafetyLongitudinalBufferCm+
                projection.HalfLengthCm){
                out.Traffic=PitCorridorTraffic::Blocked;
                ++out.TrafficWarnings;
            }else if(projection.EstimatedArrivalSeconds>=
                         settings.MinimumTimeToConflictSeconds &&
                     projection.EstimatedArrivalSeconds<=
                         settings.MaximumPredictSeconds){
                out.MinimumPredictedConflictSeconds=std::min(
                    out.MinimumPredictedConflictSeconds,
                    projection.EstimatedArrivalSeconds);
                if(out.Traffic!=PitCorridorTraffic::Blocked)
                    out.Traffic=PitCorridorTraffic::WatchApproach;
                ++out.TrafficWarnings;
            }
        }
    }
    if(!haveDriver){
        out.Fault=PitCorridorFault::WrongActor;
        return out;
    }
    out.Valid=true;
    out.Fault=PitCorridorFault::None;
    return out;
}
} // namespace latebrake
