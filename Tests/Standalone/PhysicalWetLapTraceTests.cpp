#include "Racecraft/PhysicalWetLapTrace.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void Check(bool yes,const char* why)
{
    if(!yes)
    {
        std::cerr<<"PhysicalWetLapTrace FAIL: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

latebrake::PhysicalReplayFrame Fleet(
    std::uint64_t frame,double clock,double speed=1500.0)
{
    using namespace latebrake;
    PhysicalReplayFrame f;
    f.Session=41u;
    f.SessionRevision=2u;
    f.SourceFrame=frame;
    f.ActiveClockSeconds=clock;
    f.SourceDeltaSeconds=0.05;
    f.AllSixEngineActorsObserved=true;
    f.SourceOnly=true;
    for(unsigned i=0u;i<WetLapTraceCarCount;++i)
    {
        auto& car=f.Cars[i];
        car.Actor=5000u+i;
        car.Session=f.Session;
        car.SessionRevision=f.SessionRevision;
        car.SourceFrame=frame;
        car.WorldXcm=1000.0+400.0*i+
            speed*0.05*static_cast<double>(frame-1u);
        car.WorldYcm=1000.0+300.0*i;
        car.WorldZcm=100.0;
        car.VelocityXcmps=speed;
        car.VelocityYcmps=0.0;
        car.VelocityZcmps=0.0;
        car.HeadingRadians=0.0;
        car.BrakeNormalized=0.1;
        car.ThrottleNormalized=0.5;
        car.RoadContactConfidence=0.99;
        car.ActualChaosBodyObserved=true;
        car.OriginalRoadPhysicalContact=true;
        car.EngineClockRunning=true;
    }
    return f;
}

latebrake::ObservedWaterResult Water(
    const latebrake::WorldScaleVariantRecipe& circuit,
    std::uint64_t frame,double clock,double film=2.0)
{
    using namespace latebrake;
    ObservedWaterResult w;
    w.Valid=true;
    w.Fault=ObservedWaterFault::None;
    w.Circuit=circuit.Id;
    w.Session=41u;
    w.Revision=2u;
    w.SourceFrame=frame;
    w.ClockSeconds=clock;
    w.StepSeconds=0.05;
    w.CellLengthCm=circuit.LapLengthCm/
        static_cast<double>(ObservedWaterCellCount);
    w.FilmMm.fill(film);
    w.RenderAdvisoryOnly=true;
    w.OriginalRaceUnaffected=true;
    return w;
}

std::array<latebrake::WetLapTraceRoadWitness,
    latebrake::WetLapTraceCarCount> Road(
    std::uint64_t frame,double clock,double station=10000.0)
{
    using namespace latebrake;
    std::array<WetLapTraceRoadWitness,WetLapTraceCarCount> out{};
    for(unsigned i=0u;i<WetLapTraceCarCount;++i)
    {
        auto& p=out[i];
        p.Actor=5000u+i;
        p.SourceFrame=frame;
        p.RoadBody=999u; // same real asphalt mesh for every car
        p.SourceClockSeconds=clock;
        p.MeasuredStationCm=station+
            150.0*i+75.0*static_cast<double>(frame-1u);
        p.MeasuredLateralCm=100.0+10.0*i;
        p.MeasuredGripFraction=0.7;
        p.MeasuredConfidence=0.97;
        p.ActualRoadProjectionObserved=true;
        p.ActualOriginalAsphaltContact=true;
        p.GripIndependentlyMeasured=true;
        p.NativeRaceClockUnchanged=true;
    }
    return out;
}
}
int main()
{
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    auto settings=WetLapTraceSettings{};
    Check(ValidWetLapTraceSettings(settings),
          "strict original source-only wet lap settings valid");
    settings.NeverWriteChaosWheelForces=false;
    Check(!ValidWetLapTraceSettings(settings),
          "portable trace cannot apply real wheel grip");
    settings={};
    settings.MaximumObservedSpeedCmps=nan;
    Check(!ValidWetLapTraceSettings(settings),
          "nonfinite speed ceiling denied");

    unsigned verified=0u;
    for(unsigned track=0u;track<WorldScaleVariantCount;++track)
    {
        const auto circuit=WorldScaleOriginalVariantRecipes[track];
        PhysicalWetLapTraceJournal log;
        Check(log.Count()==0u &&
              log.LastFault()==WetLapTraceFault::None,
              "new trace never fabricates physical racing frames");

        auto fleet=Fleet(1u,10.0);
        auto water=Water(circuit,1u,10.0);
        auto projection=Road(1u,10.0);
        Check(ValidatePhysicalReplayFrame(fleet)==
                  PhysicalReplayReject::None &&
              ValidWetLapTraceRoadWitness(
                  fleet.Cars[0],projection[0],fleet),
              "original physical road sensor and six Chaos actor identities");
        const auto frame=BuildWetLapTraceFrame(
            circuit,fleet,water,projection);
        Check(frame.Valid &&
              frame.Circuit==circuit.Id &&
              frame.Session==41u &&
              frame.Revision==2u &&
              frame.SourceFrame==1u &&
              frame.VerifiedRoadBodies==6u &&
              frame.WetVehicleCount==6u &&
              frame.StandingWaterVehicleCount==0u &&
              frame.MaximumSourceWaterMm==2.0 &&
              frame.NativeChaosUnchanged &&
              frame.SourceOnly &&
              frame.NoOfficialLapTimeChange,
              "six authenticated cars can share one authentic collision road");
        Check(frame.Cars[0].Valid &&
              frame.Cars[0].Actor==5000u &&
              frame.Cars[0].ActualGripFraction==0.7 &&
              frame.Cars[0].ActualWaterFilmMm==2.0 &&
              frame.Cars[0].WetPhysicalRoad &&
              frame.Cars[0].PlanarSpeedCmps==1500.0 &&
              std::abs(frame.Cars[0].LateralSlipRadians)<1.0e-9 &&
              frame.Cars[0].NoWheelContactModified,
              "native speed, slip and separately measured grip retained");
        Check(log.Record(frame).Accepted && log.Count()==1u &&
              !log.Record(frame).Accepted &&
              log.LastFault()==WetLapTraceFault::NonmonotonicSourceTime,
              "first frame accepted and duplicate native frame rejected");

        fleet=Fleet(2u,10.05);
        water=Water(circuit,2u,10.05,4.0);
        projection=Road(2u,10.05);
        const auto second=BuildWetLapTraceFrame(
            circuit,fleet,water,projection);
        Check(second.Valid &&
              second.StandingWaterVehicleCount==6u &&
              WetLapTracePlausibleContinuity(frame,second) &&
              log.Record(second).Accepted,
              "measured standing water remains physical and continuous");
        const auto genuine=second;
        auto moved=second;
        moved.Cars[0].WorldXcm+=7000.0;
        Check(!WetLapTracePlausibleContinuity(frame,moved) &&
              !log.Record(moved).Accepted &&
              log.Count()==2u &&
              log.LastFault()==WetLapTraceFault::NonmonotonicSourceTime,
              "duplicate frame never replaces existing evidence");
        fleet=Fleet(3u,10.10);
        water=Water(circuit,3u,10.10);
        projection=Road(3u,10.10);
        auto third=BuildWetLapTraceFrame(
            circuit,fleet,water,projection);
        moved=third;
        moved.Cars[0].WrappedStationCm+=5000.0;
        Check(!log.Record(moved).Accepted &&
              log.LastFault()==WetLapTraceFault::ImpossibleCarMovement,
              "teleport within source road cannot earn replay continuity");
        Check(log.Record(third).Accepted &&
              log.Count()==3u,
              "good source frame can resume after a rejected teleport");

        projection[0].ActualRoadProjectionObserved=false;
        Check(!BuildWetLapTraceFrame(
            circuit,fleet,water,projection).Valid,
            "unwitnessed native route station cannot qualify wet telemetry");
        projection=Road(3u,10.10);
        projection[1].MeasuredGripFraction=nan;
        Check(!BuildWetLapTraceFrame(
            circuit,fleet,water,projection).Valid,
            "nonfinite measured physical grip cannot be extrapolated");
        projection=Road(3u,10.10);
        projection[2].Actor=projection[0].Actor;
        Check(!BuildWetLapTraceFrame(
            circuit,fleet,water,projection).Valid,
            "same physical car cannot take two identities");
        projection=Road(3u,10.10);
        water.Revision=3u;
        Check(!BuildWetLapTraceFrame(
            circuit,fleet,water,projection).Valid,
            "rain from another official session cannot be borrowed");

        fleet=Fleet(4u,10.15);
        fleet.Cars[0].HeadingRadians=0.28;
        water=Water(circuit,4u,10.15);
        projection=Road(4u,10.15);
        const auto corner=BuildWetLapTraceFrame(
            circuit,fleet,water,projection);
        Check(corner.Valid &&
              std::abs(corner.Cars[0].LateralSlipRadians)>0.2 &&
              log.Record(corner).Accepted,
              "real measured yaw and velocity produce finite slip angle");
        for(std::uint64_t n=5u;n<=110u;++n)
        {
            const double clock=10.0+0.05*(n-1u);
            const auto f=Fleet(n,clock);
            const auto w=Water(circuit,n,clock,n>=60u?0.1:2.0);
            const auto p=Road(n,clock);
            const auto sample=BuildWetLapTraceFrame(
                circuit,f,w,p);
            Check(sample.Valid && log.Record(sample).Accepted &&
                  log.Count()<=WetLapTraceCapacity,
                  "96-frame water/racing trace ring stays fixed capacity");
        }
        Check(log.Count()==WetLapTraceCapacity,
              "valid source history rolls over at the exact fixed capacity");
        WetLapTraceFrame oldest;
        WetLapTraceFrame newest;
        Check(log.Read(0u,oldest) &&
              oldest.SourceFrame==15u &&
              log.Read(log.Count()-1u,newest) &&
              newest.SourceFrame==110u &&
              !log.Read(log.Count(),oldest) &&
              !oldest.Valid,
              "rolled source history retains chronological actor identities");
        log.Reset();
        Check(log.Count()==0u &&
              log.LastFault()==WetLapTraceFault::None,
              "new original race session resets all wet driving evidence");

        const auto seamA=Road(1u,10.0,circuit.LapLengthCm-100.0);
        const auto seamB=Road(2u,10.05,circuit.LapLengthCm-100.0);
        const auto start=BuildWetLapTraceFrame(
            circuit,Fleet(1u,10.0),
            Water(circuit,1u,10.0),seamA);
        const auto finish=BuildWetLapTraceFrame(
            circuit,Fleet(2u,10.05),
            Water(circuit,2u,10.05),seamB);
        Check(start.Valid && finish.Valid &&
              WetLapTracePlausibleContinuity(start,finish),
              "real route projection wraps full lap without false teleport");
        ++verified;
    }
    Check(verified==WorldScaleVariantCount,
          "six independent physical circuits passed wet lap telemetry QA");
    std::cout<<"PhysicalWetLapTrace: six circuits passed\n";
    return EXIT_SUCCESS;
}
