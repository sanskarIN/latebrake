#pragma once

#include "Racing/DrivingCoachReview.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Records sample-only chassis exposure BETWEEN accepted checkpoint receipts.
// Source clock and gate ordinal are supplied only AFTER RaceProgress accepts
// a gate. This journal cannot add checkpoints, alter timing, spend credits,
// declare a winner, or prove the engine actually generated those receipts.
inline constexpr int DrivingCoachMaximumGates=16;
inline constexpr std::size_t DrivingCoachSectorHistoryCapacity=64;
inline constexpr double DrivingCoachSectorClockTolerance=0.02;

enum class DrivingCoachSectorStatus : std::uint8_t
{
    Unavailable,
    InvalidInput,
    AwaitingSecondGate,
    Recorded,
    GapOrOutOfOrder,
    SessionChanged,
    PausedOrRecovered,
    InsufficientSamples,
    SourceClockMismatch
};

inline const char* DrivingCoachSectorStatusLabel(
    DrivingCoachSectorStatus status) noexcept
{
    switch(status)
    {
    case DrivingCoachSectorStatus::Unavailable:
        return "NO CHECKPOINT-BOUND COACH SAMPLES";
    case DrivingCoachSectorStatus::InvalidInput:
        return "INVALID GATE OR CHASSIS SAMPLE";
    case DrivingCoachSectorStatus::AwaitingSecondGate:
        return "FIRST ACCEPTED GATE: WAITING FOR NEXT";
    case DrivingCoachSectorStatus::Recorded:
        return "TWO REAL GATES WITH CONTINUOUS CHASSIS SAMPLING";
    case DrivingCoachSectorStatus::GapOrOutOfOrder:
        return "GATE ORDER GAP: SECTOR WITHHELD";
    case DrivingCoachSectorStatus::SessionChanged:
        return "PHYSICAL ACTOR CHANGED: SECTOR WITHHELD";
    case DrivingCoachSectorStatus::PausedOrRecovered:
        return "PAUSE OR RECOVERY: SECTOR WITHHELD";
    case DrivingCoachSectorStatus::InsufficientSamples:
        return "INSUFFICIENT SAMPLES BETWEEN GATES";
    case DrivingCoachSectorStatus::SourceClockMismatch:
        return "CHASSIS SAMPLES DISAGREE WITH RACE CLOCK";
    default:return "DRIVING SECTOR UNAVAILABLE";
    }
}

struct DrivingCoachGateObservation final
{
    bool AcceptedByRaceProgress=false;
    bool DriverPhysicsReady=false;
    bool MenuOrFocusSuspended=false;
    std::uint64_t ActorToken=0;
    int GateOrdinal=0; // 1-based accepted ordinal of this attempt
    int GateCount=0;
    double ActiveClockSeconds=0.0; // excludes recovery fees
    DrivingCoachReadout Chassis{};
};

struct DrivingCoachSectorSample final
{
    bool Valid=false;
    std::uint64_t Sequence=0;
    int FirstGateOrdinal=0;
    int EndGateOrdinal=0;
    int SectorIndex=0; // checkpoint *ending* the observed interval
    int LapIndex=0;
    double StartClockSeconds=0.0;
    double EndClockSeconds=0.0;
    double ClockIntervalSeconds=0.0;
    double ChassisSampleSeconds=0.0;
    double ChassisCoverageFraction=0.0;
    double SampledDistanceKm=0.0;
    double MeanSpeedKmh=0.0;
    double BrakingSeconds=0.0;
    double HighSlipSeconds=0.0;
    double BrakingFraction=0.0;
    double HighSlipFraction=0.0;
    std::uint64_t SampledFrames=0;
    bool AuthenticEngineWitness=false;
    bool OfficialSectorTime=false;
    bool ChangesRaceOutcome=false;
};

struct DrivingCoachSectorReview final
{
    bool Valid=false;
    DrivingCoachSectorStatus Status=DrivingCoachSectorStatus::Unavailable;
    std::uint64_t AcceptedGateReceipts=0;
    std::uint64_t RetainedSectorSamples=0;
    std::uint64_t TotalRecordedSectors=0;
    std::uint64_t RefusedInputs=0;
    std::uint64_t Discontinuities=0;
    std::uint64_t ActorToken=0;
    int GateCount=0;
    int LastGateOrdinal=0;
    std::size_t BestSampleSectorCount=0;
    double TotalSampledSectorSeconds=0.0;
    double TotalSampledSectorDistanceKm=0.0;
    // These are the quickest observed chassis samples per gate segment,
    // NOT the best official split; another lap may include missing frames.
    std::array<DrivingCoachSectorSample,DrivingCoachMaximumGates> BestSamples{};
    bool NoRewardAuthority=true;
    bool NoRaceClockAuthority=true;
    bool NoUnrealAuthenticityClaim=true;
};

