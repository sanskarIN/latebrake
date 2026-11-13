#include "Audio/PhysicalSixCarAudioStage.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace latebrake::audio;
static void Check(bool valid,const char* message)
{
    if(!valid)
    {
        std::cerr<<"Six-car spatial audio FAIL: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
static PhysicalSoundFleetSample SixOriginalCars()
{
    PhysicalSoundFleetSample source;
    source.Session=281;
    source.Revision=1;
    source.Frame=50;
    source.PhysicsClockSeconds=5.0;
    source.DeltaSeconds=0.05;
    source.Listener.Position={0.0,0.0,250.0};
    source.Listener.Right={0.0,1.0,0.0};
    source.Listener.PositionMeasured=true;
    source.Listener.OrientationMeasured=true;
    for(std::size_t i=0;i<PhysicalSoundFleetCars;++i)
    {
        auto& actor=source.Cars[i];
        actor.ActualWorldPositionMeasured=true;
        actor.WorldPosition={
            450.0+static_cast<double>(i)*500.0,
            (static_cast<double>(i)-2.5)*350.0,0.0};
        auto& sound=actor.Audio;
        sound.Session=source.Session;
        sound.SessionRevision=source.Revision;
        sound.Actor=8000+i;
        sound.Frame=source.Frame;
        sound.ClockSeconds=source.PhysicsClockSeconds;
        sound.DeltaSeconds=source.DeltaSeconds;
        sound.EngineRpm=3600.0+120.0*static_cast<double>(i);
        sound.EngineLoad=0.8;
        sound.Throttle=0.65;
        sound.ForwardSpeedCmS=3800.0;
        sound.WheelLateralSlip=0.14;
        sound.WheelLongitudinalSlip=0.04;
        sound.WheelBrakeTorqueNm=500.0;
        sound.PhysicalContactWheels=4;
        sound.Gear=3;
        sound.SensorConfidence=0.95;
        sound.RaceRunning=true;
        sound.OriginalChassisVerified=true;
        sound.EngineRpmMeasured=true;
        sound.OriginalWheelsMeasured=true;
        sound.OriginalRoadContactMeasured=true;
        sound.OriginalCollisionImpulseMeasured=true;
    }
    return source;
}
static void Next(PhysicalSoundFleetSample& s)
{
    ++s.Frame;
    s.PhysicsClockSeconds+=0.05;
    for(auto& car:s.Cars)
    {
        car.Audio.Frame=s.Frame;
        car.Audio.ClockSeconds=s.PhysicsClockSeconds;
    }
}
static void ValidPhysicalFleetMix()
{
    auto sample=SixOriginalCars();
    PhysicalSixCarAudioStage stage;
    Check(stage.Configure({}),"original six car mixer configures");
    const auto frame=stage.Step(sample);
    Check(frame.Valid && frame.Fault==PhysicalSoundFleetFault::None &&
          frame.PhysicallyQualifiedCars==6 &&
          frame.AudibleCars==6 && frame.Session==281 &&
          !frame.UnrealAudioActuated &&
          stage.Frames()==1 && stage.SourceEpochs()==1,
          "all authenticated cars are audibly mixed without writing Unreal audio");
    double left=0.0;
    double right=0.0;
    for(std::size_t i=0;i<6;++i)
    {
        const auto& voice=frame.Voices[i];
        Check(voice.Actor==8000+i &&
              voice.Slot==static_cast<int>(i) &&
              voice.SourceQualified && voice.Audible &&
              voice.Evidence.Valid &&
              !voice.Evidence.UnrealAudioIssued &&
              voice.DistanceAttenuation>0.0,
              "real source actor and fixed slot are preserved");
        double voiceLeft=0.0;
        double voiceRight=0.0;
        for(const auto& bus:voice.Buses)
        {
            Check(bus[0]>=0.0 && bus[1]>=0.0 &&
                  bus[0]<=1.0 && bus[1]<=1.0,
                  "bounded stereo sample");
            voiceLeft+=bus[0];
            voiceRight+=bus[1];
        }
        Check(std::abs(voiceLeft-voice.LeftGain)<1e-9 &&
              std::abs(voiceRight-voice.RightGain)<1e-9,
              "individual stereo bus sums reconcile");
        left+=voice.LeftGain;
        right+=voice.RightGain;
    }
    Check(std::abs(left-frame.TotalLeftGain)<1e-9 &&
          std::abs(right-frame.TotalRightGain)<1e-9 &&
          left<=3.000001 && right<=3.000001,
          "global six-car speaker bus safety budget");
    Check(frame.Voices[0].Pan<0.0 &&
          frame.Voices[5].Pan>0.0,
          "verified camera right vector gives deterministic stereo placement");
    Check(frame.Voices[0].DistanceCm<
          frame.Voices[5].DistanceCm,
          "real distances are observed and ordered without guessed car movement");
    Next(sample);
    const auto follow=stage.Step(sample);
    Check(follow.Valid && follow.Frame==51 &&
          stage.Frames()==2 && stage.SourceEpochs()==1,
          "consecutive engine frames preserve independent voice histories");
    Check(stage.Snapshot().Frame==follow.Frame,
          "snapshot is read only");
}
static void RejectUntrustedFleet()
{
    auto original=SixOriginalCars();
    PhysicalSixCarAudioStage stage;
    Check(stage.Step(original).Valid,"initial fleet qualifies");
    Next(original);
    const auto one=original;
    for(int index=0;index<15;++index)
    {
        auto forged=one;
        switch(index)
        {
        case 0:forged.Cars[2].Audio.Actor=
                   forged.Cars[1].Audio.Actor;break;
        case 1:forged.Cars[2].Audio.Actor=111111;break;
        case 2:forged.Cars[3].ActualWorldPositionMeasured=false;break;
        case 3:forged.Listener.PositionMeasured=false;break;
        case 4:forged.Listener.OrientationMeasured=false;break;
        case 5:forged.Listener.Right={0.0,0.0,0.0};break;
        case 6:forged.Cars[0].WorldPosition.X=
                   std::numeric_limits<double>::quiet_NaN();break;
        case 7:forged.Cars[1].Audio.ClockSeconds+=0.03;break;
        case 8:forged.Cars[1].Audio.Frame+=1;break;
        case 9:forged.Cars[3].Audio.SessionRevision+=1;break;
        case 10:forged.Cars[4].Audio.EngineRpmMeasured=false;break;
        case 11:forged.Cars[5].Audio.CollisionImpulseNs=-7.0;break;
        case 12:forged.Cars[3].Audio.RaceRunning=false;break;
        case 13:forged.Cars[0].Audio.OriginalWheelsMeasured=false;break;
        default:forged.Session=0;break;
        }
        const auto rejected=stage.Step(forged);
        Check(!rejected.Valid && !rejected.UnrealAudioActuated &&
              rejected.TotalLeftGain==0.0 &&
              rejected.TotalRightGain==0.0 &&
              stage.Frames()==1 &&
              stage.Snapshot().Frame==50,
              "invalid actor, sensor, camera, clock or roster fails closed");
    }
    Check(stage.Rejections()==15,"every forged frame recorded");
    Check(stage.Step(one).Valid && stage.Frames()==2,
          "valid sequential frame may follow rejected impostors");
    Check(!stage.Step(one).Valid &&
          stage.Snapshot().Frame==51,
          "replay of the same frame is rejected");
    Next(original);
    Next(original);
    original.PhysicsClockSeconds+=0.4;
    for(auto& car:original.Cars)
        car.Audio.ClockSeconds=original.PhysicsClockSeconds;
    Check(!stage.Step(original).Valid,
          "synthetic engine clock jump may not masquerade as physics");
}
static void BudgetDistanceAndQuiet()
{
    auto sample=SixOriginalCars();
    PhysicalSoundFleetSettings config;
    config.MaximumAudibleCars=2;
    config.MaximumSummedStereoGain=0.50;
    PhysicalSixCarAudioStage stage;
    Check(stage.Configure(config),"bounded two voice budget configures");
    auto frame=stage.Step(sample);
    Check(frame.Valid && frame.AudibleCars==2 &&
          frame.BudgetCulledCars==4 &&
          frame.OutputLimiterScale<1.0 &&
          frame.TotalLeftGain<=0.500001 &&
          frame.TotalRightGain<=0.500001 &&
          frame.Voices[0].Audible &&
          frame.Voices[1].Audible,
          "closest measured vehicles win deterministic priority; limiter caps speakers");
    Next(sample);
    sample.Cars[5].WorldPosition.X=15000.0;
    frame=stage.Step(sample);
    Check(frame.Valid &&
          frame.DistanceCulledCars==1 &&
          !frame.Voices[5].Audible,
          "out of range physical car is not an invented audible actor");
    Next(sample);
    for(auto& member:sample.Cars)
        member.Audio.Paused=true;
    frame=stage.Step(sample);
    Check(frame.Valid && frame.AudibleCars==0 &&
          frame.TotalLeftGain==0.0 &&
          frame.TotalRightGain==0.0,
          "physics pause silences all six voices immediately");
    Next(sample);
    for(auto& member:sample.Cars)
        member.Audio.Paused=false;
    frame=stage.Step(sample);
    Check(frame.Valid && frame.AudibleCars==2,
          "actual resumed engine frames regain sound");
    PhysicalSoundFleetSettings silent;
    silent.MasterGain=0.0;
    Check(stage.Configure(silent),"zero master gain is a valid quiet setting");
    frame=stage.Step(sample);
    Check(frame.Valid && frame.AudibleCars==0 &&
          frame.TotalLeftGain==0.0,
          "user mute emits zero acoustic energy");
}
static void EpochsAndCollisionIntegrity()
{
    auto sample=SixOriginalCars();
    PhysicalSixCarAudioStage stage;
    sample.Cars[3].Audio.CollisionImpulseNs=1500.0;
    auto frame=stage.Step(sample);
    Check(frame.Valid && frame.ActualCollisionCues==1 &&
          frame.Voices[3].CollisionPhysicsObserved,
          "real verified collision triggers exactly one diagnostic cue");
    Next(sample);
    frame=stage.Step(sample);
    Check(frame.Valid && frame.ActualCollisionCues==0,
          "unchanged physical impulse cannot be replayed as a new hit");
    sample.Revision=2;
    sample.Frame=1;
    sample.PhysicsClockSeconds=0.05;
    for(auto& car:sample.Cars)
    {
        car.Audio.SessionRevision=2;
        car.Audio.Frame=1;
        car.Audio.ClockSeconds=0.05;
    }
    sample.Cars[2].Audio.Actor=98555;
    frame=stage.Step(sample);
    Check(frame.Valid && frame.Voices[2].Actor==98555 &&
          stage.SourceEpochs()==2,
          "new authenticated engine epoch may legitimately replace the roster");
    Next(sample);
    sample.Revision=1;
    for(auto& car:sample.Cars)
        car.Audio.SessionRevision=1;
    Check(!stage.Step(sample).Valid,
          "old session revision cannot be replayed after an epoch change");
}
static void SettingsSafety()
{
    PhysicalSoundFleetSettings settings;
    Check(PhysicalSoundValidSettings(settings),"safe default audio settings");
    settings.MaximumAudibleCars=7;
    Check(!PhysicalSoundValidSettings(settings),
          "more than six real actors cannot be mixed");
    settings={};
    settings.MasterGain=1.5;
    Check(!PhysicalSoundValidSettings(settings),
          "unbounded master volume rejected");
    settings={};
    settings.NeverActuateUnrealAudio=false;
    Check(!PhysicalSoundValidSettings(settings),
          "portable stage cannot claim to actuate Unreal audio");
    settings={};
    settings.Voice.RequireRealWheelPhysics=false;
    Check(!PhysicalSoundValidSettings(settings),
          "no unverifiable wheel-source fallback");
}
int main()
{
    ValidPhysicalFleetMix();
    RejectUntrustedFleet();
    BudgetDistanceAndQuiet();
    EpochsAndCollisionIntegrity();
    SettingsSafety();
    std::cout<<"Six-car audio: physical roster, world-space attenuation, stereo budgets and engine chronology passed\n";
}
