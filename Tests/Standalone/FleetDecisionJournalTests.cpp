#include "AI/FleetDecisionJournal.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;

void Require(bool value,const char* why)
{
    if(!value)
    {
        std::cerr<<"FleetDecisionJournal: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b,double epsilon=1e-8){return std::abs(a-b)<=epsilon;}

FleetDecisionSample Request(int slot=1)
{
    FleetDecisionSample s;
    s.Slot=slot;
    s.Actor=100+static_cast<std::uint64_t>(slot);
    s.GateOrdinal=0;
    s.SpeedCmS=1200;
    s.RoadQualified=true;
    s.TrafficObserved=true;
    s.TrafficQualified=true;
    s.Risk=FleetRiskKind::Clear;
    s.Stages.fill({0.8,0.1,false,false});
    return s;
}

void BrakeAt(FleetDecisionSample& s,FleetDecisionStage stage)
{
    for(std::size_t i=FleetDecisionIndex(stage);i<s.Stages.size();++i)
    {
        s.Stages[i].Throttle=0;
        s.Stages[i].Brake=true;
    }
}

struct Fixture final
{
    FleetDecisionJournal Journal;
    FleetDecisionIdentity Identity{100,101,102,103,104,105};
    FleetDecisionAdmission Admission{1,1,101,1,0.05,0.05};
    std::array<FleetDecisionRouteEvidence,StartGridCapacity-1> Evidence{};
    Fixture()
    {
        Require(Journal.Configure(1,Identity,12),"configure");
        for(auto& e:Evidence)e.Verified=true;
    }
    FleetDecisionSample Bind(FleetDecisionSample s) const
    {
        s.Attempt=Admission.Attempt;s.Frame=Admission.Frame;
        s.EngineFrame=Admission.EngineFrame;s.Epoch=Admission.Epoch;
        return s;
    }
    void Open(){Require(Journal.BeginFrame(Admission),"admit frame");}
    void Add(FleetDecisionSample s){Require(Journal.Observe(Bind(s)),"accept native command sample");}
    void Next()
    {
        ++Admission.Frame;++Admission.EngineFrame;
        Admission.ClockSeconds+=Admission.AdvancedSeconds;
    }
    FleetDecisionReadout Read(bool finished=false) const
    {
        return Journal.Read(1,Identity,Admission.ClockSeconds,Admission.Frame,finished,Evidence);
    }
};

void AnalyzeExactStageEffects()
{
    auto s=Request();
    s.Set(FleetDecisionStage::Corner,{0.5,0.1,false,false});
    s.Set(FleetDecisionStage::Difficulty,{0.58,0.1,false,false});
    s.Set(FleetDecisionStage::Grip,{0.35,0.1,false,false});
    s.Set(FleetDecisionStage::SurfaceDamage,{0.35,0.1,false,false});
    s.Set(FleetDecisionStage::EngineHeat,{0.3,0.1,false,false});
    s.Set(FleetDecisionStage::HumanFollowing,{0.3,0.1,false,false});
    s.Set(FleetDecisionStage::Passing,{0.3,0.4,false,false});
    s.Set(FleetDecisionStage::ImmediateTraffic,{0.2,0.4,false,false});
    s.Set(FleetDecisionStage::PredictiveTraffic,{0.0,0.1,true,false});
    s.Set(FleetDecisionStage::Yaw,{0.0,0.05,true,false});
    s.Set(FleetDecisionStage::Spin,{0.0,0.05,true,false});
    s.Risk=FleetRiskKind::SweptContact;
    s.BrakeHold=true;
    s.LimitingSlot=2;
    const auto e=AnalyzeFleetDecision(s);
    Require(e.Valid && e.Final.Brake && e.Final.Throttle==0 &&
        e.BrakeOrigin==FleetDecisionStage::PredictiveTraffic &&
        e.LargestReduction==FleetDecisionStage::Corner &&
        Near(e.LargestThrottleReduction,0.3),"actual stage order and final brake");
    Require(e.Effects[FleetDecisionIndex(FleetDecisionStage::Difficulty)].ThrottleRaised,
        "difficulty increase not labelled restriction");
    Require(e.Effects[FleetDecisionIndex(FleetDecisionStage::Passing)].SteeringChanged &&
        e.Effects[FleetDecisionIndex(FleetDecisionStage::Yaw)].SteeringChanged,
        "actual steer changes");
    Require(!e.Effects[FleetDecisionIndex(FleetDecisionStage::SurfaceDamage)].ThrottleReduced,
        "unchanged stage no invented restriction");
    Require(e.Effects[FleetDecisionIndex(FleetDecisionStage::PredictiveTraffic)].BrakeAsserted,
        "actual brake assertion");

    s=Request();
    BrakeAt(s,FleetDecisionStage::Route);
    for(std::size_t i=FleetDecisionIndex(FleetDecisionStage::Grip);i<s.Stages.size();++i)
        s.Stages[i]={0.5,0.1,false,false};
    BrakeAt(s,FleetDecisionStage::Spin);
    const auto last=AnalyzeFleetDecision(s);
    Require(last.Valid && last.BrakeOrigin==FleetDecisionStage::Spin &&
        last.Effects[FleetDecisionIndex(FleetDecisionStage::Grip)].BrakeReleased,
        "later reasserted brake owns final origin");
}

void InvalidSamplesAndIdentity()
{
    auto s=Request();
    const auto reject=[&](FleetDecisionSample bad){Require(!AnalyzeFleetDecision(bad).Valid,"invalid sample");};
    auto bad=s;bad.Actor=0;reject(bad);
    bad=s;bad.Slot=0;reject(bad);
    bad=s;bad.Slot=6;reject(bad);
    bad=s;bad.GateOrdinal=-1;reject(bad);
    bad=s;bad.GateOrdinal=25600;reject(bad);
    bad=s;bad.SpeedCmS=std::numeric_limits<double>::quiet_NaN();reject(bad);
    bad=s;bad.SpeedCmS=15001;reject(bad);
    bad=s;bad.Stages[3].Throttle=1.1;reject(bad);
    bad=s;bad.Stages[11].Steering=std::numeric_limits<double>::infinity();reject(bad);
    bad=s;bad.Risk=static_cast<FleetRiskKind>(99);reject(bad);
    bad=s;bad.Passing=static_cast<FleetPassPhase>(99);reject(bad);
    bad=s;bad.Queue=static_cast<FleetQueueStatus>(99);reject(bad);
    bad=s;bad.LimitingSlot=1;reject(bad);
    bad=s;bad.OffroadBlockers=6;reject(bad);
    bad=s;bad.TrafficObserved=false;reject(bad);
    bad=s;bad.TrafficQualified=false;reject(bad);
    bad=s;bad.Risk=FleetRiskKind::Unqualified;reject(bad);
    bad=s;bad.TrafficObserved=false;bad.TrafficQualified=false;bad.Risk=FleetRiskKind::Unqualified;
    Require(AnalyzeFleetDecision(bad).Valid,"explicit absent traffic evidence");
    bad.BrakeHold=true;reject(bad);

    FleetDecisionJournal journal;
    FleetDecisionIdentity ids{100,101,102,103,104,105};
    auto malformed=ids;malformed[5]=malformed[0];
    Require(!journal.Configure(1,malformed,12) &&
        !journal.Configure(0,ids,12) && !journal.Configure(1,ids,0) &&
        !journal.Configure(1,ids,25601),"configuration bounds");
    Require(journal.Configure(1,ids,12) && !journal.Configure(1,ids,12),"one bound attempt");
}

void NativeFrameAuthority()
{
    Fixture f;
    auto s=Request();
    Require(!f.Journal.Observe(f.Bind(s)),"no frame authority");
    f.Open();
    auto bad=f.Bind(s);++bad.Attempt;
    Require(!f.Journal.Observe(bad),"foreign attempt");
    bad=f.Bind(s);++bad.Frame;
    Require(!f.Journal.Observe(bad),"foreign frame");
    bad=f.Bind(s);++bad.EngineFrame;
    Require(!f.Journal.Observe(bad),"foreign engine frame");
    bad=f.Bind(s);++bad.Epoch;
    Require(!f.Journal.Observe(bad),"foreign epoch");
    bad=f.Bind(s);bad.Actor=999;
    Require(!f.Journal.Observe(bad),"foreign actor");
    f.Add(s);
    Require(!f.Journal.Observe(f.Bind(s)),"duplicate accepted sample");
    auto r=f.Read();
    Require(r.Valid && r.ObservedRivals==1 && r.CurrentRivals==1 &&
        r.Rivals[0].History.Samples==1,"no duplicate history");
    Require(!f.Journal.BeginFrame(f.Admission),"duplicate admission");
    Require(f.Read().CurrentRivals==0,"revoked frame not current");
    f.Next();f.Open();f.Add(s);
    Require(f.Read().Rivals[0].History.ObservedSeconds==0,"duplicate admission cuts continuity");
    f.Next();f.Open();f.Add(s);
    Require(Near(f.Read().Rivals[0].History.ObservedSeconds,0.05),"fresh interval");
    const auto h=f.Read().Rivals[0].History;
    Require(h.Samples==3 && h.Intervals==1 && h.Breaks==1,"exact continuity counters");
}

void PerStageAndEpisodeAccounting()
{
    Fixture f;
    auto s=Request();
    s.SpeedCmS=50;
    BrakeAt(s,FleetDecisionStage::PredictiveTraffic);
    s.Risk=FleetRiskKind::StoppingDistance;
    s.BrakeHold=true;
    s.Queue=FleetQueueStatus::Waiting;
    s.OffroadBlockers=1;
    for(int i=0;i<5;++i)
    {
        f.Open();f.Add(s);f.Next();
    }
    auto r=f.Read(true);
    const auto& h=r.Rivals[0].History;
    Require(h.Samples==5 && h.Intervals==4 && Near(h.ObservedSeconds,0.2),"first sample no extrapolation");
    Require(Near(h.BrakingSeconds,0.2) && Near(h.NoDriveSeconds,0.2) &&
        Near(h.StationarySeconds,0.2) && Near(h.QueueSeconds,0.2) &&
        Near(h.EmergencyForecastSeconds,0.2) && Near(h.TrafficHoldSeconds,0.2) &&
        Near(h.DegradedRoadSeconds,0.2),"independent endpoint durations");
    Require(Near(h.LongestBrake.Duration(),0.2) &&
        h.LongestBrake.BrakeOrigin==FleetDecisionStage::PredictiveTraffic &&
        Near(h.LongestStationary.Duration(),0.2),"continuous reason episodes");
    Require(h.DominantBrakeOrigin()==FleetDecisionStage::PredictiveTraffic &&
        Near(h.MeanSpeedKmh(),1.8) && h.MeanThrottle()==0,"sampled averages");
    const auto& stage=h.Stages[FleetDecisionIndex(FleetDecisionStage::PredictiveTraffic)];
    Require(stage.BrakeAssertionSamples==5 && stage.ThrottleReductionSamples==5 &&
        Near(stage.BrakeOriginSeconds,0.2) && Near(stage.ReductionSeconds,0.2) &&
        Near(stage.MaxReduction,0.8),"stage counts and observed duration");

    auto changed=Request();
    changed.SpeedCmS=1000;
    BrakeAt(changed,FleetDecisionStage::Corner);
    f.Open();f.Add(changed);f.Next();
    f.Open();f.Add(changed);
    r=f.Read();
    const auto& next=r.Rivals[0].History;
    Require(Near(next.BrakingSeconds,0.3) &&
        Near(next.Stages[FleetDecisionIndex(FleetDecisionStage::Corner)].BrakeOriginSeconds,0.05),
        "brake continues but reason transition not attributed across endpoints");
    Require(Near(next.LongestBrake.Duration(),0.2),"different origin no episode bridge");
    Require(next.NoDriveSeconds<next.BrakingSeconds && next.MeanSpeedKmh()>1.8,
        "motion is actually sampled");
}

void MissingFramesAndRecovery()
{
    Fixture f;
    auto s=Request();
    for(int i=0;i<3;++i){f.Open();f.Add(s);f.Next();}
    f.Open();f.Next(); // no sample this frame
    f.Open();f.Add(s);
    Require(Near(f.Read().Rivals[0].History.ObservedSeconds,0.1),"missing frame cut");
    f.Next();++s.TeleportRevision;
    f.Open();f.Add(s);
    Require(Near(f.Read().Rivals[0].History.ObservedSeconds,0.1),"teleport cut");
    f.Next();++f.Admission.Epoch;
    f.Open();f.Add(s);
    Require(Near(f.Read().Rivals[0].History.ObservedSeconds,0.1),"epoch cut");
    f.Next();f.Open();f.Add(s);
    Require(Near(f.Read().Rivals[0].History.ObservedSeconds,0.15),"resumed interval");
    f.Journal.Suspend();
    f.Next();f.Open();f.Add(s);
    Require(Near(f.Read().Rivals[0].History.ObservedSeconds,0.15),"explicit suspension cut");
    f.Next();f.Open();
    auto invalid=f.Bind(s);invalid.Stages[0].Throttle=-0.1;
    Require(!f.Journal.Observe(invalid),"bad current command");
    Require(!f.Journal.Observe(f.Bind(s)),"no second chance same frame");
    f.Next();f.Open();f.Add(s);
    const auto h=f.Read().Rivals[0].History;
    Require(h.Refused==1 && Near(h.ObservedSeconds,0.15) &&
        h.Breaks>=5,"invalid sample cannot bridge");
}

void CurrentRowsAndIndependentRouteEvidence()
{
    Fixture f;
    f.Open();
    auto s=Request();
    BrakeAt(s,FleetDecisionStage::Corner);
    f.Add(s);
    auto p=Request(2);p.Passing=FleetPassPhase::Holding;f.Add(p);
    auto q=Request(3);q.BrakeHold=true;
    BrakeAt(q,FleetDecisionStage::PredictiveTraffic);f.Add(q);
    auto r=f.Read();
    Require(r.CurrentRivals==3 && r.CurrentBraking==2 && r.CurrentPassing==1 &&
        r.CurrentTrafficHold==1,"fresh current rows");
    f.Evidence[1].Verified=false;
    r=f.Read();
    Require(r.CurrentRivals==2 && r.ObservedRivals==2 && !r.Rivals[1].Valid,
        "unverified route hides exact slot");
    f.Evidence[1].Verified=true;
    f.Evidence[0].AcceptedGates=1;
    r=f.Read();
    Require(r.Rivals[0].Valid && !r.Rivals[0].LiveValid,
        "gate changed after command; no stale current state");
    const auto finish=f.Read(true);
    Require(finish.ObservedRivals==3 && finish.CurrentRivals==0,
        "finish retains history but no live controls");
    auto ids=f.Identity;ids[2]=999;
    Require(!f.Journal.Read(1,ids,0.05,1,false,f.Evidence).Valid,"wrong roster");
    Require(!f.Journal.Read(2,f.Identity,0.05,1,false,f.Evidence).Valid,"wrong attempt");
    f.Next();f.Open();s.GateOrdinal=1;f.Add(s);
    f.Evidence[0].AcceptedGates=0;
    Require(!f.Read().Rivals[0].Valid,"history cannot exceed actual accepted ordinal");
    f.Evidence[0].AcceptedGates=1;
    f.Next();f.Open();s.GateOrdinal=0;
    Require(!f.Journal.Observe(f.Bind(s)),"route rollback refused");
    f.Journal.Reset();
    Require(!f.Read().Valid,"restart clears evidence");
}

void RandomizedCoverageAndInvariants()
{
    Fixture f;
    std::mt19937 rng(83171);
    std::array<std::uint64_t,StartGridCapacity-1> accepted{};
    std::array<std::uint64_t,StartGridCapacity-1> revisions{};
    for(int frame=0;frame<4000;++frame)
    {
        f.Open();
        for(int slot=1;slot<StartGridCapacity;++slot)
        {
            const auto i=static_cast<std::size_t>(slot-1);
            if(rng()%13==0) continue;
            auto s=Request(slot);
            s.TeleportRevision=revisions[i];
            s.SpeedCmS=static_cast<double>(rng()%3000);
            if(rng()%3==0)
                BrakeAt(s,static_cast<FleetDecisionStage>(rng()%FleetDecisionStageCount));
            if(rng()%9==0)s.Queue=FleetQueueStatus::Waiting;
            if(rng()%11==0)s.Passing=FleetPassPhase::Holding;
            f.Add(s);++accepted[i];
        }
        const auto r=f.Read();
        for(std::size_t i=0;i<r.Rivals.size();++i)
        {
            const auto& row=r.Rivals[i];
            if(!row.Valid) continue;
            const auto& h=row.History;
            Require(h.Samples==accepted[i] && h.Intervals<h.Samples &&
                h.ObservedSeconds<=f.Admission.ClockSeconds+1e-6,
                "time and sample coverage bounded");
            Require(h.NoDriveSeconds<=h.StationarySeconds+1e-8 &&
                h.BrakingSeconds<=h.ObservedSeconds+1e-8 &&
                h.QueueSeconds<=h.ObservedSeconds+1e-8 &&
                h.PassingSeconds<=h.ObservedSeconds+1e-8 &&
                h.MeanThrottle()>=0 && h.MeanThrottle()<=1.0+1e-8 &&
                h.MeanSpeedKmh()<=h.MaxSpeedCmS*0.036+1e-8,
                "observed conditional duration and averages");
            double attribution=0;
            for(const auto& stage:h.Stages)
            {
                attribution+=stage.BrakeOriginSeconds;
                Require(stage.ReductionSeconds<=h.ObservedSeconds+1e-8 &&
                    stage.SteeringChangeSeconds<=h.ObservedSeconds+1e-8,
                    "stage intervals bounded");
            }
            Require(attribution<=h.BrakingSeconds+1e-8,"brake origins do not double count");
        }
        f.Next();
        if(rng()%37==0) ++revisions[rng()%revisions.size()];
        if(rng()%103==0) ++f.Admission.Epoch;
    }
}
}

int main()
{
    AnalyzeExactStageEffects();
    InvalidSamplesAndIdentity();
    NativeFrameAuthority();
    PerStageAndEpisodeAccounting();
    MissingFramesAndRecovery();
    CurrentRowsAndIndependentRouteEvidence();
    RandomizedCoverageAndInvariants();
    std::cout<<"Fleet AI command-stage observation and continuity tests passed\n";
}
