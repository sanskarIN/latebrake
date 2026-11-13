#pragma once
#include "Audio/PhysicalVehicleSoundMix.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::audio
{
inline constexpr std::size_t PhysicalSoundFleetCars=6;
inline constexpr std::size_t PhysicalSoundStereoChannels=2;
struct PhysicalSoundPoint final
{
    double X=0.0;
    double Y=0.0;
    double Z=0.0;
};
struct PhysicalSoundListener final
{
    PhysicalSoundPoint Position{};
    PhysicalSoundPoint Right{0.0,1.0,0.0};
    bool PositionMeasured=false;
    bool OrientationMeasured=false;
};
struct PhysicalSoundActorSample final
{
    PhysicalVehicleAudioSource Audio{};
    PhysicalSoundPoint WorldPosition{};
    bool ActualWorldPositionMeasured=false;
};
struct PhysicalSoundFleetSample final
{
    std::uint64_t Session=0;
    std::uint64_t Revision=0;
    std::uint64_t Frame=0;
    double PhysicsClockSeconds=0.0;
    double DeltaSeconds=0.0;
    PhysicalSoundListener Listener{};
    std::array<PhysicalSoundActorSample,PhysicalSoundFleetCars> Cars{};
};
struct PhysicalSoundFleetSettings final
{
    PhysicalVehicleSoundSettings Voice{};
    double FalloffHalfDistanceCm=1400.0;
    double SilenceDistanceCm=12000.0;
    double MaximumWorldCoordinateCm=100000000.0;
    double MaximumVoiceGain=0.95;
    double MaximumStereoBusGain=0.95;
    double MaximumSummedStereoGain=3.0;
    double MasterGain=0.85;
    double ClockAlignmentToleranceSeconds=0.0001;
    double RightVectorLengthTolerance=0.02;
    int MaximumAudibleCars=6;
    bool RequireAllSixAuthenticatedCars=true;
    bool NeverActuateUnrealAudio=true;
};
enum class PhysicalSoundFleetFault : std::uint8_t
{
    None, InvalidSettings, MissingPhysicalSources,
    InvalidSession, DuplicateActor, ChangedRoster,
    UnverifiedListener, InvalidWorldPosition,
    InconsistentEngineFrame, DiscontinuousEngineFrame,
    UnverifiedVehicleSensors, InvalidVoiceOutput
};
struct PhysicalSoundStereoVoice final
{
    std::uint64_t Actor=0;
    int Slot=-1;
    PhysicalVehicleAudioFrame Evidence{};
    std::array<std::array<double,PhysicalSoundStereoChannels>,VehicleSoundBusCount> Buses{};
    double DistanceCm=0.0;
    double Pan=0.0;
    double DistanceAttenuation=0.0;
    double Priority=0.0;
    double LeftGain=0.0;
    double RightGain=0.0;
    bool Audible=false;
    bool SourceQualified=false;
    bool CollisionPhysicsObserved=false;
};
struct PhysicalSoundFleetFrame final
{
    std::array<PhysicalSoundStereoVoice,PhysicalSoundFleetCars> Voices{};
    std::uint64_t Session=0;
    std::uint64_t Revision=0;
    std::uint64_t Frame=0;
    double PhysicsClockSeconds=0.0;
    double TotalLeftGain=0.0;
    double TotalRightGain=0.0;
    double OutputLimiterScale=1.0;
    int AudibleCars=0;
    int PhysicallyQualifiedCars=0;
    int DistanceCulledCars=0;
    int BudgetCulledCars=0;
    int ActualCollisionCues=0;
    PhysicalSoundFleetFault Fault=PhysicalSoundFleetFault::MissingPhysicalSources;
    bool Valid=false;
    bool UnrealAudioActuated=false;
};
inline bool PhysicalSoundFinite(double x) noexcept
{
    return std::isfinite(x);
}
inline bool PhysicalSoundValidPoint(
    const PhysicalSoundPoint& point,double coordinateCap) noexcept
{
    return PhysicalSoundFinite(point.X) &&
        PhysicalSoundFinite(point.Y) &&
        PhysicalSoundFinite(point.Z) &&
        std::abs(point.X)<=coordinateCap &&
        std::abs(point.Y)<=coordinateCap &&
        std::abs(point.Z)<=coordinateCap;
}
inline double PhysicalSoundDistance(
    const PhysicalSoundPoint& a,
    const PhysicalSoundPoint& b) noexcept
{
    return std::hypot(a.X-b.X,a.Y-b.Y,a.Z-b.Z);
}
inline bool PhysicalSoundValidSettings(
    const PhysicalSoundFleetSettings& p) noexcept
{
    return ValidVehicleSoundSettings(p.Voice) &&
        PhysicalSoundFinite(p.FalloffHalfDistanceCm) &&
        p.FalloffHalfDistanceCm>=100.0 &&
        PhysicalSoundFinite(p.SilenceDistanceCm) &&
        p.SilenceDistanceCm>p.FalloffHalfDistanceCm &&
        PhysicalSoundFinite(p.MaximumWorldCoordinateCm) &&
        p.MaximumWorldCoordinateCm>=p.SilenceDistanceCm &&
        PhysicalSoundFinite(p.MaximumVoiceGain) &&
        p.MaximumVoiceGain>0.0 &&
        p.MaximumVoiceGain<=1.0 &&
        PhysicalSoundFinite(p.MaximumStereoBusGain) &&
        p.MaximumStereoBusGain>0.0 &&
        p.MaximumStereoBusGain<=1.0 &&
        PhysicalSoundFinite(p.MaximumSummedStereoGain) &&
        p.MaximumSummedStereoGain>0.0 &&
        p.MaximumSummedStereoGain<=6.0 &&
        PhysicalSoundFinite(p.MasterGain) &&
        p.MasterGain>=0.0 && p.MasterGain<=1.0 &&
        PhysicalSoundFinite(p.ClockAlignmentToleranceSeconds) &&
        p.ClockAlignmentToleranceSeconds>=0.0 &&
        p.ClockAlignmentToleranceSeconds<=0.002 &&
        PhysicalSoundFinite(p.RightVectorLengthTolerance) &&
        p.RightVectorLengthTolerance>=0.0 &&
        p.RightVectorLengthTolerance<=0.10 &&
        p.MaximumAudibleCars>=1 &&
        p.MaximumAudibleCars<=static_cast<int>(PhysicalSoundFleetCars) &&
        p.RequireAllSixAuthenticatedCars &&
        p.NeverActuateUnrealAudio &&
        p.Voice.RequireOriginalChaosChassis &&
        p.Voice.RequireRealWheelPhysics &&
        p.Voice.NeverWriteUnrealAudio;
}
inline bool PhysicalSoundValidListener(
    const PhysicalSoundListener& listener,
    const PhysicalSoundFleetSettings& p) noexcept
{
    if(!listener.PositionMeasured ||
       !listener.OrientationMeasured ||
       !PhysicalSoundValidPoint(
           listener.Position,p.MaximumWorldCoordinateCm) ||
       !PhysicalSoundFinite(listener.Right.X) ||
       !PhysicalSoundFinite(listener.Right.Y) ||
       !PhysicalSoundFinite(listener.Right.Z))
        return false;
    const double squared=
        listener.Right.X*listener.Right.X+
        listener.Right.Y*listener.Right.Y+
        listener.Right.Z*listener.Right.Z;
    return PhysicalSoundFinite(squared) &&
        std::abs(std::sqrt(squared)-1.0)<=
            p.RightVectorLengthTolerance;
}
inline PhysicalSoundFleetFault ValidatePhysicalSoundFleetSample(
    const PhysicalSoundFleetSample& sample,
    const PhysicalSoundFleetSettings& p) noexcept
{
    if(!PhysicalSoundValidSettings(p))
        return PhysicalSoundFleetFault::InvalidSettings;
    if(sample.Session==0 || sample.Frame==0 ||
       !PhysicalSoundFinite(sample.PhysicsClockSeconds) ||
       sample.PhysicsClockSeconds<0.0 ||
       !PhysicalSoundFinite(sample.DeltaSeconds) ||
       sample.DeltaSeconds<=0.0 ||
       sample.DeltaSeconds>p.Voice.MaximumFrameSeconds)
        return PhysicalSoundFleetFault::InvalidSession;
    if(!PhysicalSoundValidListener(sample.Listener,p))
        return PhysicalSoundFleetFault::UnverifiedListener;
    for(std::size_t i=0;i<PhysicalSoundFleetCars;++i)
    {
        const auto& member=sample.Cars[i];
        const auto& audio=member.Audio;
        if(!ValidVehicleAudioSource(audio,p.Voice))
            return PhysicalSoundFleetFault::UnverifiedVehicleSensors;
        if(audio.Session!=sample.Session ||
           audio.SessionRevision!=sample.Revision ||
           audio.Frame!=sample.Frame ||
           std::abs(audio.ClockSeconds-sample.PhysicsClockSeconds)>
               p.ClockAlignmentToleranceSeconds ||
           std::abs(audio.DeltaSeconds-sample.DeltaSeconds)>
               p.ClockAlignmentToleranceSeconds)
            return PhysicalSoundFleetFault::InconsistentEngineFrame;
        if(!member.ActualWorldPositionMeasured ||
           !PhysicalSoundValidPoint(
               member.WorldPosition,p.MaximumWorldCoordinateCm))
            return PhysicalSoundFleetFault::InvalidWorldPosition;
        for(std::size_t j=0;j<i;++j)
        {
            if(sample.Cars[j].Audio.Actor==audio.Actor)
                return PhysicalSoundFleetFault::DuplicateActor;
        }
        if(audio.Paused!=sample.Cars[0].Audio.Paused ||
           audio.RaceRunning!=sample.Cars[0].Audio.RaceRunning)
            return PhysicalSoundFleetFault::InconsistentEngineFrame;
    }
    return PhysicalSoundFleetFault::None;
}
inline PhysicalSoundStereoVoice SpatializePhysicalSound(
    const PhysicalSoundActorSample& member,
    const PhysicalSoundListener& listener,
    const PhysicalVehicleAudioFrame& source,
    const PhysicalSoundFleetSettings& p,
    int slot) noexcept
{
    PhysicalSoundStereoVoice voice;
    voice.Actor=member.Audio.Actor;
    voice.Slot=slot;
    voice.Evidence=source;
    voice.SourceQualified=source.Valid &&
        source.OriginalPhysicsQualified &&
        !source.UnrealAudioIssued;
    voice.CollisionPhysicsObserved=
        voice.SourceQualified && source.RealCollisionEvent;
    voice.DistanceCm=PhysicalSoundDistance(
        member.WorldPosition,listener.Position);
    if(!voice.SourceQualified ||
       !PhysicalSoundFinite(voice.DistanceCm) ||
       voice.DistanceCm>=p.SilenceDistanceCm)
        return voice;
    const double distanceRatio=
        voice.DistanceCm/p.FalloffHalfDistanceCm;
    voice.DistanceAttenuation=
        1.0/(1.0+distanceRatio*distanceRatio);
    const double dx=member.WorldPosition.X-listener.Position.X;
    const double dy=member.WorldPosition.Y-listener.Position.Y;
    const double dz=member.WorldPosition.Z-listener.Position.Z;
    if(voice.DistanceCm>1.0e-6)
    {
        voice.Pan=std::clamp(
            (dx*listener.Right.X+
             dy*listener.Right.Y+
             dz*listener.Right.Z)/voice.DistanceCm,
            -1.0,1.0);
    }
    voice.Priority=source.TotalGain*
        voice.DistanceAttenuation*p.MasterGain;
    return voice;
}
inline void RenderPhysicalSoundStereo(
    PhysicalSoundStereoVoice& voice,
    const PhysicalSoundFleetSettings& p) noexcept
{
    if(voice.Priority<=0.0)
        return;
    const double leftWeight=std::sqrt((1.0-voice.Pan)*0.5);
    const double rightWeight=std::sqrt((1.0+voice.Pan)*0.5);
    const double attenuation=voice.DistanceAttenuation*p.MasterGain;
    const double proportionalVoiceCap=
        voice.Evidence.TotalGain>0.0 ?
        std::min(1.0,p.MaximumVoiceGain/
            (voice.Evidence.TotalGain*attenuation)):1.0;
    for(std::size_t bus=0;bus<VehicleSoundBusCount;++bus)
    {
        const double gain=std::clamp(
            voice.Evidence.Gain[bus]*attenuation*
                proportionalVoiceCap,
            0.0,p.MaximumStereoBusGain);
        voice.Buses[bus][0]=gain*leftWeight;
        voice.Buses[bus][1]=gain*rightWeight;
        voice.LeftGain+=voice.Buses[bus][0];
        voice.RightGain+=voice.Buses[bus][1];
    }
    voice.Audible=voice.LeftGain>0.0 || voice.RightGain>0.0;
}
class PhysicalSixCarAudioStage final
{
public:
    bool Configure(PhysicalSoundFleetSettings settings) noexcept
    {
        if(!PhysicalSoundValidSettings(settings))
            return false;
        Settings=settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        for(auto& supervisor:Supervisors)
            supervisor.Reset();
        Previous={};
        Last={};
        AcceptedFrames=0;
        RejectedFrames=0;
        Epochs=0;
        HasPrevious=false;
    }
    PhysicalSoundFleetFrame Step(
        const PhysicalSoundFleetSample& sample) noexcept
    {
        const auto fault=Preflight(sample);
        if(fault!=PhysicalSoundFleetFault::None)
        {
            ++RejectedFrames;
            PhysicalSoundFleetFrame rejected;
            rejected.Fault=fault;
            return rejected;
        }
        const bool newEpoch=!HasPrevious ||
            sample.Session!=Previous.Session ||
            sample.Revision!=Previous.Revision;
        if(newEpoch)
        {
            for(auto& supervisor:Supervisors)
                supervisor.Reset();
            ++Epochs;
        }
        PhysicalSoundFleetFrame next;
        next.Session=sample.Session;
        next.Revision=sample.Revision;
        next.Frame=sample.Frame;
        next.PhysicsClockSeconds=sample.PhysicsClockSeconds;
        next.Fault=PhysicalSoundFleetFault::None;
        for(std::size_t i=0;i<PhysicalSoundFleetCars;++i)
        {
            // Full preflight above prevents partially mutating a fleet
            // when one chassis has forged, missing or stale evidence.
            const auto source=Supervisors[i].Step(
                sample.Cars[i].Audio);
            if(!source.Valid)
            {
                // Defensive fallback: stage never promises a partially
                // observed fleet if an internal voice rejects evidence.
                ++RejectedFrames;
                next.Fault=PhysicalSoundFleetFault::InvalidVoiceOutput;
                return next;
            }
            next.Voices[i]=SpatializePhysicalSound(
                sample.Cars[i],sample.Listener,source,
                Settings,static_cast<int>(i));
            ++next.PhysicallyQualifiedCars;
            if(next.Voices[i].CollisionPhysicsObserved)
                ++next.ActualCollisionCues;
        }
        std::array<std::size_t,PhysicalSoundFleetCars> ranking{};
        for(std::size_t i=0;i<PhysicalSoundFleetCars;++i)
            ranking[i]=i;
        std::stable_sort(ranking.begin(),ranking.end(),
            [&next](std::size_t left,std::size_t right)
            {
                return next.Voices[left].Priority>
                    next.Voices[right].Priority;
            });
        for(std::size_t place=0;place<PhysicalSoundFleetCars;++place)
        {
            auto& voice=next.Voices[ranking[place]];
            if(voice.Priority<=0.0)
            {
                ++next.DistanceCulledCars;
                continue;
            }
            if(place>=static_cast<std::size_t>(Settings.MaximumAudibleCars))
            {
                ++next.BudgetCulledCars;
                continue;
            }
            RenderPhysicalSoundStereo(voice,Settings);
            if(voice.Audible)
            {
                ++next.AudibleCars;
                next.TotalLeftGain+=voice.LeftGain;
                next.TotalRightGain+=voice.RightGain;
            }
        }
        const double largest=std::max(
            next.TotalLeftGain,next.TotalRightGain);
        if(largest>Settings.MaximumSummedStereoGain)
        {
            next.OutputLimiterScale=
                Settings.MaximumSummedStereoGain/largest;
            for(auto& voice:next.Voices)
            {
                for(auto& bus:voice.Buses)
                {
                    bus[0]*=next.OutputLimiterScale;
                    bus[1]*=next.OutputLimiterScale;
                }
                voice.LeftGain*=next.OutputLimiterScale;
                voice.RightGain*=next.OutputLimiterScale;
            }
            next.TotalLeftGain*=next.OutputLimiterScale;
            next.TotalRightGain*=next.OutputLimiterScale;
        }
        next.Valid=true;
        next.UnrealAudioActuated=false;
        Previous=sample;
        HasPrevious=true;
        Last=next;
        ++AcceptedFrames;
        return Last;
    }
    PhysicalSoundFleetFrame Snapshot() const noexcept
    {
        return Last;
    }
    std::uint64_t Frames() const noexcept
    {
        return AcceptedFrames;
    }
    std::uint64_t Rejections() const noexcept
    {
        return RejectedFrames;
    }
    std::uint64_t SourceEpochs() const noexcept
    {
        return Epochs;
    }
private:
    PhysicalSoundFleetFault Preflight(
        const PhysicalSoundFleetSample& sample) const noexcept
    {
        const auto valid=ValidatePhysicalSoundFleetSample(
            sample,Settings);
        if(valid!=PhysicalSoundFleetFault::None)
            return valid;
        if(!HasPrevious)
            return PhysicalSoundFleetFault::None;
        const bool sameEpoch=
            sample.Session==Previous.Session &&
            sample.Revision==Previous.Revision;
        if(!sameEpoch)
        {
            // A new session or greater session revision may rebind all
            // six real actors. Replaying an old revision may not.
            if(sample.Session==Previous.Session &&
               sample.Revision<=Previous.Revision)
                return PhysicalSoundFleetFault::DiscontinuousEngineFrame;
            return PhysicalSoundFleetFault::None;
        }
        for(std::size_t i=0;i<PhysicalSoundFleetCars;++i)
        {
            if(sample.Cars[i].Audio.Actor!=
               Previous.Cars[i].Audio.Actor)
                return PhysicalSoundFleetFault::ChangedRoster;
        }
        if(sample.Frame!=Previous.Frame+1 ||
           sample.PhysicsClockSeconds<=
               Previous.PhysicsClockSeconds)
            return PhysicalSoundFleetFault::DiscontinuousEngineFrame;
        const double clockStep=
            sample.PhysicsClockSeconds-
                Previous.PhysicsClockSeconds;
        if(clockStep>
                Settings.Voice.MaximumFrameSeconds+
                    Settings.Voice.MaximumClockSkewSeconds ||
           std::abs(clockStep-sample.DeltaSeconds)>
                Settings.Voice.MaximumClockSkewSeconds)
            return PhysicalSoundFleetFault::DiscontinuousEngineFrame;
        return PhysicalSoundFleetFault::None;
    }
    PhysicalSoundFleetSettings Settings{};
    std::array<PhysicalVehicleAudioSupervisor,
        PhysicalSoundFleetCars> Supervisors{};
    PhysicalSoundFleetSample Previous{};
    PhysicalSoundFleetFrame Last{};
    std::uint64_t AcceptedFrames=0;
    std::uint64_t RejectedFrames=0;
    std::uint64_t Epochs=0;
    bool HasPrevious=false;
};
} // namespace latebrake::audio
