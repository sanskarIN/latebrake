#pragma once
#include "Audio/PhysicalSixCarAudioStage.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::audio
{
inline constexpr int OriginalPcmDefaultRate=48000;
inline constexpr int OriginalPcmMaximumRate=48000;
inline constexpr std::size_t OriginalPcmMaxFramesPerTick=4800;
inline constexpr std::size_t OriginalPcmChannelCount=2;
inline constexpr double OriginalPcmPi=3.14159265358979323846;
inline constexpr double OriginalPcmTwoPi=2.0*OriginalPcmPi;
enum class OriginalPcmFault : std::uint8_t
{
    None, InvalidSettings, UnverifiedPhysicalFleet,
    InvalidVoiceEvidence, ChangedRoster, StaleFrame,
    InvalidClock, InvalidMix, OutputCapacityExceeded
};
struct OriginalPcmSettings final
{
    int SampleRate=OriginalPcmDefaultRate;
    double MaximumFrameSeconds=0.10;
    double MaximumClockSkewSeconds=0.025;
    double MasterGain=0.48;
    double EngineGain=0.57;
    double TireGain=0.32;
    double GravelGain=0.28;
    double WindGain=0.23;
    double CollisionGain=0.66;
    double SquealGain=0.27;
    double AlertGain=0.19;
    double MaximumPeak=0.93;
    double ClipKnee=1.10;
    double AirNoiseHighpass=0.83;
    double DcRejection=0.998;
    double CollisionDecaySeconds=0.090;
    double SmallSignalSilence=0.0000001;
    bool RequiresAllSixRealCars=true;
    bool NeverAccessUnrealAudio=true;
    bool NeverUseSyntheticActorIdentity=true;
};
struct OriginalPcmFrame final
{
    std::array<std::int16_t,
        OriginalPcmMaxFramesPerTick*OriginalPcmChannelCount> Interleaved{};
    std::uint64_t Session=0;
    std::uint64_t Revision=0;
    std::uint64_t Frame=0;
    int SampleRate=0;
    int SampleFrames=0;
    int MeasuredVoiceCount=0;
    int ActiveVoiceCount=0;
    int SourceCollisionEvents=0;
    double MaximumAbsoluteGain=0.0;
    double RmsGain=0.0;
    OriginalPcmFault Fault=OriginalPcmFault::UnverifiedPhysicalFleet;
    bool Valid=false;
    bool Silence=true;
    bool UnrealAudioActuated=false;
};
inline bool ValidOriginalPcmSettings(
    const OriginalPcmSettings& p) noexcept
{
    return p.SampleRate>=8000 &&
        p.SampleRate<=OriginalPcmMaximumRate &&
        p.SampleRate%1000==0 &&
        std::isfinite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=0.1 &&
        std::isfinite(p.MaximumClockSkewSeconds) &&
        p.MaximumClockSkewSeconds>=0.0 &&
        p.MaximumClockSkewSeconds<=0.025 &&
        std::isfinite(p.MasterGain) &&
        p.MasterGain>=0.0 && p.MasterGain<=1.0 &&
        std::isfinite(p.EngineGain) &&
        p.EngineGain>=0.0 && p.EngineGain<=1.0 &&
        std::isfinite(p.TireGain) &&
        p.TireGain>=0.0 && p.TireGain<=1.0 &&
        std::isfinite(p.GravelGain) &&
        p.GravelGain>=0.0 && p.GravelGain<=1.0 &&
        std::isfinite(p.WindGain) &&
        p.WindGain>=0.0 && p.WindGain<=1.0 &&
        std::isfinite(p.CollisionGain) &&
        p.CollisionGain>=0.0 && p.CollisionGain<=1.0 &&
        std::isfinite(p.SquealGain) &&
        p.SquealGain>=0.0 && p.SquealGain<=1.0 &&
        std::isfinite(p.AlertGain) &&
        p.AlertGain>=0.0 && p.AlertGain<=1.0 &&
        std::isfinite(p.MaximumPeak) &&
        p.MaximumPeak>=0.1 && p.MaximumPeak<=0.99 &&
        std::isfinite(p.ClipKnee) &&
        p.ClipKnee>=0.5 && p.ClipKnee<=3.0 &&
        std::isfinite(p.AirNoiseHighpass) &&
        p.AirNoiseHighpass>=0.0 &&
        p.AirNoiseHighpass<=0.99 &&
        std::isfinite(p.DcRejection) &&
        p.DcRejection>=0.9 && p.DcRejection<1.0 &&
        std::isfinite(p.CollisionDecaySeconds) &&
        p.CollisionDecaySeconds>=0.01 &&
        p.CollisionDecaySeconds<=0.15 &&
        std::isfinite(p.SmallSignalSilence) &&
        p.SmallSignalSilence>=0.0 &&
        p.SmallSignalSilence<=1.0e-5 &&
        p.RequiresAllSixRealCars &&
        p.NeverAccessUnrealAudio &&
        p.NeverUseSyntheticActorIdentity;
}
inline bool ValidOriginalPcmFleetInput(
    const PhysicalSoundFleetFrame& frame,
    const OriginalPcmSettings& p) noexcept
{
    if(!ValidOriginalPcmSettings(p) ||
       !frame.Valid || frame.UnrealAudioActuated ||
       frame.Fault!=PhysicalSoundFleetFault::None ||
       frame.Session==0 || frame.Frame==0 ||
       !std::isfinite(frame.PhysicsClockSeconds) ||
       frame.PhysicsClockSeconds<0.0 ||
       frame.PhysicallyQualifiedCars!=static_cast<int>(PhysicalSoundFleetCars) ||
       frame.AudibleCars<0 ||
       frame.AudibleCars>static_cast<int>(PhysicalSoundFleetCars) ||
       !std::isfinite(frame.TotalLeftGain) ||
       !std::isfinite(frame.TotalRightGain) ||
       frame.TotalLeftGain<0.0 || frame.TotalRightGain<0.0 ||
       frame.TotalLeftGain>6.0 || frame.TotalRightGain>6.0)
        return false;
    double totalLeft=0.0;
    double totalRight=0.0;
    for(std::size_t i=0;i<PhysicalSoundFleetCars;++i)
    {
        const auto& voice=frame.Voices[i];
        if(!voice.SourceQualified ||
           voice.Actor==0 ||
           voice.Slot!=static_cast<int>(i) ||
           !voice.Evidence.Valid ||
           !voice.Evidence.OriginalPhysicsQualified ||
           voice.Evidence.UnrealAudioIssued ||
           voice.Evidence.Session!=frame.Session ||
           voice.Evidence.Frame!=frame.Frame ||
           voice.Evidence.Actor!=voice.Actor ||
           !std::isfinite(voice.Evidence.EnginePitch) ||
           voice.Evidence.EnginePitch<0.4 ||
           voice.Evidence.EnginePitch>2.2 ||
           !std::isfinite(voice.LeftGain) ||
           !std::isfinite(voice.RightGain) ||
           voice.LeftGain<0.0 || voice.RightGain<0.0 ||
           voice.LeftGain>6.0 || voice.RightGain>6.0)
            return false;
        for(std::size_t j=0;j<i;++j)
        {
            if(frame.Voices[j].Actor==voice.Actor)
                return false;
        }
        double sumLeft=0.0;
        double sumRight=0.0;
        for(const auto& bus:voice.Buses)
        {
            if(!std::isfinite(bus[0]) ||
               !std::isfinite(bus[1]) ||
               bus[0]<0.0 || bus[1]<0.0 ||
               bus[0]>1.0 || bus[1]>1.0)
                return false;
            sumLeft+=bus[0];
            sumRight+=bus[1];
        }
        if(std::abs(sumLeft-voice.LeftGain)>1.0e-5 ||
           std::abs(sumRight-voice.RightGain)>1.0e-5)
            return false;
        if((voice.LeftGain>1e-8 || voice.RightGain>1e-8)!=
           voice.Audible)
            return false;
        totalLeft+=voice.LeftGain;
        totalRight+=voice.RightGain;
    }
    return std::abs(totalLeft-frame.TotalLeftGain)<=1.0e-5 &&
        std::abs(totalRight-frame.TotalRightGain)<=1.0e-5;
}
inline double OriginalPcmSaturate(double value) noexcept
{
    if(!std::isfinite(value))
        return 0.0;
    return std::clamp(value,-1.0,1.0);
}
inline std::uint32_t OriginalPcmNextNoise(
    std::uint32_t& state) noexcept
{
    if(state==0)
        state=0xa3c59ac3U;
    state^=state<<13;
    state^=state>>17;
    state^=state<<5;
    return state;
}
inline double OriginalPcmBipolarNoise(
    std::uint32_t& state) noexcept
{
    const auto value=OriginalPcmNextNoise(state);
    return static_cast<double>(value)/2147483647.5-1.0;
}
class OriginalSixCarPcmSynth final
{
public:
    bool Configure(OriginalPcmSettings settings) noexcept
    {
        if(!ValidOriginalPcmSettings(settings))
            return false;
        Settings=settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Voices={};
        Previous={};
        HavePrevious=false;
        Accepted=0;
        Rejected=0;
        StartedEpochs=0;
        GeneratedSampleFrames=0;
        PreviousInputLeft=0.0;
        PreviousInputRight=0.0;
        PreviousOutputLeft=0.0;
        PreviousOutputRight=0.0;
        Last={};
    }
    OriginalPcmFrame Step(
        const PhysicalSoundFleetFrame& input,
        double deltaSeconds) noexcept
    {
        OriginalPcmFrame result;
        result.Fault=Preflight(input,deltaSeconds);
        if(result.Fault!=OriginalPcmFault::None)
        {
            ++Rejected;
            return result;
        }
        const int count=static_cast<int>(
            std::llround(deltaSeconds*Settings.SampleRate));
        if(count<=0 ||
           count>static_cast<int>(OriginalPcmMaxFramesPerTick))
        {
            result.Fault=OriginalPcmFault::OutputCapacityExceeded;
            ++Rejected;
            return result;
        }
        const bool newEpoch=!HavePrevious ||
            input.Session!=Previous.Session ||
            input.Revision!=Previous.Revision;
        if(newEpoch)
        {
            Voices={};
            ++StartedEpochs;
            PreviousInputLeft=0.0;
            PreviousInputRight=0.0;
            PreviousOutputLeft=0.0;
            PreviousOutputRight=0.0;
            for(std::size_t i=0;i<PhysicalSoundFleetCars;++i)
            {
                auto& voice=Voices[i];
                const std::uint64_t actor=input.Voices[i].Actor;
                voice.Actor=actor;
                voice.Noise=static_cast<std::uint32_t>(
                    actor^(actor>>32)^0x6e624eb7U);
                if(voice.Noise==0)
                    voice.Noise=0x9e3779b9U;
            }
        }
        result.Session=input.Session;
        result.Revision=input.Revision;
        result.Frame=input.Frame;
        result.SampleRate=Settings.SampleRate;
        result.SampleFrames=count;
        result.MeasuredVoiceCount=static_cast<int>(PhysicalSoundFleetCars);
        result.Fault=OriginalPcmFault::None;
        result.Valid=true;
        result.UnrealAudioActuated=false;
        for(const auto& voice:input.Voices)
        {
            if(voice.CollisionPhysicsObserved)
                ++result.SourceCollisionEvents;
            if(voice.Audible)
                ++result.ActiveVoiceCount;
        }
        const double step=1.0/static_cast<double>(Settings.SampleRate);
        double energy=0.0;
        double peak=0.0;
        for(int sample=0;sample<count;++sample)
        {
            double channels[2]{0.0,0.0};
            for(std::size_t car=0;car<PhysicalSoundFleetCars;++car)
            {
                const auto& observed=input.Voices[car];
                auto& state=Voices[car];
                const double speedFactor=observed.Evidence.EnginePitch;
                const double carCharacteristic=
                    1.0+static_cast<double>(car)*0.042;
                const double fundamental=std::clamp(
                    75.0*speedFactor*carCharacteristic,
                    30.0,880.0);
                state.EnginePhase+=OriginalPcmTwoPi*fundamental*step;
                if(state.EnginePhase>=OriginalPcmTwoPi)
                    state.EnginePhase=std::fmod(
                        state.EnginePhase,OriginalPcmTwoPi);
                state.BrakePhase+=OriginalPcmTwoPi*
                    (1200.0+fundamental*0.22)*step;
                if(state.BrakePhase>=OriginalPcmTwoPi)
                    state.BrakePhase=std::fmod(
                        state.BrakePhase,OriginalPcmTwoPi);
                state.AlertPhase+=OriginalPcmTwoPi*290.0*step;
                if(state.AlertPhase>=OriginalPcmTwoPi)
                    state.AlertPhase=std::fmod(
                        state.AlertPhase,OriginalPcmTwoPi);
                const double noise=OriginalPcmBipolarNoise(state.Noise);
                const double highpassed=noise-
                    Settings.AirNoiseHighpass*state.LastNoise;
                state.LastNoise=noise;
                const double engine=(
                    std::sin(state.EnginePhase)+
                    0.31*std::sin(2.0*state.EnginePhase)+
                    0.14*std::sin(3.0*state.EnginePhase))/
                    1.45;
                const double intake=(
                    std::sin(3.0*state.EnginePhase)+
                    0.21*std::sin(5.0*state.EnginePhase))/1.21;
                const double squeal=std::sin(state.BrakePhase);
                const double alert=std::sin(state.AlertPhase);
                const double gravel=std::clamp(
                    noise*2.1,-1.0,1.0);
                if(observed.CollisionPhysicsObserved && sample==0)
                {
                    state.CollisionRemaining=static_cast<int>(
                        Settings.CollisionDecaySeconds*
                        Settings.SampleRate);
                    state.CollisionTotal=
                        std::max(1,state.CollisionRemaining);
                }
                double impact=0.0;
                if(state.CollisionRemaining>0)
                {
                    const double sustain=static_cast<double>(
                        state.CollisionRemaining)/state.CollisionTotal;
                    impact=noise*sustain*sustain;
                    --state.CollisionRemaining;
                }
                const auto& buses=observed.Buses;
                const auto index=[](VehicleSoundBus bus)
                {
                    return static_cast<std::size_t>(bus);
                };
                for(int side=0;side<2;++side)
                {
                    double sampleGain=0.0;
                    sampleGain+=Settings.EngineGain*engine*
                        (0.54*buses[index(VehicleSoundBus::EngineIdle)][side]+
                         buses[index(VehicleSoundBus::EngineLoad)][side]);
                    sampleGain+=Settings.EngineGain*0.35*intake*
                        buses[index(VehicleSoundBus::Intake)][side];
                    sampleGain+=Settings.TireGain*highpassed*
                        buses[index(VehicleSoundBus::TireScrub)][side];
                    sampleGain+=Settings.GravelGain*gravel*
                        buses[index(VehicleSoundBus::Gravel)][side];
                    sampleGain+=Settings.SquealGain*squeal*
                        buses[index(VehicleSoundBus::BrakeSqueal)][side];
                    sampleGain+=Settings.WindGain*highpassed*
                        buses[index(VehicleSoundBus::Wind)][side];
                    sampleGain+=Settings.CollisionGain*impact*
                        buses[index(VehicleSoundBus::Collision)][side];
                    sampleGain+=Settings.AlertGain*alert*
                        buses[index(VehicleSoundBus::ReverseAlert)][side];
                    channels[side]+=sampleGain;
                }
            }
            for(int side=0;side<2;++side)
            {
                const double raw=channels[side]*Settings.MasterGain;
                const double previousInput=side==0?
                    PreviousInputLeft:PreviousInputRight;
                const double previousOutput=side==0?
                    PreviousOutputLeft:PreviousOutputRight;
                const double dcFiltered=raw-previousInput+
                    Settings.DcRejection*previousOutput;
                if(side==0)
                {
                    PreviousInputLeft=raw;
                    PreviousOutputLeft=dcFiltered;
                }
                else
                {
                    PreviousInputRight=raw;
                    PreviousOutputRight=dcFiltered;
                }
                const double limited=Settings.MaximumPeak*
                    std::tanh(dcFiltered*Settings.ClipKnee)/
                    std::tanh(Settings.ClipKnee);
                const double output=std::clamp(
                    limited,-Settings.MaximumPeak,
                    Settings.MaximumPeak);
                const auto index=static_cast<std::size_t>(
                    sample*2+side);
                result.Interleaved[index]=static_cast<std::int16_t>(
                    std::lround(output*32767.0));
                peak=std::max(peak,std::abs(output));
                energy+=output*output;
            }
        }
        if(input.AudibleCars==0 || Settings.MasterGain==0.0)
        {
            // Silence must be immediate on pause or mute, not a decaying
            // DC filter artifact that sounds like a stopped race running.
            result.Interleaved.fill(0);
            energy=0.0;
            peak=0.0;
            PreviousInputLeft=PreviousInputRight=0.0;
            PreviousOutputLeft=PreviousOutputRight=0.0;
        }
        result.MaximumAbsoluteGain=peak;
        result.RmsGain=std::sqrt(
            energy/static_cast<double>(count*2));
        result.Silence=peak<=Settings.SmallSignalSilence;
        Previous=input;
        HavePrevious=true;
        ++Accepted;
        GeneratedSampleFrames+=static_cast<std::uint64_t>(count);
        Last=result;
        return result;
    }
    OriginalPcmFrame Snapshot() const noexcept
    {
        return Last;
    }
    std::uint64_t AcceptedFrames() const noexcept
    {
        return Accepted;
    }
    std::uint64_t RejectedFrames() const noexcept
    {
        return Rejected;
    }
    std::uint64_t SessionEpochs() const noexcept
    {
        return StartedEpochs;
    }
    std::uint64_t ProducedSampleFrames() const noexcept
    {
        return GeneratedSampleFrames;
    }
private:
    struct VoiceState final
    {
        std::uint64_t Actor=0;
        std::uint32_t Noise=0;
        double EnginePhase=0.0;
        double BrakePhase=0.0;
        double AlertPhase=0.0;
        double LastNoise=0.0;
        int CollisionRemaining=0;
        int CollisionTotal=1;
    };
    OriginalPcmFault Preflight(
        const PhysicalSoundFleetFrame& input,
        double deltaSeconds) const noexcept
    {
        if(!ValidOriginalPcmSettings(Settings))
            return OriginalPcmFault::InvalidSettings;
        if(!ValidOriginalPcmFleetInput(input,Settings))
            return OriginalPcmFault::UnverifiedPhysicalFleet;
        if(!std::isfinite(deltaSeconds) ||
           deltaSeconds<=0.0 ||
           deltaSeconds>Settings.MaximumFrameSeconds)
            return OriginalPcmFault::InvalidClock;
        if(!HavePrevious)
            return OriginalPcmFault::None;
        const bool sameEpoch=input.Session==Previous.Session &&
            input.Revision==Previous.Revision;
        if(!sameEpoch)
        {
            if(input.Session==Previous.Session &&
               input.Revision<=Previous.Revision)
                return OriginalPcmFault::StaleFrame;
            return OriginalPcmFault::None;
        }
        if(input.Frame!=Previous.Frame+1)
            return OriginalPcmFault::StaleFrame;
        const double elapsed=input.PhysicsClockSeconds-
            Previous.PhysicsClockSeconds;
        if(elapsed<=0.0 ||
           elapsed>Settings.MaximumFrameSeconds+
               Settings.MaximumClockSkewSeconds ||
           std::abs(elapsed-deltaSeconds)>
               Settings.MaximumClockSkewSeconds)
            return OriginalPcmFault::InvalidClock;
        for(std::size_t i=0;i<PhysicalSoundFleetCars;++i)
        {
            if(input.Voices[i].Actor!=Voices[i].Actor)
                return OriginalPcmFault::ChangedRoster;
        }
        return OriginalPcmFault::None;
    }
    OriginalPcmSettings Settings{};
    std::array<VoiceState,PhysicalSoundFleetCars> Voices{};
    PhysicalSoundFleetFrame Previous{};
    OriginalPcmFrame Last{};
    bool HavePrevious=false;
    std::uint64_t Accepted=0;
    std::uint64_t Rejected=0;
    std::uint64_t StartedEpochs=0;
    std::uint64_t GeneratedSampleFrames=0;
    double PreviousInputLeft=0.0;
    double PreviousInputRight=0.0;
    double PreviousOutputLeft=0.0;
    double PreviousOutputRight=0.0;
};
} // namespace latebrake::audio
