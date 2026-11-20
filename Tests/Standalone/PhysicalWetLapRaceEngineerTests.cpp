#include "Racecraft/PhysicalWetLapRaceEngineer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void Check(bool value,const char* message)
{
    if(!value)
    {
        std::cerr<<"PhysicalWetLapRaceEngineer FAIL: "
                 <<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b,double tolerance=1e-7)
{
    return std::abs(a-b)<=tolerance;
}

latebrake::WetLapTraceFrame Frame(
    const latebrake::WorldScaleVariantRecipe& circuit,
    std::uint64_t frame,double film,double slip,
    double braking,double speed=1500.0)
{
    using namespace latebrake;
    const double clock=20.0+0.05*(frame-1u);
    PhysicalReplayFrame fleet;
    fleet.Session=100u;
    fleet.SessionRevision=5u;
    fleet.SourceFrame=frame;
    fleet.ActiveClockSeconds=clock;
    fleet.SourceDeltaSeconds=0.05;
    fleet.AllSixEngineActorsObserved=true;
    ObservedWaterResult water;
    water.Valid=true;
    water.Fault=ObservedWaterFault::None;
    water.Circuit=circuit.Id;
    water.Session=fleet.Session;
    water.Revision=fleet.SessionRevision;
    water.SourceFrame=frame;
    water.ClockSeconds=clock;
    water.CellLengthCm=circuit.LapLengthCm/
        static_cast<double>(ObservedWaterCellCount);
    water.FilmMm.fill(film);
    water.RenderAdvisoryOnly=true;
    water.OriginalRaceUnaffected=true;
    std::array<WetLapTraceRoadWitness,WetLapTraceCarCount> sources{};
    for(unsigned i=0u;i<WetLapTraceCarCount;++i)
    {
        auto& car=fleet.Cars[i];
        car.Actor=2000u+i;
        car.Session=fleet.Session;
        car.SessionRevision=fleet.SessionRevision;
        car.SourceFrame=frame;
        car.WorldXcm=1000.0+400.0*i+
            speed*0.05*static_cast<double>(frame-1u);
        car.WorldYcm=1000.0+300.0*i;
        car.WorldZcm=100.0;
        car.VelocityXcmps=speed;
        car.VelocityYcmps=0.0;
        car.VelocityZcmps=0.0;
        car.HeadingRadians=slip;
        car.BrakeNormalized=braking;
        car.ThrottleNormalized=braking>0.3?0.0:0.5;
        car.RoadContactConfidence=0.99;
        car.ActualChaosBodyObserved=true;
        car.OriginalRoadPhysicalContact=true;
        car.EngineClockRunning=true;
        auto& w=sources[i];
        w.Actor=car.Actor;
        w.RoadBody=999u;
        w.SourceFrame=frame;
        w.SourceClockSeconds=clock;
        w.MeasuredStationCm=10000.0+200.0*i+
            0.05*speed*static_cast<double>(frame-1u);
        w.MeasuredLateralCm=25.0*i;
        w.MeasuredGripFraction=0.75;
        w.MeasuredConfidence=0.97;
        w.ActualRoadProjectionObserved=true;
        w.ActualOriginalAsphaltContact=true;
        w.GripIndependentlyMeasured=true;
        w.NativeRaceClockUnchanged=true;
    }
    return BuildWetLapTraceFrame(circuit,fleet,water,sources);
}
}
int main()
{
    using namespace latebrake;
    auto settings=WetLapEngineerSettings{};
    Check(ValidWetLapEngineerSettings(settings),
          "source-only telemetry review policy is bounded");
    settings.NeverTreatStatisticsAsOfficialLapTime=false;
    Check(!ValidWetLapEngineerSettings(settings),
          "review cannot claim official race classification");
    settings={};
    settings.MinimumSourceFrames=0u;
    Check(!ValidWetLapEngineerSettings(settings),
          "no one-frame invented lap statistics");
    Check(ClassifyObservedWetLapSurface(0.0)==
              WetLapObservedSurface::Dry &&
          ClassifyObservedWetLapSurface(0.1)==
              WetLapObservedSurface::Damp &&
          ClassifyObservedWetLapSurface(1.0)==
              WetLapObservedSurface::Wet &&
          ClassifyObservedWetLapSurface(5.0)==
              WetLapObservedSurface::StandingWater,
          "real water depth classified without guessing friction");

    unsigned passed=0u;
    for(unsigned k=0u;k<WorldScaleVariantCount;++k)
    {
        const auto circuit=WorldScaleOriginalVariantRecipes[k];
        PhysicalWetLapTraceJournal journal;
        Check(!BuildPhysicalWetLapRaceEngineer(journal).Valid,
              "empty lap cannot generate measured race engineer review");
        for(std::uint64_t frame=1u;frame<=6u;++frame)
        {
            const auto f=Frame(circuit,frame,0.0,0.0,0.1);
            Check(f.Valid && journal.Record(f).Accepted,
                  "real six-car dry driving evidence accepted");
        }
        const auto dry=BuildPhysicalWetLapRaceEngineer(journal);
        Check(dry.Valid &&
              dry.Fault==WetLapEngineerFault::None &&
              dry.Circuit==circuit.Id &&
              dry.Session==100u &&
              dry.Revision==5u &&
              dry.QualifiedPhysicalCars==6u &&
              dry.PhysicalFramesReviewed==6u &&
              dry.FirstSourceFrame==1u &&
              dry.LastSourceFrame==6u &&
              Near(dry.MeasuredSourceSeconds,0.25) &&
              Near(dry.MeanFleetObservedSpeedCmps,1500.0) &&
              Near(dry.MaximumObservedWetnessMm,0.0) &&
              Near(dry.TotalWetVehicleSeconds,0.0) &&
              dry.WetCarsObserved==0u &&
              dry.StandingCarsObserved==0u &&
              dry.StrongSlipCarsObserved==0u &&
              dry.PhysicalRaceAuthorityUnchanged &&
              dry.NoOfficialClassificationIssued &&
              dry.SourceOnly,
              "dry racing review uses only native observed time and distance");
        for(const auto& car:dry.Cars)
        {
            Check(car.Valid &&
                  car.SourceFrameCount==6u &&
                  car.SourceDrySamples==6u &&
                  car.SourceWetSamples==0u &&
                  car.SourceStandingSamples==0u &&
                  car.Cue==WetLapEngineerCue::NormalObservedDriving &&
                  Near(car.MeanObservedSpeedCmps,1500.0) &&
                  Near(car.ObservedSeconds,0.25) &&
                  Near(car.SourceFractionWet,0.0) &&
                  Near(car.SourceFractionBraking,0.0) &&
                  car.MeasuredTravelDistanceCm>0.0 &&
                  car.RouteProgressDistanceCm>0.0 &&
                  car.MinimumMeasuredGrip==0.75 &&
                  car.MaximumMeasuredGrip==0.75 &&
                  car.NoNativeDrivingControlChanged &&
                  car.NoOfficialLapRecordIssued,
                  "each car independently retains exact dry racing provenance");
        }

        for(std::uint64_t frame=7u;frame<=12u;++frame)
        {
            const auto f=Frame(circuit,frame,1.5,0.0,0.75);
            Check(f.Valid && journal.Record(f).Accepted,
                  "measured wet driving and physical braking observations");
        }
        const auto wet=BuildPhysicalWetLapRaceEngineer(journal);
        Check(wet.Valid &&
              wet.LastSourceFrame==12u &&
              wet.PhysicalFramesReviewed==12u &&
              wet.WetCarsObserved==6u &&
              wet.StandingCarsObserved==0u &&
              wet.WetBrakingCarsObserved==6u &&
              wet.TotalWetVehicleSeconds>0.0 &&
              wet.MaximumObservedWetnessMm==1.5 &&
              wet.MeanFleetObservedSpeedCmps==1500.0,
              "water and braking exposure are observations, not force edits");
        for(const auto& car:wet.Cars)
        {
            Check(car.Valid &&
                  car.SourceWetSamples==6u &&
                  car.SourceDrySamples==6u &&
                  car.BrakeWitnessSamples==6u &&
                  car.SourceStandingSamples==0u &&
                  car.WetSeconds>0.0 &&
                  car.BrakeInputSeconds>0.0 &&
                  car.Cue==WetLapEngineerCue::RepeatedWetBraking &&
                  car.SourceFractionWet>0.0 &&
                  car.SourceFractionWet<1.0 &&
                  car.SourceFractionBraking>0.0 &&
                  car.MeanWetSpeedCmps>0.0 &&
                  car.MeanDrySpeedCmps>0.0,
                  "real wet brake samples appear in source-only coaching");
        }
        const auto compare=ComparePhysicalWetLapEngineer(dry,wet);
        Check(compare.Valid &&
              compare.Session==100u &&
              compare.EarlierLastSourceFrame==6u &&
              compare.LaterLastSourceFrame==12u &&
              compare.MoreWetCars==6u &&
              compare.HigherSlipCars==0u &&
              compare.SimilarMeanSpeedCars==6u &&
              compare.SlowerMeanSpeedCars==0u &&
              Near(compare.MeanFleetSpeedChangeCmps,0.0) &&
              compare.CausalityNotEstablished &&
              compare.ObservationalComparisonOnly &&
              compare.NoOfficialTimingAuthority,
              "wet evidence does not fabricate an unmeasured speed penalty");

        for(std::uint64_t frame=13u;frame<=18u;++frame)
        {
            const auto f=Frame(circuit,frame,5.0,0.28,0.65);
            Check(f.Valid && journal.Record(f).Accepted,
                  "measured standing water and real slip preserve car identities");
        }
        const auto standing=BuildPhysicalWetLapRaceEngineer(journal);
        Check(standing.Valid &&
              standing.StandingCarsObserved==6u &&
              standing.StrongSlipCarsObserved==6u &&
              standing.TotalStandingWaterVehicleSeconds>0.0 &&
              standing.TotalWetVehicleSeconds>
                  wet.TotalWetVehicleSeconds &&
              standing.PeakObservedChassisSpeedCmps==1500.0,
              "standing film and slip hazards classified independently");
        for(const auto& car:standing.Cars)
            Check(car.Valid &&
                  car.SourceStandingSamples==6u &&
                  car.StrongSlipWitnessSamples>=6u &&
                  car.MaximumObservedSlipRadians>0.2 &&
                  car.Cue==WetLapEngineerCue::StandingWaterCaution,
                  "actual deep road film takes precedence over slip cue");
        Check(!ComparePhysicalWetLapEngineer(standing,dry).Valid,
              "time reversal cannot be presented as a new race review");
        // A distinct native recording slows during later observed wet
        // intervals. Verify wet and dry speeds are calculated from those
        // exact intervals, not from a shared all-weather sector average.
        PhysicalWetLapTraceJournal changedPace;
        for(std::uint64_t frame=1u;frame<=8u;++frame)
        {
            const bool wetInterval=frame>=5u;
            const auto observation=Frame(
                circuit,frame,wetInterval?1.5:0.0,
                0.0,0.1,wetInterval?1200.0:1500.0);
            Check(observation.Valid &&
                  changedPace.Record(observation).Accepted,
                  "real moderate wet pace remains physically continuous");
        }
        const auto pace=BuildPhysicalWetLapRaceEngineer(changedPace);
        Check(pace.Valid && pace.QualifiedPhysicalCars==6u &&
              pace.WetCarsObserved==6u,
              "wet-vs-dry pace sample is physically source-eligible");
        for(const auto& car:pace.Cars)
            Check(car.Valid &&
                  Near(car.MeanDrySpeedCmps,1500.0,1e-5) &&
                  Near(car.MeanWetSpeedCmps,1237.5,1e-5) &&
                  car.MeanWetSpeedCmps<car.MeanDrySpeedCmps,
                  "wet and dry speeds reflect separate native time intervals");
        journal.Reset();
        Check(!BuildPhysicalWetLapRaceEngineer(journal).Valid,
              "new physical racing session clears all old wet review data");
        ++passed;
    }
    Check(passed==WorldScaleVariantCount,
          "all six original circuit recordings independently passed");
    std::cout<<"PhysicalWetLapRaceEngineer: six circuits passed\n";
    return EXIT_SUCCESS;
}
