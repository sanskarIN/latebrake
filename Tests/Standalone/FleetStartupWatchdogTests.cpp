#include "AI/FleetStartupWatchdog.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake;
void Require(bool value,const char* message)
{
    if(!value){std::cerr<<message<<'\n';std::exit(EXIT_FAILURE);}
}
FleetStartupSample Sample(std::uint64_t attempt,std::uint64_t frame,double dt=0.05)
{
    FleetStartupSample s;
    s.Attempt=attempt;s.Frame=frame;s.SessionEpoch=1;s.DeltaSeconds=dt;
    s.Active=true;s.CircuitReady=true;s.LocalDriverReady=true;
    s.PlacementReady=true;s.GridAccepted=true;
    s.AttemptState.AttemptToken=attempt;s.AttemptState.LastFrame=frame;
    s.AttemptState.Required=true;s.AttemptState.CountdownAllowed=true;
    s.AttemptState.Phase=FleetAttemptPhase::CountdownQualified;
    s.AttemptState.Grid={LiveGridFailure::None,-1,-1};
    return s;
}
void StableCountdown()
{
    for(double dt:{0.025,0.05,0.1})
    {
        FleetStartupWatchdog w;RaceStartLights lights;
        Require(w.Begin(1),"new attempt");
        std::uint64_t frame=0;
        bool gotPermission=false;
        for(int i=0;i<160;++i)
        {
            const auto s=Sample(1,++frame,dt);
            const auto v=w.Observe(s);
            if(v.CountdownAllowed)
            {
                Require(v.StableSeconds+1e-9>=FleetStartupStableSeconds,
                    "minimum contiguous readiness before countdown");
                Require(w.IsCurrentPermission(1,frame,1),"exact frame authorization");
                Require(!w.IsCurrentPermission(1,frame-1,1),"old frame denied");
                Require(!w.IsCurrentPermission(1,frame,2),"old session denied");
                gotPermission=true;
            }
            lights.Advance(dt,v.CountdownAllowed);
            if(lights.IsGreen()) break;
        }
        Require(gotPermission && lights.IsGreen(),"ready grid reaches genuine GO");
        Require(w.Readout().ActiveSeconds>=3.19 &&
            w.Readout().ActiveSeconds<=3.41,"countdown includes stability time");
    }
}
void Interruptions()
{
    FleetStartupWatchdog w;RaceStartLights lights;
    Require(w.Begin(2),"second attempt");
    for(std::uint64_t i=1;i<=20;++i)
    {
        auto v=w.Observe(Sample(2,i));
        lights.Advance(0.05,v.CountdownAllowed);
    }
    Require(lights.SecondsUntilGreen()<3.0,"red countdown advanced");
    auto loss=Sample(2,21);
    loss.GridAccepted=false;
    loss.AttemptState.Grid={LiveGridFailure::CarOverlap,4,5};
    auto v=w.Observe(loss);
    Require(v.ResetCountdown && !v.CountdownAllowed && v.StableSeconds==0.0,
        "unsafe grid breaks entire partial countdown");
    Require(v.Grid.Slot==4 && v.Grid.OtherSlot==5,"specific blocking slot preserved");
    if(v.ResetCountdown)lights.Reset();
    Require(lights.SecondsUntilGreen()==3.0,"adapter restart remains red");
    const double before=v.ActiveSeconds;
    for(std::uint64_t i=22;i<122;++i)
    {
        auto pause=Sample(2,i);pause.Active=false;
        v=w.Observe(pause);
        Require(!v.CountdownAllowed && !v.ResetCountdown &&
            v.ActiveSeconds==before,"pause neither spends budget nor repeats reset");
    }
    auto resume=Sample(2,122);resume.SessionEpoch=2;
    v=w.Observe(resume);
    Require(v.Phase==FleetStartupPhase::Suspended && v.ActiveSeconds==before,
        "epoch transition rebases without time");
    resume.Frame=123;resume.AttemptState.LastFrame=123;
    Require(w.Observe(resume).Phase==FleetStartupPhase::Settling,
        "first fresh frame starts stability window");
    auto stale=resume;
    Require(!w.Observe(stale).CountdownAllowed,"duplicate frame refused");
    Require(!w.Begin(2),"same attempt cannot reset watchdog");
    Require(w.Begin(3),"new attempt resets state");
    Require(w.Readout().ActiveSeconds==0.0 &&
        w.Readout().RemainingSeconds==FleetStartupBudgetSeconds,"clean restart");
}
void BudgetAndTerminal()
{
    FleetStartupWatchdog w;
    Require(w.Begin(9),"new attempt");
    for(std::uint64_t frame=1;frame<=150;++frame)
    {
        auto missing=Sample(9,frame,0.1);missing.LocalDriverReady=false;
        auto v=w.Observe(missing);
        Require(!v.CountdownAllowed,"missing player never authorizes start");
        if(frame<150)Require(v.Cause==FleetStartupCause::DriverUnavailable,"blocking cause");
    }
    auto v=w.Readout();
    Require(v.Phase==FleetStartupPhase::TimedOut &&
        v.Cause==FleetStartupCause::DriverUnavailable && v.RemainingSeconds==0.0,
        "bounded missing driver timeout");
    Require(w.Observe(Sample(9,151)).Phase==FleetStartupPhase::TimedOut,
        "late arrival cannot silently heal timeout");
    Require(w.Begin(10),"explicit restart after timeout");
    for(std::uint64_t f=1;f<=100;++f)
    {
        auto bad=Sample(10,f,0.11);
        Require(w.Observe(bad).ActiveSeconds==0.0,"hitch does not consume budget");
    }
    auto bad=Sample(10,101,std::numeric_limits<double>::quiet_NaN());
    Require(w.Observe(bad).ActiveSeconds==0.0,"NaN does not consume budget");
    bad=Sample(10,102);bad.AttemptState.LastFrame=101;
    Require(!w.Observe(bad).CountdownAllowed,"stale gate snapshot denied");
    auto fault=Sample(10,103);fault.AttemptState.Phase=FleetAttemptPhase::Faulted;
    Require(w.Observe(fault).Phase==FleetStartupPhase::Terminal,"terminal attempt revokes permission");
    Require(w.Observe(Sample(10,104)).Phase==FleetStartupPhase::Terminal,
        "terminal attempt cannot resume");
    ExperimentalFleetAttempt attempt;
    Require(attempt.Begin({true,true,false,1}),"physical attempt");
    attempt.ReportGridTimeout(2);
    Require(attempt.CanAttemptSpawn(),"foreign timeout ignored");
    attempt.ReportGridTimeout(1);
    Require(attempt.Readout().Fault==FleetAttemptFault::GridTimeout &&
        !attempt.CanAttemptSpawn(),"timeout faults real attempt");
    Require(attempt.Begin({true,true,false,2}),"physical restart");
}
void Flapping()
{
    FleetStartupWatchdog w;
    Require(w.Begin(4),"flapping attempt");
    bool anyPermission=false;
    for(std::uint64_t f=1;f<=300;++f)
    {
        auto s=Sample(4,f);
        if(f%4==0){s.GridAccepted=false;s.AttemptState.Grid={LiveGridFailure::MovingBeforeGo,2,-1};}
        anyPermission|=w.Observe(s).CountdownAllowed;
    }
    Require(!anyPermission && w.Readout().Phase==FleetStartupPhase::TimedOut,
        "flapping grid cannot accumulate nonconsecutive readiness");
    Require(w.Begin(5),"flapping reset");
    auto s=Sample(5,1);
    s.AttemptState.Phase=FleetAttemptPhase::AwaitingRoster;
    Require(w.Observe(s).Cause==FleetStartupCause::RosterUnavailable,"specific roster message");
    s=Sample(5,2);s.CircuitReady=false;
    Require(w.Observe(s).Cause==FleetStartupCause::CircuitUnavailable,"specific circuit message");
    s=Sample(5,3);s.PlacementReady=false;
    Require(w.Observe(s).Cause==FleetStartupCause::RestartPlacementUnverified,"specific placement message");
}
}
int main()
{
    StableCountdown();Interruptions();BudgetAndTerminal();Flapping();
    std::cout<<"Fleet startup watchdog and countdown continuity passed\n";
}
