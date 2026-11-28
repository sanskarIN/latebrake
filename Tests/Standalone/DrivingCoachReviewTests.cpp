#include "Racing/DrivingCoachReview.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;

void Assert(bool yes,const char* reason)
{
    if(!yes)
    {
        std::cerr<<"DrivingCoachReview: "<<reason<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double a,double b)
{
    return std::abs(a-b)<=1e-6*std::max({1.0,std::abs(a),std::abs(b)});
}

void Feed(DrivingCoachAccumulator& accumulator,double seconds,double speed,
    bool brake=false,double lateral=0.0)
{
    const int count=static_cast<int>(std::round(seconds/0.05));
    for(int i=0;i<count;++i)
        Assert(accumulator.Observe({0.05,speed,lateral,brake}),
            "fixture observation accepted");
}

void BasicReviewAndEmptyRejection()
{
    DrivingCoachAccumulator c;
    Assert(!BuildDrivingCoachReview(c.Review()).Valid,
        "no observations yield no descriptive output");
    Feed(c,3.0,1200.0);
    const auto observed=c.Review();
    Assert(ValidDrivingCoachReadout(observed),
        "all raw aggregates match independent readout invariants");
    const auto review=BuildDrivingCoachReview(observed);
    Assert(review.Valid && review.AdvisoryOnly &&
        review.Evidence==DrivingCoachEvidence::Limited &&
        review.Trend==DrivingCoachPaceTrend::Unavailable,
        "short source can yield descriptive summary without claiming trend");
    Assert(review.DominantSpeedBand==1 &&
        Near(review.DominantSpeedSeconds,3.0) &&
        Near(review.MeanObservedSpeedKmh,43.2),
        "dominant speed band uses actual time weighting");
    Assert(!review.HasLongestBrake && !review.HasLongestSlip &&
        review.AvailableCompleteWindows==0,
        "empty episode categories remain explicitly unavailable");
}

void CompleteWindowPaceTrend()
{
    DrivingCoachAccumulator c;
    Feed(c,10.0,1000.0);
    Feed(c,10.0,2000.0);
    auto review=BuildDrivingCoachReview(c.Review());
    Assert(review.Valid && review.AvailableCompleteWindows==4 &&
        review.Trend==DrivingCoachPaceTrend::FasterRecentSamples,
        "observed 10s recent period faster than observed early period");
    Assert(review.Early.Valid && review.Recent.Valid &&
        review.SameContinuityPeriod,
        "contiguous windows qualify continuous descriptive comparison");
    Assert(Near(review.Early.MeanSpeedKmh,36.0) &&
        Near(review.Recent.MeanSpeedKmh,72.0) &&
        Near(review.EarlyToRecentSpeedKmh,36.0) &&
        Near(review.EarlyToRecentSpeedFraction,1.0),
        "comparison uses elapsed-time-weighted speeds, not count of samples");
    Feed(c,10.0,1000.0);
    review=BuildDrivingCoachReview(c.Review());
    Assert(review.Trend==DrivingCoachPaceTrend::NearlyEqual,
        "bounded recent/early windows show equal first and last period");
}

void WindowClaimsAndGaps()
{
    DrivingCoachAccumulator c;
    Feed(c,10.0,1000.0);
    c.Suspend();
    Feed(c,10.0,2000.0);
    auto review=BuildDrivingCoachReview(c.Review());
    Assert(review.Valid && review.Trend==DrivingCoachPaceTrend::Unavailable &&
        !review.SameContinuityPeriod,
        "cross-pause pace comparison refuses misleading continuity claims");
    // The raw event stream uses distinct sequence numbers across suspensions;
    // the reviewer cannot infer that wall time was observed during the pause.
    Assert(review.Early.ObservedSeconds>0.0 && review.Recent.ObservedSeconds>0.0,
        "phase windows contain accepted elapsed sample time only");

    const auto original=c.Review();
    auto corrupt=original;
    corrupt.Windows[0].Sequence=corrupt.Windows[1].Sequence;
    Assert(!ValidDrivingCoachReadout(corrupt),
        "duplicate source window identity rejected");
    corrupt=original;
    corrupt.Windows[0].ContinuityEpoch=0;
    Assert(!ValidDrivingCoachReadout(corrupt),
        "zero source continuity epoch rejected");
    corrupt=original;
    corrupt.Windows[0].StartedSeconds=-1.0;
    Assert(!ValidDrivingCoachReadout(corrupt),
        "negative source clock refused");
    corrupt=original;
    corrupt.Windows[0].EndedSeconds=std::numeric_limits<double>::infinity();
    Assert(!ValidDrivingCoachReadout(corrupt),
        "infinite source clock refused");
    corrupt=original;
    corrupt.Windows[1].EndedSeconds=corrupt.Windows[0].StartedSeconds+2.0;
    Assert(!ValidDrivingCoachReadout(corrupt),
        "overlapping different samples cannot be presented as disjoint windows");
}

void PhaseBoundsAndPartialExclusion()
{
    DrivingCoachAccumulator c;
    Feed(c,20.0,2000.0,true);
    Feed(c,2.0,1000.0,false);
    auto observations=c.Review();
    Assert(observations.RetainedWindows==5 &&
        !observations.Windows[0].Complete,
        "two-second tail is incomplete evidence");
    Assert(!SummarizeDrivingCoachPhase(observations,0,1).Valid,
        "cannot turn partial window into a qualified phase");
    Assert(!SummarizeDrivingCoachPhase(observations,4,2).Valid,
        "out-of-bounds range is invalid, not silently truncated");
    const auto phase=SummarizeDrivingCoachPhase(observations,1,3);
    Assert(phase.Valid && phase.WindowCount==3 &&
        Near(phase.ObservedSeconds,15.0) && Near(phase.MeanSpeedKmh,72.0) &&
        Near(phase.BrakeFraction,1.0),
        "explicit phase summary respects complete sampled windows");
    Assert(phase.LastSequence>phase.FirstSequence,
        "phase source sequence bounds remain ordered");
    const auto review=BuildDrivingCoachReview(observations);
    Assert(review.Valid && review.AvailableCompleteWindows==4 &&
        review.Trend==DrivingCoachPaceTrend::NearlyEqual,
        "incomplete tail excluded from first-versus-recent trend");
}

void EpisodeReviewAndValidation()
{
    DrivingCoachAccumulator c;
    Feed(c,0.2,2000.0,true,800.0);
    Feed(c,0.1,1600.0,false);
    Feed(c,0.8,1500.0,true,700.0);
    Feed(c,0.1,1200.0,false);
    const auto readout=c.Review();
    Assert(ValidDrivingCoachReadout(readout),
        "multiple episode types retain valid sampled evidence");
    const auto report=BuildDrivingCoachReview(readout);
    Assert(report.HasLongestBrake && report.HasLongestSlip &&
        Near(report.LongestBrake.DurationSeconds,0.8) &&
        Near(report.LongestSlip.DurationSeconds,0.8),
        "longest retained observed episodes win without fabricated braking distance");
    Assert(report.RetainedBrakeEpisodeCount==2 &&
        report.RetainedSlipEpisodeCount==2,
        "event counts are bounded retained histories, not all-time counts");
    auto corrupt=readout;
    corrupt.Brakes[0].DurationSeconds=-0.1;
    Assert(!ValidDrivingCoachReadout(corrupt),
        "negative episode duration rejected");
    corrupt=readout;
    corrupt.Brakes[0].EndedSeconds=corrupt.Brakes[0].StartedSeconds;
    Assert(!ValidDrivingCoachReadout(corrupt),
        "zero-duration episode rejected");
    corrupt=readout;
    corrupt.Brakes[0].MinimumSpeedKmh=400.0;
    Assert(!ValidDrivingCoachReadout(corrupt),
        "minimum speed larger than episode entry/exit rejected");
    corrupt=readout;
    corrupt.Slips[0].DistanceKm=999.0;
    Assert(!ValidDrivingCoachReadout(corrupt),
        "impossible slip-episode distance rejected");
}

void TamperAndAuthorityChecks()
{
    DrivingCoachAccumulator c;
    Feed(c,10.0,2500.0,true,900.0);
    const auto source=c.Review();
    Assert(ValidDrivingCoachReadout(source),"reference review is internally coherent");
    auto mutate=[&](auto change)
    {
        auto copy=source;
        change(copy);
        Assert(!ValidDrivingCoachReadout(copy),
            "tampered untrusted review cannot be advertised as valid");
        Assert(!BuildDrivingCoachReview(copy).Valid,
            "untrusted review cannot be converted to player-facing summary");
    };
    mutate([](auto& r){r.NoGameplayAuthority=false;});
    mutate([](auto& r){r.NoPersistentData=false;});
    mutate([](auto& r){r.NoOfficialTiming=false;});
    mutate([](auto& r){r.AcceptedSamples=0;});
    mutate([](auto& r){r.AcceptedSamples+=1;});
    mutate([](auto& r){r.SpeedBands[0].Seconds=4.0;});
    mutate([](auto& r){r.SpeedBands[0].Samples=7;});
    mutate([](auto& r){r.SpeedBands[0].LowKmh=-3.0;});
    mutate([](auto& r){r.SpeedBands[0].HighKmh=400.0;});
    mutate([](auto& r){r.BrakeFraction=4.0;});
    mutate([](auto& r){r.HighSlipFraction=-1.0;});
    mutate([](auto& r){r.MeanSpeedKmh=r.PeakSpeedKmh+20.0;});
    mutate([](auto& r){r.RetainedWindows=DrivingCoachWindowCapacity+1;});
    mutate([](auto& r){r.RetainedBrakeEpisodes=DrivingCoachEpisodeCapacity+1;});
    mutate([](auto& r){r.RetainedSlipEpisodes=DrivingCoachEpisodeCapacity+1;});
    mutate([](auto& r){r.CompletedBrakeEpisodes=r.AcceptedBrakeEpisodes+1;});
    mutate([](auto& r){r.CompletedSlipEpisodes=r.AcceptedSlipEpisodes+1;});
}

void SeededReviewInvariants()
{
    std::mt19937_64 gen(0xD1A915CAFEULL);
    std::uniform_real_distribution<double> speed(0.0,10000.0);
    std::uniform_real_distribution<double> lateral(-9000.0,9000.0);
    std::uniform_real_distribution<double> delta(0.025,0.1);
    DrivingCoachAccumulator coach;
    for(int i=0;i<20000;++i)
    {
        if((i%173)==0)coach.Suspend();
        coach.Observe({delta(gen),speed(gen),lateral(gen),(i%7)==0});
        if((i%151)==0)
        {
            const auto raw=coach.Review();
            Assert(ValidDrivingCoachReadout(raw),
                "every accepted random prefix is structurally consistent");
            const auto review=BuildDrivingCoachReview(raw);
            Assert(review.Valid && review.AdvisoryOnly &&
                review.RetainedBrakeEpisodeCount<=DrivingCoachEpisodeCapacity &&
                review.RetainedSlipEpisodeCount<=DrivingCoachEpisodeCapacity,
                "review bounded regardless of accepted frame count");
            if(review.Recent.Valid)
            {
                Assert(review.Early.Valid &&
                    review.Early.WindowCount==review.Recent.WindowCount &&
                    review.Early.WindowCount>=2,
                    "two real comparison cohorts required");
                Assert(review.Trend!=DrivingCoachPaceTrend::Unavailable,
                    "qualified cohorts assign one descriptive trend");
            }
        }
    }
}

void EventLabels()
{
    for(const auto value:{DrivingCoachPaceTrend::Unavailable,
        DrivingCoachPaceTrend::NearlyEqual,
        DrivingCoachPaceTrend::FasterRecentSamples,
        DrivingCoachPaceTrend::SlowerRecentSamples})
        Assert(DrivingCoachPaceTrendLabel(value)[0]!='\0',
            "every portable pace advisory has a visible label");
}
}

int main()
{
    BasicReviewAndEmptyRejection();
    CompleteWindowPaceTrend();
    WindowClaimsAndGaps();
    PhaseBoundsAndPartialExclusion();
    EpisodeReviewAndValidation();
    TamperAndAuthorityChecks();
    SeededReviewInvariants();
    EventLabels();
    std::cout<<"DrivingCoachReview: all source-only review invariants passed.\n";
    return EXIT_SUCCESS;
}
