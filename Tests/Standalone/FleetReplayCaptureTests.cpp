#include "Racing/FleetReplayCapture.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;

void Require(bool ok,const char* message)
{
    if(!ok)
    {
        std::cerr<<"FleetReplayCapture: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Fixture final
{
    FleetReplayCapture Capture;
    FleetReplayFrameAdmission Admission{1,1,101,1,0.05,0.05,{101,102,103,104,105,106}};
    FleetReplayIdentity Teleport{};
    std::array<double,PhysicalReplayCarCount> Side{0,400,800,1200,1600,2000};
    std::array<double,PhysicalReplayCarCount> Along{0,900,1800,2700,3600,4500};

    FleetReplayContribution Sample(int slot) const
    {
        const auto i=static_cast<std::size_t>(slot);
        FleetReplayContribution c;
        c.Slot=slot;
        c.Attempt=Admission.Attempt;
        c.LocalFrame=Admission.LocalFrame;
        c.EngineFrame=Admission.EngineFrame;
        c.SessionEpoch=Admission.SessionEpoch;
        c.TeleportRevision=Teleport[i];
        c.Car.Actor=Admission.Identity[i];
        c.Car.WorldXcm=Along[i]+1000.0*Admission.ActiveSeconds;
        c.Car.WorldYcm=Side[i];
        c.Car.WorldZcm=100;
        c.Car.VelocityXcmps=1000;
        c.Car.HeadingRadians=0;
        c.Car.SteeringNormalized=0.1;
        c.Car.ThrottleNormalized=0.6;
        c.Car.BrakeNormalized=0;
        c.Car.RoadContactConfidence=1.0;
        c.Car.ActualChaosBodyObserved=true;
        c.Car.OriginalRoadPhysicalContact=true;
        return c;
    }

    void Open()
    {
        Require(Capture.BeginFrame(Admission),"admitted frame");
    }

    void FinishFrame()
    {
        for(int slot=0;slot<6;++slot) Require(Capture.Submit(Sample(slot)),"submit actor");
    }

    void Advance()
    {
        ++Admission.LocalFrame;
        ++Admission.EngineFrame;
        Admission.ActiveSeconds+=Admission.AdvancedSeconds;
    }

    void Frame()
    {
        Open();
        FinishFrame();
        Advance();
    }

    FleetReplayReview Freeze(bool eligible=true)
    {
        return Capture.Freeze(Admission.Attempt,Admission.Identity,eligible);
    }
};

void AdmissionValidation()
{
    Fixture f;
    auto a=f.Admission;
    Require(ValidFleetReplayAdmission(a),"valid admission");
    auto reject=[&](FleetReplayFrameAdmission bad)
    {
        Require(!f.Capture.BeginFrame(bad),"invalid admission");
    };
    auto bad=a;bad.Attempt=0;reject(bad);
    bad=a;bad.LocalFrame=0;reject(bad);
    bad=a;bad.EngineFrame=0;reject(bad);
    bad=a;bad.SessionEpoch=0;reject(bad);
    bad=a;bad.Identity[5]=bad.Identity[0];reject(bad);
    bad=a;bad.Identity[3]=0;reject(bad);
    bad=a;bad.ActiveSeconds=0;reject(bad);
    bad=a;bad.ActiveSeconds=86401;reject(bad);
    bad=a;bad.ActiveSeconds=std::numeric_limits<double>::quiet_NaN();reject(bad);
    bad=a;bad.AdvancedSeconds=0.0001;reject(bad);
    bad=a;bad.AdvancedSeconds=0.101;reject(bad);
    bad=a;bad.AdvancedSeconds=std::numeric_limits<double>::infinity();reject(bad);
    Require(f.Capture.ReadCoverage().RefusedAdmissions==12,"count invalid admissions");
    f.Open();f.FinishFrame();
    Require(!f.Capture.BeginFrame(a),"same stamp denied");
    f.Advance();f.Frame();
    const auto c=f.Capture.ReadCoverage();
    Require(c.RecordedFrames==2 && c.Segments==1 && c.ContinuityBreaks==0,
        "replayed admission cannot break current interpolation");
}

void PartialAndDuplicateAssembly()
{
    Fixture f;
    f.Open();
    Require(f.Capture.Submit(f.Sample(3)),"arbitrary first slot");
    Require(f.Capture.Submit(f.Sample(1)),"arbitrary second slot");
    Require(f.Capture.ReadCoverage().RetainedFrames==0,"partial never publishes");
    Require(!f.Capture.Submit(f.Sample(1)),"duplicate poisons frame");
    Require(!f.Capture.Submit(f.Sample(0)),"closed poisoned assembly");
    f.Advance();f.Open();
    for(int slot=5;slot>=0;--slot)
        Require(f.Capture.Submit(f.Sample(slot)),"reverse order accepted");
    f.Advance();f.Open();
    for(int slot=0;slot<5;++slot) Require(f.Capture.Submit(f.Sample(slot)),"partial subsequent");
    f.Advance();f.Frame();
    const auto c=f.Capture.ReadCoverage();
    Require(c.AdmittedFrames==4 && c.RecordedFrames==2 &&
        c.IncompleteFrames==2 && c.RetainedFrames==1 && c.RefusedContributions==2,
        "missing sixth actor resets continuity");
}

void ForeignAndInvalidContributions()
{
    Fixture f;
    f.Open();
    auto c=f.Sample(0);c.Attempt=9;
    Require(!f.Capture.Submit(c),"foreign attempt");
    c=f.Sample(0);++c.EngineFrame;
    Require(!f.Capture.Submit(c),"foreign engine stamp");
    c=f.Sample(0);++c.SessionEpoch;
    Require(!f.Capture.Submit(c),"foreign epoch");
    c=f.Sample(0);++c.LocalFrame;
    Require(!f.Capture.Submit(c),"foreign local stamp");
    f.FinishFrame();
    Require(f.Capture.ReadCoverage().RecordedFrames==1,"foreign callback not poison");
    f.Advance();f.Open();
    c=f.Sample(0);c.Car.Actor=f.Admission.Identity[1];
    Require(!f.Capture.Submit(c),"wrong actor");
    f.Advance();f.Open();
    c=f.Sample(5);c.Car.OriginalRoadPhysicalContact=false;
    Require(!f.Capture.Submit(c),"missing asphalt evidence");
    f.Advance();f.Open();
    c=f.Sample(4);c.Car.ActualChaosBodyObserved=false;
    Require(!f.Capture.Submit(c),"missing actual body");
    f.Advance();f.Open();
    c=f.Sample(3);c.Car.VelocityXcmps=std::numeric_limits<double>::quiet_NaN();
    Require(!f.Capture.Submit(c),"nonfinite velocity");
    f.Advance();f.Open();
    c=f.Sample(2);c.Car.ThrottleNormalized=1.01;
    Require(!f.Capture.Submit(c),"invalid controls");
    f.Advance();f.Frame();f.Frame();
    Require(f.Capture.ReadCoverage().HasReplay,"fresh complete frames recover");
}

void FreezeAndInterpolation()
{
    Fixture f;
    for(int i=0;i<20;++i) f.Frame();
    Require(!f.Capture.EvaluateFrozen(1,0.5).Valid,"no live playback");
    const auto r=f.Freeze();
    Require(r.Valid && r.Coverage.Frozen && r.Coverage.RetainedFrames==20 &&
        r.Coverage.RecordedFrames==20,"frozen complete timeline");
    const auto p=f.Capture.EvaluateFrozen(1,0.525);
    Require(p.Valid && p.Session==1 && p.NoCollisionAuthority &&
        p.NoOfficialTimingAuthority,"observational interpolation");
    for(std::size_t i=0;i<p.Cars.size();++i)
        Require(p.Cars[i].Actor==f.Admission.Identity[i] && p.Cars[i].ProvenanceMatched &&
            std::abs(p.Cars[i].Xcm-(f.Along[i]+525))<1e-8,"same slot interpolation");
    Require(!f.Capture.EvaluateFrozen(2,0.525).Valid,"foreign playback");
    Require(!f.Capture.EvaluateFrozen(1,0).Valid &&
        !f.Capture.EvaluateFrozen(1,1.1).Valid,"no extrapolation");
    Require(!f.Capture.BeginFrame(f.Admission) && !f.Capture.Submit(f.Sample(0)),"immutable finish");
    Require(f.Freeze().Coverage.RecordedFrames==r.Coverage.RecordedFrames,"idempotent freeze");
    Require(!f.Capture.Freeze(2,f.Admission.Identity,true).Valid,"foreign freeze");
}

void ContinuityBreaks()
{
    Fixture f;
    f.Frame();f.Frame();
    f.Capture.Suspend();
    Require(f.Capture.ReadCoverage().HasReplay,"suspend retains old complete segment");
    f.Frame();
    Require(f.Capture.ReadCoverage().RetainedFrames==1,"resume new segment");
    f.Frame();
    ++f.Teleport[4];f.Frame();
    Require(f.Capture.ReadCoverage().RetainedFrames==1,"rival teleport");
    f.Frame();
    ++f.Admission.SessionEpoch;f.Frame();
    Require(f.Capture.ReadCoverage().RetainedFrames==1,"epoch boundary");
    f.Frame();
    ++f.Admission.EngineFrame;f.Frame();
    Require(f.Capture.ReadCoverage().RetainedFrames==1,"engine hitch");
    f.Frame();
    f.Along[3]+=5000;f.Frame();
    Require(f.Capture.ReadCoverage().RetainedFrames==1,"impossible displacement");
    f.Frame();
    const auto r=f.Freeze();
    Require(r.Valid && r.Coverage.Segments==6 &&
        r.Coverage.ContinuityBreaks==5 && r.Coverage.RetainedFrames==2,
        "latest segment and explicit coverage");
    Require(!f.Capture.EvaluateFrozen(1,0.05).Valid,"old segment not stitched");
}

void TeleportRollbackCannotBecomeNewSegment()
{
    Fixture f;
    f.Teleport[2]=5;
    f.Frame();f.Frame();
    f.Teleport[2]=4;
    f.Open();
    Require(f.Capture.Submit(f.Sample(0)),"first actor before rollback");
    Require(!f.Capture.Submit(f.Sample(2)),"authenticated revision rollback refused");
    Require(!f.Capture.Submit(f.Sample(1)),"poisoned frame closed");
    const auto bad=f.Capture.ReadCoverage();
    Require(bad.TeleportRollbackRefusals==1 && bad.RecordedFrames==2 &&
        bad.IncompleteFrames==1 && bad.ContinuityBreaks==1 &&
        bad.Status==FleetReplayCaptureStatus::InvalidContribution,
        "rollback cannot start a replay segment");
    f.Advance();
    f.Teleport[2]=5;f.Frame();f.Frame();
    Require(f.Capture.ReadCoverage().Segments==2 &&
        f.Capture.ReadCoverage().RetainedFrames==2,"current revision resumes in new segment");

    f.Open();
    f.Teleport[4]=17;
    Require(f.Capture.Submit(f.Sample(4)),"authenticated revision in partial frame");
    f.Advance();
    f.Teleport[4]=16;
    f.Open();
    Require(!f.Capture.Submit(f.Sample(4)),"partial observation also establishes high water");
    f.Advance();
    f.Teleport[4]=17;
    f.Frame();f.Frame();
    auto r=f.Freeze();
    Require(r.Valid && r.Coverage.TeleportRollbackRefusals==2 &&
        r.Coverage.RecordedFrames==6 && r.Coverage.IncompleteFrames==3,
        "only six-car qualified frames contribute");

    f.Capture.Reset();
    ++f.Admission.Attempt;f.Admission.LocalFrame=1;f.Admission.ActiveSeconds=0.05;
    f.Teleport={};
    f.Frame();f.Frame();
    r=f.Freeze();
    Require(r.Valid && r.Coverage.TeleportRollbackRefusals==0,
        "new attempt clears revision history");
}

void ForeignOrMalformedRevisionCannotPoisonIdentity()
{
    Fixture f;
    f.Frame();f.Frame();
    f.Open();
    auto bogus=f.Sample(3);bogus.TeleportRevision=999;++bogus.Attempt;
    Require(!f.Capture.Submit(bogus),"foreign attempt cannot establish revision");
    f.FinishFrame();f.Advance();
    f.Open();
    bogus=f.Sample(3);bogus.TeleportRevision=999;bogus.Car.Actor=777;
    Require(!f.Capture.Submit(bogus),"wrong actor cannot establish revision");
    f.Advance();f.Open();
    bogus=f.Sample(3);bogus.TeleportRevision=999;bogus.Car.OriginalRoadPhysicalContact=false;
    Require(!f.Capture.Submit(bogus),"malformed actor evidence cannot establish revision");
    f.Advance();f.Frame();f.Frame();
    const auto r=f.Freeze();
    Require(r.Valid && r.Coverage.TeleportRollbackRefusals==0 &&
        r.Coverage.RecordedFrames==5,"genuine identity remains usable");
}

void FinishBeforeSixthActorAndRestart()
{
    Fixture f;
    for(int i=0;i<12;++i) f.Frame();
    const auto last=f.Capture.ReadCoverage().LastClockSeconds;
    f.Open();
    Require(f.Capture.Submit(f.Sample(0)),"human before finish");
    Require(f.Capture.Submit(f.Sample(2)),"rival before finish");
    const auto r=f.Freeze();
    Require(r.Valid && r.Coverage.IncompleteFrames==1 &&
        r.Coverage.LastClockSeconds==last,"unfinished frame omitted");
    f.Capture.Reset();
    Require(!f.Capture.FrozenReview().Valid && !f.Capture.ReadCoverage().HasReplay,"restart clears");
    ++f.Admission.Attempt;
    f.Admission.LocalFrame=1;
    f.Admission.ActiveSeconds=0.05;
    f.Frame();f.Frame();
    Require(!f.Freeze(false).Valid && !f.Capture.EvaluateFrozen(2,0.075).Valid,"ineligible finish");
}

void ForeignAdmissionAndFinishIsolation()
{
    Fixture f;
    f.Frame();f.Frame();
    f.Open();
    Require(f.Capture.Submit(f.Sample(0)),"first real contribution");
    const auto before=f.Capture.ReadCoverage();
    auto foreign=f.Admission;
    ++foreign.Attempt;
    Require(!f.Capture.BeginFrame(foreign),"foreign admission rejected");
    foreign=f.Admission;
    foreign.Identity[3]=700;
    Require(!f.Capture.BeginFrame(foreign),"foreign identity rejected");
    auto stale=f.Admission;
    --stale.LocalFrame;--stale.EngineFrame;
    stale.ActiveSeconds-=stale.AdvancedSeconds;
    Require(!f.Capture.BeginFrame(stale),"stale admission rejected");
    Require(!f.Capture.BeginFrame(f.Admission),"duplicate admission rejected");
    Require(!f.Capture.Freeze(99,f.Admission.Identity,true).Valid,
        "foreign eligible finish ignored");
    auto wrong=f.Admission.Identity;wrong[0]=700;
    Require(!f.Capture.Freeze(f.Admission.Attempt,wrong,false).Valid,
        "wrong roster ineligible finish ignored");
    const auto after=f.Capture.ReadCoverage();
    Require(!after.Frozen && after.Status==FleetReplayCaptureStatus::AwaitingCars &&
        after.IncompleteFrames==before.IncompleteFrames &&
        after.ContinuityBreaks==before.ContinuityBreaks &&
        after.RefusedAdmissions==before.RefusedAdmissions+4,
        "foreign callbacks preserve pending frame");
    for(int i=1;i<6;++i) Require(f.Capture.Submit(f.Sample(i)),"pending frame completes");
    const auto r=f.Freeze();
    Require(r.Valid && r.Coverage.RetainedFrames==3 && r.Coverage.Segments==1 &&
        !r.Coverage.EarlierSegmentUsed,"actual finish succeeds");
}

void PreviousSegmentFallback()
{
    Fixture f;
    for(int i=0;i<20;++i) f.Frame();
    const double last=f.Capture.ReadCoverage().LastClockSeconds;
    ++f.Teleport[2];f.Along[2]+=5000;
    f.Frame();
    Require(f.Capture.ReadCoverage().RetainedFrames==1,"latest single sample");
    const double later=f.Capture.ReadCoverage().LastClockSeconds;
    const auto r=f.Freeze();
    Require(r.Valid && r.Coverage.EarlierSegmentUsed &&
        r.Coverage.RetainedFrames==20 && r.Coverage.RecordedFrames==21 &&
        r.Coverage.Segments==2 && r.Coverage.LastClockSeconds==last,
        "restore complete earlier window only");
    Require(f.Capture.EvaluateFrozen(1,last).Valid &&
        !f.Capture.EvaluateFrozen(1,later).Valid,"no interpolation across recovery");
    for(unsigned i=0;i<r.HighlightCount;++i)
        Require(r.Highlights[i].EndSeconds<=last,"highlight from restored segment");
    f.Capture.Reset();
    Require(!f.Capture.ReadCoverage().EarlierSegmentUsed,"restart clears fallback flag");
    ++f.Admission.Attempt;f.Admission.LocalFrame=1;
    f.Admission.ActiveSeconds=0.05;
    f.Frame();
    Require(!f.Freeze().Valid,"earlier attempt cannot be restored");
}

void MostRecentPlayableSegment()
{
    Fixture f;
    for(int i=0;i<15;++i) f.Frame();
    f.Capture.Suspend();
    f.Frame();f.Frame();f.Frame();
    const auto recent=f.Capture.ReadCoverage();
    Require(recent.RetainedFrames==3,"recent window playable");
    f.Capture.Suspend();f.Frame();
    f.Capture.Suspend();f.Frame();
    const auto r=f.Freeze();
    Require(r.Valid && r.Coverage.EarlierSegmentUsed &&
        r.Coverage.RetainedFrames==3 &&
        r.Coverage.FirstClockSeconds==recent.FirstClockSeconds &&
        r.Coverage.LastClockSeconds==recent.LastClockSeconds,
        "most recent playable segment survives repeated one-frame fragments");
}

void EligibleLatestAndIneligibleDiscard()
{
    Fixture f;
    for(int i=0;i<15;++i) f.Frame();
    f.Capture.Suspend();f.Frame();f.Frame();
    const auto r=f.Freeze();
    Require(r.Valid && !r.Coverage.EarlierSegmentUsed &&
        r.Coverage.RetainedFrames==2,"latest playable segment preferred");
    f.Capture.Reset();
    ++f.Admission.Attempt;f.Admission.LocalFrame=1;f.Admission.ActiveSeconds=0.05;
    f.Frame();f.Frame();
    f.Capture.Suspend();f.Frame();
    Require(!f.Freeze(false).Valid && !f.Capture.ReadCoverage().HasReplay &&
        !f.Capture.ReadCoverage().EarlierSegmentUsed,"ineligible finish discards all segments");
}

void RingAndHighlightScope()
{
    Fixture f;
    for(int i=0;i<600;++i)
    {
        f.Along[1]=220.0+2.0*static_cast<double>(i%80<40?i%40:39-i%40);
        f.Frame();
    }
    const auto r=f.Freeze();
    Require(r.Valid && r.Coverage.RecordedFrames==600 &&
        r.Coverage.RetainedFrames==PhysicalReplayFrameCapacity,"bounded ring");
    Require(r.Coverage.FirstClockSeconds>0.05 &&
        r.Coverage.LastClockSeconds>r.Coverage.FirstClockSeconds,"clock bounds");
    for(unsigned i=0;i<r.HighlightCount;++i)
    {
        const auto& clip=r.Highlights[i];
        Require(clip.Valid && clip.FocusSlot>=0 && clip.FocusSlot<6 &&
            clip.StartSeconds>=r.Coverage.FirstClockSeconds &&
            clip.EndSeconds<=r.Coverage.LastClockSeconds &&
            clip.EventSeconds>=clip.StartSeconds && clip.EventSeconds<=clip.EndSeconds &&
            std::isfinite(clip.SampledSpeedKmh),"highlight retained identity and clock");
    }
    Require(!f.Capture.EvaluateFrozen(1,0.05).Valid,"overwritten frame unavailable");
}

void RandomizedAssembly()
{
    Fixture f;
    std::mt19937 rng(28764);
    std::array<int,6> slots{0,1,2,3,4,5};
    std::uint64_t accepted=0;
    for(int frame=0;frame<1800;++frame)
    {
        f.Open();
        std::shuffle(slots.begin(),slots.end(),rng);
        const bool incomplete=rng()%11==0;
        for(int i=0;i<(incomplete?5:6);++i)
            Require(f.Capture.Submit(f.Sample(slots[static_cast<std::size_t>(i)])),
                "shuffled actor");
        if(!incomplete) ++accepted;
        Require(f.Capture.ReadCoverage().RecordedFrames==accepted,"only complete assemblies count");
        f.Advance();
        if(rng()%37==0) ++f.Teleport[rng()%6];
    }
    const auto r=f.Freeze();
    Require(r.Coverage.RecordedFrames==accepted &&
        r.Coverage.RecordedFrames+r.Coverage.IncompleteFrames==r.Coverage.AdmittedFrames,
        "account every admission at finish");
}
}

int main()
{
    AdmissionValidation();
    PartialAndDuplicateAssembly();
    ForeignAndInvalidContributions();
    FreezeAndInterpolation();
    ContinuityBreaks();
    TeleportRollbackCannotBecomeNewSegment();
    ForeignOrMalformedRevisionCannotPoisonIdentity();
    FinishBeforeSixthActorAndRestart();
    ForeignAdmissionAndFinishIsolation();
    PreviousSegmentFallback();
    MostRecentPlayableSegment();
    EligibleLatestAndIneligibleDiscard();
    RingAndHighlightScope();
    RandomizedAssembly();
    std::cout<<"Fleet replay assembly and frozen observation tests passed\n";
}
