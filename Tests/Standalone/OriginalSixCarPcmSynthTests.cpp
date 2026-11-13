#include "Audio/OriginalSixCarPcmSynth.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace latebrake::audio;
static void Require(bool valid,const char* message)
{
    if(!valid)
    {
        std::cerr<<"PCM synth FAIL: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
static PhysicalSoundFleetSample OriginalCars()
{
    PhysicalSoundFleetSample s;
    s.Session=99;
    s.Revision=2;
    s.Frame=80;
    s.PhysicsClockSeconds=8.0;
    s.DeltaSeconds=0.05;
    s.Listener.PositionMeasured=true;
    s.Listener.OrientationMeasured=true;
    for(std::size_t i=0;i<6;++i)
    {
        auto& actor=s.Cars[i];
        actor.WorldPosition={
            450.0+static_cast<double>(i)*350.0,
            130.0*static_cast<double>(i),0.0};
        actor.ActualWorldPositionMeasured=true;
        auto& audio=actor.Audio;
        audio.Session=s.Session;
        audio.SessionRevision=s.Revision;
        audio.Actor=1000+i;
        audio.Frame=s.Frame;
        audio.ClockSeconds=s.PhysicsClockSeconds;
        audio.DeltaSeconds=s.DeltaSeconds;
        audio.EngineRpm=3000.0+100.0*static_cast<double>(i);
        audio.EngineLoad=0.80;
        audio.Throttle=0.75;
        audio.ForwardSpeedCmS=3000.0;
        audio.WheelLateralSlip=0.16;
        audio.WheelLongitudinalSlip=0.06;
        audio.WheelBrakeTorqueNm=900.0;
        audio.PhysicalContactWheels=4;
        audio.Gear=3;
        audio.SensorConfidence=0.96;
        audio.RaceRunning=true;
        audio.OriginalChassisVerified=true;
        audio.EngineRpmMeasured=true;
        audio.OriginalWheelsMeasured=true;
        audio.OriginalRoadContactMeasured=true;
        audio.OriginalCollisionImpulseMeasured=true;
    }
    return s;
}
static void Next(PhysicalSoundFleetSample& s)
{
    s.Frame++;
    s.PhysicsClockSeconds+=0.05;
    for(auto& car:s.Cars)
    {
        car.Audio.Frame=s.Frame;
        car.Audio.ClockSeconds=s.PhysicsClockSeconds;
    }
}
static void OriginalPcmIsDeterministic()
{
    PhysicalSixCarAudioStage stage;
    OriginalSixCarPcmSynth pcm;
    Require(pcm.Configure({}) && ValidOriginalPcmSettings({}),
            "bounded synthesis policy configures");
    auto s=OriginalCars();
    const auto stereo=stage.Step(s);
    Require(ValidOriginalPcmFleetInput(stereo,{}) &&
            stereo.PhysicallyQualifiedCars==6,
            "actual six-car source is required before PCM");
    const auto sample=pcm.Step(stereo,s.DeltaSeconds);
    Require(sample.Valid &&
            sample.SampleRate==48000 &&
            sample.SampleFrames==2400 &&
            sample.MeasuredVoiceCount==6 &&
            sample.ActiveVoiceCount==6 &&
            !sample.UnrealAudioActuated &&
            !sample.Silence &&
            sample.RmsGain>0.0 &&
            sample.MaximumAbsoluteGain<=0.93,
            "first physically qualified stereo PCM is bounded and audible");
    std::int64_t sum=0;
    int nonzero=0;
    for(int i=0;i<sample.SampleFrames*2;++i)
    {
        sum+=sample.Interleaved[static_cast<std::size_t>(i)];
        if(sample.Interleaved[static_cast<std::size_t>(i)]!=0)
            ++nonzero;
    }
    Require(nonzero>sample.SampleFrames &&
            std::abs(sum)<static_cast<std::int64_t>(
                sample.SampleFrames*32767),
            "generated harmonic, road and wind sample buffer is nontrivial");
    pcm.Reset();
    const auto replay=pcm.Step(stereo,s.DeltaSeconds);
    Require(replay.Valid && replay.SampleFrames==sample.SampleFrames,
            "reset may recreate identical original source output");
    for(int i=0;i<sample.SampleFrames*2;++i)
    {
        Require(replay.Interleaved[static_cast<std::size_t>(i)]==
                sample.Interleaved[static_cast<std::size_t>(i)],
                "PCM must be repeatable for the same verified original frame");
    }
    Next(s);
    const auto next=pcm.Step(stage.Step(s),0.05);
    Require(next.Valid &&
            pcm.AcceptedFrames()==2 &&
            pcm.ProducedSampleFrames()==4800 &&
            pcm.SessionEpochs()==1,
            "multi-frame engine oscillator preserves source continuity");
}
static void RejectForgeryAndDiscontinuities()
{
    PhysicalSixCarAudioStage stage;
    OriginalSixCarPcmSynth synth;
    auto s=OriginalCars();
    auto good=stage.Step(s);
    Require(synth.Step(good,0.05).Valid,"authenticated first sample");
    auto forged=good;
    forged.Voices[0].Actor=forged.Voices[1].Actor;
    Require(!synth.Step(forged,0.05).Valid &&
            synth.AcceptedFrames()==1,
            "duplicate actual actor identities fail closed");
    forged=good;
    forged.Voices[0].Buses[0][0]=
        std::numeric_limits<double>::quiet_NaN();
    Require(!synth.Step(forged,0.05).Valid,
            "invalid speaker gain cannot enter PCM synthesis");
    forged=good;
    forged.Valid=false;
    Require(!synth.Step(forged,0.05).Valid,
            "unqualified stereo mixer cannot be promoted to audio");
    Require(!synth.Step(good,0.05).Valid &&
            synth.Snapshot().Frame==80,
            "same frame replay cannot synthesize twice");
    Next(s);
    good=stage.Step(s);
    Require(!synth.Step(good,0.0).Valid,
            "zero physics duration is rejected");
    Require(!synth.Step(good,0.2).Valid,
            "oversized physics duration is rejected");
    Require(synth.Step(good,0.05).Valid,
            "genuine sequential frame survives failed impostors");
    Next(s);
    good=stage.Step(s);
    auto wrong=good;
    wrong.Voices[5].Actor=123456789;
    wrong.Voices[5].Evidence.Actor=wrong.Voices[5].Actor;
    Require(!synth.Step(wrong,0.05).Valid &&
            synth.Snapshot().Frame==81,
            "source changes actor roster without new engine epoch");
    Require(synth.Step(good,0.05).Valid,
            "original stable actor source remains usable");
    Require(synth.RejectedFrames()>=7 &&
            synth.AcceptedFrames()==3,
            "rejections are counted without advancing accepted audio frame");
}
static void PauseMuteAndImpulse()
{
    PhysicalSixCarAudioStage stage;
    OriginalSixCarPcmSynth synth;
    auto s=OriginalCars();
    s.Cars[2].Audio.CollisionImpulseNs=1700.0;
    auto first=stage.Step(s);
    const auto hit=synth.Step(first,0.05);
    Require(hit.Valid &&
            hit.SourceCollisionEvents==1 &&
            hit.ActiveVoiceCount==6 &&
            hit.MaximumAbsoluteGain>0.0,
            "observed real contact produces transient source-limited audio");
    Next(s);
    first=stage.Step(s);
    const auto repeated=synth.Step(first,0.05);
    Require(repeated.Valid && repeated.SourceCollisionEvents==0,
            "held impulse does not invent a second collision event");
    Next(s);
    for(auto& car:s.Cars)
        car.Audio.Paused=true;
    const auto silence=synth.Step(stage.Step(s),0.05);
    Require(silence.Valid && silence.Silence &&
            silence.MaximumAbsoluteGain==0.0 &&
            silence.RmsGain==0.0 &&
            silence.ActiveVoiceCount==0,
            "physical pause mutes all sources without filter residue");
    for(int i=0;i<silence.SampleFrames*2;++i)
        Require(silence.Interleaved[static_cast<std::size_t>(i)]==0,
                "all paused PCM output must be exactly silent");
    Next(s);
    for(auto& car:s.Cars)
        car.Audio.Paused=false;
    Require(synth.Step(stage.Step(s),0.05).Valid,
            "resumption requires fresh authenticated engine frame");
}
static void SettingsAndEpochGuard()
{
    OriginalPcmSettings settings;
    settings.MasterGain=1.5;
    Require(!ValidOriginalPcmSettings(settings),
            "unbounded master cannot be configured");
    settings={};
    settings.RequiresAllSixRealCars=false;
    Require(!ValidOriginalPcmSettings(settings),
            "partial roster cannot be treated as whole racing pack");
    settings={};
    settings.NeverAccessUnrealAudio=false;
    Require(!ValidOriginalPcmSettings(settings),
            "portable stage cannot pretend to invoke native audio");
    settings={};
    settings.SampleRate=44100;
    Require(!ValidOriginalPcmSettings(settings),
            "current fixed-rate implementation is restricted to tested rates");
    settings={};
    settings.MasterGain=0.0;
    OriginalSixCarPcmSynth synth;
    Require(synth.Configure(settings),"silent user profile accepted");
    auto s=OriginalCars();
    PhysicalSixCarAudioStage stage;
    auto out=synth.Step(stage.Step(s),0.05);
    Require(out.Valid && out.Silence && out.RmsGain==0.0,
            "zero master gain is silent at all samples");
    s.Revision=3;
    s.Frame=1;
    s.PhysicsClockSeconds=0.05;
    for(auto& car:s.Cars)
    {
        car.Audio.SessionRevision=s.Revision;
        car.Audio.Frame=s.Frame;
        car.Audio.ClockSeconds=s.PhysicsClockSeconds;
    }
    out=synth.Step(stage.Step(s),0.05);
    Require(out.Valid && synth.SessionEpochs()==2,
            "fresh real physics epoch may restart audio synthesis");
    Next(s);
    s.Revision=2;
    for(auto& car:s.Cars)
        car.Audio.SessionRevision=s.Revision;
    Require(!synth.Step(stage.Step(s),0.05).Valid,
            "old session revision replay is rejected");
}
int main()
{
    OriginalPcmIsDeterministic();
    RejectForgeryAndDiscontinuities();
    PauseMuteAndImpulse();
    SettingsAndEpochGuard();
    std::cout<<"Six-car PCM synth source chronology, deterministic harmonic and bounded stereo output passed\n";
}
