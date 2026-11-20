#include "Racecraft/PhysicalSixCarPitCorridor.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Check(bool value,const char* label){
    if(!value){std::cerr<<"PhysicalSixCarPitCorridor FAIL: "<<label<<'\n';std::exit(EXIT_FAILURE);}
}
latebrake::PhysicalReplayFrame Fleet(){
    using namespace latebrake;
    PhysicalReplayFrame frame;
    frame.Session=77u;
    frame.SessionRevision=5u;
    frame.SourceFrame=4u;
    frame.ActiveClockSeconds=6.0;
    frame.SourceDeltaSeconds=0.05;
    frame.AllSixEngineActorsObserved=true;
    for(unsigned i=0u;i<PitCorridorCarCount;++i){
        auto& car=frame.Cars[i];
        car.Actor=800u+i;
        car.Session=77u;
        car.SessionRevision=5u;
        car.SourceFrame=4u;
        car.WorldXcm=1000.0+2000.0*i;
        car.WorldYcm=1000.0+600.0*i;
        car.WorldZcm=90.0;
        car.VelocityXcmps=500.0;
        car.HeadingRadians=0.0;
        car.RoadContactConfidence=1.0;
        car.ActualChaosBodyObserved=true;
        car.OriginalRoadPhysicalContact=true;
        car.EngineClockRunning=true;
    }
    return frame;
}
std::array<latebrake::PitCorridorPhysicalSource,
    latebrake::PitCorridorCarCount> Project(
    const latebrake::WorldScaleVariantRecipe& road,
    double pit)
{
    using namespace latebrake;
    std::array<PitCorridorPhysicalSource,
        PitCorridorCarCount> values{};
    for(unsigned i=0u;i<PitCorridorCarCount;++i){
        auto& source=values[i];
        source.Actor=800u+i;
        source.SourceFrame=4u;
        source.ClockSeconds=6.0;
        source.StationCm=pit-8000.0-i*1000.0;
        source.SignedLateralCm=850.0;
        source.LongitudinalSpeedCmps=500.0;
        source.MeasuredWidthCm=185.0;
        source.MeasuredLengthCm=455.0;
        source.SensorConfidence=0.98;
        source.RealWorldStaticContact=true;
        source.NativeRoadStationObserved=true;
        source.NativeLateralOffsetObserved=true;
        source.RealVelocityObserved=true;
        source.OriginalRaceAuthorityIntact=true;
    }
    values[0].StationCm=pit-250.0;
    values[0].SignedLateralCm=0.0;
    values[0].LongitudinalSpeedCmps=60.0;
    return values;
}
}
int main(){
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    auto settings=PitCorridorSettings{};
    Check(ValidPitCorridorSettings(settings),
        "default service approach limits are bounded");
    settings.NeverIssuePitService=false;
    Check(!ValidPitCorridorSettings(settings),
        "read-only pit model cannot issue real pit service");
    settings={};
    settings.MaximumPredictSeconds=nan;
    Check(!ValidPitCorridorSettings(settings),
        "nonfinite forward approach horizon rejected");
    settings={};
    unsigned verified=0u;
    for(unsigned k=0u;k<WorldScaleVariantCount;++k){
        const auto road=WorldScaleOriginalVariantRecipes[k];
        const double pit=road.LapLengthCm*0.20;
        const auto fleet=Fleet();
        const auto base=Project(road,pit);
        Check(ValidPitCorridorSource(road,fleet,base),
            "all six native projectors and independent Chaos actors exist");
        const auto direct=AssessSixCarPitCorridor(
            road,fleet,base,800u,pit);
        Check(direct.Valid &&
              direct.Fault==PitCorridorFault::None &&
              direct.Circuit==road.Id &&
              direct.SourceFrame==4u &&
              direct.SourceClockSeconds==6.0 &&
              direct.QualifiedCars==6u &&
              direct.DriverActor==800u &&
              direct.Traffic==PitCorridorTraffic::Vacant &&
              direct.TrafficWarnings==0u &&
              direct.NoNativePitServiceIssued &&
              direct.OriginalRaceTimingUnchanged,
              "no rivals near pit means advisory vacant, not issued service");
        Check(direct.Cars[0].Valid &&
              direct.Cars[0].CouldEnterService &&
              direct.Cars[0].ActualPitEntryNotGranted &&
              direct.Cars[0].NativeLongitudinalSpeedCmps==60.0,
              "native driver physically settled without automatic repair");
        Check(direct.Cars[1].Valid &&
              !direct.Cars[1].CouldEnterService &&
              direct.Cars[1].SeparationFromPitCenterCm>0.0,
              "cars outside side corridor do not occupy pit service lane");
        Check(std::abs(PitCorridorForwardGap(
            road,pit-road.LapLengthCm,pit))<1e-5 &&
            std::abs(PitCorridorForwardGap(
            road,pit+road.LapLengthCm,pit))<1e-5,
            "forward gap wraps signed exact lap seam");
        auto blocked=base;
        blocked[1].StationCm=pit-500.0;
        blocked[1].SignedLateralCm=50.0;
        auto congestion=AssessSixCarPitCorridor(
            road,fleet,blocked,800u,pit);
        Check(congestion.Valid &&
              congestion.Traffic==PitCorridorTraffic::Blocked &&
              congestion.TrafficWarnings>=1u &&
              congestion.MinimumRivalDistanceCm<=500.0 &&
              congestion.NoNativePitServiceIssued,
              "real rival physically occupying pit zone blocks forecast");
        blocked[1].StationCm=pit-2400.0;
        blocked[1].LongitudinalSpeedCmps=1000.0;
        congestion=AssessSixCarPitCorridor(
            road,fleet,blocked,800u,pit);
        Check(congestion.Valid &&
              congestion.Traffic==PitCorridorTraffic::WatchApproach &&
              congestion.TrafficWarnings>=1u &&
              congestion.MinimumPredictedConflictSeconds>0.0 &&
              congestion.MinimumPredictedConflictSeconds<=4.0,
              "measured approaching rival creates bounded service warning");
        blocked[1].SignedLateralCm=900.0;
        congestion=AssessSixCarPitCorridor(
            road,fleet,blocked,800u,pit);
        Check(congestion.Valid &&
              congestion.Traffic==PitCorridorTraffic::Vacant,
              "lateral safe route difference removes false service warning");

        auto invalid=base;
        invalid[3].NativeRoadStationObserved=false;
        Check(!AssessSixCarPitCorridor(
            road,fleet,invalid,800u,pit).Valid,
            "unknown wheel route station cannot be fabricated");
        invalid=base;
        invalid[1].Actor=0u;
        Check(!AssessSixCarPitCorridor(
            road,fleet,invalid,800u,pit).Valid,
            "foreign actor cannot impersonate six car route evidence");
        invalid=base;
        invalid[2].SignedLateralCm=nan;
        Check(!AssessSixCarPitCorridor(
            road,fleet,invalid,800u,pit).Valid,
            "nonfinite lateral occupancy denied");
        invalid=base;
        invalid[4].ClockSeconds=5.0;
        Check(!AssessSixCarPitCorridor(
            road,fleet,invalid,800u,pit).Valid,
            "out-of-epoch native road sensor discarded");
        Check(!AssessSixCarPitCorridor(
            road,fleet,base,99999u,pit).Valid,
            "unrecognized service driver never inherits other car rights");
        Check(!AssessSixCarPitCorridor(
            road,fleet,base,800u,nan).Valid,
            "nonfinite pit station rejected before physical prediction");
        ++verified;
    }
    Check(verified==WorldScaleVariantCount,
        "all six original course pits source-audited independently");
    std::cout<<"PhysicalSixCarPitCorridor: six roads and six cars passed\n";
    return EXIT_SUCCESS;
}
