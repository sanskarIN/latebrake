#include "Racecraft/PhysicalRaceWeatherEventJournal.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void Check(bool ok,const char* message)
{
    if(!ok)
    {
        std::cerr<<"PhysicalRaceWeatherEventJournal FAIL: "
                 <<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

latebrake::NightWetSafetyFleet Night(
    latebrake::WorldScaleVariantId circuit,
    std::uint64_t frame,double clock,double risk=1.2)
{
    using namespace latebrake;
    NightWetSafetyFleet a;
    a.Valid=true;
    a.Fault=NightWetFault::None;
    a.Circuit=circuit;
    a.Session=44u;
    a.Revision=3u;
    a.SourceFrame=frame;
    a.SourceClockSeconds=clock;
    a.SourceQualifiedCars=NightWetVehicleCount;
    a.MinimumSightReserveCm=
        std::numeric_limits<double>::infinity();
    a.MinimumWitnessedLux=
        std::numeric_limits<double>::infinity();
    for(unsigned i=0u;i<NightWetVehicleCount;++i)
    {
        auto& c=a.Cars[i];
        c.Valid=true;
        c.Actor=500u+i;
        c.WrappedStationCm=1000.0+500.0*i;
        c.RoadWaterFilmMm=2.0;
        c.SourceTrackLux=40.0;
        c.SourceLampLux=80.0;
        c.EstimatedSightDistanceCm=2000.0;
        c.MeasuredStoppingDistanceCm=2000.0*risk;
        c.BrakingDistanceReserveCm=
            c.EstimatedSightDistanceCm-c.MeasuredStoppingDistanceCm;
        c.ConservativeDistanceRatio=risk;
        c.Band=risk>=1.9?
            NightWetSafetyBand::Critical:
            risk>=1.1?NightWetSafetyBand::LimitedSight:
            NightWetSafetyBand::Comfortable;
        c.RealOcclusionUnverified=true;
        c.NoPhysicalBrakingApplied=true;
        a.MaximumObservedWaterMm=std::max(
            a.MaximumObservedWaterMm,c.RoadWaterFilmMm);
        a.MinimumSightReserveCm=std::min(
            a.MinimumSightReserveCm,c.BrakingDistanceReserveCm);
        a.MinimumWitnessedLux=std::min(
            a.MinimumWitnessedLux,c.SourceTrackLux);
        if(c.Band==NightWetSafetyBand::Critical)++a.CriticalCars;
        if(c.Band==NightWetSafetyBand::Critical ||
           c.Band==NightWetSafetyBand::LimitedSight ||
           c.Band==NightWetSafetyBand::Caution)++a.CautionCars;
    }
    return a;
}
latebrake::WetPitDecisionReceipt Pit(
    std::uint64_t frame,double clock,
    latebrake::WetPitDecision decision=
        latebrake::WetPitDecision::WaitForTraffic)
{
    using namespace latebrake;
    WetPitDecisionReceipt p;
    p.Valid=true;
    p.Fault=WetPitDecisionFault::None;
    p.Proposed=decision;
    p.Confirmed=decision;
    p.SourceSession=44u;
    p.SourceFrame=frame;
    p.SourceClockSeconds=clock;
    p.OriginalPitHoldRequired=true;
    p.NoActualServiceIssued=true;
    p.NoRaceOrTyreChange=true;
    return p;
}
}
int main()
{
    using namespace latebrake;
    auto settings=PhysicalWeatherEventSettings{};
    Check(ValidPhysicalWeatherEventSettings(settings),
          "strict source-only event journal settings valid");
    settings.NeverClaimActualGPUOcclusion=false;
    Check(!ValidPhysicalWeatherEventSettings(settings),
          "cannot elevate telemetry into an actual GPU occlusion claim");
    settings={};
    settings.MaximumFrameStepSeconds=
        std::numeric_limits<double>::quiet_NaN();
    Check(!ValidPhysicalWeatherEventSettings(settings),
          "nonfinite native clock cadence rejected");

    unsigned verified=0u;
    for(unsigned i=0u;i<WorldScaleVariantCount;++i)
    {
        const auto circuit=WorldScaleOriginalVariantRecipes[i];
        PhysicalRaceWeatherEventJournal journal;
        Check(journal.Size()==0u &&
              journal.CurrentDigest()==0u &&
              journal.CriticalCount()==0u,
              "new source journal never invents past racing events");
        auto night=Night(circuit.Id,1u,1.0);
        auto pit=Pit(1u,1.0);
        Check(ValidPhysicalWeatherJournalInput(night,pit,{}),
              "six authenticated cars and real pit advisory can be correlated");
        auto first=journal.Observe(night,pit);
        Check(first.Accepted &&
              first.Fault==PhysicalWeatherEventFault::None &&
              first.StoredEvents==1u &&
              first.EventDigest!=0u &&
              first.SourcePitWaitEvents==1u &&
              first.NoNativeActuation &&
              first.NoOfficialRaceTimingChange,
              "first physical frame is accepted without changing race state");
        PhysicalWeatherEvent item;
        Check(journal.Read(0u,item) &&
              item.Valid &&
              item.Kind==PhysicalWeatherEventKind::PitWaitForTraffic &&
              item.SourceCars==6u &&
              item.ConfirmedPit==WetPitDecision::WaitForTraffic &&
              item.PreviousDigest==0u &&
              item.Digest==first.EventDigest &&
              item.Digest==PhysicalWeatherEventDigest(item) &&
              !journal.Read(1u,item) &&
              !item.Valid,
              "event digest includes witnessed physical source state");
        Check(!journal.Observe(night,pit).Accepted &&
              journal.LastFault()==
                  PhysicalWeatherEventFault::NonmonotonicFrame &&
              journal.Size()==1u,
              "duplicate or backward frame cannot alter source journal");

        night=Night(circuit.Id,2u,1.05,2.0);
        pit=Pit(2u,1.05);
        const auto critical=journal.Observe(night,pit);
        Check(critical.Accepted &&
              critical.CriticalSightEvents==1u &&
              critical.StoredEvents==2u,
              "real measured stopping-distance risk generates critical event");
        Check(journal.Read(1u,item) &&
              item.Kind==PhysicalWeatherEventKind::CriticalSight &&
              item.PreviousDigest==first.EventDigest &&
              item.Digest==critical.EventDigest &&
              item.CriticalCars==6u,
              "digest chain tracks source-evidenced sight hazard");

        night=Night(circuit.Id,3u,1.10,0.65);
        pit=Pit(3u,1.10,WetPitDecision::StayOnRoad);
        Check(journal.Observe(night,pit).Accepted &&
              journal.Read(2u,item) &&
              item.Kind==PhysicalWeatherEventKind::SightRecovered,
              "source-only conditions recover without claiming a lap reward");

        night=Night(circuit.Id,4u,1.15,0.65);
        pit=Pit(4u,1.15,WetPitDecision::ApproachWithCaution);
        auto bad=night;
        bad.Cars[1].Actor=bad.Cars[0].Actor;
        Check(!journal.Observe(bad,pit).Accepted &&
              journal.Size()==3u,
              "duplicated native car identity cannot enter evidence chain");
        bad=night;
        bad.Cars[2].RoadWaterFilmMm=5.0;
        Check(!journal.Observe(bad,pit).Accepted,
              "inconsistent film maximum fails source invariant");
        bad=night;
        bad.Cars[3].BrakingDistanceReserveCm=
            std::numeric_limits<double>::quiet_NaN();
        Check(!journal.Observe(bad,pit).Accepted,
              "nonfinite physical stopping reserve cannot be recorded");
        bad=night;
        bad.SourceFrame=5u;
        Check(!journal.Observe(bad,pit).Accepted,
              "weather and pit source frame mismatch rejected");
        Check(journal.Observe(night,pit).Accepted &&
              journal.Size()==4u,
              "legitimate source frame succeeds after rejected samples");

        night=Night(circuit.Id,5u,1.20,0.65);
        pit=Pit(5u,1.20);
        night.Revision=4u;
        Check(!journal.Observe(night,pit).Accepted &&
              journal.LastFault()==PhysicalWeatherEventFault::SessionChanged &&
              journal.Size()==4u,
              "native session revision prevents cross-restart journal joins");

        night=Night(circuit.Id,5u,1.20,0.65);
        pit=Pit(5u,1.20);
        Check(journal.Observe(night,pit).Accepted,
              "same physical race epoch can resume after bad restart");
        for(std::uint64_t frame=6u;frame<=90u;++frame)
        {
            const double clock=1.0+(frame-1u)*0.05;
            const auto n=Night(circuit.Id,frame,clock,0.7);
            const auto p=Pit(frame,clock,WetPitDecision::StayOnRoad);
            const auto receipt=journal.Observe(n,p);
            Check(receipt.Accepted &&
                  receipt.StoredEvents<=PhysicalWeatherEventCapacity,
                  "authentic frame stream holds bounded 64-entry memory");
        }
        Check(journal.Size()==PhysicalWeatherEventCapacity,
              "ring evicts old visual advice after exact capacity");
        Check(journal.Read(0u,item) &&
              item.NativeFrame==27u &&
              journal.Read(journal.Size()-1u,item) &&
              item.NativeFrame==90u &&
              item.Digest==journal.CurrentDigest(),
              "journal preserves chronological access after rollover");
        for(std::size_t k=0u;k<journal.Size();++k)
        {
            PhysicalWeatherEvent current;
            Check(journal.Read(k,current) &&
                  current.Digest==PhysicalWeatherEventDigest(current),
                  "every retained event has reproducible canonical digest");
            if(k>0u)
            {
                PhysicalWeatherEvent previous;
                Check(journal.Read(k-1u,previous) &&
                      current.PreviousDigest==previous.Digest,
                      "retained journal digest chain remains connected");
            }
        }
        journal.Reset();
        Check(journal.Size()==0u &&
              journal.CurrentDigest()==0u &&
              journal.LastFault()==PhysicalWeatherEventFault::None,
              "explicit physical race restart clears old identity state");
        ++verified;
    }
    Check(verified==WorldScaleVariantCount,
          "all six original road circuits pass independent weather evidence QA");
    std::cout<<"PhysicalRaceWeatherEventJournal: six circuits passed\n";
    return EXIT_SUCCESS;
}
