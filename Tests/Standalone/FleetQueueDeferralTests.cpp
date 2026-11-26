#include "AI/FleetQueueDeferral.h"

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
RivalFleetTrafficSample Queue()
{
    RivalFleetTrafficSample s;
    s.Self={true,1,101,{{-1000.0,-1800.0},0.0},0.0,0.0};
    s.BlockerCount=5;s.SessionQualified=true;
    s.Blockers[0]={true,0,100,{{-200.0,-1800.0},0.0},0.0,0.0};
    s.Blockers[1]={true,2,102,{{-2000.0,-2200.0},0.0},0.0,0.0};
    s.Blockers[2]={true,3,103,{{2800.0,0.0},90.0},0.0,0.0};
    s.Blockers[3]={true,4,104,{{0.0,1800.0},180.0},0.0,0.0};
    s.Blockers[4]={true,5,105,{{-2800.0,0.0},-90.0},0.0,0.0};
    return s;
}
void Evidence()
{
    const auto good=Queue();
    const auto evidence=InspectFleetQueue(good);
    Require(evidence.ValidRoster && evidence.HasQueue() &&
        evidence.BlockerSlot==0 && evidence.ForwardDistanceCm==800.0,
        "actual stationary aligned human ahead");
    auto s=good;s.Blockers[0].Pose.Position.X=-3000.0;
    Require(!InspectFleetQueue(s).HasQueue(),"rear car is not a queue");
    s=good;s.Blockers[0].Pose.Position.Y+=400.0;
    Require(!InspectFleetQueue(s).HasQueue(),"different lane not queue");
    s=good;s.Blockers[0].Pose.HeadingDegrees=180.0;
    Require(!InspectFleetQueue(s).HasQueue(),"oncoming car not queue");
    s=good;s.Blockers[0].Pose.Position.X=-650.0;
    Require(!InspectFleetQueue(s).HasQueue(),"overlapping car is a collision not queue");
    s=good;s.Blockers[0].VelocityX=251.0;
    Require(!InspectFleetQueue(s).HasQueue(),"quickly moving car not queue");
    s=good;s.Blockers[0].VelocityX=-11.0;
    Require(!InspectFleetQueue(s).HasQueue(),"reversing car not queue");
    s=good;s.Self.VelocityX=80.0;
    Require(!InspectFleetQueue(s).HasQueue(),"nonstationary self not deferred");
    s=good;s.BlockerCount=4;
    Require(!InspectFleetQueue(s).ValidRoster,"missing blocker invalidates evidence");
    s=good;s.Blockers[4].ActorToken=s.Blockers[0].ActorToken;
    Require(!InspectFleetQueue(s).ValidRoster,"duplicate token invalidates evidence");
    s=good;s.Blockers[4].Slot=s.Blockers[0].Slot;
    Require(!InspectFleetQueue(s).ValidRoster,"duplicate slot invalidates evidence");
    s=good;s.Blockers[4].VelocityX=std::numeric_limits<double>::quiet_NaN();
    Require(!InspectFleetQueue(s).ValidRoster,"unrelated malformed body invalidates evidence");
    s=good;s.Self.Pose.Position.Y=-800.0;
    Require(!InspectFleetQueue(s).HasQueue(),"off-road self not qualified");
    s=good;s.Blockers[1].Pose={{-100.0,-1800.0},0.0};
    Require(InspectFleetQueue(s).BlockerSlot==0,"nearest verified blocker wins");
    std::swap(s.Blockers[0],s.Blockers[1]);
    Require(InspectFleetQueue(s).BlockerSlot==0,"order-independent blocker selection");
}
void FiniteBudget()
{
    FleetQueueDeferral queue;
    auto s=Queue();
    for(int i=0;i<120;++i)
    {
        const auto r=queue.Observe(0,0.1,s);
        Require(r.DeferStallRecovery && r.Status==FleetQueueStatus::Waiting,
            "exact finite queue grace");
    }
    auto r=queue.Observe(0,0.1,s);
    Require(!r.DeferStallRecovery && r.Status==FleetQueueStatus::BudgetExhausted,
        "queue budget exhausted after 12 seconds");
    queue.Suspend();
    Require(queue.Observe(0,0.1,s).Status==FleetQueueStatus::BudgetExhausted,
        "pause cannot refill current route");
    Require(queue.Observe(1,0.1,s).DeferStallRecovery,"new actual gate permits new allowance");
    const auto used=queue.Readout().DeferredSeconds;
    Require(queue.Observe(0,0.1,s).Status==FleetQueueStatus::InvalidRoute,
        "backwards route cannot refill allowance");
    Require(queue.Observe(1,0.11,s).DeferredSeconds==used,"hitch does not spend allowance");
    auto missing=s;missing.SessionQualified=false;
    Require(!queue.Observe(1,0.1,missing).DeferStallRecovery &&
        queue.Readout().DeferredSeconds==used,"missing traffic revokes permission without refill");
    queue.Reset();
    Require(queue.Observe(0,0.1,s).DeferredSeconds==0.1,"explicit new attempt resets");
}
void RouteComposition()
{
    FleetQueueDeferral queue;
    RivalRouteProgress route;
    const auto sample=Queue();
    for(int i=0;i<120;++i)
    {
        const auto hold=queue.Observe(0,0.1,sample);
        auto obs=route.ObserveVerifiedGate(2500.0,0.0,0.1,false,hold.DeferStallRecovery);
        Require(!obs.NeedsRecovery && !obs.Advanced,"verified wait defers stationary stall only");
    }
    for(int i=0;i<80;++i)
    {
        const auto hold=queue.Observe(0,0.1,sample);
        const auto obs=route.ObserveVerifiedGate(2500.0,0.0,0.1,false,hold.DeferStallRecovery);
        Require(obs.NeedsRecovery==(i==79),"normal eight-second recovery after wait budget");
    }
    Require(route.ReportRecoveryResult(false),"real pending outcome accepted");
    Require(!queue.Observe(0,0.1,sample).DeferStallRecovery,
        "same gate recovery does not refill wait");
    const auto gate=route.ObserveVerifiedGate(0.0,1000.0,0.05,true,true);
    Require(gate.Advanced && route.GetNextWaypoint()==1,
        "verified forward crossing takes precedence over traffic deferral");
    Require(queue.Observe(1,0.05,sample).DeferStallRecovery,"new route segment");
    route.Reset();
    for(int i=0;i<=80;++i)
    {
        const auto obs=route.ObserveVerifiedGate(2500.0,1000.0,0.1,false,true);
        Require(obs.NeedsRecovery==(i==80),"moving no-approach stall is never deferred");
    }
    route.Reset();
    for(int i=0;i<80;++i)
    {
        const auto obs=route.ObserveVerifiedGate(2500.0,0.0,0.1,false);
        Require(obs.NeedsRecovery==(i==79),"legacy ordinary rival semantics unchanged");
    }
}
}
int main()
{
    Evidence();FiniteBudget();RouteComposition();
    std::cout<<"Fleet queue evidence and bounded recovery deferral passed\n";
}
