#pragma once

#include "Racecraft/PhysicalNightWetVisibilityAudit.h"
#include "Racecraft/PhysicalWetPitDecisionGate.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// A deterministic, bounded source-only event journal for corroborated
// six-car night/wet observations and manual pit suggestions.
// It cannot award race time, simulate weather, change native visibility,
// control the car, acknowledge pit service, or attest engine-build QA.
inline constexpr std::size_t PhysicalWeatherEventCapacity=64u;
inline constexpr std::size_t PhysicalWeatherEventCars=NightWetVehicleCount;

enum class PhysicalWeatherEventFault : std::uint8_t
{
    None,
    UntrustedNightEvidence,
    UntrustedPitEvidence,
    CrossSourceMismatch,
    InvalidSettings,
    SessionChanged,
    NonmonotonicFrame,
    ClockDiscontinuity,
    ExcessiveRiskJump,
    InvalidEvent
};

enum class PhysicalWeatherEventKind : std::uint8_t
{
    SessionStart,
    ObserveStable,
    SightCaution,
    CriticalSight,
    SightRecovered,
    PitWaitForTraffic,
    PitApproachCaution,
    PitSourceSafetyCheck,
    WetTrackObserved
};

struct PhysicalWeatherEventSettings final
{
    double MaximumClockSkewSeconds=0.05;
    double MinimumFrameStepSeconds=0.001;
    double MaximumFrameStepSeconds=0.20;
    double MaximumSightReserveMagnitudeCm=250000.0;
    double MaximumCarRiskRatio=100.0;
    double MaximumRecordedFilmMm=ObservedWaterMaximumMm;
    double MaximumRecordedLux=800.0;
    double MaximumRiskRatioChangePerSecond=180.0;
    unsigned MaximumSkippedFrames=8u;
    unsigned MaximumRecoveringFrames=3u;
    bool RequireOriginalSixVehicleSources=true;
    bool RequireIndependentPitServiceAuthority=true;
    bool PreserveExactNativeSession=true;
    bool NeverChangeVehiclePhysics=true;
    bool NeverChangeRaceResults=true;
    bool NeverClaimActualGPUOcclusion=true;
};

struct PhysicalWeatherCarEventSample final
{
    std::uint64_t Actor=0u;
    double SightDistanceCm=0.0;
    double StoppingDistanceCm=0.0;
    double SightMarginCm=0.0;
    double RiskRatio=0.0;
    double RoadWaterFilmMm=0.0;
    double TrackLux=0.0;
    NightWetSafetyBand Safety=NightWetSafetyBand::Unqualified;
    bool OriginalChaosCarUnchanged=true;
    bool ActualRayOcclusionUnverified=true;
};

struct PhysicalWeatherEvent final
{
    bool Valid=false;
    PhysicalWeatherEventKind Kind=PhysicalWeatherEventKind::ObserveStable;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t NativeFrame=0u;
    double NativeClockSeconds=0.0;
    std::array<PhysicalWeatherCarEventSample,
        PhysicalWeatherEventCars> Cars{};
    WetPitDecision SuggestedPit=WetPitDecision::Unqualified;
    WetPitDecision ConfirmedPit=WetPitDecision::Unqualified;
    unsigned SourceCars=0u;
    unsigned CautionCars=0u;
    unsigned CriticalCars=0u;
    double MaximumSourceFilmMm=0.0;
    double MinimumSourceSightMarginCm=0.0;
    std::uint64_t PreviousDigest=0u;
    std::uint64_t Digest=0u;
    bool SourceOnly=true;
    bool RealRaceDataUnaffected=true;
};

struct PhysicalWeatherJournalReceipt final
{
    bool Accepted=false;
    PhysicalWeatherEventFault Fault=PhysicalWeatherEventFault::InvalidEvent;
    std::uint64_t AcceptedFrame=0u;
    std::uint64_t EventDigest=0u;
    unsigned StoredEvents=0u;
    unsigned CriticalSightEvents=0u;
    unsigned SourcePitWaitEvents=0u;
    double LatestClockSeconds=0.0;
    bool NoNativeActuation=true;
    bool NoOfficialRaceTimingChange=true;
};

