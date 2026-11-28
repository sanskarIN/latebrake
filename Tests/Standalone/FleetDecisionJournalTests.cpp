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

void RecordCommands(FleetDecisionSample& s,const FleetDecisionTrace::CommandsType& commands)
{
    s.Trace={};
    for(std::size_t i=0;i<commands.size();++i)
        s.Set(static_cast<FleetDecisionStage>(i),commands[i]);
}

template<class Edit>
void EditCommands(FleetDecisionSample& s,Edit edit)
{
    auto commands=s.Trace.Commands();
    edit(commands);
    RecordCommands(s,commands);
}

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
    FleetDecisionTrace::CommandsType commands{};
    commands.fill({0.8,0.1,false,false});
    RecordCommands(s,commands);
    return s;
}

void BrakeAt(FleetDecisionSample& s,FleetDecisionStage stage)
{
    EditCommands(s,[stage](auto& commands)
    {
        for(std::size_t i=FleetDecisionIndex(stage);i<commands.size();++i)
        {
            commands[i].Throttle=0;
            commands[i].Brake=true;
        }
    });
}

struct Fixture final
{
    FleetDecisionJournal Journal;
    FleetDecisionIdentity Identity{100,101,102,103,104,105};
    FleetDecisionAdmission Admission{1,1,101,1,0.05,0.05};
    std::array<FleetDecisionRouteEvidence,StartGridCapacity-1> Evidence{};
    Fixture()
    {
        Require(Journal.Configure(1,Identity,12,4),"configure");
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
    s.Trace={};
    s.Set(FleetDecisionStage::Route,{0.8,0.1,false,false});
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
    EditCommands(s,[](auto& commands)
    {
        for(std::size_t i=FleetDecisionIndex(FleetDecisionStage::Grip);i<commands.size();++i)
            commands[i]={0.5,0.1,false,false};
    });
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
    bad=s;EditCommands(bad,[](auto& c){c[3].Throttle=1.1;});reject(bad);
    bad=s;EditCommands(bad,[](auto& c){c[11].Steering=std::numeric_limits<double>::infinity();});reject(bad);
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
    Require(!journal.Configure(1,malformed,12,4) &&
        !journal.Configure(0,ids,12,4) && !journal.Configure(1,ids,0,4) &&
        !journal.Configure(1,ids,25601,4),"configuration bounds");
    Require(journal.Configure(1,ids,12,4) && !journal.Configure(1,ids,12,4),"one bound attempt");
}

void ExplicitStageTraceRequired()
{
    const auto normal=Request();
    Require(normal.Trace.Complete(),"complete ordered stages");
    auto empty=normal;empty.Trace={};
    Require(!AnalyzeFleetDecision(empty).Valid,"default commands are not observations");

    for(std::size_t omitted=0;omitted<FleetDecisionStageCount;++omitted)
    {
        auto s=normal;s.Trace={};
        for(std::size_t i=0;i<FleetDecisionStageCount;++i)
            if(i!=omitted)s.Set(static_cast<FleetDecisionStage>(i),normal.Trace.Commands()[i]);
        Require(!s.Trace.Complete() && !AnalyzeFleetDecision(s).Valid,
            "every omitted stage fails closed");
        s.Set(static_cast<FleetDecisionStage>(omitted),normal.Trace.Commands()[omitted]);
        Require(!s.Trace.Complete() || omitted==FleetDecisionStageCount-1,
            "out-of-order writes cannot repair a trace");
    }

    for(std::size_t duplicate=0;duplicate<FleetDecisionStageCount;++duplicate)
    {
        auto s=normal;s.Trace={};
        for(std::size_t i=0;i<FleetDecisionStageCount;++i)
        {
            const auto stage=static_cast<FleetDecisionStage>(i);
            s.Set(stage,normal.Trace.Commands()[i]);
            if(i==duplicate)s.Set(stage,normal.Trace.Commands()[i]);
        }
        Require(!AnalyzeFleetDecision(s).Valid,"duplicate stage invalidates entire trace");
    }
    auto invalid=normal;
    invalid.Set(static_cast<FleetDecisionStage>(255),{});
    Require(!AnalyzeFleetDecision(invalid).Valid,"out-of-range stage invalidates completed trace");
    auto zero=normal;
    EditCommands(zero,[](auto& c){c.fill({});});
    Require(AnalyzeFleetDecision(zero).Valid,"explicit zero commands remain valid");

    Fixture f;
    f.Open();f.Add(normal);f.Next();
    f.Open();f.Add(normal);f.Next();
    f.Open();
    Require(!f.Journal.Observe(f.Bind(empty)),"incomplete trace refused by journal");
    Require(!f.Journal.Observe(f.Bind(normal)),"refused frame cannot be replayed");
    Require(f.Read().CurrentRivals==0,"refused trace not exposed as live");
    f.Next();f.Open();f.Add(normal);
    const auto h=f.Read().Rivals[0].History;
    Require(h.Refused==1 && h.Samples==3 && Near(h.ObservedSeconds,0.05),
        "incomplete stage trace cuts observed duration");
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

void NoDriveContextAccounting()
{
    Fixture f;
    auto s=Request();
    s.SpeedCmS=50;
    BrakeAt(s,FleetDecisionStage::PredictiveTraffic);
    s.Risk=FleetRiskKind::SweptContact;
    s.BrakeHold=true;
    s.Queue=FleetQueueStatus::Waiting;
    auto context=[](const FleetDecisionSample& sample)
    {
        return ClassifyFleetNoDrive(AnalyzeFleetDecision(sample));
    };
    Require(context(s)==FleetNoDriveContext::TrafficEmergency,
        "forecast wins exclusive context over hold and queue");
    s.RoadQualified=false;
    Require(context(s)==FleetNoDriveContext::RoadUnavailable,"road context precedes traffic");
    s.RoadQualified=true;
    s.TrafficQualified=false;
    s.Risk=FleetRiskKind::Unqualified;
    Require(context(s)==FleetNoDriveContext::TrafficUnavailable,"unqualified forecast distinct");
    s.TrafficQualified=true;
    s.Risk=FleetRiskKind::Clear;
    Require(context(s)==FleetNoDriveContext::TrafficHold,"hold context");
    s.BrakeHold=false;
    Require(context(s)==FleetNoDriveContext::Queue,"queue context");
    s.Queue=FleetQueueStatus::Inactive;
    Require(context(s)==FleetNoDriveContext::OtherBrake,"other requested brake");
    auto low=Request();
    low.SpeedCmS=50;
    EditCommands(low,[](auto& c){c.fill({0.0,0.0,false,false});});
    Require(context(low)==FleetNoDriveContext::LowThrottle,"low throttle without brake");
    Require(context(Request())==FleetNoDriveContext::None,"moving car excluded");

    f.Open();f.Add(s);f.Next();
    f.Open();f.Add(s);f.Next();
    f.Open();f.Add(low);f.Next();
    f.Open();f.Add(low);
    auto h=f.Read().Rivals[0].History;
    const auto& buckets=h.NoDriveContexts.Seconds;
    Require(Near(h.NoDriveSeconds,0.15) &&
        Near(h.NoDriveContexts.Total(),h.NoDriveSeconds) &&
        Near(buckets[static_cast<std::size_t>(FleetNoDriveContext::OtherBrake)],0.05) &&
        Near(buckets[static_cast<std::size_t>(FleetNoDriveContext::Transition)],0.05) &&
        Near(buckets[static_cast<std::size_t>(FleetNoDriveContext::LowThrottle)],0.05),
        "exclusive endpoint attribution accounts for transitions");
    Require(Near(h.RaceWindowCoverage(f.Admission.ClockSeconds),0.75),
        "coverage does not extrapolate first endpoint");
    Require(h.RaceWindowCoverage(0)==0 && h.RaceWindowCoverage(0.1)==0 &&
        h.RaceWindowCoverage(std::numeric_limits<double>::infinity())==0,
        "invalid or stale denominator refused");
    f.Next();f.Open();f.Next();
    f.Open();f.Add(low);
    h=f.Read().Rivals[0].History;
    Require(Near(h.NoDriveContexts.Total(),0.15) &&
        Near(h.RaceWindowCoverage(f.Admission.ClockSeconds),0.5),
        "missing sample reduces coverage without inventing context");
}

void TeleportRevisionCannotRollBack()
{
    Fixture f;
    auto s=Request();
    s.TeleportRevision=4;
    f.Open();f.Add(s);f.Next();
    f.Open();f.Add(s);f.Next();
    s.TeleportRevision=3;
    f.Open();
    Require(!f.Journal.Observe(f.Bind(s)),"older teleport revision rejected");
    s.TeleportRevision=4;
    Require(!f.Journal.Observe(f.Bind(s)),"invalid frame not reusable");
    f.Next();f.Open();f.Add(s);
    auto h=f.Read().Rivals[0].History;
    Require(h.Refused==1 && Near(h.ObservedSeconds,0.05),
        "rejected rollback cannot bridge previous intervals");
    f.Next();f.Open();f.Add(s);
    Require(Near(f.Read().Rivals[0].History.ObservedSeconds,0.1),
        "valid same revision resumes after gap");
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
    auto invalid=f.Bind(s);EditCommands(invalid,[](auto& c){c[0].Throttle=-0.1;});
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

void FleetCoverageUsesAllFiveSlots()
{
    Fixture f;
    f.Open();f.Add(Request());f.Add(Request(2));f.Next();
    f.Open();f.Add(Request());f.Next();
    f.Open();f.Add(Request());f.Add(Request(3));
    auto coverage=f.Read(true).Coverage;
    Require(coverage.Valid && coverage.VerifiedSlots==3 &&
        coverage.DurationSlots==1 && Near(coverage.ActiveSeconds,0.15) &&
        Near(coverage.ExpectedCarSeconds,0.75) &&
        Near(coverage.ObservedCarSeconds,0.10) &&
        Near(coverage.Fraction(),0.10/0.75),
        "all five slots in denominator, single endpoints are not duration");
    f.Evidence[0].Verified=false;
    coverage=f.Read(true).Coverage;
    Require(coverage.Valid && coverage.VerifiedSlots==2 &&
        coverage.DurationSlots==0 && coverage.Fraction()==0.0,
        "unverified history does not contribute coverage");
    f.Evidence[0].Verified=true;
    f.Next();f.Open();
    coverage=f.Read(true).Coverage;
    Require(Near(coverage.ObservedCarSeconds,0.10) &&
        Near(coverage.ExpectedCarSeconds,1.0),
        "later race clock changes denominator but not sampled duration");
    Require(!f.Journal.Read(1,f.Identity,0,0,true,f.Evidence).Coverage.Valid,
        "zero clock has no coverage denominator");
    Require(!f.Journal.Read(2,f.Identity,0.2,4,true,f.Evidence).Coverage.Valid,
        "foreign attempt cannot expose coverage");
}

void SectorAttributionAndCoverage()
{
    FleetDecisionSectorHistory history;
    Require(!history.Configure(0,12) && !history.Configure(257,257) &&
        !history.Configure(4,3) && !history.Configure(4,11) &&
        !history.Configure(1,25601),"invalid sector layout");
    Require(history.Configure(4,12) && !history.Configure(4,12),"fixed route layout");
    auto request=Request();
    request.SpeedCmS=50;
    BrakeAt(request,FleetDecisionStage::PredictiveTraffic);
    request.Queue=FleetQueueStatus::Waiting;
    auto a=AnalyzeFleetDecision(request);
    auto b=a;
    for(int i=0;i<11;++i) Require(history.Observe(a,b,0.05),"sampled gate one");
    b.GateOrdinal=1;
    Require(history.Observe(a,b,0.05),"boundary is unassigned");
    auto moving=AnalyzeFleetDecision(Request());
    moving.GateOrdinal=1;
    for(int i=0;i<14;++i) Require(history.Observe(moving,moving,0.05),"moving gate two");
    a.GateOrdinal=4;b=a;
    for(int i=0;i<3;++i) Require(history.Observe(a,b,0.05),"second lap gate one");
    b.GateOrdinal=8;
    Require(history.Observe(a,b,0.05),"same modulo different lap is not same sector");
    a.GateOrdinal=10;b=a;
    Require(history.Observe(a,b,0.05),"short third sector");
    auto review=history.Review(1.55);
    Require(review.Valid && review.ObservedSectors==3 && review.ComparableSectors==2 &&
        review.MostNoDriveSector==1 && review.MostBrakingSector==1 &&
        Near(review.AttributedSeconds,1.45) && Near(review.TransitionSeconds,0.10),
        "observed seconds partitioned without assigning transition");
    FleetDecisionSectorBucket bucket;
    Require(history.TrySector(1,bucket) && bucket.Intervals==14 &&
        Near(bucket.ObservedSeconds,0.70) && Near(bucket.NoDriveFraction(),1.0) &&
        Near(bucket.QueueSeconds,0.70) && Near(bucket.MeanSpeedKmh(),1.8),
        "same next gate aggregates observed laps");
    Require(history.TrySector(2,bucket) && Near(bucket.MeanSpeedKmh(),43.2) &&
        bucket.NoDriveSeconds==0.0,"moving observations not no-drive");
    Require(!history.TrySector(4,bucket) && !history.TrySector(0,bucket) &&
        !history.TrySector(5,bucket),"unseen and invalid sectors refused");
    Require(!history.Review(1.0).Valid,"coverage mismatch suppressed");
    b=a;b.GateOrdinal=9;
    Require(!history.Observe(a,b,0.05),"route rollback refused");
    b=a;b.GateOrdinal=12;
    Require(!history.Observe(a,b,0.05),"past route end refused");
    b=a;b.SpeedCmS=std::numeric_limits<double>::quiet_NaN();
    Require(!history.Observe(a,b,0.05),"invalid sample speed");
    Require(!history.Observe(a,a,0.0) && !history.Observe(a,a,0.2) &&
        !history.Observe(a,a,std::numeric_limits<double>::infinity()),"invalid intervals");
    Require(history.Review(1.55).Valid,"rejections do not modify history");
    history.Reset();
    Require(!history.Review(1.55).Valid && history.Configure(12,12),"reset clears route");
}

void SectorNativeJournalContinuity()
{
    Fixture f;
    auto sample=Request();
    sample.SpeedCmS=50;
    BrakeAt(sample,FleetDecisionStage::Corner);
    for(int i=0;i<12;++i){f.Open();f.Add(sample);f.Next();}
    sample.GateOrdinal=1;f.Evidence[0].AcceptedGates=1;
    f.Open();f.Add(sample);f.Next();
    f.Open();f.Add(sample);f.Next();
    f.Open();f.Next(); // missing frame
    f.Open();f.Add(sample);f.Next();
    ++sample.TeleportRevision;
    f.Open();f.Add(sample);
    auto r=f.Read();
    const auto& row=r.Rivals[0];
    Require(row.Sectors.Valid && row.Sectors.ObservedSectors==2 &&
        Near(row.Sectors.AttributedSeconds,0.60) &&
        Near(row.Sectors.TransitionSeconds,0.05) &&
        Near(row.History.ObservedSeconds,0.65) &&
        row.Sectors.MostNoDriveSector==1,
        "missing frame and teleport excluded from sector sums");
    f.Evidence[0].Verified=false;
    r=f.Read(true);
    Require(!r.Rivals[0].Sectors.Valid,"no sector review without verified route");
    f.Evidence[0].Verified=true;
    f.Evidence[0].AcceptedGates=0;
    Require(!f.Read(true).Rivals[0].Sectors.Valid,"inconsistent ordinal hides sector review");
    f.Journal.Reset();
    Require(!f.Read().Valid,"restart clears sector review");
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
        Require(r.Coverage.Valid && r.Coverage.VerifiedSlots==r.ObservedRivals &&
            r.Coverage.DurationSlots<=r.Coverage.VerifiedSlots &&
            r.Coverage.Fraction()>=0.0 && r.Coverage.Fraction()<=1.0,
            "fleet coverage bounded across asynchronous slots");
        for(std::size_t i=0;i<r.Rivals.size();++i)
        {
            const auto& row=r.Rivals[i];
            if(!row.Valid) continue;
            const auto& h=row.History;
            Require(row.Sectors.Valid &&
                Near(row.Sectors.AttributedSeconds+row.Sectors.TransitionSeconds,h.ObservedSeconds) &&
                row.Sectors.ComparableSectors<=row.Sectors.ObservedSectors,
                "every observed interval accounted for by sector or transition");
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
            Require(Near(h.NoDriveContexts.Total(),h.NoDriveSeconds) &&
                h.RaceWindowCoverage(f.Admission.ClockSeconds)>=0.0 &&
                h.RaceWindowCoverage(f.Admission.ClockSeconds)<=1.0,
                "exclusive contexts and race-window coverage bounded");
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
    ExplicitStageTraceRequired();
    NativeFrameAuthority();
    PerStageAndEpisodeAccounting();
    NoDriveContextAccounting();
    TeleportRevisionCannotRollBack();
    MissingFramesAndRecovery();
    CurrentRowsAndIndependentRouteEvidence();
    FleetCoverageUsesAllFiveSlots();
    SectorAttributionAndCoverage();
    SectorNativeJournalContinuity();
    RandomizedCoverageAndInvariants();
    std::cout<<"Fleet AI command-stage observation and continuity tests passed\n";
}