inline bool ValidDrivingCoachGateObservation(
    const DrivingCoachGateObservation& input) noexcept
{
    return input.AcceptedByRaceProgress && input.DriverPhysicsReady &&
        !input.MenuOrFocusSuspended && input.ActorToken!=0 &&
        input.GateCount>=1 && input.GateCount<=DrivingCoachMaximumGates &&
        input.GateOrdinal>0 && input.GateOrdinal<=16000 &&
        std::isfinite(input.ActiveClockSeconds) &&
        input.ActiveClockSeconds>=0.0 &&
        input.ActiveClockSeconds<=86400.0 &&
        ValidDrivingCoachReadout(input.Chassis);
}

inline bool ValidDrivingCoachSectorSample(
    const DrivingCoachSectorSample& sample,int gateCount) noexcept
{
    return sample.Valid && sample.Sequence>0 &&
        gateCount>=1 && gateCount<=DrivingCoachMaximumGates &&
        sample.FirstGateOrdinal>0 &&
        sample.EndGateOrdinal==sample.FirstGateOrdinal+1 &&
        sample.EndGateOrdinal<=16000 &&
        sample.SectorIndex==(sample.EndGateOrdinal-1)%gateCount &&
        sample.LapIndex==(sample.EndGateOrdinal-1)/gateCount &&
        std::isfinite(sample.StartClockSeconds) &&
        std::isfinite(sample.EndClockSeconds) &&
        sample.StartClockSeconds>=0.0 &&
        sample.EndClockSeconds>sample.StartClockSeconds &&
        std::isfinite(sample.ClockIntervalSeconds) &&
        std::abs(sample.EndClockSeconds-sample.StartClockSeconds-
            sample.ClockIntervalSeconds)<=1e-6 &&
        sample.ClockIntervalSeconds>0.0 &&
        std::isfinite(sample.ChassisSampleSeconds) &&
        sample.ChassisSampleSeconds>0.0 &&
        sample.ChassisSampleSeconds<=sample.ClockIntervalSeconds+
            DrivingCoachSectorClockTolerance &&
        std::isfinite(sample.ChassisCoverageFraction) &&
        sample.ChassisCoverageFraction>0.0 &&
        sample.ChassisCoverageFraction<=1.0 &&
        std::isfinite(sample.SampledDistanceKm) &&
        sample.SampledDistanceKm>=0.0 &&
        sample.SampledDistanceKm<=sample.ChassisSampleSeconds*0.1+1e-6 &&
        std::isfinite(sample.MeanSpeedKmh) &&
        sample.MeanSpeedKmh>=0.0 && sample.MeanSpeedKmh<=360.0+1e-6 &&
        std::abs(sample.MeanSpeedKmh-
            sample.SampledDistanceKm*3600.0/sample.ChassisSampleSeconds)<=1e-5 &&
        std::isfinite(sample.BrakingSeconds) &&
        std::isfinite(sample.HighSlipSeconds) &&
        sample.BrakingSeconds>=0.0 &&
        sample.HighSlipSeconds>=0.0 &&
        sample.BrakingSeconds<=sample.ChassisSampleSeconds+1e-6 &&
        sample.HighSlipSeconds<=sample.ChassisSampleSeconds+1e-6 &&
        std::isfinite(sample.BrakingFraction) &&
        std::isfinite(sample.HighSlipFraction) &&
        sample.BrakingFraction>=0.0 && sample.BrakingFraction<=1.0 &&
        sample.HighSlipFraction>=0.0 && sample.HighSlipFraction<=1.0 &&
        sample.SampledFrames>=2 &&
        !sample.AuthenticEngineWitness && !sample.OfficialSectorTime &&
        !sample.ChangesRaceOutcome;
}

