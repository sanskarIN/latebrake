#include "AI/FleetPlayerTraffic.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;

void Require(bool value, const char* message)
{
    if (!value)
    {
        std::cerr << "FleetPlayerTraffic: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double a, double b)
{
    return std::abs(a - b) <= 1.0e-8;
}

struct Fixture final
{
    SixCarIdentity Ids{100,101,102,103,104,105};
    FleetPhysicalFrame Frame;
    Fixture()
    {
        Frame.AttemptToken = 1;
        Frame.FrameToken = 1;
        Frame.Count = StartGridCapacity;
        Frame.Cars[0] = {true,0,Ids[0],{{0,0},0},0,0};
        for (int slot=1;slot<StartGridCapacity;++slot)
            Move(slot, 3000.0+500.0*slot, 3000.0, 0, 0);
    }

    void Move(int slot, double x, double y, double vx, double vy, double yaw=0)
    {
        Frame.Cars[static_cast<std::size_t>(slot)] =
            {true,slot,Ids[static_cast<std::size_t>(slot)],{{x,y},yaw},vx,vy};
    }

    FleetPlayerTrafficReadout Read() const
    {
        return MeasureFleetPlayerTraffic(Frame,Ids);
    }

    FleetPlayerTrafficSample Sample(std::uint64_t frame, double clock,
                                   double delta=0.05, std::uint64_t epoch=1) const
    {
        FleetPlayerTrafficSample out;
        out.Physical=Frame;
        out.Physical.FrameToken=frame;
        out.Identity=Ids;
        out.EngineFrame=100+frame;
        out.SessionEpoch=epoch;
        out.ActiveSeconds=clock;
        out.AdvancedSeconds=delta;
        return out;
    }
};

void DirectionAndRotation()
{
    Require(FleetTrafficDirection(1,0)==FleetPlayerTrafficDirection::Ahead,"ahead");
    Require(FleetTrafficDirection(1,1)==FleetPlayerTrafficDirection::AheadRight,"front right");
    Require(FleetTrafficDirection(0,1)==FleetPlayerTrafficDirection::Right,"right");
    Require(FleetTrafficDirection(-1,1)==FleetPlayerTrafficDirection::BehindRight,"rear right");
    Require(FleetTrafficDirection(-1,0)==FleetPlayerTrafficDirection::Behind,"behind");
    Require(FleetTrafficDirection(-1,-1)==FleetPlayerTrafficDirection::BehindLeft,"rear left");
    Require(FleetTrafficDirection(0,-1)==FleetPlayerTrafficDirection::Left,"left");
    Require(FleetTrafficDirection(1,-1)==FleetPlayerTrafficDirection::AheadLeft,"front left");
    Require(FleetTrafficDirection(0,0)==FleetPlayerTrafficDirection::SameCentre,"coincident centres");
    Require(FleetTrafficDirection(std::numeric_limits<double>::quiet_NaN(),0)==
        FleetPlayerTrafficDirection::Unavailable,"invalid direction");
    Fixture f;
    f.Move(0,0,0,0,0,90);
    f.Move(1,0,800,0,0,90);
    f.Move(2,-800,0,0,0,90);
    auto r=f.Read();
    Require(r.Valid && r.Rivals[0].Direction==FleetPlayerTrafficDirection::Ahead &&
        r.Rivals[1].Direction==FleetPlayerTrafficDirection::Right,"rotated player basis");
    f.Move(0,0,0,0,0,-90);
    r=f.Read();
    Require(r.Rivals[0].Direction==FleetPlayerTrafficDirection::Behind &&
        r.Rivals[1].Direction==FleetPlayerTrafficDirection::Left,"opposite heading");
}

void AllDirectionsAndThreatPriority()
{
    Fixture f;
    auto r=f.Read();
    Require(r.Valid && r.WarningCount==0 && r.StrongestSlot==-1 &&
        r.NearestSlot==1,"clear nearby context, no invented alert");
    f.Move(1,0,800,0,0);
    f.Move(2,-1500,0,1200,0);
    f.Move(3,100,0,0,0);
    r=f.Read();
    Require(r.Valid && r.NearbyCount==2 && r.WarningCount==3,
        "all five observations considered");
    Require(r.Rivals[0].Severity==FleetPlayerTrafficSeverity::Nearby &&
        r.Rivals[1].Severity==FleetPlayerTrafficSeverity::ProjectedOverlap &&
        r.Rivals[1].Direction==FleetPlayerTrafficDirection::Behind &&
        r.Rivals[1].RadialClosingCmS==1200.0,"rear predicted path remains visible");
    Require(r.Rivals[2].Severity==FleetPlayerTrafficSeverity::PaddedOverlap &&
        r.StrongestSlot==3 && r.PaddedOverlapCount==1,"current padded overlap priority");
    f.Move(3,3500,3000,0,0);
    r=f.Read();
    Require(r.StrongestSlot==2 && r.ProjectedOverlapCount==1 &&
        r.Rivals[1].ForecastSeconds>0.0 &&
        r.Rivals[1].ForecastSeconds<=FleetForecastHorizonSeconds,"earliest rear projection");
    f.Move(2,-1500,0,-1200,0);
    r=f.Read();
    Require(r.Rivals[1].Severity==FleetPlayerTrafficSeverity::Clear &&
        r.Rivals[1].RadialClosingCmS<0.0 && r.StrongestSlot==1,
        "moving away cannot create approach alert");
    f.Move(4,1600,0,-250,0);
    r=f.Read();
    Require(r.Rivals[3].Severity==FleetPlayerTrafficSeverity::Approaching &&
        r.StrongestSlot==4,"measured approach outside immediate forecast");
}

void ExactTieAndEarliestForecast()
{
    Fixture f;
    f.Move(1,0,700,0,0);
    f.Move(2,0,-700,0,0);
    auto r=f.Read();
    Require(r.NearestSlot==1 && r.StrongestSlot==1,"stable slot tie");
    f.Move(1,1700,0,-1000,0);
    f.Move(2,-1200,0,600,0);
    f.Move(3,-1000,0,1000,0);
    r=f.Read();
    Require(r.ProjectedOverlapCount==3 && r.StrongestSlot==3,
        "earliest projected path wins independent of array order");
    const auto& a=r.Rivals[0];
    const auto& b=r.Rivals[1];
    Require(a.ForecastSeconds>b.ForecastSeconds,"fixture forecast order");
    Require(PreferFleetPlayerTrafficRow(b,a) &&
        !PreferFleetPlayerTrafficRow(a,b),"earliest path preferred");
}

void MalformedRoster()
{
    Fixture f;
    f.Frame.Inactive=true;
    Require(!f.Read().Valid,"inactive no advice");
    f.Frame.Inactive=false;
    f.Frame.Count=5;
    Require(!f.Read().Valid,"partial fleet");
    f.Frame.Count=6;
    f.Frame.AttemptToken=0;
    Require(!f.Read().Valid,"uninitialized attempt");
    f.Frame.AttemptToken=1;
    f.Frame.FrameToken=0;
    Require(!f.Read().Valid,"uninitialized frame");
    f.Frame.FrameToken=1;
    f.Frame.Cars[5].ActorToken=f.Ids[4];
    Require(!f.Read().Valid,"duplicate roster");
    f.Frame.Cars[5].ActorToken=f.Ids[5];
    f.Frame.Cars[5].VelocityY=std::numeric_limits<double>::infinity();
    Require(!f.Read().Valid,"nonfinite fifth rival fails whole snapshot");
    f.Frame.Cars[5].VelocityY=15001;
    Require(!f.Read().Valid,"implausible speed rejected");
    f.Frame.Cars[5].VelocityY=0;
    f.Frame.Cars[5].Pose.Position.X=100001;
    Require(!f.Read().Valid,"implausible position rejected");
    f.Frame.Cars[5].Pose.Position.X=5000;
    auto other=f.Ids;other[0]=999;
    Require(!MeasureFleetPlayerTraffic(f.Frame,other).Valid,"wrong bound generation");
    Require(f.Read().Valid,"fresh valid roster restored");
}

void JournalContinuityAndUnion()
{
    Fixture f;
    f.Move(1,0,800,0,0);
    f.Move(2,0,-800,0,0);
    FleetPlayerTrafficJournal journal;
    Require(!journal.Observe(f.Sample(1,0.05)),"unconfigured");
    Require(journal.Configure(1,f.Ids),"configure");
    Require(!journal.Configure(2,f.Ids),"no silent rebind");
    Require(journal.Observe(f.Sample(1,0.05)),"first endpoint");
    auto r=journal.Readout();
    Require(r.HasSamples && r.AcceptedSamples==1 && r.ObservedSeconds==0.0 &&
        r.MaximumConcurrentWarnings==2 && r.MinimumCentreDistanceCm==800.0 &&
        r.NearestObservedSlot==1,"no first-frame extrapolation");
    Require(journal.Observe(f.Sample(2,0.10)),"second endpoint");
    r=journal.Readout();
    Require(r.ContiguousIntervals==1 && Near(r.ObservedSeconds,0.05) &&
        Near(r.AnyNearbySeconds,0.05) && Near(r.AnyWarningSeconds,0.05) &&
        Near(r.Rivals[0].NearbySeconds,0.05) && Near(r.Rivals[1].NearbySeconds,0.05),
        "union counted once, each physical car tracked");
    Require(!journal.Observe(f.Sample(2,0.10)),"duplicate cannot add time");
    Require(journal.Observe(f.Sample(3,0.15)),"fresh after duplicate");
    Require(Near(journal.Readout().ObservedSeconds,0.05),"duplicate breaks interval");
    Require(journal.Observe(f.Sample(4,0.20)),"next contiguous");
    Require(Near(journal.Readout().ObservedSeconds,0.10),"next valid interval");
    journal.Suspend();
    Require(journal.Observe(f.Sample(5,0.25)),"fresh resume");
    Require(Near(journal.Readout().ObservedSeconds,0.10),"pause cannot bridge");
    Require(journal.Observe(f.Sample(6,0.30,0.05,2)),"changed session accepted as endpoint");
    Require(Near(journal.Readout().ObservedSeconds,0.10),"epoch change cannot bridge");
    auto teleport=f.Sample(7,0.35,0.05,2);
    teleport.TeleportRevisions[5]=1;
    Require(journal.Observe(teleport),"changed rival teleport accepted as endpoint");
    Require(Near(journal.Readout().ObservedSeconds,0.10),"rival recovery breaks interval");
    auto next=f.Sample(8,0.40,0.05,2);
    next.TeleportRevisions[5]=1;
    Require(journal.Observe(next),"new stable continuity");
    Require(Near(journal.Readout().ObservedSeconds,0.15),"post recovery intervals qualify");
    next=f.Sample(10,0.50,0.05,2);next.TeleportRevisions[5]=1;
    Require(journal.Observe(next),"skipped frame accepted endpoint");
    Require(Near(journal.Readout().ObservedSeconds,0.15),"skipped frame not extrapolated");
    next=f.Sample(11,0.60,0.05,2);next.TeleportRevisions[5]=1;
    Require(journal.Observe(next),"clock discrepancy accepted as endpoint");
    Require(Near(journal.Readout().ObservedSeconds,0.15),"clock delta mismatch breaks interval");
    next=f.Sample(12,0.65,0.05,2);next.TeleportRevisions[5]=1;
    next.EngineFrame+=4;
    Require(journal.Observe(next),"engine hitch endpoint");
    Require(Near(journal.Readout().ObservedSeconds,0.15),"engine frame discontinuity");
    Require(!journal.Observe(f.Sample(13,0.70,0.20,2)),"oversized delta rejected");
    Require(Near(journal.Readout().ObservedSeconds,0.15),"invalid frame leaves history");
    journal.Reset();
    Require(!journal.Readout().HasSamples && !journal.Observe(f.Sample(14,0.75)),
        "restart clears history");
}

void EndpointTransitionsAndForeignCalls()
{
    Fixture f;
    FleetPlayerTrafficJournal journal;
    auto duplicate=f.Ids;duplicate[5]=duplicate[0];
    Require(!journal.Configure(1,duplicate),"duplicate config");
    Require(journal.Configure(1,f.Ids),"real config");
    f.Move(1,100,0,0,0);
    Require(journal.Observe(f.Sample(1,0.05)),"first overlap");
    f.Move(1,4000,3000,0,0);
    f.Move(2,100,0,0,0);
    Require(journal.Observe(f.Sample(2,0.10)),"different rival overlap");
    auto r=journal.Readout();
    Require(Near(r.ObservedSeconds,0.05) && r.AnyPaddedOverlapSeconds==0.0 &&
        r.AnyProjectedSeconds==0.0 && r.AnyWarningSeconds==0.0,
        "different rivals at endpoints never invent sustained overlap");
    auto foreign=f.Sample(3,0.15);
    foreign.Physical.AttemptToken=2;
    Require(!journal.Observe(foreign),"foreign attempt ignored");
    foreign=f.Sample(3,0.15);foreign.Identity[2]=999;
    Require(!journal.Observe(foreign),"foreign roster ignored");
    Require(journal.Observe(f.Sample(3,0.15)),"current attempt still continuous");
    r=journal.Readout();
    Require(Near(r.ObservedSeconds,0.10) && Near(r.AnyPaddedOverlapSeconds,0.05) &&
        Near(r.Rivals[1].PaddedOverlapSeconds,0.05) &&
        r.Rivals[0].PaddedOverlapSeconds==0.0,"only matching physical slot accumulates");
    f.Frame.Cars[4].Verified=false;
    Require(!journal.Observe(f.Sample(4,0.20)),"malformed current sample");
    f.Frame.Cars[4].Verified=true;
    Require(journal.Observe(f.Sample(5,0.25)),"resume valid roster");
    Require(Near(journal.Readout().ObservedSeconds,0.10),"invalid roster cannot bridge");
}

void RandomizedFrameAndHistoryInvariants()
{
    Fixture f;
    FleetPlayerTrafficJournal journal;
    Require(journal.Configure(1,f.Ids),"sweep configure");
    std::mt19937 rng(48682);
    std::uniform_real_distribution<double> position(-3000.0,3000.0);
    std::uniform_real_distribution<double> velocity(-1800.0,1800.0);
    std::uniform_real_distribution<double> heading(-360.0,360.0);
    for (std::uint64_t frame=1;frame<=4096;++frame)
    {
        for (int slot=0;slot<StartGridCapacity;++slot)
            f.Move(slot,position(rng),position(rng),velocity(rng),velocity(rng),heading(rng));
        f.Frame.FrameToken=frame;
        const auto read=f.Read();
        Require(read.Valid && read.NearestSlot>=1 && read.NearestSlot<=5,
            "valid finite roster");
        int warnings=0, nearby=0, projections=0, overlaps=0;
        for (const auto& row:read.Rivals)
        {
            Require(row.Slot>=1 && row.Slot<=5 &&
                std::isfinite(row.CentreDistanceCm) && row.CentreDistanceCm>=0,
                "finite physical row");
            if (IsFleetPlayerTrafficWarning(row.Severity)) ++warnings;
            if (row.Nearby) ++nearby;
            if (row.ForecastOverlap) ++projections;
            if (row.PaddedOverlap) ++overlaps;
            if (row.ForecastOverlap)
                Require(row.ForecastSeconds>=0.0 && row.ForecastSeconds<=FleetForecastHorizonSeconds,
                    "bounded forecast");
            if (read.StrongestSlot>0)
                Require(!PreferFleetPlayerTrafficRow(row,
                    read.Rivals[static_cast<std::size_t>(read.StrongestSlot-1)]),
                    "selected warning dominates all rivals");
        }
        Require(warnings==read.WarningCount && nearby==read.NearbyCount &&
            projections==read.ProjectedOverlapCount && overlaps==read.PaddedOverlapCount,
            "exact counts");
        Require((warnings==0)==(read.StrongestSlot==-1),"no phantom alert");
        Require(journal.Observe(f.Sample(frame,frame*0.05)),"sweep sample");
        const auto summary=journal.Readout();
        Require(summary.AcceptedSamples==frame &&
            summary.ContiguousIntervals==frame-1 &&
            Near(summary.ObservedSeconds,(frame-1)*0.05),"continuous clock accounting");
        Require(summary.AnyNearbySeconds<=summary.ObservedSeconds+1e-8 &&
            summary.AnyWarningSeconds<=summary.ObservedSeconds+1e-8 &&
            summary.AnyProjectedSeconds<=summary.AnyWarningSeconds+1e-8 &&
            summary.AnyPaddedOverlapSeconds<=summary.AnyProjectedSeconds+1e-8 &&
            summary.MaximumConcurrentWarnings<=5,"bounded observed union");
        for (const auto& slot:summary.Rivals)
            Require(slot.NearbySeconds<=summary.AnyNearbySeconds+1e-8 &&
                slot.WarningSeconds<=summary.AnyWarningSeconds+1e-8 &&
                slot.ProjectedSeconds<=summary.AnyProjectedSeconds+1e-8 &&
                slot.PaddedOverlapSeconds<=summary.AnyPaddedOverlapSeconds+1e-8,
                "per-car intervals are bounded by fleet union");
    }
}

} // namespace

int main()
{
    DirectionAndRotation();
    AllDirectionsAndThreatPriority();
    ExactTieAndEarliestForecast();
    MalformedRoster();
    JournalContinuityAndUnion();
    EndpointTransitionsAndForeignCalls();
    RandomizedFrameAndHistoryInvariants();
    std::cout<<"Fleet player proximity, prediction and observation journal passed\n";
}
