#include "Garage/PhysicalTyreServiceForecast.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Require(bool pass,const char* message) {
    if(!pass){std::cerr<<"PhysicalTyreServiceForecast FAIL: "<<message<<'\n';std::exit(EXIT_FAILURE);}
}
latebrake::PhysicalTyreTrendSample Frame(
    unsigned sourceFrame,
    latebrake::WorldScaleVariantId circuit,
    unsigned vehicle)
{
    using namespace latebrake;
    PhysicalTyreTrendSample source;
    source.Session=100u;
    source.Revision=2u;
    source.OwnerActor=900u;
    source.Frame=sourceFrame;
    source.ActiveClockSeconds=0.05*sourceFrame;
    source.StepSeconds=0.05;
    source.OriginalVehicleId=static_cast<int>(vehicle);
    source.Circuit=circuit;
    source.OriginalEngineTyresObserved=true;
    source.NoUnrealForceChanges=true;
    for(unsigned i=0u;i<TyreSetupWheelCount;++i){
        auto& w=source.Wheels[i];
        w.OwnerActor=900u;
        w.NativeWheel=1000u+i;
        w.SourceFrame=sourceFrame;
        w.ClockSeconds=source.ActiveClockSeconds;
        w.HotPressureKpa=210.0+sourceFrame*0.10;
        w.CarcassTemperatureC=78.0+sourceFrame*0.15;
        w.SurfaceTemperatureC=80.0+sourceFrame*0.10;
        w.TreadDepthMm=7.0-0.0007*sourceFrame;
        w.SensorConfidence=1.0;
        w.ActualChaosWheelObserved=true;
        w.ActualOriginalRoadContact=true;
        w.ActualPressureMeasurement=true;
        w.ActualTemperatureMeasurement=true;
        w.ActualTreadMeasurement=true;
    }
    return source;
}
latebrake::PhysicalTyreServiceRaceWitness Witness(
    const latebrake::PhysicalTyreTrendSummary& summary)
{
    latebrake::PhysicalTyreServiceRaceWitness r;
    r.Session=summary.Session;
    r.Revision=summary.Revision;
    r.SourceFrame=summary.LastFrame;
    r.DriverActor=summary.OwnerActor;
    r.NativePitFacility=9000u;
    r.ClockSeconds=summary.LastClockSeconds;
    r.OfficialLapSeconds=45.0;
    r.MeasuredFuelReserveLiters=18.0;
    r.OfficialRemainingLaps=12u;
    r.OriginalNativePitBoxVerified=true;
    r.ActualWheelCrewAvailable=true;
    r.ActualPitSpeedLimitVerified=true;
    r.OriginalRaceClockUntouched=true;
    return r;
}
}
int main() {
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    PhysicalTyreServiceSettings settings;
    Require(ValidPhysicalTyreServiceSettings(settings),
        "original tire pit review defaults are finite");
    settings.NeverBookPitOrApplyRealRepair=false;
    Require(!ValidPhysicalTyreServiceSettings(settings),
        "forecast cannot directly schedule actual service");
    settings={};
    settings.TargetTreadReserveMm=nan;
    Require(!ValidPhysicalTyreServiceSettings(settings),
        "nonfinite tire reserve invalidates entire source plan");
    settings={};
    unsigned qualified=0u;
    for(unsigned route=0u;route<WorldScaleVariantCount;++route){
        const auto circuit=WorldScaleOriginalVariantRecipes[route];
        PhysicalTyreStintTrend tracker;
        for(unsigned frame=1u;frame<=20u;++frame)
            Require(tracker.Append(Frame(frame,circuit.Id,0u)),
                "authentic four-wheel input produces finite readout");
        const auto summary=tracker.Summarize();
        Require(summary.Valid &&
                summary.Circuit==circuit.Id &&
                summary.StoredSourceFrames==20u &&
                summary.ObservedDurationSeconds>0.1,
                "physical tire trend retains exact original source circuit");
        auto race=Witness(summary);
        Require(ValidPhysicalTyreServiceRaceWitness(
            race,summary,settings),
            "native pit crew, clock, lap and car identity required");
        const auto planned=PlanPhysicalTyreService(
            summary,race);
        Require(planned.Valid &&
                planned.Circuit==circuit.Id &&
                planned.Session==100u &&
                planned.SourceFrame==summary.LastFrame &&
                planned.DriverActor==900u &&
                planned.OfficialRemainingLaps==12u &&
                planned.Wheels.size()==4u &&
                planned.ShortestTreadReserveLaps>=0.0 &&
                planned.ProposedServiceWindowLap<=12.0 &&
                planned.ConservativePitLossSeconds>0.0 &&
                planned.ConservativePitLossSeconds<120.0 &&
                !planned.PitReservationSubmitted &&
                !planned.UnrealTyreForceChanged &&
                !planned.CareerCreditsSpent &&
                planned.OfficialTimeUnaffected,
                "original source forecasts four independently measured tires");
        for(const auto& wheel:planned.Wheels)
            Require(wheel.Valid &&
                    wheel.NativeWheel>=1000u &&
                    wheel.NativeWheel<=1003u &&
                    wheel.LatestTreadMm>0.0 &&
                    wheel.EstimatedEndOfStintTreadMm>=0.0 &&
                    wheel.LapsUntilTreadReserve>=0.0 &&
                    wheel.LapsUntilTreadReserve<=
                       settings.MaximumRecommendedHorizonLaps &&
                    wheel.PredictedWearMmPerLap>=0.0 &&
                    wheel.OriginalWheelUnmodified,
                    "all physically witnessed tire maintenance windows stay advisory");
        auto invalid=race;
        invalid.NativePitFacility=0u;
        Require(!PlanPhysicalTyreService(summary,invalid).Valid,
                "no real pit box witness means no service authority");
        invalid=race;
        invalid.ActualWheelCrewAvailable=false;
        Require(!PlanPhysicalTyreService(summary,invalid).Valid,
                "no actual wheel crew cannot be replaced by a software estimate");
        invalid=race;
        invalid.OfficialLapSeconds=nan;
        Require(!PlanPhysicalTyreService(summary,invalid).Valid,
                "nonfinite official lap duration cannot become tire life");
        invalid=race;
        invalid.SourceFrame++;
        Require(!PlanPhysicalTyreService(summary,invalid).Valid,
                "outdated wheel observation frame cannot be forwarded");
        invalid=race;
        invalid.Revision++;
        Require(!PlanPhysicalTyreService(summary,invalid).Valid,
                "new session revision cannot reuse earlier vehicle setup");
        auto altered=summary;
        altered.Valid=false;
        Require(!PlanPhysicalTyreService(altered,race).Valid,
                "invalid physical tire history never creates a service suggestion");
        altered=summary;
        altered.Wheels[0].LatestCarcassTemperatureC=150.0;
        const auto hot=PlanPhysicalTyreService(altered,race);
        Require(hot.Valid && hot.HotWheels==1u &&
                hot.FleetUrgency==
                    PhysicalTyreServiceUrgency::ImmediateReview &&
                hot.ServiceRecommendedWheels>=1u &&
                !hot.PitReservationSubmitted,
                "native severe tire heat issues urgent source-only pit review");
        altered=summary;
        altered.Wheels[1].LatestTreadMm=1.4;
        const auto worn=PlanPhysicalTyreService(altered,race);
        Require(worn.Valid && worn.WornWheels>=1u &&
                worn.FleetUrgency==
                    PhysicalTyreServiceUrgency::ImmediateReview &&
                worn.ServiceRecommendedWheels>=1u,
                "measured critical tire tread warrants review not fictitious service");
        const auto wheel=ForecastPhysicalTyreServiceWheel(
            summary.Wheels[0],12u,45.0);
        Require(wheel.Valid && wheel.WheelIndex==0u &&
                wheel.Reason!=PhysicalTyreServiceReason::InsufficientEvidence,
                "finite physical tire telemetry yields safe recommendation");
        ++qualified;
    }
    Require(qualified==WorldScaleVariantCount,
            "all six distinct source-only circuits independently audited");
    std::cout<<"PhysicalTyreServiceForecast: six original courses passed\n";
    return EXIT_SUCCESS;
}
