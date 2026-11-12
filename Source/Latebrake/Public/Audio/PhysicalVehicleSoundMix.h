#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake::audio
{
enum class VehicleSoundBus : std::uint8_t
{
    EngineIdle, EngineLoad, Intake, TireScrub, Gravel,
    BrakeSqueal, Wind, Collision, ReverseAlert, Count
};
inline constexpr std::size_t VehicleSoundBusCount=
    static_cast<std::size_t>(VehicleSoundBus::Count);
inline constexpr std::size_t Index(VehicleSoundBus bus) noexcept
{
    return static_cast<std::size_t>(bus);
}
struct PhysicalVehicleSoundSettings final
{
    double MinimumEngineRpm=650.0;
    double MaximumEngineRpm=9500.0;
    double IdleBandEndRpm=3100.0;
    double MinimumScrubSlip=0.035;
    double MaximumScrubSlip=0.75;
    double MinimumMovingSpeedCmS=180.0;
    double FullWindSpeedCmS=8500.0;
    double MaximumVehicleSpeedCmS=16000.0;
    double MinimumBrakeTorqueNm=90.0;
    double MaximumBrakeTorqueNm=6500.0;
    double MaximumContactImpulseNs=30000.0;
    double CollisionSoundThresholdNs=250.0;
    double FullCollisionSoundNs=4000.0;
    double MinimumImpactRiseNs=90.0;
    double MaximumGainPerBus=0.90;
    double MaximumSummedGain=3.40;
    double MaximumGainRisePerSecond=3.0;
    double MaximumGainFallPerSecond=4.5;
    double MaximumFrameSeconds=0.10;
    double MaximumClockSkewSeconds=0.025;
    bool RequireOriginalChaosChassis=true;
    bool RequireRealWheelPhysics=true;
    bool NeverWriteUnrealAudio=true;
};
struct PhysicalVehicleAudioSource final
{
    std::uint64_t Session=0;
    std::uint64_t SessionRevision=0;
    std::uint64_t Actor=0;
    std::uint64_t Frame=0;
    double ClockSeconds=0.0;
    double DeltaSeconds=0.0;
    double EngineRpm=0.0;
    double Throttle=0.0;
    double EngineLoad=0.0;
    double ForwardSpeedCmS=0.0;
    double LateralSpeedCmS=0.0;
    double WheelLateralSlip=0.0;
    double WheelLongitudinalSlip=0.0;
    double WheelBrakeTorqueNm=0.0;
    double GravelContactFraction=0.0;
    double CollisionImpulseNs=0.0;
    double SensorConfidence=1.0;
    int Gear=0;
    int PhysicalContactWheels=0;
    bool RaceRunning=false;
    bool Paused=false;
    bool OriginalChassisVerified=false;
    bool EngineRpmMeasured=false;
    bool OriginalWheelsMeasured=false;
    bool OriginalRoadContactMeasured=false;
    bool OriginalCollisionImpulseMeasured=false;
};
struct PhysicalVehicleAudioFrame final
{
    std::uint64_t Session=0;
    std::uint64_t Actor=0;
    std::uint64_t Frame=0;
    std::array<double,VehicleSoundBusCount> Gain{};
    double EnginePitch=1.0;
    double TotalGain=0.0;
    double ClockSeconds=0.0;
    bool Valid=false;
    bool Quiet=false;
    bool OriginalPhysicsQualified=false;
    bool RealCollisionEvent=false;
    bool UnrealAudioIssued=false;
};
inline bool FiniteAudio(double x) noexcept
{
    return std::isfinite(x);
}
inline double AudioUnit(double x) noexcept
{
    return FiniteAudio(x)?std::clamp(x,0.0,1.0):0.0;
}
inline bool ValidVehicleSoundSettings(
    const PhysicalVehicleSoundSettings& s) noexcept
{
    return FiniteAudio(s.MinimumEngineRpm) &&
        s.MinimumEngineRpm>0.0 &&
        FiniteAudio(s.MaximumEngineRpm) &&
        s.MaximumEngineRpm>s.MinimumEngineRpm &&
        FiniteAudio(s.IdleBandEndRpm) &&
        s.IdleBandEndRpm>s.MinimumEngineRpm &&
        s.IdleBandEndRpm<s.MaximumEngineRpm &&
        FiniteAudio(s.MinimumScrubSlip) &&
        s.MinimumScrubSlip>=0.0 &&
        FiniteAudio(s.MaximumScrubSlip) &&
        s.MaximumScrubSlip>s.MinimumScrubSlip &&
        FiniteAudio(s.MinimumMovingSpeedCmS) &&
        s.MinimumMovingSpeedCmS>=0.0 &&
        FiniteAudio(s.FullWindSpeedCmS) &&
        s.FullWindSpeedCmS>s.MinimumMovingSpeedCmS &&
        FiniteAudio(s.MaximumVehicleSpeedCmS) &&
        s.MaximumVehicleSpeedCmS>=s.FullWindSpeedCmS &&
        FiniteAudio(s.MinimumBrakeTorqueNm) &&
        s.MinimumBrakeTorqueNm>=0.0 &&
        FiniteAudio(s.MaximumBrakeTorqueNm) &&
        s.MaximumBrakeTorqueNm>s.MinimumBrakeTorqueNm &&
        FiniteAudio(s.MaximumContactImpulseNs) &&
        s.MaximumContactImpulseNs>0.0 &&
        FiniteAudio(s.CollisionSoundThresholdNs) &&
        s.CollisionSoundThresholdNs>=0.0 &&
        FiniteAudio(s.FullCollisionSoundNs) &&
        s.FullCollisionSoundNs>s.CollisionSoundThresholdNs &&
        s.MaximumContactImpulseNs>=s.FullCollisionSoundNs &&
        FiniteAudio(s.MinimumImpactRiseNs) &&
        s.MinimumImpactRiseNs>=0.0 &&
        FiniteAudio(s.MaximumGainPerBus) &&
        s.MaximumGainPerBus>0.0 &&
        s.MaximumGainPerBus<=1.0 &&
        FiniteAudio(s.MaximumSummedGain) &&
        s.MaximumSummedGain>0.0 &&
        FiniteAudio(s.MaximumGainRisePerSecond) &&
        s.MaximumGainRisePerSecond>0.0 &&
        FiniteAudio(s.MaximumGainFallPerSecond) &&
        s.MaximumGainFallPerSecond>0.0 &&
        FiniteAudio(s.MaximumFrameSeconds) &&
        s.MaximumFrameSeconds>0.0 &&
        s.MaximumFrameSeconds<=0.10 &&
        FiniteAudio(s.MaximumClockSkewSeconds) &&
        s.MaximumClockSkewSeconds>=0.0 &&
        s.MaximumClockSkewSeconds<=s.MaximumFrameSeconds;
}
inline bool ValidVehicleAudioSource(
    const PhysicalVehicleAudioSource& x,
    const PhysicalVehicleSoundSettings& p) noexcept
{
    return ValidVehicleSoundSettings(p) &&
        x.Session!=0 && x.Actor!=0 && x.Frame!=0 &&
        x.OriginalChassisVerified &&
        x.EngineRpmMeasured &&
        x.OriginalWheelsMeasured &&
        x.OriginalRoadContactMeasured &&
        x.OriginalCollisionImpulseMeasured &&
        FiniteAudio(x.ClockSeconds) &&
        x.ClockSeconds>=0.0 &&
        FiniteAudio(x.DeltaSeconds) &&
        x.DeltaSeconds>0.0 &&
        x.DeltaSeconds<=p.MaximumFrameSeconds &&
        FiniteAudio(x.EngineRpm) &&
        x.EngineRpm>=0.0 &&
        x.EngineRpm<=p.MaximumEngineRpm &&
        FiniteAudio(x.Throttle) &&
        x.Throttle>=0.0 && x.Throttle<=1.0 &&
        FiniteAudio(x.EngineLoad) &&
        x.EngineLoad>=0.0 && x.EngineLoad<=1.0 &&
        FiniteAudio(x.ForwardSpeedCmS) &&
        std::abs(x.ForwardSpeedCmS)<=p.MaximumVehicleSpeedCmS &&
        FiniteAudio(x.LateralSpeedCmS) &&
        std::abs(x.LateralSpeedCmS)<=p.MaximumVehicleSpeedCmS &&
        FiniteAudio(x.WheelLateralSlip) &&
        std::abs(x.WheelLateralSlip)<=2.0 &&
        FiniteAudio(x.WheelLongitudinalSlip) &&
        std::abs(x.WheelLongitudinalSlip)<=2.0 &&
        FiniteAudio(x.WheelBrakeTorqueNm) &&
        x.WheelBrakeTorqueNm>=0.0 &&
        x.WheelBrakeTorqueNm<=p.MaximumBrakeTorqueNm &&
        FiniteAudio(x.GravelContactFraction) &&
        x.GravelContactFraction>=0.0 &&
        x.GravelContactFraction<=1.0 &&
        FiniteAudio(x.CollisionImpulseNs) &&
        x.CollisionImpulseNs>=0.0 &&
        x.CollisionImpulseNs<=p.MaximumContactImpulseNs &&
        FiniteAudio(x.SensorConfidence) &&
        x.SensorConfidence>=0.8 &&
        x.SensorConfidence<=1.0 &&
        x.Gear>=-1 && x.Gear<=8 &&
        x.PhysicalContactWheels>=0 &&
        x.PhysicalContactWheels<=4;
}
inline void LimitTotalAudioGain(
    PhysicalVehicleAudioFrame& frame,
    const PhysicalVehicleSoundSettings& p) noexcept
{
    double sum=0.0;
    for(double& gain:frame.Gain)
    {
        gain=std::clamp(gain,0.0,p.MaximumGainPerBus);
        sum+=gain;
    }
    if(sum>p.MaximumSummedGain)
    {
        const double scale=p.MaximumSummedGain/sum;
        sum=0.0;
        for(double& gain:frame.Gain)
        {
            gain*=scale;
            sum+=gain;
        }
    }
    frame.TotalGain=sum;
}
inline PhysicalVehicleAudioFrame ProposePhysicalVehicleSound(
    const PhysicalVehicleAudioSource& x,
    const PhysicalVehicleSoundSettings& p={}) noexcept
{
    PhysicalVehicleAudioFrame out;
    if(!ValidVehicleAudioSource(x,p))
        return out;
    out.Session=x.Session;
    out.Actor=x.Actor;
    out.Frame=x.Frame;
    out.ClockSeconds=x.ClockSeconds;
    out.Valid=true;
    out.OriginalPhysicsQualified=true;
    out.UnrealAudioIssued=false;
    out.Quiet=!x.RaceRunning || x.Paused;
    if(out.Quiet)
        return out;
    const double speed=std::hypot(
        x.ForwardSpeedCmS,x.LateralSpeedCmS);
    const double moving=AudioUnit(
        (speed-p.MinimumMovingSpeedCmS)/
        (p.FullWindSpeedCmS-p.MinimumMovingSpeedCmS));
    const double rpm=AudioUnit(
        (x.EngineRpm-p.MinimumEngineRpm)/
        (p.MaximumEngineRpm-p.MinimumEngineRpm));
    const double lowRpm=AudioUnit(
        (p.IdleBandEndRpm-x.EngineRpm)/
        (p.IdleBandEndRpm-p.MinimumEngineRpm));
    const double loaded=AudioUnit(
        x.EngineLoad*0.65+x.Throttle*0.35);
    const double contact=x.PhysicalContactWheels>0?
        static_cast<double>(x.PhysicalContactWheels)/4.0:0.0;
    const double slip=std::max(
        std::abs(x.WheelLateralSlip),
        std::abs(x.WheelLongitudinalSlip));
    const double scrub=AudioUnit(
        (slip-p.MinimumScrubSlip)/
        (p.MaximumScrubSlip-p.MinimumScrubSlip));
    const double brake=AudioUnit(
        (x.WheelBrakeTorqueNm-p.MinimumBrakeTorqueNm)/
        (p.MaximumBrakeTorqueNm-p.MinimumBrakeTorqueNm));
    const double impact=AudioUnit(
        (x.CollisionImpulseNs-p.CollisionSoundThresholdNs)/
        (p.FullCollisionSoundNs-p.CollisionSoundThresholdNs));
    out.Gain[Index(VehicleSoundBus::EngineIdle)]=
        0.22+0.35*lowRpm;
    out.Gain[Index(VehicleSoundBus::EngineLoad)]=
        (0.12+0.64*loaded)*rpm;
    out.Gain[Index(VehicleSoundBus::Intake)]=
        loaded*(0.15+0.50*rpm);
    out.Gain[Index(VehicleSoundBus::TireScrub)]=
        0.85*contact*scrub*moving;
    out.Gain[Index(VehicleSoundBus::Gravel)]=
        0.75*contact*x.GravelContactFraction*moving;
    out.Gain[Index(VehicleSoundBus::BrakeSqueal)]=
        0.58*contact*brake*scrub*moving;
    out.Gain[Index(VehicleSoundBus::Wind)]=
        0.72*moving*moving;
    out.Gain[Index(VehicleSoundBus::Collision)]=impact;
    out.Gain[Index(VehicleSoundBus::ReverseAlert)]=
        x.Gear==-1 && speed>p.MinimumMovingSpeedCmS ?
            0.30*moving:0.0;
    out.EnginePitch=0.72+1.12*rpm;
    out.RealCollisionEvent=
        x.CollisionImpulseNs>=p.CollisionSoundThresholdNs;
    LimitTotalAudioGain(out,p);
    return out;
}
class PhysicalVehicleAudioSupervisor final
{
public:
    bool Configure(PhysicalVehicleSoundSettings settings) noexcept
    {
        if(!ValidVehicleSoundSettings(settings))
            return false;
        Settings=settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Previous={};
        LastImpactNs=0.0;
        HaveFrame=false;
        QualifiedFrames=0;
        RejectedFrames=0;
        CollisionCues=0;
    }
    PhysicalVehicleAudioFrame Step(
        const PhysicalVehicleAudioSource& sample) noexcept
    {
        PhysicalVehicleAudioFrame next=
            ProposePhysicalVehicleSound(sample,Settings);
        const bool sameEpoch=HaveFrame &&
            Previous.Session==sample.Session &&
            Previous.SessionRevision==sample.SessionRevision &&
            Previous.Actor==sample.Actor;
        const double elapsed=sample.ClockSeconds-
            Previous.ClockSeconds;
        if(!next.Valid ||
           (sameEpoch &&
            (sample.Frame<=Previous.Frame ||
             elapsed<=0.0 ||
             elapsed>Settings.MaximumFrameSeconds+
                 Settings.MaximumClockSkewSeconds ||
             std::abs(elapsed-sample.DeltaSeconds)>
                 Settings.MaximumClockSkewSeconds)))
        {
            ++RejectedFrames;
            Last={};
            // Keep the prior valid frame ID; a rejected replay may
            // never gain authority just by being resubmitted.
            return {};
        }
        if(!sameEpoch)
        {
            Last={};
            LastImpactNs=0.0;
        }
        if(next.RealCollisionEvent &&
           sample.CollisionImpulseNs-LastImpactNs<
               Settings.MinimumImpactRiseNs)
        {
            next.Gain[Index(VehicleSoundBus::Collision)]=0.0;
            next.RealCollisionEvent=false;
        }
        if(next.RealCollisionEvent)
            ++CollisionCues;
        if(sameEpoch && !next.Quiet && !Last.Quiet)
        {
            const double up=Settings.MaximumGainRisePerSecond*
                sample.DeltaSeconds;
            const double down=Settings.MaximumGainFallPerSecond*
                sample.DeltaSeconds;
            for(std::size_t i=0;i<next.Gain.size();++i)
            {
                // Actual collision contacts are transient evidence,
                // not an attack curve that might hide the first impact.
                if(i==Index(VehicleSoundBus::Collision))
                    continue;
                next.Gain[i]=std::clamp(
                    next.Gain[i],Last.Gain[i]-down,
                    Last.Gain[i]+up);
            }
        }
        LimitTotalAudioGain(next,Settings);
        Previous=sample;
        HaveFrame=true;
        LastImpactNs=sample.CollisionImpulseNs;
        ++QualifiedFrames;
        Last=next;
        return Last;
    }
    PhysicalVehicleAudioFrame Snapshot() const noexcept
    {return Last;}
    std::uint64_t Frames() const noexcept
    {return QualifiedFrames;}
    std::uint64_t Rejections() const noexcept
    {return RejectedFrames;}
    std::uint64_t PhysicalCollisionCues() const noexcept
    {return CollisionCues;}
private:
    PhysicalVehicleSoundSettings Settings{};
    PhysicalVehicleAudioFrame Last{};
    PhysicalVehicleAudioSource Previous{};
    double LastImpactNs=0.0;
    std::uint64_t QualifiedFrames=0;
    std::uint64_t RejectedFrames=0;
    std::uint64_t CollisionCues=0;
    bool HaveFrame=false;
};
} // namespace latebrake::audio
