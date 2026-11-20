#include "Track/PhysicalWaterFrameArchive.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Check(bool ok,const char* msg){
    if(!ok){std::cerr<<"PhysicalWaterFrameArchive FAIL: "<<msg<<'\n';std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b,double epsilon=1.0e-7){
    return std::abs(a-b)<=epsilon;
}
latebrake::ObservedWaterResult Measured(
    latebrake::WorldScaleVariantId circuit,
    std::uint64_t frame,double clock,double film)
{
    using namespace latebrake;
    ObservedWaterResult out;
    out.Valid=true;
    out.Fault=ObservedWaterFault::None;
    out.Circuit=circuit;
    out.Session=70u;
    out.Revision=4u;
    out.SourceFrame=frame;
    out.ClockSeconds=clock;
    out.StepSeconds=0.05;
    out.CellLengthCm=100.0;
    out.RenderAdvisoryOnly=true;
    out.OriginalRaceUnaffected=true;
    out.FilmMm.fill(film);
    out.Budget.MassResidualMm=0.0;
    out.Budget.MeanFilmMm=film;
    return out;
}
}
int main(){
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    auto settings=PhysicalWaterArchiveSettings{};
    Check(ValidPhysicalWaterArchiveSettings(settings),
        "finite fixed ring and observed rain budgets valid");
    settings.NeverInterpolateForPhysics=false;
    Check(!ValidPhysicalWaterArchiveSettings(settings),
        "archive display cannot be used to set Chaos contact grip");
    settings={};
    settings.MaximumSensorChangeSlackMm=nan;
    Check(!ValidPhysicalWaterArchiveSettings(settings),
        "nonfinite weather measurement tolerance rejected");
    settings={};
    unsigned qualified=0u;
    for(unsigned k=0u;k<WorldScaleVariantCount;++k){
        const auto circuit=WorldScaleOriginalVariantRecipes[k].Id;
        PhysicalWaterFrameArchive tape;
        Check(tape.Count()==0u &&
              !tape.HasPlayback() &&
              !tape.DisplayAt(0.0).Valid,
              "blank evidence ring never fabricates a film");
        auto source=Measured(circuit,1u,0.05,1.01);
        Check(ValidPhysicalWaterArchiveFrame(source),
              "original measured source frame accepted");
        auto receipt=tape.Record(source);
        Check(receipt.Accepted &&
              receipt.Fault==PhysicalWaterArchiveFault::None &&
              receipt.RetainedFrames==1u &&
              receipt.SourceFrame==1u &&
              receipt.NoLiveWeatherChanges &&
              !tape.HasPlayback(),
              "first authentic film snapshot creates source baseline");
        source=Measured(circuit,2u,0.10,1.02);
        receipt=tape.Record(source);
        Check(receipt.Accepted && tape.HasPlayback() &&
              tape.Count()==2u,
              "two true source frames can be interpolated visually");
        const auto between=tape.DisplayAt(0.075);
        Check(between.Valid &&
              between.Session==70u &&
              between.Revision==4u &&
              between.LowerFrame==1u &&
              between.UpperFrame==2u &&
              Near(between.InterpolationFraction,0.5) &&
              Near(between.MeanFilmMm,1.015) &&
              Near(between.MinimumFilmMm,1.015) &&
              Near(between.MaximumFilmMm,1.015) &&
              between.WetSections==ObservedWaterCellCount &&
              between.SourceOnly &&
              between.VisualInterpolationOnly &&
              between.NoOfficialPhysicalForceAuthority,
              "visual film interpolation explicitly has no force authority");
        Check(!tape.DisplayAt(0.04).Valid &&
              !tape.DisplayAt(0.11).Valid &&
              !tape.DisplayAt(nan).Valid,
              "past/future or nonfinite time cannot be guessed");
        source=Measured(circuit,3u,0.15,1.03);
        source.FilmMm[33]=4.0;
        receipt=tape.Record(source);
        Check(!receipt.Accepted &&
              receipt.Fault==
                  PhysicalWaterArchiveFault::ImpossibleWaterChange &&
              tape.Count()==2u,
              "single source cell impossible jump cannot spoof weather");
        source=Measured(circuit,3u,0.15,1.03);
        source.Revision=5u;
        receipt=tape.Record(source);
        Check(!receipt.Accepted &&
              receipt.Fault==PhysicalWaterArchiveFault::WrongSession,
              "weather session restart cannot inherit prior physical proof");
        source=Measured(circuit,3u,0.15,1.03);
        source.ClockSeconds=0.09;
        receipt=tape.Record(source);
        Check(!receipt.Accepted &&
              receipt.Fault==PhysicalWaterArchiveFault::TimeRollback,
              "replayed or rewound source clock discarded");
        source=Measured(circuit,15u,0.75,1.15);
        receipt=tape.Record(source);
        Check(!receipt.Accepted &&
              receipt.Fault==PhysicalWaterArchiveFault::FrameGap,
              "large unobserved source gap cannot interpolate rain");
        source=Measured(circuit,3u,0.15,1.03);
        Check(tape.Record(source).Accepted,
              "a failed frame does not contaminate later good samples");

        for(std::uint64_t frame=4u;frame<=50u;++frame){
            const auto next=Measured(circuit,frame,
                frame*0.05,1.0+frame*0.01);
            const auto result=tape.Record(next);
            Check(result.Accepted &&
                  result.RetainedFrames<=PhysicalWaterArchiveCapacity &&
                  result.LargestObservedCellChangeMm<=0.35 &&
                  result.NoLiveWeatherChanges,
                  "fixed water ring accepts bounded witnessed sequence");
        }
        Check(tape.Count()==PhysicalWaterArchiveCapacity &&
              Near(tape.LatestClock(),2.50) &&
              Near(tape.EarliestClock(),0.95),
              "32-frame ring drops oldest source evidence after wrap");
        ObservedWaterResult oldest;
        ObservedWaterResult newest;
        Check(tape.TryRead(0u,oldest) &&
              tape.TryRead(tape.Count()-1u,newest) &&
              oldest.SourceFrame==19u &&
              newest.SourceFrame==50u &&
              !tape.TryRead(tape.Count(),newest) &&
              newest.SourceFrame==0u,
              "chronological access always bounds oldest/newest and clears stale reads");
        Check(!tape.DisplayAt(0.94).Valid &&
              tape.DisplayAt(0.95).Valid &&
              tape.DisplayAt(2.475).Valid &&
              tape.DisplayAt(2.50).Valid &&
              !tape.DisplayAt(2.51).Valid,
              "binary search remains valid after ring rollover");
        auto wrong=Measured(circuit,51u,2.55,1.51);
        wrong.Circuit=WorldScaleOriginalVariantRecipes[
            (k+1u)%WorldScaleVariantCount].Id;
        Check(!tape.Record(wrong).Accepted &&
              tape.LastRejection()==PhysicalWaterArchiveFault::WrongRoad,
              "foreign circuit cannot borrow weather source frames");
        wrong=Measured(circuit,51u,2.55,1.51);
        wrong.FilmMm[5]=nan;
        Check(!tape.Record(wrong).Accepted &&
              tape.LastRejection()==PhysicalWaterArchiveFault::InvalidFrame,
              "nonfinite road-water value fails before ring mutation");
        tape.Reset();
        Check(tape.Count()==0u &&
              !tape.HasPlayback() &&
              tape.LastRejection()==PhysicalWaterArchiveFault::None &&
              !tape.DisplayAt(2.5).Valid,
              "native session reset removes old source water history");
        ++qualified;
    }
    Check(qualified==WorldScaleVariantCount,
        "all six original circuit water tapes independently qualified");
    std::cout<<"PhysicalWaterFrameArchive: six original courses passed\n";
    return EXIT_SUCCESS;
}