// The first gate is a conservative baseline: without an accepted chassis
// sample at the start line, a run-start-to-gate interval cannot be measured.
class DrivingCoachSectorJournal final
{
public:
    void Reset(int gateCount=4) noexcept
    {
        Checkpoints=gateCount>=1 && gateCount<=DrivingCoachMaximumGates?
            gateCount:0;
        Previous={};
        Actor=0;
        HasPrevious=false;
        Frozen=false;
        LastOrdinal=0;
        NextSequence=1;
        Cursor=Count=0;
        Entries={};
        ReceiptCount=RecordedCount=RefusalCount=BreakCount=0;
        TotalSeconds=TotalDistance=0.0;
        Status=DrivingCoachSectorStatus::Unavailable;
    }

    void Suspend() noexcept
    {
        if(HasPrevious)Increment(BreakCount);
        HasPrevious=false;
        Previous={};
        if(!Frozen)Status=DrivingCoachSectorStatus::PausedOrRecovered;
    }

    void Freeze() noexcept
    {
        Suspend();
        Frozen=true;
    }

    bool ObserveGate(const DrivingCoachGateObservation& input) noexcept
    {
        if(Frozen || !ValidDrivingCoachGateObservation(input) ||
           Checkpoints==0 || Checkpoints!=input.GateCount)
        {
            Increment(RefusalCount);
            Suspend();
            Status=DrivingCoachSectorStatus::InvalidInput;
            return false;
        }
        // Never permit a new actor to inherit an old vehicle's sampled
        // interval. Race restart explicitly calls Reset before this point.
        if(Actor!=0 && input.ActorToken!=Actor)
        {
            Increment(RefusalCount);
            Suspend();
            Status=DrivingCoachSectorStatus::SessionChanged;
            return false;
        }
        if(LastOrdinal!=0 && input.GateOrdinal<=LastOrdinal)
        {
            Increment(RefusalCount);
            Suspend();
            Status=DrivingCoachSectorStatus::GapOrOutOfOrder;
            return false;
        }
        if(Actor==0) Actor=input.ActorToken;
        Increment(ReceiptCount);
        const bool contiguous=LastOrdinal!=0 &&
            input.GateOrdinal==LastOrdinal+1;
        if(LastOrdinal!=0 && !contiguous)
        {
            Increment(BreakCount);
            Status=DrivingCoachSectorStatus::GapOrOutOfOrder;
        }
        LastOrdinal=input.GateOrdinal;
        const auto& now=input.Chassis;
        const auto makeBaseline=[&]() noexcept
        {
            Previous=input;
            HasPrevious=true;
        };
        if(!HasPrevious || !contiguous)
        {
            makeBaseline();
            if(contiguous || ReceiptCount==1)
                Status=DrivingCoachSectorStatus::AwaitingSecondGate;
            return false;
        }
        const auto& before=Previous.Chassis;
        if(now.AcceptedSamples<=before.AcceptedSamples ||
           now.ObservedSeconds<=before.ObservedSeconds ||
           now.DistanceKm+1e-9<before.DistanceKm ||
           now.BrakeSeconds+1e-9<before.BrakeSeconds ||
           now.HighSlipSeconds+1e-9<before.HighSlipSeconds ||
           now.Suspensions!=before.Suspensions)
        {
            Increment(BreakCount);
            Status=DrivingCoachSectorStatus::PausedOrRecovered;
            makeBaseline();
            return false;
        }
        const double dt=input.ActiveClockSeconds-Previous.ActiveClockSeconds;
        const double ds=now.ObservedSeconds-before.ObservedSeconds;
        const double dd=now.DistanceKm-before.DistanceKm;
        const double db=now.BrakeSeconds-before.BrakeSeconds;
        const double dl=now.HighSlipSeconds-before.HighSlipSeconds;
        if(!std::isfinite(dt) || !std::isfinite(ds) ||
           dt<=0.0 || ds<=0.0 ||
           ds>dt+DrivingCoachSectorClockTolerance ||
           dd< -1e-9 || db< -1e-9 || dl< -1e-9 ||
           db>ds+1e-7 || dl>ds+1e-7)
        {
            Increment(BreakCount);
            Status=DrivingCoachSectorStatus::SourceClockMismatch;
            makeBaseline();
            return false;
        }
        const auto frames=now.AcceptedSamples-before.AcceptedSamples;
        if(frames<2 || ds<0.10)
        {
            Increment(BreakCount);
            Status=DrivingCoachSectorStatus::InsufficientSamples;
            makeBaseline();
            return false;
        }
        DrivingCoachSectorSample sector;
        sector.Valid=true;
        sector.Sequence=NextSequence;
        sector.FirstGateOrdinal=Previous.GateOrdinal;
        sector.EndGateOrdinal=input.GateOrdinal;
        sector.SectorIndex=(input.GateOrdinal-1)%Checkpoints;
        sector.LapIndex=(input.GateOrdinal-1)/Checkpoints;
        sector.StartClockSeconds=Previous.ActiveClockSeconds;
        sector.EndClockSeconds=input.ActiveClockSeconds;
        sector.ClockIntervalSeconds=dt;
        sector.ChassisSampleSeconds=ds;
        sector.ChassisCoverageFraction=std::clamp(ds/dt,0.0,1.0);
        sector.SampledDistanceKm=std::max(0.0,dd);
        sector.MeanSpeedKmh=sector.SampledDistanceKm*3600.0/ds;
        sector.BrakingSeconds=std::max(0.0,db);
        sector.HighSlipSeconds=std::max(0.0,dl);
        sector.BrakingFraction=std::clamp(db/ds,0.0,1.0);
        sector.HighSlipFraction=std::clamp(dl/ds,0.0,1.0);
        sector.SampledFrames=frames;
        if(!ValidDrivingCoachSectorSample(sector,Checkpoints))
        {
            Increment(BreakCount);
            Status=DrivingCoachSectorStatus::InvalidInput;
            makeBaseline();
            return false;
        }
        Entries[Cursor]=sector;
        Cursor=(Cursor+1u)%Entries.size();
        if(Count<Entries.size())++Count;
        Increment(RecordedCount);
        if(NextSequence<std::numeric_limits<std::uint64_t>::max())
            ++NextSequence;
        TotalSeconds=std::min(DrivingCoachMaximumActiveSeconds,
            TotalSeconds+sector.ChassisSampleSeconds);
        TotalDistance=std::min(1000000.0,
            TotalDistance+sector.SampledDistanceKm);
        Status=DrivingCoachSectorStatus::Recorded;
        makeBaseline();
        return true;
    }

