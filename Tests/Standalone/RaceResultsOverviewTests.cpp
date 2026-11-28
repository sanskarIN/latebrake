#include "UI/RaceResultsOverview.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <string_view>

namespace
{
using namespace latebrake;

void Require(bool ok,const char* reason)
{
    if(!ok)
    {
        std::cerr<<"RaceResultsOverview: "<<reason<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b,double tolerance=1e-7)
{
    return std::abs(a-b)<=tolerance;
}

RaceProgress Finish()
{
    RaceProgress race(2,3);
    for(int lap=0;lap<3;++lap)
    {
        race.Advance(3.0+lap);
        Require(race.PassCheckpoint(0),"first gate");
        race.Advance(4.0);
        if(lap==2)Require(race.AddTimePenalty(2.0),"real charged fee");
        Require(race.PassCheckpoint(1),"lap completion");
    }
    Require(race.IsFinished() && CanAwardFinishedRace(race),"qualified finish");
    return race;
}

ResultsOverviewContext Context()
{
    ResultsOverviewContext context;
    context.Event=EventKind::Circuit;
    context.SavedBest=26;
    return context;
}

void RealClockAndCompletion()
{
    const auto race=Finish();
    const auto result=ReadResultsOverview(race,Context());
    Require(result.Valid && result.Outcome==ResultsOverviewOutcome::Complete &&
        result.Eligibility==FinishTimingEligibility::Eligible &&
        result.CompletedLaps==3 && result.RequiredLaps==3 && result.TimedLaps==3,
        "actual finish counters");
    Require(result.ClockValid && result.BreakdownValid && result.BestLapValid &&
        Near(result.ElapsedSeconds,26) && Near(result.ActiveSeconds,24) &&
        Near(result.PenaltySeconds,2) && Near(result.BestLapSeconds,7),
        "actual clock and lap metrics, no double-counted fee");
    Require(result.Persistence==ResultsOverviewPersistence::FirstBest &&
        Near(result.SavedBestSeconds,26),"first record from matching persisted clock");

    RaceProgress running(2,3);
    auto partial=ReadResultsOverview(running,Context());
    Require(partial.Valid && partial.Outcome==ResultsOverviewOutcome::Incomplete &&
        partial.Persistence==ResultsOverviewPersistence::NotApplicable &&
        partial.ClockValid && partial.ElapsedSeconds==0 && !partial.BestLapValid,
        "empty attempt is not a result");
    running.Advance(5);
    Require(running.PassCheckpoint(0),"first live gate");
    Require(running.AddTimePenalty(1),"live fee");
    partial=ReadResultsOverview(running,Context());
    Require(partial.Outcome==ResultsOverviewOutcome::Incomplete &&
        partial.ClockValid && Near(partial.ElapsedSeconds,6) &&
        Near(partial.ActiveSeconds,5) && Near(partial.PenaltySeconds,1),
        "incomplete elapsed is observational only");
}

void PersistenceAndModes()
{
    const auto race=Finish();
    auto context=Context();
    context.PreviousBest=30;
    auto result=ReadResultsOverview(race,context);
    Require(result.Persistence==ResultsOverviewPersistence::ImprovedBest &&
        Near(result.RecordDeltaSeconds,4),"verified improved saved time");
    context.PreviousBest=26;
    result=ReadResultsOverview(race,context);
    Require(result.Persistence==ResultsOverviewPersistence::MatchedBest,
        "matching saved personal best");
    context.PreviousBest=20;context.SavedBest=20;
    result=ReadResultsOverview(race,context);
    Require(result.Persistence==ResultsOverviewPersistence::ExistingBest &&
        Near(result.RecordDeltaSeconds,6),"slower finish retains previous record");
    context=Context();context.SavedBest=15;
    Require(ReadResultsOverview(race,context).Persistence==
        ResultsOverviewPersistence::NotVerified,"unrelated saved time cannot claim first best");
    context.SavedBest=0;
    Require(ReadResultsOverview(race,context).Persistence==
        ResultsOverviewPersistence::NotVerified,"missing persisted time");
    context=Context();context.Career=true;
    Require(ReadResultsOverview(race,context).Persistence==
        ResultsOverviewPersistence::NotVerified,"career award is not a quick-race PB");
    context=Context();context.ReadOnly=true;
    Require(ReadResultsOverview(race,context).Persistence==
        ResultsOverviewPersistence::ReadOnly,"read-only state never claims a save");
    context.SaveFailed=true;
    Require(ReadResultsOverview(race,context).Persistence==
        ResultsOverviewPersistence::SaveFailed,"actual save error surfaced");
    context=Context();context.Practice=true;
    result=ReadResultsOverview(race,context);
    Require(result.Outcome==ResultsOverviewOutcome::PracticeComplete &&
        result.Persistence==ResultsOverviewPersistence::SessionOnly &&
        result.SavedBestSeconds==0 && result.BestLapValid,"practice is not a persisted PB");
    context=Context();context.Experimental=true;
    result=ReadResultsOverview(race,context);
    Require(result.Outcome==ResultsOverviewOutcome::ExperimentalComplete &&
        result.Persistence==ResultsOverviewPersistence::SessionOnly &&
        result.RecordDeltaSeconds==0,"experimental fleet remains non-awarding");
    context.Practice=true;
    Require(!ReadResultsOverview(race,context).Valid,"conflicting modes rejected");
    context=Context();context.Event=static_cast<EventKind>(255);
    Require(!ReadResultsOverview(race,context).Valid,"unknown event rejected");
}

void NonRecordableFinishes()
{
    RaceProgress zero(1,1);
    Require(zero.PassCheckpoint(0),"zero-time gate completion");
    const auto invalid=ReadResultsOverview(zero,Context());
    Require(invalid.Valid && invalid.Outcome==ResultsOverviewOutcome::InvalidFinish &&
        invalid.Eligibility==FinishTimingEligibility::MissingActiveLapTiming &&
        invalid.Persistence==ResultsOverviewPersistence::NotApplicable &&
        !invalid.BestLapValid,"zero-time finish cannot earn result");
    RaceProgress fee(1,1);
    Require(fee.AddTimePenalty(2) && fee.PassCheckpoint(0),"fee-only finish");
    const auto penalized=ReadResultsOverview(fee,Context());
    Require(penalized.Outcome==ResultsOverviewOutcome::InvalidFinish &&
        penalized.BreakdownValid && Near(penalized.ActiveSeconds,0) &&
        Near(penalized.PenaltySeconds,2) && !penalized.BestLapValid,
        "penalty-only clock never masquerades as active timed lap");
    RaceProgress longRace(1,1);
    longRace.Advance(604801);
    Require(longRace.PassCheckpoint(0),"too-long finish");
    const auto tooLong=ReadResultsOverview(longRace,Context());
    Require(tooLong.Outcome==ResultsOverviewOutcome::InvalidFinish &&
        tooLong.Eligibility==FinishTimingEligibility::UnrecordableClock &&
        !tooLong.BestLapValid,"record maximum preserved");
    RaceProgress boundary(1,1);
    boundary.Advance(604800);
    Require(boundary.PassCheckpoint(0),"maximum recordable clock");
    Require(ReadResultsOverview(boundary,Context()).Outcome==ResultsOverviewOutcome::Complete,
        "inclusive seven-day eligibility unchanged");
}

struct View final
{
    double Width=0,Height=0;
    HudLayout Hud{};
    HudMenuVerticalPlan Vertical{};
    HudMenuReadablePanel Readable{};
    DesktopMenuMouseLayout Dock{};
    double SafeWidth=0;
    View(double w=1280,double h=720,bool touch=false):Width(w),Height(h)
    {
        Hud=CalculateHudLayout(w,h);
        Vertical=CalculateHudMenuVerticalPlan(w,h,Hud,touch);
        const auto text=CalculateHudTextBounds(w,h,Hud,touch);
        Dock=CalculateDesktopMenuMouseLayout(w,h,Hud,touch,true);
        SafeWidth=text.MaxWidthPixels;
        if(Dock.Valid)SafeWidth=std::min(SafeWidth,Dock.Left-text.Left-12*Hud.Scale);
        Readable=CalculateHudMenuReadablePanel(w,h,Hud,Vertical,SafeWidth,true);
    }
    ResultsOverviewLayout Plan(int pinned=2,bool results=true,bool touch=false) const
    {
        return CalculateResultsOverviewLayout(Hud,Vertical,Readable,SafeWidth,
            Hud.Top+pinned*28*Hud.Scale,pinned,results,touch);
    }
};

void CheckLayout(const View& v,const ResultsOverviewLayout& p,int pinned)
{
    Require(p.Valid && HudRadarContains({0,0,v.Width,v.Height},p.Panel),
        "bounded results card");
    Require(p.Panel.Width<=v.SafeWidth+1e-7 && p.Panel.X==v.Hud.Left,
        "existing left text lane");
    const int remaining=v.Vertical.MaxDetailsRows-pinned-p.ReservedRows;
    Require(remaining>=ResultsOverviewMinimumDetails &&
        p.NextDetailY>=p.Panel.Y+p.Panel.Height &&
        p.NextDetailY+(remaining-1)*28*v.Hud.Scale<=
            v.Vertical.OverflowNoticeY-1.25*28*v.Hud.Scale+1e-7,
        "preserve actual detail pager capacity");
    if(v.Dock.Valid)
        for(const auto& button:v.Dock.Buttons)
            Require(!HudRadarOverlaps(p.Panel,button.Bounds),"right navigation untouched");
    for(std::size_t i=0;i<p.Metrics.size();++i)
    {
        const auto& tile=p.Metrics[i];
        Require(HudRadarContains(p.Panel,tile.Background) &&
            HudRadarContains(tile.Background,tile.Label) &&
            HudRadarContains(tile.Background,tile.Value) &&
            !HudRadarOverlaps(tile.Label,tile.Value),"bounded metric typography");
        for(std::size_t j=0;j<i;++j)
            Require(!HudRadarOverlaps(tile.Background,p.Metrics[j].Background),
                "independent metric tiles");
    }
}

ResultsOverviewMeasurements Measures(const ResultsOverviewLayout& p)
{
    ResultsOverviewMeasurements m;
    const auto measure=[](const DesktopMenuMouseRect& r)
    {return HudRaceStatusTextSize{r.Width*0.75,r.Height*0.75};};
    m.Outcome=measure(p.Outcome);m.Persistence=measure(p.Persistence);
    m.Timing=measure(p.TimingLabel);
    for(std::size_t i=0;i<p.Metrics.size();++i)
    {
        m.Labels[i]=measure(p.Metrics[i].Label);
        m.Values[i]=measure(p.Metrics[i].Value);
    }
    return m;
}

void DesktopFallbackAndFontMeasurement()
{
    for(const auto& size:std::array<std::array<double,2>,6>{{
        {900,600},{1120,600},{1280,720},{1920,1080},{2560,1440},{3440,1440}}})
    {
        const View v(size[0],size[1]);
        CheckLayout(v,v.Plan(),2);
    }
    const View v;
    Require(!v.Plan(2,false).Valid && !v.Plan(2,true,true).Valid &&
        !View(640,480).Plan().Valid,"compact or unrelated menu fallback");
    auto bad=v;bad.SafeWidth=100;
    Require(!bad.Plan().Valid,"narrow lane");
    bad=v;bad.Vertical.MaxDetailsRows=9;
    Require(!bad.Plan().Valid,"preserve meaningful details");
    bad=v;bad.Readable.Valid=false;
    Require(!bad.Plan().Valid,"no detached card");
    const auto p=v.Plan();
    const auto good=Measures(p);
    Require(CanDrawResultsOverview(p,good),"complete measured summary");
    for(std::size_t i=0;i<p.Metrics.size();++i)
    {
        auto m=good;m.Values[i].Width=p.Metrics[i].Value.Width+1;
        Require(!CanDrawResultsOverview(p,m),"number clipping refused");
        m=good;m.Labels[i].Height=0;
        Require(!CanDrawResultsOverview(p,m),"missing label rejected");
    }
    auto m=good;m.Persistence.Width=p.Persistence.Width+1;
    Require(!CanDrawResultsOverview(p,m),"save status cannot be hidden");
    m=good;m.Outcome.Height=std::numeric_limits<double>::quiet_NaN();
    Require(!CanDrawResultsOverview(p,m),"invalid outcome metrics");
    m=good;m.Timing.Height=p.TimingLabel.Height+1;
    Require(!CanDrawResultsOverview(p,m),"timing labels fit transactionally");
}

void TimePartitionConservation()
{
    const auto p=View().Plan();
    const auto result=ReadResultsOverview(Finish(),Context());
    const auto bar=BuildResultsOverviewTimingBar(p.TimingTrack,result);
    Require(bar.Valid && Near(bar.Active.Width/p.TimingTrack.Width,24.0/26) &&
        Near(bar.Fees.Width/p.TimingTrack.Width,2.0/26),
        "active and fee portions sum to authoritative elapsed");
    for(int pct=0;pct<=100;++pct)
    {
        auto sample=result;
        sample.ElapsedSeconds=100;sample.ActiveSeconds=100-pct;sample.PenaltySeconds=pct;
        const auto b=BuildResultsOverviewTimingBar(p.TimingTrack,sample);
        Require(b.Valid && Near(b.Active.Width+b.Fees.Width,p.TimingTrack.Width) &&
            !HudRadarOverlaps(b.Active,b.Fees),"all fee fractions conserve track width");
    }
    auto bad=result;bad.ActiveSeconds+=1;
    Require(!BuildResultsOverviewTimingBar(p.TimingTrack,bad).Valid,"double-counted time refused");
    bad=result;bad.PenaltySeconds=-1;
    Require(!BuildResultsOverviewTimingBar(p.TimingTrack,bad).Valid,"negative fee rejected");
    bad=result;bad.ElapsedSeconds=0;
    Require(!BuildResultsOverviewTimingBar(p.TimingTrack,bad).Valid,"zero denominator");
    bad=result;bad.BreakdownValid=false;
    Require(!BuildResultsOverviewTimingBar(p.TimingTrack,bad).Valid,"unknown breakdown");
}

void RandomActualAttempts()
{
    std::mt19937 rng(40629);
    int visible=0;
    for(int i=0;i<5000;++i)
    {
        const View v(240+rng()%3800,240+rng()%2000);
        const auto layout=v.Plan();
        const int laps=1+static_cast<int>(rng()%8);
        RaceProgress race(2,laps);
        double active=0,fees=0;
        for(int lap=0;lap<laps;++lap)
        {
            const double a=1+static_cast<double>(rng()%100)/10;
            const double b=1+static_cast<double>(rng()%100)/10;
            race.Advance(a);Require(race.PassCheckpoint(0),"random first gate");
            race.Advance(b);active+=a+b;
            if(rng()%3==0)
            {
                const double fee=0.1+static_cast<double>(rng()%20)/10;
                const double priorElapsed=race.GetElapsedSeconds();
                const double priorFees=race.GetTotalPenaltySeconds();
                const int priorCount=race.GetTimePenaltyCount();
                if(race.AddTimePenalty(fee))
                {
                    const double charged=race.GetElapsedSeconds()-priorElapsed;
                    Require(charged>=fee && race.GetTimePenaltyCount()==priorCount+1,
                        "accepted charge represents the requested fee");
                    fees+=charged;
                }
                else
                {
                    Require(race.GetElapsedSeconds()==priorElapsed &&
                        race.GetTotalPenaltySeconds()==priorFees &&
                        race.GetTimePenaltyCount()==priorCount,
                        "unrepresentable random fee leaves race unchanged");
                }
            }
            Require(race.PassCheckpoint(1),"random second gate");
        }
        auto context=Context();context.Practice=true;
        const auto data=ReadResultsOverview(race,context);
        Require(data.Valid && data.Outcome==ResultsOverviewOutcome::PracticeComplete &&
            data.Persistence==ResultsOverviewPersistence::SessionOnly &&
            Near(data.ActiveSeconds,active) && Near(data.PenaltySeconds,fees),
            "observed random finish matches actual source clock");
        if(!layout.Valid)continue;
        ++visible;CheckLayout(v,layout,2);
        Require(CanDrawResultsOverview(layout,Measures(layout)),"bounded native measurement contract");
        Require(BuildResultsOverviewTimingBar(layout.TimingTrack,data).Valid,
            "clock partition survives real multi-lap fees");
    }
    Require(visible>2000,"useful desktop coverage");
}
} // namespace

int main()
{
    RealClockAndCompletion();PersistenceAndModes();NonRecordableFinishes();
    DesktopFallbackAndFontMeasurement();TimePartitionConservation();RandomActualAttempts();
    std::cout<<"Race results overview timing, persistence and responsive layout tests passed\n";
}
