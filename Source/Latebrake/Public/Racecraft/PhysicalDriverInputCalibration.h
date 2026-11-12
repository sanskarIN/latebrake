#pragma once
#include "Racecraft/RacecraftFrames.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
// Deterministic source-only input sanitation. Never fabricates input:
// samples must be read from the real player controller each engine frame.
enum class DriverInputDevice : std::uint8_t
{
    Unknown, Keyboard, Gamepad, SteeringWheel, Accessibility
};
enum class DriverInputSourceFault : std::uint8_t
{
    None, DeviceDisconnected, InputStale, WrongSession,
    InvalidDevice, InvalidAxis, UnsafeCalibration, Conflict,
    UnrealAuthorityMissing, FocusLost
};
enum class DriverAxis : std::uint8_t
{
    Throttle, Brake, Clutch, Steering, Handbrake, Count
};
inline constexpr std::size_t DriverAxisCount=
    static_cast<std::size_t>(DriverAxis::Count);
struct DriverAxisCalibration final
{
    double MinimumRaw=-1.0;
    double MaximumRaw=1.0;
    double NeutralRaw=0.0;
    double Deadzone=0.05;
    double Saturation=0.98;
    double Exponent=1.4;
    double Inversion=1.0;
    double Offset=0.0;
    double Gain=1.0;
    double MaximumRisePerSecond=5.0;
    double MaximumFallPerSecond=8.0;
    double FilterSeconds=0.025;
    double HysteresisFraction=0.01;
    double DriftTolerance=0.02;
    bool Bidirectional=false;
    bool PreferAnalog=true;
    bool ZeroIfMissing=true;
};
struct DriverInputSettings final
{
    std::array<DriverAxisCalibration,DriverAxisCount> Axes{};
    double MaximumFrameSeconds=0.10;
    double MaximumSampleAgeSeconds=0.10;
    double MaximumClockSkewSeconds=0.04;
    double MaximumSteeringFraction=1.0;
    double MaximumThrottleFraction=1.0;
    double MaximumBrakeFraction=1.0;
    double MaximumClutchFraction=1.0;
    double MaximumHandbrakeFraction=1.0;
    double BrakeOverThrottleThreshold=0.05;
    double MinimumBrakeForConflict=0.20;
    double FocusRestoreGuardSeconds=0.15;
    double DeviceReconnectionGuardSeconds=0.12;
    double MinimumSampleConfidence=0.75;
    double MaximumAxisDifferenceFromObserved=1.0;
    double MaximumSimultaneousThrottleAndBrake=1.0;
    double ZeroSpeedSteeringAllowance=1.0;
    double MaximumInputLatencyMs=125.0;
    double ControllerClockPrecisionSeconds=0.001;
    bool RequireActualPlayerController=true;
    bool RequireGameWindowFocus=true;
    bool RequireEngineFrameIdentity=true;
    bool EmergencyBrakeOnLostFocus=true;
    bool InterlockThrottleDuringBrake=true;
    bool PreservePlayerBrake=true;
};
struct DriverInputAxisSample final
{
    double RawValue=0.0;
    double SourceTimeSeconds=0.0;
    double LastDigitalEdgeSeconds=0.0;
    double SensorConfidence=1.0;
    bool Present=false;
    bool AnalogSource=false;
    bool DeviceAuthoritative=false;
    bool ValidHardwareEvent=false;
};
struct DriverInputObservation final
{
    RaceIdentity Identity{};
    std::array<DriverInputAxisSample,DriverAxisCount> Axes{};
    DriverInputDevice Device=DriverInputDevice::Unknown;
    std::uint64_t DeviceToken=0;
    double PhysicsClockSeconds=0.0;
    double ControllerClockSeconds=0.0;
    double DeltaSeconds=0.0;
    double MeasuredInputLatencyMs=0.0;
    double VehicleSpeedCmS=0.0;
    bool OriginalPlayerPawn=false;
    bool ActualControllerReady=false;
    bool WindowHasFocus=false;
    bool DeviceConnected=false;
    bool RaceInputAllowed=false;
    bool HumanDriverAuthenticated=false;
};
struct DriverAxisResult final
{
    double RawValue=0.0;
    double CalibratedValue=0.0;
    double DeadzoneOutput=0.0;
    double CurveOutput=0.0;
    double SmoothedValue=0.0;
    double AppliedRateLimit=0.0;
    double SourceAgeSeconds=0.0;
    double SourceConfidence=0.0;
    double ResponseGain=0.0;
    double LastEdgeAgeSeconds=0.0;
    bool DeadzoneApplied=false;
    bool Inverted=false;
    bool DigitalOverride=false;
    bool SourceQualified=false;
    bool Valid=false;
};
struct DriverInputDecision final
{
    RaceIdentity Identity{};
    DriverInputDevice Device=DriverInputDevice::Unknown;
    std::uint64_t DeviceToken=0;
    std::array<DriverAxisResult,DriverAxisCount> Axes{};
    double Throttle=0.0;
    double Brake=1.0;
    double Steering=0.0;
    double Clutch=0.0;
    double Handbrake=0.0;
    double MaximumSourceAgeSeconds=0.0;
    double MaximumLatencyMs=0.0;
    double MinimumSourceConfidence=0.0;
    double BrakeThrottleOverlap=0.0;
    double SteeringRateFraction=0.0;
    int QualifiedAxes=0;
    int MissingAxes=0;
    int DeadzoneAxes=0;
    DriverInputSourceFault Fault=DriverInputSourceFault::None;
    bool DriverBrakePriority=false;
    bool EmergencySafetyStop=false;
    bool PhysicalControllerObserved=false;
    bool ActualUnrealControlsApplied=false;
    bool Valid=false;
};
inline bool ValidDriverAxisCalibration(
    DriverAxisCalibration p) noexcept
{
    return Finite(p.MinimumRaw) &&
        Finite(p.MaximumRaw) &&
        p.MaximumRaw>p.MinimumRaw &&
        Finite(p.NeutralRaw) &&
        p.NeutralRaw>=p.MinimumRaw &&
        p.NeutralRaw<=p.MaximumRaw &&
        Finite(p.Deadzone) &&
        p.Deadzone>=0.0 &&
        p.Deadzone<0.90 &&
        Finite(p.Saturation) &&
        p.Saturation>p.Deadzone &&
        p.Saturation<=1.0 &&
        Finite(p.Exponent) &&
        p.Exponent>=0.30 &&
        p.Exponent<=5.0 &&
        Finite(p.Inversion) &&
        std::abs(p.Inversion)==1.0 &&
        Finite(p.Offset) &&
        std::abs(p.Offset)<=1.0 &&
        Finite(p.Gain) &&
        p.Gain>0.0 &&
        p.Gain<=4.0 &&
        Finite(p.MaximumRisePerSecond) &&
        p.MaximumRisePerSecond>0.0 &&
        Finite(p.MaximumFallPerSecond) &&
        p.MaximumFallPerSecond>0.0 &&
        Finite(p.FilterSeconds) &&
        p.FilterSeconds>=0.0 &&
        Finite(p.HysteresisFraction) &&
        p.HysteresisFraction>=0.0 &&
        p.HysteresisFraction<=p.Deadzone &&
        Finite(p.DriftTolerance) &&
        p.DriftTolerance>=0.0 &&
        p.DriftTolerance<=p.Deadzone;
}
inline bool ValidDriverInputSettings(
    const DriverInputSettings& p) noexcept
{
    for(const auto& axis:p.Axes)
        if (!ValidDriverAxisCalibration(axis))
            return false;
    return Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MaximumSampleAgeSeconds) &&
        p.MaximumSampleAgeSeconds>0.0 &&
        Finite(p.MaximumClockSkewSeconds) &&
        p.MaximumClockSkewSeconds>=0.0 &&
        Finite(p.MaximumSteeringFraction) &&
        p.MaximumSteeringFraction>0.0 &&
        p.MaximumSteeringFraction<=1.0 &&
        Finite(p.MaximumThrottleFraction) &&
        p.MaximumThrottleFraction>0.0 &&
        p.MaximumThrottleFraction<=1.0 &&
        Finite(p.MaximumBrakeFraction) &&
        p.MaximumBrakeFraction>0.0 &&
        p.MaximumBrakeFraction<=1.0 &&
        Finite(p.MaximumClutchFraction) &&
        p.MaximumClutchFraction>0.0 &&
        p.MaximumClutchFraction<=1.0 &&
        Finite(p.MaximumHandbrakeFraction) &&
        p.MaximumHandbrakeFraction>0.0 &&
        p.MaximumHandbrakeFraction<=1.0 &&
        Finite(p.BrakeOverThrottleThreshold) &&
        p.BrakeOverThrottleThreshold>=0.0 &&
        p.BrakeOverThrottleThreshold<1.0 &&
        Finite(p.MinimumBrakeForConflict) &&
        p.MinimumBrakeForConflict>0.0 &&
        p.MinimumBrakeForConflict<=1.0 &&
        Finite(p.FocusRestoreGuardSeconds) &&
        p.FocusRestoreGuardSeconds>=0.0 &&
        Finite(p.DeviceReconnectionGuardSeconds) &&
        p.DeviceReconnectionGuardSeconds>=0.0 &&
        Finite(p.MinimumSampleConfidence) &&
        p.MinimumSampleConfidence>0.0 &&
        p.MinimumSampleConfidence<=1.0 &&
        Finite(p.MaximumAxisDifferenceFromObserved) &&
        p.MaximumAxisDifferenceFromObserved>=0.0 &&
        Finite(p.MaximumSimultaneousThrottleAndBrake) &&
        p.MaximumSimultaneousThrottleAndBrake>0.0 &&
        Finite(p.ZeroSpeedSteeringAllowance) &&
        p.ZeroSpeedSteeringAllowance>0.0 &&
        Finite(p.MaximumInputLatencyMs) &&
        p.MaximumInputLatencyMs>0.0 &&
        Finite(p.ControllerClockPrecisionSeconds) &&
        p.ControllerClockPrecisionSeconds>0.0;
}
inline bool ValidDriverInputObservation(
    const DriverInputObservation& o,
    const DriverInputSettings& p) noexcept
{
    if (!ValidDriverInputSettings(p) ||
        !PhysicalActorIdentityValid(o.Identity) ||
        o.Identity.Frame==0 ||
        o.Device==DriverInputDevice::Unknown ||
        o.DeviceToken==0 ||
        !o.DeviceConnected ||
        !o.RaceInputAllowed ||
        !o.HumanDriverAuthenticated ||
        (p.RequireActualPlayerController &&
         (!o.ActualControllerReady ||
          !o.OriginalPlayerPawn)) ||
        (p.RequireGameWindowFocus && !o.WindowHasFocus) ||
        !Finite(o.PhysicsClockSeconds) ||
        o.PhysicsClockSeconds<0.0 ||
        !Finite(o.ControllerClockSeconds) ||
        o.ControllerClockSeconds<0.0 ||
        std::abs(o.PhysicsClockSeconds-
                 o.ControllerClockSeconds)>
            p.MaximumClockSkewSeconds ||
        !Finite(o.DeltaSeconds) ||
        o.DeltaSeconds<=0.0 ||
        o.DeltaSeconds>p.MaximumFrameSeconds ||
        !Finite(o.MeasuredInputLatencyMs) ||
        o.MeasuredInputLatencyMs<0.0 ||
        o.MeasuredInputLatencyMs>
            p.MaximumInputLatencyMs ||
        !Finite(o.VehicleSpeedCmS) ||
        o.VehicleSpeedCmS<0.0 ||
        o.VehicleSpeedCmS>MaximumPhysicalSpeedCmS)
        return false;
    for(const auto& axis:o.Axes)
    {
        if (!Finite(axis.RawValue) ||
            !Finite(axis.SourceTimeSeconds) ||
            !Finite(axis.LastDigitalEdgeSeconds) ||
            !Finite(axis.SensorConfidence) ||
            axis.SensorConfidence<0.0 ||
            axis.SensorConfidence>1.0)
            return false;
        if (axis.Present &&
            (!axis.DeviceAuthoritative ||
             !axis.ValidHardwareEvent ||
             axis.SensorConfidence<
                 p.MinimumSampleConfidence ||
             axis.SourceTimeSeconds>o.ControllerClockSeconds ||
             o.ControllerClockSeconds-
                axis.SourceTimeSeconds>
                    p.MaximumSampleAgeSeconds))
            return false;
    }
    return true;
}
inline double NormalizeDriverRaw(
    double raw,DriverAxisCalibration p) noexcept
{
    if (!ValidDriverAxisCalibration(p) || !Finite(raw))
        return 0.0;
    const double positive=raw>=p.NeutralRaw ?
        SafeDivision(raw-p.NeutralRaw,
                     p.MaximumRaw-p.NeutralRaw):
        SafeDivision(raw-p.NeutralRaw,
                     p.NeutralRaw-p.MinimumRaw);
    return std::clamp(positive,-1.0,1.0);
}
inline double ApplyDriverAxisDeadzone(
    double normalized,DriverAxisCalibration p) noexcept
{
    if (!Finite(normalized) ||
        !ValidDriverAxisCalibration(p))
        return 0.0;
    const double magnitude=std::abs(normalized);
    if (magnitude<=p.Deadzone)
        return 0.0;
    const double scale=Clamp01(SafeDivision(
        magnitude-p.Deadzone,
        p.Saturation-p.Deadzone));
    return std::copysign(scale,normalized);
}
inline DriverAxisResult CalibrateDriverAxis(
    DriverInputAxisSample s,DriverAxisCalibration p,
    double prior,double dt,double now) noexcept
{
    DriverAxisResult out;
    if (!ValidDriverAxisCalibration(p) ||
        !Finite(s.RawValue) || !Finite(prior) ||
        !Finite(dt) || dt<=0.0 ||
        !Finite(now) ||
        !Finite(s.SourceTimeSeconds) ||
        !Finite(s.SensorConfidence) ||
        s.SensorConfidence<0.0 ||
        s.SensorConfidence>1.0 ||
        !s.Present ||
        !s.ValidHardwareEvent ||
        !s.DeviceAuthoritative)
        return out;
    out.RawValue=s.RawValue;
    out.SourceAgeSeconds=std::max(
        0.0,now-s.SourceTimeSeconds);
    out.LastEdgeAgeSeconds=std::max(
        0.0,now-s.LastDigitalEdgeSeconds);
    out.SourceConfidence=s.SensorConfidence;
    const double normalized=NormalizeDriverRaw(
        s.RawValue,p)*p.Inversion;
    out.CalibratedValue=normalized;
    out.DeadzoneOutput=ApplyDriverAxisDeadzone(
        normalized,p);
    out.DeadzoneApplied=
        out.DeadzoneOutput==0.0 &&
        std::abs(normalized)>0.0;
    const double adjusted=std::copysign(
        std::pow(std::abs(out.DeadzoneOutput),p.Exponent),
        out.DeadzoneOutput);
    out.CurveOutput=std::clamp(
        adjusted*p.Gain+p.Offset,-1.0,1.0);
    if (!p.Bidirectional)
        out.CurveOutput=std::max(0.0,out.CurveOutput);
    out.AppliedRateLimit=RateLimited(
        prior,out.CurveOutput,
        p.MaximumRisePerSecond,p.MaximumFallPerSecond,dt);
    out.SmoothedValue=ExponentialResponse(
        prior,out.AppliedRateLimit,p.FilterSeconds,dt);
    out.ResponseGain=SafeDivision(
        out.SmoothedValue,normalized);
    out.Inverted=p.Inversion<0.0;
    out.DigitalOverride=!s.AnalogSource;
    out.SourceQualified=true;
    out.Valid=Finite(out.SmoothedValue) &&
        std::abs(out.SmoothedValue)<=1.0;
    return out;
}
inline DriverInputDecision CalibratePhysicalDriverInput(
    const DriverInputObservation& o,
    const DriverInputSettings& settings,
    const std::array<double,DriverAxisCount>& previous={}) noexcept
{
    DriverInputDecision out;
    if (!ValidDriverInputObservation(o,settings))
        return out;
    out.Identity=o.Identity;
    out.Device=o.Device;
    out.DeviceToken=o.DeviceToken;
    out.MinimumSourceConfidence=1.0;
    out.MaximumLatencyMs=o.MeasuredInputLatencyMs;
    for(std::size_t i=0;i<DriverAxisCount;++i)
    {
        const auto& raw=o.Axes[i];
        if (!raw.Present)
        {
            if (!settings.Axes[i].ZeroIfMissing)
                return {};
            ++out.MissingAxes;
            out.Axes[i].Valid=true;
            continue;
        }
        const DriverAxisResult axis=CalibrateDriverAxis(
            raw,settings.Axes[i],previous[i],
            o.DeltaSeconds,o.ControllerClockSeconds);
        if (!axis.Valid)
            return {};
        out.Axes[i]=axis;
        ++out.QualifiedAxes;
        out.DeadzoneAxes+=axis.DeadzoneApplied?1:0;
        out.MinimumSourceConfidence=std::min(
            out.MinimumSourceConfidence,
            axis.SourceConfidence);
        out.MaximumSourceAgeSeconds=std::max(
            out.MaximumSourceAgeSeconds,
            axis.SourceAgeSeconds);
    }
    const auto get=[&](DriverAxis axis) noexcept
    {
        return out.Axes[static_cast<std::size_t>(axis)].
            SmoothedValue;
    };
    out.Throttle=std::clamp(
        get(DriverAxis::Throttle),0.0,
        settings.MaximumThrottleFraction);
    out.Brake=std::clamp(
        get(DriverAxis::Brake),0.0,
        settings.MaximumBrakeFraction);
    out.Clutch=std::clamp(
        get(DriverAxis::Clutch),0.0,
        settings.MaximumClutchFraction);
    out.Steering=std::clamp(
        get(DriverAxis::Steering),
        -settings.MaximumSteeringFraction,
        settings.MaximumSteeringFraction);
    out.Handbrake=std::clamp(
        get(DriverAxis::Handbrake),0.0,
        settings.MaximumHandbrakeFraction);
    out.BrakeThrottleOverlap=std::min(
        out.Brake,out.Throttle);
    out.DriverBrakePriority=
        out.Brake>settings.BrakeOverThrottleThreshold;
    if (settings.InterlockThrottleDuringBrake &&
        out.DriverBrakePriority)
        out.Throttle=0.0;
    out.SteeringRateFraction=Clamp01(
        SafeDivision(std::abs(out.Steering-
                previous[static_cast<std::size_t>(
                    DriverAxis::Steering)]),
                     o.DeltaSeconds*
                     settings.Axes[static_cast<std::size_t>(
                         DriverAxis::Steering)].
                         MaximumRisePerSecond));
    out.PhysicalControllerObserved=true;
    out.ActualUnrealControlsApplied=false;
    out.Fault=DriverInputSourceFault::None;
    out.Valid=Finite(out.Throttle) &&
        Finite(out.Brake) &&
        Finite(out.Steering) &&
        Finite(out.Clutch) &&
        Finite(out.Handbrake);
    return out;
}
class PhysicalDriverInputSupervisor final
{
public:
    bool Configure(DriverInputSettings settings) noexcept
    {
        if (!ValidDriverInputSettings(settings))return false;
        Settings=settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Previous.fill(0.0);
        Identity={};
        DeviceToken=0;
        GuardSeconds=0.0;
        Accepted=0;
        Rejected=0;
        Reconnects=0;
        FocusLosses=0;
        BrakePriorityFrames=0;
        HasIdentity=false;
    }
    DriverInputDecision Step(
        const DriverInputObservation& o) noexcept
    {
        const bool sameEpoch=HasIdentity &&
            Identity.Session==o.Identity.Session &&
            Identity.SessionRevision==
                o.Identity.SessionRevision &&
            Identity.Actor==o.Identity.Actor;
        if (sameEpoch && o.Identity.Frame<=Identity.Frame)
        {
            ++Rejected;
            Last={};
            Previous.fill(0.0);
            return {};
        }
        if (!o.WindowHasFocus || !o.DeviceConnected)
        {
            ++Rejected;
            FocusLosses+=!o.WindowHasFocus?1U:0U;
            GuardSeconds=std::max(
                GuardSeconds,Settings.FocusRestoreGuardSeconds);
            Previous.fill(0.0);
            Last={};
            return {};
        }
        if (DeviceToken!=0 && DeviceToken!=o.DeviceToken)
        {
            ++Reconnects;
            GuardSeconds=std::max(
                GuardSeconds,
                Settings.DeviceReconnectionGuardSeconds);
            Previous.fill(0.0);
        }
        auto result=CalibratePhysicalDriverInput(
            o,Settings,Previous);
        if (!result.Valid)
        {
            ++Rejected;
            Previous.fill(0.0);
            Last={};
            return {};
        }
        if (GuardSeconds>0.0)
        {
            GuardSeconds=std::max(
                0.0,GuardSeconds-o.DeltaSeconds);
            result.Throttle=0.0;
            result.Brake=1.0;
            result.EmergencySafetyStop=true;
        }
        for(std::size_t i=0;i<DriverAxisCount;++i)
            Previous[i]=result.Axes[i].SmoothedValue;
        Identity=o.Identity;
        HasIdentity=true;
        DeviceToken=o.DeviceToken;
        ++Accepted;
        BrakePriorityFrames+=result.DriverBrakePriority?1U:0U;
        Last=result;
        return Last;
    }
    DriverInputDecision Snapshot() const noexcept{return Last;}
    std::uint64_t Frames() const noexcept{return Accepted;}
    std::uint64_t Rejections() const noexcept{return Rejected;}
    std::uint64_t DeviceChanges() const noexcept{return Reconnects;}
    std::uint64_t FocusLossEvents() const noexcept
    {return FocusLosses;}
    std::uint64_t BrakePrioritySamples() const noexcept
    {return BrakePriorityFrames;}
    double PendingSafetyGuardSeconds() const noexcept
    {return GuardSeconds;}
private:
    DriverInputSettings Settings{};
    DriverInputDecision Last{};
    RaceIdentity Identity{};
    std::array<double,DriverAxisCount> Previous{};
    std::uint64_t DeviceToken=0;
    std::uint64_t Accepted=0;
    std::uint64_t Rejected=0;
    std::uint64_t Reconnects=0;
    std::uint64_t FocusLosses=0;
    std::uint64_t BrakePriorityFrames=0;
    double GuardSeconds=0.0;
    bool HasIdentity=false;
};
} // namespace latebrake::racecraft