    bool ReadNewest(std::size_t offset,DrivingCoachSectorSample& result) const noexcept
    {
        if(offset>=Count)return false;
        result=Entries[(Cursor+Entries.size()-1u-offset)%Entries.size()];
        return true;
    }

    DrivingCoachSectorReview Review() const noexcept
    {
        DrivingCoachSectorReview result;
        result.Status=Status;
        result.ActorToken=Actor;
        result.AcceptedGateReceipts=ReceiptCount;
        result.RefusedInputs=RefusalCount;
        result.Discontinuities=BreakCount;
        result.TotalRecordedSectors=RecordedCount;
        result.RetainedSectorSamples=Count;
        result.GateCount=Checkpoints;
        result.LastGateOrdinal=LastOrdinal;
        result.TotalSampledSectorSeconds=TotalSeconds;
        result.TotalSampledSectorDistanceKm=TotalDistance;
        for(std::size_t i=0;i<Count;++i)
        {
            DrivingCoachSectorSample entry;
            if(!ReadNewest(i,entry) ||
               !ValidDrivingCoachSectorSample(entry,Checkpoints))return {};
            auto& best=result.BestSamples[static_cast<std::size_t>(entry.SectorIndex)];
            if(!best.Valid || (entry.ChassisCoverageFraction>=0.95 &&
                (best.ChassisCoverageFraction<0.95 ||
                 entry.ChassisSampleSeconds<best.ChassisSampleSeconds)))
            {
                if(!best.Valid)++result.BestSampleSectorCount;
                best=entry;
            }
        }
        result.Valid=RecordedCount>0 && Count>0;
        return result;
    }

private:
    static void Increment(std::uint64_t& value) noexcept
    {
        if(value<std::numeric_limits<std::uint64_t>::max())++value;
    }
    std::array<DrivingCoachSectorSample,DrivingCoachSectorHistoryCapacity> Entries{};
    DrivingCoachGateObservation Previous{};
    std::uint64_t Actor=0;
    std::uint64_t ReceiptCount=0;
    std::uint64_t RecordedCount=0;
    std::uint64_t RefusalCount=0;
    std::uint64_t BreakCount=0;
    std::uint64_t NextSequence=1;
    std::size_t Cursor=0;
    std::size_t Count=0;
    int Checkpoints=0;
    int LastOrdinal=0;
    bool HasPrevious=false;
    bool Frozen=false;
    DrivingCoachSectorStatus Status=DrivingCoachSectorStatus::Unavailable;
    double TotalSeconds=0.0;
    double TotalDistance=0.0;
};
}
