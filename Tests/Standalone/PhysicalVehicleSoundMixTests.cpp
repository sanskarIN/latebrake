#include "Audio/PhysicalVehicleSoundMix.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace latebrake::audio;
static void Check(bool pass,const char* why)
{
    if(!pass)
    {
        std::cerr<<"PhysicalVehicleAudio FAIL: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
static PhysicalVehicleAudioSource OriginalSoundFrame()
{
    PhysicalVehicleAudioSource s;
    s.Session=88;
    s.SessionRevision=1;
    s.Actor=504;
    s.Frame=100;
    s.ClockSeconds=10.0;
    s.DeltaSeconds=0.05;
    s.EngineRpm=4100.0;
    s.Throttle=0.8;
    s.EngineLoad=0.7;
    s.ForwardSpeedCmS=4200.0;
    s.WheelLateralSlip=0.16;
    s.WheelLongitudinalSlip=0.10;
    s.WheelBrakeTorqueNm=2600.0;
    s.Gear=3;
    s.PhysicalContactWheels=4;
    s.RaceRunning=true;
    s.OriginalChassisVerified=true;
    s.EngineRpmMeasured=true;
    s.OriginalWheelsMeasured=true;
    s.OriginalRoadContactMeasured=true;
    s.OriginalCollisionImpulseMeasured=true;
    return s;
}
static void NextFrame(PhysicalVehicleAudioSource& s)
{
    ++s.Frame;
    s.ClockSeconds+=0.05;
    s.DeltaSeconds=0.05;
}
static double SumGains(const PhysicalVehicleAudioFrame& f)
{
    double sum=0.0;
    for(double gain:f.Gain)
        sum+=gain;
    return sum;
}
static void QualifiedPhysicalMix()
{
    const auto sample=OriginalSoundFrame();
    Check(ValidVehicleAudioSource(sample,{}) &&
          ValidVehicleSoundSettings({}),
          "original six-car physics observations qualify sound source");
    const auto frame=ProposePhysicalVehicleSound(sample);
    Check(frame.Valid && frame.OriginalPhysicsQualified &&
          frame.Actor==sample.Actor &&
          frame.Frame==sample.Frame &&
          frame.Gain[Index(VehicleSoundBus::EngineIdle)]>0.0 &&
          frame.Gain[Index(VehicleSoundBus::EngineLoad)]>0.0 &&
          frame.Gain[Index(VehicleSoundBus::Intake)]>0.0 &&
          frame.Gain[Index(VehicleSoundBus::TireScrub)]>0.0 &&
          frame.Gain[Index(VehicleSoundBus::BrakeSqueal)]>0.0 &&
          frame.Gain[Index(VehicleSoundBus::Wind)]>0.0 &&
          frame.Gain[Index(VehicleSoundBus::Gravel)]==0.0 &&
          frame.Gain[Index(VehicleSoundBus::Collision)]==0.0 &&
          !frame.RealCollisionEvent &&
          !frame.UnrealAudioIssued,
          "measured engine, tire, brake and wind cues are diagnostic only");
    Check(frame.TotalGain<=3.40 &&
          std::abs(SumGains(frame)-frame.TotalGain)<1.0e-10,
          "combined source gain stays within safe fixed audio budget");
    auto airborne=sample;
    airborne.PhysicalContactWheels=0;
    airborne.GravelContactFraction=1.0;
    const auto air=ProposePhysicalVehicleSound(airborne);
    Check(air.Valid &&
          air.Gain[Index(VehicleSoundBus::TireScrub)]==0.0 &&
          air.Gain[Index(VehicleSoundBus::Gravel)]==0.0 &&
          air.Gain[Index(VehicleSoundBus::BrakeSqueal)]==0.0 &&
          air.Gain[Index(VehicleSoundBus::Wind)]>0.0,
          "airborne chassis cannot invent road tire or gravel sounds");
    airborne=sample;
    airborne.Gear=-1;
    airborne.ForwardSpeedCmS=-1000.0;
    const auto reverse=ProposePhysicalVehicleSound(airborne);
    Check(reverse.Valid &&
          reverse.Gain[Index(VehicleSoundBus::ReverseAlert)]>0.0 &&
          reverse.Gain[Index(VehicleSoundBus::Gravel)]==0.0,
          "reverse cue derives from actual negative gear and vehicle motion");
    airborne=sample;
    airborne.GravelContactFraction=0.9;
    Check(ProposePhysicalVehicleSound(airborne).
              Gain[Index(VehicleSoundBus::Gravel)]>0.0,
          "real gravel contact and wheel motion qualify surface cue");
    airborne=sample;
    airborne.CollisionImpulseNs=1800.0;
    const auto hit=ProposePhysicalVehicleSound(airborne);
    Check(hit.Valid && hit.RealCollisionEvent &&
          hit.Gain[Index(VehicleSoundBus::Collision)]>0.0 &&
          !hit.UnrealAudioIssued,
          "only real measured collision impulse may authorize impact cue");
    airborne=sample;
    airborne.Paused=true;
    const auto paused=ProposePhysicalVehicleSound(airborne);
    Check(paused.Valid && paused.Quiet &&
          paused.TotalGain==0.0 &&
          !paused.UnrealAudioIssued,
          "paused simulation produces silence without inventing sound effects");
}
static void RejectFabricatedAudioSensors()
{
    const auto baseline=OriginalSoundFrame();
    for(int f=0;f<11;++f)
    {
        auto s=baseline;
        if(f==0)s.EngineRpm=std::numeric_limits<double>::quiet_NaN();
        else if(f==1)s.DeltaSeconds=0.20;
        else if(f==2)s.Gear=-2;
        else if(f==3)s.PhysicalContactWheels=5;
        else if(f==4)s.OriginalChassisVerified=false;
        else if(f==5)s.EngineRpmMeasured=false;
        else if(f==6)s.OriginalRoadContactMeasured=false;
        else if(f==7)s.WheelBrakeTorqueNm=-20.0;
        else if(f==8)s.CollisionImpulseNs=-1.0;
        else if(f==9)s.SensorConfidence=0.2;
        else s.OriginalCollisionImpulseMeasured=false;
        const auto review=ProposePhysicalVehicleSound(s);
        Check(!review.Valid &&
              review.TotalGain==0.0 &&
              !review.UnrealAudioIssued,
              "bad physical identity or sensor evidence cannot generate sound");
    }
    auto settings=PhysicalVehicleSoundSettings{};
    settings.MaxGainPerBus=1.5;
    Check(!ValidVehicleSoundSettings(settings),
          "gain controls cannot be configured outside bounded safe levels");
}
static void ContinuousAudioSupervisor()
{
    auto s=OriginalSoundFrame();
    PhysicalVehicleAudioSupervisor policy;
    Check(policy.Configure({}),"sound supervisor configures");
    auto first=policy.Step(s);
    Check(first.Valid && policy.Frames()==1 &&
          !first.UnrealAudioIssued,
          "initial authentic audio frame is accepted");
    Check(!policy.Step(s).Valid &&
          policy.Rejections()==1,
          "replayed source frame cannot be mixed twice");
    NextFrame(s);
    s.WheelLateralSlip=0.7;
    auto second=policy.Step(s);
    Check(second.Valid && second.Gain[Index(VehicleSoundBus::TireScrub)]>
          first.Gain[Index(VehicleSoundBus::TireScrub)] &&
          second.Gain[Index(VehicleSoundBus::TireScrub)]-
          first.Gain[Index(VehicleSoundBus::TireScrub)]<=0.151,
          "real tire noise rises within configured frame-level gain rate");
    NextFrame(s);
    s.CollisionImpulseNs=1700.0;
    auto hit=policy.Step(s);
    Check(hit.Valid &&
          hit.RealCollisionEvent &&
          hit.Gain[Index(VehicleSoundBus::Collision)]>0.0 &&
          policy.PhysicalCollisionCues()==1,
          "first real collision impulse produces a source-grounded one-shot");
    NextFrame(s);
    const auto repeat=policy.Step(s);
    Check(repeat.Valid &&
          !repeat.RealCollisionEvent &&
          repeat.Gain[Index(VehicleSoundBus::Collision)]==0.0 &&
          policy.PhysicalCollisionCues()==1,
          "unchanged contact impulse is not replayed as a new collision");
    NextFrame(s);
    s.Paused=true;
    const auto paused=policy.Step(s);
    Check(paused.Valid && paused.TotalGain==0.0 &&
          !paused.UnrealAudioIssued,
          "pause immediately silences even continuous engine contact");
    NextFrame(s);
    s.Paused=false;
    const auto resumed=policy.Step(s);
    Check(resumed.Valid && resumed.TotalGain>0.0 &&
          policy.Frames()==6,
          "source-owned live audio restarts from real chassis after pause");
    NextFrame(s);
    s.ClockSeconds+=0.50;
    Check(!policy.Step(s).Valid &&
          policy.Rejections()==2,
          "lost engine frames cannot advance an audio authority state");
    s.SessionRevision++;
    NextFrame(s);
    Check(policy.Step(s).Valid &&
          policy.Snapshot().Actor==s.Actor &&
          !policy.Snapshot().UnrealAudioIssued,
          "fresh race epoch can start a new physical audio observation chain");
}
int main()
{
    QualifiedPhysicalMix();
    RejectFabricatedAudioSensors();
    ContinuousAudioSupervisor();
    std::cout<<"Physical vehicle audio: source provenance, cue routing and replay/clock invariants passed\n";
}