inline bool ValidPhysicalWeatherEventSettings(
    PhysicalWeatherEventSettings p) noexcept
{
    return std::isfinite(p.MaximumClockSkewSeconds) &&
        p.MaximumClockSkewSeconds>=0.0 &&
        p.MaximumClockSkewSeconds<=0.1 &&
        std::isfinite(p.MinimumFrameStepSeconds) &&
        p.MinimumFrameStepSeconds>=0.001 &&
        p.MinimumFrameStepSeconds<=0.02 &&
        std::isfinite(p.MaximumFrameStepSeconds) &&
        p.MaximumFrameStepSeconds>=0.05 &&
        p.MaximumFrameStepSeconds<=0.2 &&
        p.MaximumFrameStepSeconds>p.MinimumFrameStepSeconds &&
        std::isfinite(p.MaximumSightReserveMagnitudeCm) &&
        p.MaximumSightReserveMagnitudeCm>=30000.0 &&
        p.MaximumSightReserveMagnitudeCm<=300000.0 &&
        std::isfinite(p.MaximumCarRiskRatio) &&
        p.MaximumCarRiskRatio>=30.0 &&
        p.MaximumCarRiskRatio<=200.0 &&
        std::isfinite(p.MaximumRecordedFilmMm) &&
        p.MaximumRecordedFilmMm>=5.0 &&
        p.MaximumRecordedFilmMm<=ObservedWaterMaximumMm &&
        std::isfinite(p.MaximumRecordedLux) &&
        p.MaximumRecordedLux>=500.0 &&
        p.MaximumRecordedLux<=1000.0 &&
        std::isfinite(p.MaximumRiskRatioChangePerSecond) &&
        p.MaximumRiskRatioChangePerSecond>=100.0 &&
        p.MaximumRiskRatioChangePerSecond<=500.0 &&
        p.MaximumSkippedFrames>=1u &&
        p.MaximumSkippedFrames<=12u &&
        p.MaximumRecoveringFrames>=1u &&
        p.MaximumRecoveringFrames<=5u &&
        p.RequireOriginalSixVehicleSources &&
        p.RequireIndependentPitServiceAuthority &&
        p.PreserveExactNativeSession &&
        p.NeverChangeVehiclePhysics &&
        p.NeverChangeRaceResults &&
        p.NeverClaimActualGPUOcclusion;
}

inline bool ValidPhysicalWeatherJournalInput(
    const NightWetSafetyFleet& sight,
    const WetPitDecisionReceipt& pit,
    PhysicalWeatherEventSettings p) noexcept
{
    if(!ValidPhysicalWeatherEventSettings(p) ||
       !sight.Valid ||
       sight.Fault!=NightWetFault::None ||
       sight.Session==0u ||
       sight.Revision==0u ||
       sight.SourceFrame==0u ||
       sight.SourceQualifiedCars!=PhysicalWeatherEventCars ||
       !sight.OriginalRaceAuthorityUnchanged ||
       !sight.OriginalCarCollisionUnchanged ||
       !sight.SourceOnly ||
       !std::isfinite(sight.SourceClockSeconds) ||
       sight.SourceClockSeconds<=0.0 ||
       !std::isfinite(sight.MinimumSightReserveCm) ||
       !std::isfinite(sight.MinimumWitnessedLux) ||
       !std::isfinite(sight.MaximumObservedWaterMm) ||
       sight.MaximumObservedWaterMm<0.0 ||
       sight.MaximumObservedWaterMm>p.MaximumRecordedFilmMm ||
       !pit.Valid ||
       pit.Fault!=WetPitDecisionFault::None ||
       !pit.NoActualServiceIssued ||
       !pit.OriginalPitHoldRequired ||
       !pit.NoRaceOrTyreChange ||
       pit.SourceSession!=sight.Session ||
       pit.SourceFrame!=sight.SourceFrame ||
       !std::isfinite(pit.SourceClockSeconds) ||
       std::abs(pit.SourceClockSeconds-sight.SourceClockSeconds)>
           p.MaximumClockSkewSeconds)
        return false;
    unsigned caution=0u;
    unsigned critical=0u;
    double minMargin=std::numeric_limits<double>::infinity();
    double maxFilm=0.0;
    double minLux=std::numeric_limits<double>::infinity();
    for(std::size_t i=0u;i<PhysicalWeatherEventCars;++i)
    {
        const auto& c=sight.Cars[i];
        if(!c.Valid || c.Actor==0u ||
           c.Band==NightWetSafetyBand::Unqualified ||
           !c.RealOcclusionUnverified ||
           !c.NoPhysicalBrakingApplied ||
           !std::isfinite(c.WrappedStationCm) ||
           !std::isfinite(c.RoadWaterFilmMm) ||
           !std::isfinite(c.SourceTrackLux) ||
           !std::isfinite(c.EstimatedSightDistanceCm) ||
           !std::isfinite(c.MeasuredStoppingDistanceCm) ||
           !std::isfinite(c.BrakingDistanceReserveCm) ||
           !std::isfinite(c.ConservativeDistanceRatio) ||
           c.EstimatedSightDistanceCm<=0.0 ||
           c.MeasuredStoppingDistanceCm<0.0 ||
           c.RoadWaterFilmMm<0.0 ||
           c.RoadWaterFilmMm>p.MaximumRecordedFilmMm ||
           c.SourceTrackLux<0.0 ||
           c.SourceTrackLux>p.MaximumRecordedLux ||
           c.ConservativeDistanceRatio<0.0 ||
           c.ConservativeDistanceRatio>p.MaximumCarRiskRatio ||
           std::abs(c.BrakingDistanceReserveCm)>
               p.MaximumSightReserveMagnitudeCm ||
           std::abs(c.EstimatedSightDistanceCm-
               c.MeasuredStoppingDistanceCm-
               c.BrakingDistanceReserveCm)>1.0e-3)
            return false;
        for(std::size_t j=0u;j<i;++j)
            if(sight.Cars[j].Actor==c.Actor)
                return false;
        minMargin=std::min(minMargin,c.BrakingDistanceReserveCm);
        maxFilm=std::max(maxFilm,c.RoadWaterFilmMm);
        minLux=std::min(minLux,c.SourceTrackLux);
        if(c.Band==NightWetSafetyBand::Critical)++critical;
        if(c.Band==NightWetSafetyBand::Critical ||
           c.Band==NightWetSafetyBand::LimitedSight ||
           c.Band==NightWetSafetyBand::Caution)++caution;
    }
    return caution==sight.CautionCars &&
        critical==sight.CriticalCars &&
        std::abs(minMargin-sight.MinimumSightReserveCm)<=1e-3 &&
        std::abs(minLux-sight.MinimumWitnessedLux)<=1e-3 &&
        std::abs(maxFilm-sight.MaximumObservedWaterMm)<=1e-3;
}

