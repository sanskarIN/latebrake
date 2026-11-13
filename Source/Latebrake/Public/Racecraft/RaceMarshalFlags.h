#pragma once
#include "Racecraft/RacecraftFrames.h"
#include "Racecraft/CircuitGeometry.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake::racecraft
{
enum class RaceFlag : std::uint8_t
{
    None, Green, LocalYellow, DoubleYellow,
    FullCourseYellow, SafetyCar, Red,
    Blue, White, Black, Checkered
};
enum class FlagSource : std::uint8_t
{
    Unknown, OriginalRaceDirector,
    VerifiedTrackMarshal, PhysicalSafetyCar,
    OfficialFinishObservation
};
struct FlagZone final
{
    RaceFlag Flag=RaceFlag::None;
    FlagSource Source=FlagSource::Unknown;
    double BeginTrackDistanceCm=0.0;
    double EndTrackDistanceCm=0.0;
    double EffectiveFromRaceClockSeconds=0.0;
    double ExpiresAtRaceClockSeconds=0.0;
    std::uint64_t Session=0;
    std::uint32_t Revision=0;
    std::uint64_t EventOrdinal=0;
    int SourceIncidentSlot=-1;
    int DestinationSlot=-1;
    bool OriginalCircuitConfirmed=false;
    bool RaceDirectorAuthorized=false;
    bool PhysicalIncidentValidated=false;
};
struct MarshalParameters final
{
    double YellowSpeedFraction=0.62;
    double DoubleYellowSpeedFraction=0.40;
    double SafetyCarSpeedFraction=0.42;
    double BlueFlagSpeedFraction=1.0;
    double RedFlagThrottleCeiling=0.0;
    double DefaultFlagDurationSeconds=15.0;
    double MaximumFlagDurationSeconds=3600.0;
    double MinimumIncidentZoneLengthCm=200.0;
    double MaximumIncidentZoneLengthCm=18000.0;
    double MinimumRaceDirectorConfidence=1.0;
    int MaximumActiveFlags=24;
    bool PreventOvertakingUnderYellow=true;
    bool RequirePhysicalIncidentForYellow=true;
    bool RequireOfficialDirectorSource=true;
    bool KeepLocalYellowInsideActualTrack=true;
    bool RedFlagRequiresStop=true;
};
struct MarshalDirective final
{
    RaceFlag DominantFlag=RaceFlag::None;
    double ThrottleCeiling=1.0;
    double BrakeFloor=0.0;
    double SpeedCeilingFraction=1.0;
    double DistanceToStartCm=0.0;
    double DistanceToEndCm=0.0;
    double EstimatedRestrictionRemainingSeconds=0.0;
    int FlagsApplicable=0;
    int IncidentsNearby=0;
    bool NoOvertake=false;
    bool MustStop=false;
    bool NeedsCaution=false;
    bool OriginalMarshalEvent=true;
    bool ActualRaceDirectorModified=false;
    bool Valid=false;
};
struct MarshalReceipt final
{
    std::array<FlagZone,24> ActiveZones{};
    int ZoneCount=0;
    std::uint64_t Session=0;
    std::uint32_t Revision=0;
    std::uint64_t LastEventOrdinal=0;
    int AcceptedFlags=0;
    int RejectedFlags=0;
    int ExpiredFlags=0;
    int RedFlagEvents=0;
    int YellowEvents=0;
    double LastObservedRaceClockSeconds=0.0;
    bool CanModifyOfficialRaceState=false;
};
inline bool ValidMarshalParameters(MarshalParameters p) noexcept
{
    return Finite(p.YellowSpeedFraction) &&
        p.YellowSpeedFraction>0.0 &&
        p.YellowSpeedFraction<=1.0 &&
        Finite(p.DoubleYellowSpeedFraction) &&
        p.DoubleYellowSpeedFraction>0.0 &&
        p.DoubleYellowSpeedFraction<=p.YellowSpeedFraction &&
        Finite(p.SafetyCarSpeedFraction) &&
        p.SafetyCarSpeedFraction>0.0 &&
        p.SafetyCarSpeedFraction<=1.0 &&
        Finite(p.DefaultFlagDurationSeconds) &&
        p.DefaultFlagDurationSeconds>0.0 &&
        Finite(p.MaximumFlagDurationSeconds) &&
        p.MaximumFlagDurationSeconds>=
            p.DefaultFlagDurationSeconds &&
        Finite(p.MinimumIncidentZoneLengthCm) &&
        p.MinimumIncidentZoneLengthCm>0.0 &&
        Finite(p.MaximumIncidentZoneLengthCm) &&
        p.MaximumIncidentZoneLengthCm>=
            p.MinimumIncidentZoneLengthCm &&
        p.MaximumActiveFlags>=1 &&
        p.MaximumActiveFlags<=24;
}
inline bool ValidFlagZone(FlagZone zone,
                          MarshalParameters params) noexcept
{
    if (zone.Flag==RaceFlag::None ||
        zone.Source==FlagSource::Unknown ||
        zone.Session==0 ||zone.EventOrdinal==0 ||
        !zone.OriginalCircuitConfirmed ||
        !zone.RaceDirectorAuthorized ||
        !Finite(zone.BeginTrackDistanceCm) ||
        !Finite(zone.EndTrackDistanceCm) ||
        !Finite(zone.EffectiveFromRaceClockSeconds) ||
        !Finite(zone.ExpiresAtRaceClockSeconds) ||
        zone.EffectiveFromRaceClockSeconds<0.0 ||
        zone.ExpiresAtRaceClockSeconds<=
            zone.EffectiveFromRaceClockSeconds ||
        zone.ExpiresAtRaceClockSeconds-
            zone.EffectiveFromRaceClockSeconds>
            params.MaximumFlagDurationSeconds)
        return false;
    if (params.RequireOfficialDirectorSource &&
        zone.Source!=FlagSource::OriginalRaceDirector &&
        zone.Source!=FlagSource::OfficialFinishObservation)
        return false;
    if ((zone.Flag==RaceFlag::LocalYellow ||
         zone.Flag==RaceFlag::DoubleYellow) &&
        params.RequirePhysicalIncidentForYellow &&
        !zone.PhysicalIncidentValidated)
        return false;
    const double length=WrapDistance(
        zone.EndTrackDistanceCm-zone.BeginTrackDistanceCm,
        OriginalCircuitLapCm);
    if (zone.Flag==RaceFlag::LocalYellow ||
        zone.Flag==RaceFlag::DoubleYellow)
    {
        if (length<params.MinimumIncidentZoneLengthCm ||
            length>params.MaximumIncidentZoneLengthCm)
            return false;
    }
    return true;
}
inline bool FlagZoneContains(
    FlagZone zone,double trackCm) noexcept
{
    if (!Finite(trackCm))return false;
    const double begin=WrapDistance(
        zone.BeginTrackDistanceCm,OriginalCircuitLapCm);
    const double length=WrapDistance(
        zone.EndTrackDistanceCm-zone.BeginTrackDistanceCm,
        OriginalCircuitLapCm);
    const double relative=WrapDistance(
        trackCm-begin,OriginalCircuitLapCm);
    return relative<=length;
}
inline int FlagSeverity(RaceFlag flag) noexcept
{
    switch(flag)
    {
    case RaceFlag::Red: return 100;
    case RaceFlag::Black: return 90;
    case RaceFlag::SafetyCar: return 80;
    case RaceFlag::FullCourseYellow: return 70;
    case RaceFlag::DoubleYellow: return 60;
    case RaceFlag::LocalYellow: return 50;
    case RaceFlag::Checkered: return 40;
    case RaceFlag::Blue: return 30;
    case RaceFlag::White: return 20;
    case RaceFlag::Green: return 10;
    default: return 0;
    }
}
inline MarshalDirective EvaluateMarshalFlags(
    const MarshalReceipt& receipt,double trackCm,
    double currentRaceClock,int slot,
    MarshalParameters params={}) noexcept
{
    MarshalDirective out;
    if (!ValidMarshalParameters(params) ||
        !Finite(trackCm) ||
        !Finite(currentRaceClock) ||
        currentRaceClock<0.0 ||
        slot<0 ||slot>=PhysicalFleetCapacity)
        return out;
    out.Valid=true;
    for (int i=0;i<receipt.ZoneCount;++i)
    {
        const FlagZone& zone=receipt.ActiveZones[
            static_cast<std::size_t>(i)];
        if (!ValidFlagZone(zone,params) ||
            currentRaceClock<
                zone.EffectiveFromRaceClockSeconds ||
            currentRaceClock>=
                zone.ExpiresAtRaceClockSeconds)
            continue;
        const bool global=
            zone.Flag==RaceFlag::Red ||
            zone.Flag==RaceFlag::SafetyCar ||
            zone.Flag==RaceFlag::FullCourseYellow ||
            zone.Flag==RaceFlag::Checkered;
        if (!global && !FlagZoneContains(zone,trackCm))
            continue;
        if (zone.DestinationSlot>=0 &&
            zone.DestinationSlot!=slot)
            continue;
        ++out.FlagsApplicable;
        if (zone.PhysicalIncidentValidated)
            ++out.IncidentsNearby;
        if (FlagSeverity(zone.Flag)>
            FlagSeverity(out.DominantFlag))
            out.DominantFlag=zone.Flag;
        out.EstimatedRestrictionRemainingSeconds=std::max(
            out.EstimatedRestrictionRemainingSeconds,
            zone.ExpiresAtRaceClockSeconds-currentRaceClock);
        if (zone.Flag==RaceFlag::LocalYellow)
        {
            out.SpeedCeilingFraction=std::min(
                out.SpeedCeilingFraction,
                params.YellowSpeedFraction);
            out.NoOvertake=true;
        }
        if (zone.Flag==RaceFlag::DoubleYellow)
        {
            out.SpeedCeilingFraction=std::min(
                out.SpeedCeilingFraction,
                params.DoubleYellowSpeedFraction);
            out.NoOvertake=true;
        }
        if (zone.Flag==RaceFlag::FullCourseYellow)
        {
            out.SpeedCeilingFraction=std::min(
                out.SpeedCeilingFraction,
                params.YellowSpeedFraction);
            out.NoOvertake=true;
        }
        if (zone.Flag==RaceFlag::SafetyCar)
        {
            out.SpeedCeilingFraction=std::min(
                out.SpeedCeilingFraction,
                params.SafetyCarSpeedFraction);
            out.NoOvertake=true;
        }
        if (zone.Flag==RaceFlag::Red ||
            zone.Flag==RaceFlag::Black)
        {
            out.MustStop=true;
            out.NoOvertake=true;
        }
        if (zone.Flag==RaceFlag::Blue)
        {
            out.SpeedCeilingFraction=std::min(
                out.SpeedCeilingFraction,
                params.BlueFlagSpeedFraction);
        }
    }
    out.ThrottleCeiling=out.MustStop ?
        0.0:out.SpeedCeilingFraction;
    out.BrakeFloor=out.MustStop?1.0:0.0;
    out.NeedsCaution=out.NoOvertake ||
        out.MustStop;
    out.ActualRaceDirectorModified=false;
    return out;
}
class PhysicalRaceMarshal final
{
public:
    bool Configure(MarshalParameters settings) noexcept
    {
        if (!ValidMarshalParameters(settings))return false;
        Settings=settings;
        Reset();
        return true;
    }
    void Reset() noexcept{Receipt={};}
    bool BeginSession(std::uint64_t session,
                      std::uint32_t revision) noexcept
    {
        if (session==0)return false;
        Reset();
        Receipt.Session=session;
        Receipt.Revision=revision;
        return true;
    }
    bool SubmitFlag(FlagZone zone) noexcept
    {
        if (!ValidFlagZone(zone,Settings) ||
            Receipt.Session==0 ||
            zone.Session!=Receipt.Session ||
            zone.Revision!=Receipt.Revision ||
            zone.EventOrdinal<=Receipt.LastEventOrdinal ||
            Receipt.ZoneCount>=Settings.MaximumActiveFlags)
        {
            ++Receipt.RejectedFlags;
            return false;
        }
        Receipt.ActiveZones[
            static_cast<std::size_t>(Receipt.ZoneCount++)]=zone;
        Receipt.LastEventOrdinal=zone.EventOrdinal;
        ++Receipt.AcceptedFlags;
        Receipt.YellowEvents+=
            zone.Flag==RaceFlag::LocalYellow ||
            zone.Flag==RaceFlag::DoubleYellow ||
            zone.Flag==RaceFlag::FullCourseYellow ? 1:0;
        Receipt.RedFlagEvents+=
            zone.Flag==RaceFlag::Red?1:0;
        Receipt.CanModifyOfficialRaceState=false;
        return true;
    }
    bool AdvanceClock(double officialClockSeconds) noexcept
    {
        if (!Finite(officialClockSeconds) ||
            officialClockSeconds<
                Receipt.LastObservedRaceClockSeconds ||
            Receipt.Session==0)return false;
        Receipt.LastObservedRaceClockSeconds=
            officialClockSeconds;
        int destination=0;
        for (int i=0;i<Receipt.ZoneCount;++i)
        {
            const FlagZone& zone=Receipt.ActiveZones[
                static_cast<std::size_t>(i)];
            if (officialClockSeconds>=
                zone.ExpiresAtRaceClockSeconds)
            {
                ++Receipt.ExpiredFlags;
                continue;
            }
            Receipt.ActiveZones[
                static_cast<std::size_t>(destination++)]=zone;
        }
        Receipt.ZoneCount=destination;
        return true;
    }
    MarshalDirective At(double trackCm,int slot) const noexcept
    {
        return EvaluateMarshalFlags(
            Receipt,trackCm,
            Receipt.LastObservedRaceClockSeconds,
            slot,Settings);
    }
    MarshalReceipt Snapshot() const noexcept{return Receipt;}
private:
    MarshalParameters Settings{};
    MarshalReceipt Receipt{};
};
} // namespace latebrake::racecraft
