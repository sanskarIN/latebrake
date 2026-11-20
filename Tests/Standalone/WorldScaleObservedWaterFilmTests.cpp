#include "Track/WorldScaleObservedWaterFilm.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Require(bool ok,const char* why) {
    if(!ok){std::cerr<<"ObservedWaterFilm FAIL: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b,double epsilon=1.0e-6) {
    return std::abs(a-b)<=epsilon;
}
latebrake::ObservedWaterFrame Snapshot(
    const latebrake::WorldScaleVariantRecipe& circuit,
    double film,double rain,
    std::uint64_t sourceFrame=1u,double clock=1.0)
{
    using namespace latebrake;
    ObservedWaterFrame f;
    f.Circuit=circuit.Id;
    f.Session=75u;
    f.Revision=3u;
    f.SourceFrame=sourceFrame;
    f.ClockSeconds=clock;
    f.DeltaSeconds=0.05;
    f.AllCellsMeasured=true;
    f.TrustedEngineWeatherSource=true;
    f.CollisionRoadUnchanged=true;
    const double step=circuit.LapLengthCm/
        static_cast<double>(ObservedWaterCellCount);
    for(std::size_t i=0;i<ObservedWaterCellCount;++i) {
        auto& c=f.Cells[i];
        c.StationCm=step*i;
        c.RainMmH=rain;
        c.MeasuredFilmMm=film;
        c.MeasurementClockSeconds=clock;
        c.Confidence=1.0;
        c.RoadComponent=i+1u;
        c.OriginalCollisionSource=true;
        c.ActualRainMeasured=true;
        c.ActualFilmMeasured=true;
    }
    return f;
}
}
int main() {
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    auto settings=ObservedWaterSettings{};
    Require(ValidObservedWaterSettings(settings),"default conservative weather settings");
    settings.NeverChangeChaosFriction=false;
    Require(!ValidObservedWaterSettings(settings),"source-only model cannot own physics");
    settings={};
    settings.AlongRoadFlowPerSecond=nan;
    Require(!ValidObservedWaterSettings(settings),"nonfinite lateral exchange rejected");
    settings={};
    unsigned qualified=0;
    for(unsigned index=0;index<WorldScaleVariantCount;++index) {
        const auto circuit=WorldScaleOriginalVariantRecipes[index];
        auto frame=Snapshot(circuit,2.0,90.0);
        Require(ValidObservedWaterFrame(circuit,frame),
            "all 256 original road weather measurements required");
        const auto result=StepObservedWaterFilm(circuit,frame);
        Require(result.Valid &&
            result.Fault==ObservedWaterFault::None &&
            result.Circuit==circuit.Id &&
            result.Session==frame.Session &&
            result.SourceFrame==frame.SourceFrame &&
            result.Budget.InitialFilmMm==512.0 &&
            result.Budget.RainCells==ObservedWaterCellCount &&
            result.Budget.WetCells==ObservedWaterCellCount &&
            result.Budget.MaximumFilmMm>=result.Budget.MinimumFilmMm &&
            result.Budget.NoActualPhysicsChange &&
            result.OriginalRaceUnaffected,
            "each road has finite source-only mass conserving water plan");
        const double expectedRain=ObservedWaterCellCount*90.0*
            frame.DeltaSeconds/3600.0;
        Require(Near(result.Budget.RainInMm,expectedRain) &&
            std::abs(result.Budget.MassResidualMm)<1.0e-6 &&
            result.Budget.DrainedMm>0.0 &&
            result.Budget.EvaporationMm>0.0 &&
            Near(result.Budget.MaximumExchangeMm,0.0),
            "uniform film receives rain and loses only recorded sinks");
        const auto start=SampleObservedWaterFilm(circuit,result,0.0);
        const auto seam=SampleObservedWaterFilm(
            circuit,result,circuit.LapLengthCm);
        const auto negative=SampleObservedWaterFilm(
            circuit,result,-circuit.LapLengthCm);
        Require(start.Valid && seam.Valid && negative.Valid &&
            Near(start.FilmMm,seam.FilmMm) &&
            Near(start.FilmMm,negative.FilmMm) &&
            Near(start.StationCm,seam.StationCm) &&
            start.NoChaosGripAuthority,
            "water observations wrap signed lap boundary without grip authority");
        Require(!SampleObservedWaterFilm(circuit,result,nan).Valid,
            "nonfinite water lookup must fail closed");

        frame=Snapshot(circuit,0.0,0.0);
        auto dry=StepObservedWaterFilm(circuit,frame);
        Require(dry.Valid && Near(dry.Budget.InitialFilmMm,0.0) &&
            Near(dry.Budget.FinalFilmMm,0.0) &&
            dry.Budget.WetCells==0u &&
            dry.Budget.RainCells==0u,
            "dry physical road remains dry without fabricated rain");
        frame=Snapshot(circuit,0.0,160.0);
        auto rainy=StepObservedWaterFilm(circuit,frame);
        Require(rainy.Valid &&
            rainy.Budget.RainInMm>0.0 &&
            rainy.Budget.FinalFilmMm>0.0 &&
            rainy.Budget.RainCells==ObservedWaterCellCount,
            "measured rain is a source and not a temperature guess");

        frame=Snapshot(circuit,20.0,160.0);
        auto saturated=StepObservedWaterFilm(circuit,frame);
        Require(saturated.Valid &&
            saturated.Budget.FinalFilmMm<=
                ObservedWaterCellCount*ObservedWaterMaximumMm+1e-6 &&
            std::abs(saturated.Budget.MassResidualMm)<1e-6,
            "saturation never creates or loses unaccounted water");

        frame=Snapshot(circuit,1.0,0.0);
        frame.Cells[0].MeasuredFilmMm=12.0;
        auto asymmetric=StepObservedWaterFilm(circuit,frame);
        Require(asymmetric.Valid &&
            asymmetric.Budget.MaximumExchangeMm>0.0 &&
            asymmetric.FilmMm[1]>asymmetric.FilmMm[2] &&
            asymmetric.FilmMm[ObservedWaterCellCount-1u]>
                asymmetric.FilmMm[ObservedWaterCellCount-2u] &&
            std::abs(asymmetric.Budget.MassResidualMm)<1e-6,
            "diffusion transfers equal and opposite water through lap seam");

        auto nextFrame=Snapshot(circuit,1.0,0.0,2u,1.05);
        const auto next=StepObservedWaterFilm(circuit,nextFrame);
        Require(next.Valid && ObservedWaterContinuity(result,next),
            "next authentic rain frame preserves session continuity");
        nextFrame.Revision=4u;
        const auto altered=StepObservedWaterFilm(circuit,nextFrame);
        Require(altered.Valid && !ObservedWaterContinuity(result,altered),
            "new official session revision rejects stale water history");

        auto missing=Snapshot(circuit,1.0,0.0);
        missing.Cells[33].ActualRainMeasured=false;
        Require(!StepObservedWaterFilm(circuit,missing).Valid,
            "missing rain probe never assumes dry or wet");
        missing=Snapshot(circuit,1.0,0.0);
        missing.Cells[33].MeasuredFilmMm=nan;
        Require(!StepObservedWaterFilm(circuit,missing).Valid,
            "nonfinite measured water cannot enter conservation solver");
        missing=Snapshot(circuit,1.0,0.0);
        missing.Cells[33].StationCm+=4.0;
        Require(!StepObservedWaterFilm(circuit,missing).Valid,
            "station source mismatch must not corrupt physical route");
        missing=Snapshot(circuit,1.0,0.0);
        // Clock 1.0 and measurement 0.0 is only one second old:
        // increase the source clock so one probe genuinely exceeds
        // the documented two-second maximum age.
        missing=Snapshot(circuit,1.0,0.0,1u,5.0);
        missing.Cells[33].MeasurementClockSeconds=1.0;
        Require(!StepObservedWaterFilm(circuit,missing).Valid,
            "stale weather measurements cannot generate current race advice");
        missing=Snapshot(circuit,1.0,0.0);
        missing.TrustedEngineWeatherSource=false;
        Require(!StepObservedWaterFilm(circuit,missing).Valid,
            "no native weather witness means no rain model result");
        ++qualified;
    }
    Require(qualified==WorldScaleVariantCount,
        "all six independent course-water budgets have been audited");
    std::cout<<"WorldScaleObservedWaterFilm: six circuits passed\n";
    return EXIT_SUCCESS;
}