inline PhysicalWeatherEventKind ClassifyPhysicalWeatherEvent(
    const NightWetSafetyFleet& frame,
    const WetPitDecisionReceipt& pit,
    const PhysicalWeatherEvent* previous=nullptr) noexcept
{
    if(frame.CriticalCars>0u)
        return PhysicalWeatherEventKind::CriticalSight;
    if(previous!=nullptr &&
       previous->CriticalCars>0u &&
       frame.CriticalCars==0u &&
       frame.CautionCars==0u)
        return PhysicalWeatherEventKind::SightRecovered;
    if(pit.Confirmed==WetPitDecision::WaitForTraffic)
        return PhysicalWeatherEventKind::PitWaitForTraffic;
    if(pit.Confirmed==WetPitDecision::RequestNativePitSafetyCheck)
        return PhysicalWeatherEventKind::PitSourceSafetyCheck;
    if(pit.Confirmed==WetPitDecision::ApproachWithCaution)
        return PhysicalWeatherEventKind::PitApproachCaution;
    if(frame.CautionCars>0u)
        return PhysicalWeatherEventKind::SightCaution;
    if(frame.MaximumObservedWaterMm>=1.0)
        return PhysicalWeatherEventKind::WetTrackObserved;
    if(previous==nullptr)
        return PhysicalWeatherEventKind::SessionStart;
    return PhysicalWeatherEventKind::ObserveStable;
}

inline std::uint64_t PhysicalWeatherHashWord(
    std::uint64_t digest,std::uint64_t value) noexcept
{
    constexpr std::uint64_t prime=1099511628211ull;
    for(unsigned n=0u;n<8u;++n)
    {
        digest^=(value&255ull);
        digest*=prime;
        value>>=8u;
    }
    return digest;
}

inline std::uint64_t PhysicalWeatherQuantize(
    double value,double unitsPerNumber) noexcept
{
    const double scaled=std::clamp(
        value*unitsPerNumber,-1.0e12,1.0e12);
    const auto signedNumber=static_cast<std::int64_t>(
        std::llround(scaled));
    return static_cast<std::uint64_t>(signedNumber);
}

