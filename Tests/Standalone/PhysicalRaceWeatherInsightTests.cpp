#include "Racecraft/PhysicalRaceWeatherInsight.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void Check(bool yes,const char* what)
{
    if(!yes)
    {
        std::cerr<<"PhysicalRaceWeatherInsight FAIL: "<<what<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double lhs,double rhs,double epsilon=1e-7)
{
    return std::abs(lhs-rhs)<=epsilon;
}
latebrake::NightWetSafetyFleet Night(
    latebrake::WorldScaleVariantId circuit,
    std::uint64_t frame,double clock,
    double risk,double wet)
{
    using namespace latebrake;
    NightWetSafetyFleet source;
    source.Valid=true;
    source.Fault=NightWetFault::None;
    source.Circuit=circuit;
    source.Session=777u;
    source.Revision=4u;
    source.SourceFrame=frame;
    source.SourceClockSeconds=clock;
    source.SourceQualifiedCars=NightWetVehicleCount;
    source.MinimumSightReserveCm=
        std::numeric_limits<double>::infinity();
    source.MinimumWitnessedLux=
        std::numeric_limits<double>::infinity();
    for(unsigned i=0u;i<NightWetVehicleCount;++i)
    {
        auto& car=source.Cars[i];
        car.Valid=true;
        car.Actor=900u+i;
        car.WrappedStationCm=1000.0+i*500.0;
        car.RoadWaterFilmMm=wet;
        car.SourceTrackLux=4.0;
        car.SourceLampLux=25.0;
        car.EstimatedSightDistanceCm=4000.0;
        car.MeasuredStoppingDistanceCm=4000.0*risk;
        car.BrakingDistanceReserveCm=
            car.EstimatedSightDistanceCm-car.MeasuredStoppingDistanceCm;
        car.ConservativeDistanceRatio=risk;
        car.Band=risk>=1.9?NightWetSafetyBand::Critical:
            risk>=1.1?NightWetSafetyBand::LimitedSight:
            NightWetSafetyBand::Comfortable;
        car.RealOcclusionUnverified=true;
        car.NoPhysicalBrakingApplied=true;
        source.MinimumSightReserveCm=std::min(
            source.MinimumSightReserveCm,car.BrakingDistanceReserveCm);
        source.MinimumWitnessedLux=std::min(
            source.MinimumWitnessedLux,car.SourceTrackLux);
        source.MaximumObservedWaterMm=std::max(
            source.MaximumObservedWaterMm,car.RoadWaterFilmMm);
        if(car.Band==NightWetSafetyBand::Critical)
            ++source.CriticalCars;
        if(car.Band==NightWetSafetyBand::LimitedSight ||
           car.Band==NightWetSafetyBand::Caution ||
           car.Band==NightWetSafetyBand::Critical)
            ++source.CautionCars;
    }
    return source;
}
latebrake::WetPitDecisionReceipt Pit(
    std::uint64_t frame,double clock,
    latebrake::WetPitDecision decision=
        latebrake::WetPitDecision::StayOnRoad)
{
    using namespace latebrake;
    WetPitDecisionReceipt p;
    p.Valid=true;
    p.Fault=WetPitDecisionFault::None;
    p.Proposed=decision;
    p.Confirmed=decision;
    p.SourceSession=777u;
    p.SourceFrame=frame;
    p.SourceClockSeconds=clock;
    p.NoActualServiceIssued=true;
    p.NoRaceOrTyreChange=true;
    p.OriginalPitHoldRequired=true;
    return p;
}
}
int main()
{
    using namespace latebrake;
    PhysicalWeatherInsightSettings settings;
    Check(ValidPhysicalWeatherInsightSettings(settings),
          "read-only six-car weather analytics settings valid");
    settings.NeverPublishOfficialRaceResults=false;
    Check(!ValidPhysicalWeatherInsightSettings(settings),
          "review metrics can never award official race results");
    settings={};
    settings.MaximumFrameGapSeconds=
        std::numeric_limits<double>::quiet_NaN();
    Check(!ValidPhysicalWeatherInsightSettings(settings),
          "nonfinite sampling gap rejected");
    Check(PhysicalWeatherRiskTrend(0.1,0.8)==
            PhysicalWeatherInsightTrend::SightDeteriorating &&
          PhysicalWeatherRiskTrend(0.8,0.1)==
            PhysicalWeatherInsightTrend::SightImproving &&
          PhysicalWeatherRiskTrend(0.8,0.8)==
            PhysicalWeatherInsightTrend::Unchanged,
          "source risk trend retains distinct direction and deadband");

    unsigned tracks=0u;
    for(unsigned circuitIndex=0u;
        circuitIndex<WorldScaleVariantCount;++circuitIndex)
    {
        const auto circuit=WorldScaleOriginalVariantRecipes[
            circuitIndex];
        PhysicalRaceWeatherEventJournal journal;
        Check(!BuildPhysicalWeatherRaceInsight(journal).Valid,
              "no real source observations means no artificial review");
        for(std::uint64_t f=1u;f<=5u;++f)
        {
            const double now=1.0+(f-1u)*0.05;
            const auto wet=Night(circuit.Id,f,now,0.8,0.5);
            const auto pit=Pit(f,now);
            const auto receipt=journal.Observe(wet,pit);
            Check(receipt.Accepted,
                  "short genuine source session accepted");
        }
        auto first=BuildPhysicalWeatherRaceInsight(journal);
        Check(first.Valid &&
              first.Fault==PhysicalWeatherInsightFault::None &&
              first.Circuit==circuit.Id &&
              first.Session==777u &&
              first.FirstNativeFrame==1u &&
              first.LastNativeFrame==5u &&
              first.QualifiedCars==6u &&
              first.QualifiedEvents==5u &&
              Near(first.TotalObservedSeconds,0.2) &&
              Near(first.MeanFleetRiskRatio,0.8) &&
              Near(first.PeakFleetRiskRatio,0.8) &&
              Near(first.MaximumObservedWaterMm,0.5) &&
              first.LowestSightReserveCm>0.0 &&
              first.MinimumMeasuredLux==4.0 &&
              Near(first.ProportionCarsCautioned,0.0) &&
              first.TotalLowLightVehicleSeconds>0.0 &&
              first.TotalCriticalVehicleSeconds==0.0 &&
              first.TotalWetRoadVehicleSeconds==0.0 &&
              first.NoOfficialResultIssued &&
              first.NoNativeCommandIssued,
              "time-weighted mean and wet/light thresholds remain physical");
        for(const auto& c:first.Cars)
            Check(c.Valid &&
                  c.Actor>=900u &&
                  c.Actor<=905u &&
                  c.QualifiedSamples==5u &&
                  c.CautionSamples==0u &&
                  Near(c.MeanRiskRatio,0.8) &&
                  Near(c.MeasuredExposureSeconds,0.2) &&
                  c.LowLightSamples==5u &&
                  c.WetRoadSamples==0u &&
                  c.Trend==PhysicalWeatherInsightTrend::Unchanged &&
                  c.NoActualBrakingChanges,
                  "per-car risk metrics cannot invent a braking command");

        for(std::uint64_t f=6u;f<=10u;++f)
        {
            const double now=1.0+(f-1u)*0.05;
            const double risk=1.55;
            const auto wet=Night(circuit.Id,f,now,risk,3.0);
            const auto pit=Pit(f,now,WetPitDecision::WaitForTraffic);
            Check(journal.Observe(wet,pit).Accepted,
                  "real severe weather and traffic frames remain auditable");
        }
        const auto final=BuildPhysicalWeatherRaceInsight(journal);
        Check(final.Valid &&
              final.QualifiedEvents==10u &&
              final.QualifiedCars==6u &&
              Near(final.TotalObservedSeconds,0.45) &&
              final.MeanFleetRiskRatio>first.MeanFleetRiskRatio &&
              final.PeakFleetRiskRatio>first.PeakFleetRiskRatio &&
              final.TotalCautionVehicleSeconds>0.0 &&
              final.TotalWetRoadVehicleSeconds>0.0 &&
              final.TotalLowLightVehicleSeconds>0.0 &&
              final.PitWaitEventCount>=1u &&
              final.MaximumObservedWaterMm==3.0 &&
              final.LowestSightReserveCm<first.LowestSightReserveCm &&
              final.LastVerifiedDigest==journal.CurrentDigest(),
              "weather analytics aggregate genuine new evidence only");
        for(const auto& c:final.Cars)
            Check(c.Valid &&
                  c.MaximumRiskRatio>=1.5 &&
                  c.CautionSamples>=5u &&
                  c.WetRoadSamples>=5u &&
                  c.LargestObservedRatioChange>0.0 &&
                  c.Trend==PhysicalWeatherInsightTrend::SightDeteriorating &&
                  c.CautionExposureSeconds>0.0 &&
                  c.WetRoadExposureSeconds>0.0,
                  "all six observed car conditions worsened coherently");

        const auto comparison=ComparePhysicalWeatherInsights(
            first,final);
        Check(comparison.Valid &&
              comparison.Session==777u &&
              comparison.EarlierLastFrame==5u &&
              comparison.LaterLastFrame==10u &&
              comparison.MeanFleetRiskChange>0.0 &&
              comparison.SourceWetnessChangeMm>0.0 &&
              comparison.MinimumSightReserveChangeCm<0.0 &&
              comparison.DeterioratingCars==6u &&
              comparison.ImprovingCars==0u &&
              comparison.Trend==
                  PhysicalWeatherInsightTrend::SightDeteriorating &&
              comparison.SourceOnly &&
              comparison.NoRaceProgressionChange,
              "review comparison labels only measured deterioration");
        Check(!ComparePhysicalWeatherInsights(final,first).Valid,
              "cannot compare time backwards and call it a new review");
        PhysicalWeatherEvent item;
        Check(journal.Read(9u,item) &&
              item.Valid &&
              PhysicalWeatherInsightValidEvent(item,{}),
              "canonical digest corroborates the last original source");
        item.Cars[1].TrackLux+=1.0;
        Check(!PhysicalWeatherInsightValidEvent(item,{}),
              "tampering one optical sample invalidates source digest");
        item.Digest=PhysicalWeatherEventDigest(item);
        item.Cars[1].Actor=item.Cars[0].Actor;
        item.Digest=PhysicalWeatherEventDigest(item);
        Check(!PhysicalWeatherInsightValidEvent(item,{}),
              "duplicate car identity cannot pass even with recomputed hash");
        ++tracks;
    }
    Check(tracks==WorldScaleVariantCount,
          "six distinct original courses passed complete racing review QA");
    std::cout<<"PhysicalRaceWeatherInsight: six circuits passed\n";
    return EXIT_SUCCESS;
}
