#include "Racing/DrivingCoachFocusAreas.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <string>

namespace
{
using namespace latebrake;
void Check(bool pass,const std::string& label)
{
    if(!pass)
    {
        std::cerr << "DrivingCoachFocusAreas: " << label << '\n';
        std::exit(EXIT_FAILURE);
    }
}
void Window(DrivingCoachAccumulator& accumulator,double seconds,
    double speed,double lateral,bool brake)
{
    const int n=static_cast<int>(seconds/0.1+0.5);
    for(int i=0;i<n;++i)
        Check(accumulator.Observe({0.1,speed,lateral,brake}),
            "valid 10Hz sample");
}
void UnavailableAndUntrustworthy()
{
    Check(!BuildDrivingCoachFocusAreas({}).Valid,
        "no report from nonexistent samples");
    DrivingCoachAccumulator accumulator;
    Window(accumulator,5.0,2100.0,200.0,false);
    const auto readout=accumulator.Review();
    Check(readout.Valid,"valid observed source");
    const auto initial=BuildDrivingCoachFocusAreas(readout);
    Check(initial.Valid && initial.Count==0 && initial.EligibleWindows==1 &&
        initial.QualifyingSeconds>=4.99,"stable samples do not invent issues");
    auto tamper=readout;
    tamper.Windows[0].ObservedSeconds=100.0;
    Check(!BuildDrivingCoachFocusAreas(tamper).Valid,
        "malformed observed window fails closed");
    tamper=readout;
    tamper.NoOfficialTiming=false;
    Check(!BuildDrivingCoachFocusAreas(tamper).Valid,
        "forged timing authority withheld");
}
void SlipsAndBrakesFromActualAccumulator()
{
    DrivingCoachAccumulator a;
    Window(a,5.0,2200.0,1800.0,false);
    Window(a,5.0,1400.0,0.0,true);
    Window(a,5.0,2100.0,0.0,false);
    auto readout=a.Review();
    Check(ValidDrivingCoachReadout(readout),"all retained windows independently valid");
    const auto review=BuildDrivingCoachFocusAreas(readout);
    Check(ValidDrivingCoachFocusReview(review),"valid ordered shortlist");
    Check(review.EligibleWindows==3 && review.Count>=2,
        "multiple distinct observed focus windows");
    bool hasSlip=false,hasBraking=false;
    for(std::size_t i=0;i<review.Count;++i)
    {
        hasSlip|=review.Areas[i].Kind==DrivingCoachFocusKind::HighSampledSlip;
        hasBraking|=review.Areas[i].Kind==DrivingCoachFocusKind::LargeSampledBrakeShare;
        Check(!review.Areas[i].OfficialSector &&
            !review.Areas[i].RaceOutcomeChanged &&
            review.Areas[i].BasedOnPhysicalEvidenceOnly,
            "results have no new gameplay authority");
        if(i>0)
            Check(!DrivingCoachFocusPreferred(review.Areas[i],
                review.Areas[i-1]),"sorted severity descending");
    }
    Check(hasSlip && hasBraking,
        "braking/slip observations are classified with different meanings");
}
void FragmentAndPauseBoundaries()
{
    DrivingCoachAccumulator a;
    Window(a,2.0,800.0,0.0,false);
    a.Suspend();
    Window(a,5.0,2000.0,0.0,false);
    a.Suspend();
    Window(a,5.0,9000.0,0.0,false);
    const auto review=BuildDrivingCoachFocusAreas(a.Review());
    Check(review.Valid && review.SkippedIncompleteWindows>=1 &&
        review.EligibleWindows==2,"pause excludes incomplete fragment");
    for(std::size_t i=0;i<review.Count;++i)
        Check(review.Areas[i].Kind!=
            DrivingCoachFocusKind::ChangesBetweenAdjacentWindows,
            "no pace trend across recovery/paused epochs");
}
void DirectClassifierContract()
{
    DrivingCoachWindow window;
    window.Valid=true;
    window.Complete=true;
    window.Sequence=2;
    window.ContinuityEpoch=4;
    window.StartedSeconds=5.0;
    window.EndedSeconds=10.0;
    window.ObservedSeconds=5.0;
    window.DistanceKm=0.15;
    window.MeanSpeedKmh=108.0;
    window.Samples=50;
    window.SlipFraction=0.15;
    auto choice=ClassifyDrivingCoachWindow(window,nullptr);
    Check(choice.Valid && choice.Kind==DrivingCoachFocusKind::HighSampledSlip,
        "slip qualifies independent of price, map or race placement");
    window.SlipFraction=0.0;
    window.BrakingFraction=0.6;
    choice=ClassifyDrivingCoachWindow(window,nullptr);
    Check(choice.Valid && choice.Kind==DrivingCoachFocusKind::LargeSampledBrakeShare,
        "large measured braking share qualifies");
    window.BrakingFraction=0.0;
    window.MeanSpeedKmh=20.0;
    window.DistanceKm=20.0/3600.0*5.0;
    choice=ClassifyDrivingCoachWindow(window,nullptr);
    Check(choice.Valid && choice.Kind==DrivingCoachFocusKind::LowerObservedMeanSpeed,
        "slow measured window remains descriptive and non-authoritative");
    auto older=window;
    older.Sequence=1;
    older.StartedSeconds=0;
    older.EndedSeconds=5;
    older.MeanSpeedKmh=180;
    older.DistanceKm=0.25;
    window.MeanSpeedKmh=90;
    window.DistanceKm=0.125;
    choice=ClassifyDrivingCoachWindow(window,&older);
    Check(choice.Valid && choice.Kind==DrivingCoachFocusKind::ChangesBetweenAdjacentWindows,
        "relative adjacent window change is observed within same continuity");
    older.ContinuityEpoch=3;
    Check(!ClassifyDrivingCoachWindow(window,&older).Valid,
        "epoch mismatch prevents a fake speed contrast");
    older.ContinuityEpoch=4;
    older.Sequence=0;
    Check(!ClassifyDrivingCoachWindow(window,&older).Valid,
        "unqualified prior-window sequence rejected");
    window.Samples=19;
    Check(!ClassifyDrivingCoachWindow(window,&older).Valid,
        "low-evidence individual sample window excluded");
    window.Samples=50;
    window.ObservedSeconds=3.0;
    Check(!ClassifyDrivingCoachWindow(window,&older).Valid,
        "short individual sample window excluded");
}
void BoundedRankAndTamperCases()
{
    DrivingCoachAccumulator a;
    for(int i=0;i<26;++i)
        Window(a,5.0,1500.0+(i%4)*100.0,
            (i%3)==0 ? 1400.0:0.0, (i%5)==0);
    const auto r=BuildDrivingCoachFocusAreas(a.Review());
    Check(ValidDrivingCoachFocusReview(r),
        "retained ring has stable ranked focus shortlist");
    Check(r.Count<=DrivingCoachMaximumFocusAreas &&
        r.EligibleWindows<=DrivingCoachWindowCapacity,
        "memory bounded to latest 16 sample windows");
    for(std::size_t i=0;i<r.Count;++i)
        for(std::size_t j=0;j<i;++j)
            Check(r.Areas[i].Sequence!=r.Areas[j].Sequence,
                "one focus outcome per source window");
    auto forged=r;
    if(forged.Count>0)
    {
        forged.Areas[0].RaceOutcomeChanged=true;
        Check(!ValidDrivingCoachFocusReview(forged),
            "forged race outcome flag fails closed");
        forged=r;
        forged.Areas[0].Severity=std::numeric_limits<double>::quiet_NaN();
        Check(!ValidDrivingCoachFocusReview(forged),
            "nonfinite severity rejected");
    }
}
void FuzzWithAcceptedObservations()
{
    std::mt19937 rng{0xEC0C001};
    std::uniform_real_distribution<double> vel(0,9500), lateral(-8000,8000);
    DrivingCoachAccumulator accumulator;
    for(int i=0;i<20000;++i)
    {
        if(i%603==0) accumulator.Suspend();
        Check(accumulator.Observe({0.1,vel(rng),lateral(rng),rng()%3==0}),
            "seeded accepted sample");
        if(i%163==0)
        {
            const auto review=BuildDrivingCoachFocusAreas(accumulator.Review());
            Check(ValidDrivingCoachFocusReview(review),
                "valid focus review under changing source windows");
        }
    }
}
}
int main()
{
    UnavailableAndUntrustworthy();
    SlipsAndBrakesFromActualAccumulator();
    FragmentAndPauseBoundaries();
    DirectClassifierContract();
    BoundedRankAndTamperCases();
    FuzzWithAcceptedObservations();
    std::cout << "Driving coach sampled focus area tests passed.\n";
    return EXIT_SUCCESS;
}