inline std::uint64_t PhysicalWeatherEventDigest(
    const PhysicalWeatherEvent& event) noexcept
{
    if(!event.Valid)return 0u;
    std::uint64_t value=14695981039346656037ull;
    value=PhysicalWeatherHashWord(value,event.PreviousDigest);
    value=PhysicalWeatherHashWord(value,event.Session);
    value=PhysicalWeatherHashWord(value,event.Revision);
    value=PhysicalWeatherHashWord(value,event.NativeFrame);
    value=PhysicalWeatherHashWord(
        value,static_cast<std::uint64_t>(event.Circuit));
    value=PhysicalWeatherHashWord(
        value,static_cast<std::uint64_t>(event.Kind));
    value=PhysicalWeatherHashWord(
        value,PhysicalWeatherQuantize(event.NativeClockSeconds,1000.0));
    value=PhysicalWeatherHashWord(
        value,static_cast<std::uint64_t>(event.ConfirmedPit));
    for(const auto& car:event.Cars)
    {
        value=PhysicalWeatherHashWord(value,car.Actor);
        value=PhysicalWeatherHashWord(
            value,static_cast<std::uint64_t>(car.Safety));
        value=PhysicalWeatherHashWord(
            value,PhysicalWeatherQuantize(car.SightMarginCm,10.0));
        value=PhysicalWeatherHashWord(
            value,PhysicalWeatherQuantize(car.RoadWaterFilmMm,1000.0));
        value=PhysicalWeatherHashWord(
            value,PhysicalWeatherQuantize(car.TrackLux,100.0));
    }
    return value==0u?1u:value;
}

inline PhysicalWeatherEvent BuildPhysicalWeatherEvent(
    const NightWetSafetyFleet& source,
    const WetPitDecisionReceipt& pit,
    const PhysicalWeatherEvent* previous=nullptr,
    PhysicalWeatherEventSettings p={}) noexcept
{
    PhysicalWeatherEvent out;
    if(!ValidPhysicalWeatherJournalInput(source,pit,p))
        return out;
    out.Valid=true;
    out.Circuit=source.Circuit;
    out.Session=source.Session;
    out.Revision=source.Revision;
    out.NativeFrame=source.SourceFrame;
    out.NativeClockSeconds=source.SourceClockSeconds;
    out.Kind=ClassifyPhysicalWeatherEvent(source,pit,previous);
    out.SuggestedPit=pit.Proposed;
    out.ConfirmedPit=pit.Confirmed;
    out.SourceCars=source.SourceQualifiedCars;
    out.CautionCars=source.CautionCars;
    out.CriticalCars=source.CriticalCars;
    out.MaximumSourceFilmMm=source.MaximumObservedWaterMm;
    out.MinimumSourceSightMarginCm=source.MinimumSightReserveCm;
    if(previous!=nullptr)out.PreviousDigest=previous->Digest;
    for(std::size_t i=0u;i<PhysicalWeatherEventCars;++i)
    {
        const auto& from=source.Cars[i];
        auto& car=out.Cars[i];
        car.Actor=from.Actor;
        car.SightDistanceCm=from.EstimatedSightDistanceCm;
        car.StoppingDistanceCm=from.MeasuredStoppingDistanceCm;
        car.SightMarginCm=from.BrakingDistanceReserveCm;
        car.RiskRatio=from.ConservativeDistanceRatio;
        car.RoadWaterFilmMm=from.RoadWaterFilmMm;
        car.TrackLux=from.SourceTrackLux;
        car.Safety=from.Band;
    }
    out.Digest=PhysicalWeatherEventDigest(out);
    return out;
}

class PhysicalRaceWeatherEventJournal final
{
public:
    void Reset() noexcept
    {
        Count=0u;
        NextSlot=0u;
        Session=0u;
        Revision=0u;
        LastDigest=0u;
        CriticalEvents=0u;
        PitWaitEvents=0u;
        Rejection=PhysicalWeatherEventFault::None;
    }

    std::size_t Size() const noexcept {return Count;}
    std::uint64_t CurrentDigest() const noexcept {return LastDigest;}
    unsigned CriticalCount() const noexcept {return CriticalEvents;}
    unsigned PitWaitCount() const noexcept {return PitWaitEvents;}
    PhysicalWeatherEventFault LastFault() const noexcept {
        return Rejection;
    }

    bool Read(std::size_t chronological,PhysicalWeatherEvent& event) const noexcept
    {
        event={};
        if(chronological>=Count)return false;
        event=At(chronological);
        return true;
    }

    PhysicalWeatherJournalReceipt Observe(
        const NightWetSafetyFleet& night,
        const WetPitDecisionReceipt& pit,
        PhysicalWeatherEventSettings settings={}) noexcept
    {
        PhysicalWeatherJournalReceipt receipt;
        if(!ValidPhysicalWeatherEventSettings(settings))
        {
            receipt.Fault=PhysicalWeatherEventFault::InvalidSettings;
            Rejection=receipt.Fault;
            return receipt;
        }
        if(!ValidPhysicalWeatherJournalInput(night,pit,settings))
        {
            receipt.Fault=PhysicalWeatherEventFault::UntrustedNightEvidence;
            Rejection=receipt.Fault;
            return receipt;
        }
        const PhysicalWeatherEvent* previous=
            Count>0u?&At(Count-1u):nullptr;
        if(previous!=nullptr)
        {
            if(night.Session!=Session || night.Revision!=Revision ||
               night.Circuit!=previous->Circuit)
            {
                receipt.Fault=PhysicalWeatherEventFault::SessionChanged;
                Rejection=receipt.Fault;
                return receipt;
            }
            if(night.SourceFrame<=previous->NativeFrame ||
               night.SourceFrame-previous->NativeFrame>
                   settings.MaximumSkippedFrames)
            {
                receipt.Fault=PhysicalWeatherEventFault::NonmonotonicFrame;
                Rejection=receipt.Fault;
                return receipt;
            }
            const double delta=night.SourceClockSeconds-
                previous->NativeClockSeconds;
            if(!std::isfinite(delta) ||
               delta<settings.MinimumFrameStepSeconds ||
               delta>settings.MaximumFrameStepSeconds)
            {
                receipt.Fault=PhysicalWeatherEventFault::ClockDiscontinuity;
                Rejection=receipt.Fault;
                return receipt;
            }
            for(unsigned i=0u;i<PhysicalWeatherEventCars;++i)
            {
                if(night.Cars[i].Actor!=previous->Cars[i].Actor)
                {
                    receipt.Fault=PhysicalWeatherEventFault::CrossSourceMismatch;
                    Rejection=receipt.Fault;
                    return receipt;
                }
                const double change=std::abs(
                    night.Cars[i].ConservativeDistanceRatio-
                    previous->Cars[i].RiskRatio);
                if(change>settings.MaximumRiskRatioChangePerSecond*
                   delta)
                {
                    receipt.Fault=PhysicalWeatherEventFault::ExcessiveRiskJump;
                    Rejection=receipt.Fault;
                    return receipt;
                }
            }
        }
        auto next=BuildPhysicalWeatherEvent(
            night,pit,previous,settings);
        if(!next.Valid || next.Digest==0u ||
           (previous!=nullptr && next.PreviousDigest!=LastDigest))
        {
            receipt.Fault=PhysicalWeatherEventFault::InvalidEvent;
            Rejection=receipt.Fault;
            return receipt;
        }
        if(Count==0u)
        {
            Session=night.Session;
            Revision=night.Revision;
        }
        Events[NextSlot]=next;
        NextSlot=(NextSlot+1u)%PhysicalWeatherEventCapacity;
        if(Count<PhysicalWeatherEventCapacity)++Count;
        LastDigest=next.Digest;
        if(next.Kind==PhysicalWeatherEventKind::CriticalSight)
            ++CriticalEvents;
        if(next.Kind==PhysicalWeatherEventKind::PitWaitForTraffic)
            ++PitWaitEvents;
        Rejection=PhysicalWeatherEventFault::None;
        receipt.Accepted=true;
        receipt.Fault=PhysicalWeatherEventFault::None;
        receipt.AcceptedFrame=next.NativeFrame;
        receipt.EventDigest=next.Digest;
        receipt.StoredEvents=static_cast<unsigned>(Count);
        receipt.CriticalSightEvents=CriticalEvents;
        receipt.SourcePitWaitEvents=PitWaitEvents;
        receipt.LatestClockSeconds=next.NativeClockSeconds;
        return receipt;
    }

private:
    const PhysicalWeatherEvent& At(std::size_t chronological) const noexcept
    {
        const auto oldest=(NextSlot+PhysicalWeatherEventCapacity-
            Count)%PhysicalWeatherEventCapacity;
        return Events[(oldest+chronological)%
            PhysicalWeatherEventCapacity];
    }

    std::array<PhysicalWeatherEvent,PhysicalWeatherEventCapacity> Events{};
    std::size_t Count=0u;
    std::size_t NextSlot=0u;
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t LastDigest=0u;
    unsigned CriticalEvents=0u;
    unsigned PitWaitEvents=0u;
    PhysicalWeatherEventFault Rejection=PhysicalWeatherEventFault::None;
};
} // namespace latebrake
