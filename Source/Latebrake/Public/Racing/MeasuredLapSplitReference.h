#pragma once

#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace latebrake
{
// Read-only checkpoint-clock comparison. A reference is one actual
// completed lap, not an ideal lap composed of unrelated sector minima.
// A fee-free reference requires active clock evidence at EVERY gate.
// The original RaceProgress record/reward policy is not changed.
inline constexpr int MeasuredLapMaxGates=256;
inline constexpr double MeasuredLapClockLimit=604800.0;
inline constexpr double MeasuredLapTolerance=1e-7;

struct MeasuredLapGate final
{
    double CumulativeSeconds=0.0;
    double CumulativeFeeSeconds=0.0;
};

struct MeasuredLapTrace final
{
    int LapNumber=0;
    int GateCount=0;
    bool Complete=false;
    bool AllGatesActivelyTimed=false;
    bool FeeFree=false;
    double LapSeconds=0.0;
    std::array<MeasuredLapGate,MeasuredLapMaxGates> Gates{};
};

struct MeasuredGateComparison final
{
    bool Valid=false;
    int LapNumber=0;
    int ReferenceLapNumber=0;
    int GateIndex=-1;
    double ActualCumulativeSeconds=0.0;
    double ReferenceCumulativeSeconds=0.0;
    double CumulativeDeltaSeconds=0.0;
    double ActualSectorSeconds=0.0;
    double ReferenceSectorSeconds=0.0;
    double SectorDeltaSeconds=0.0;
    double IncludedFeeSeconds=0.0;
    double CumulativeFeeSeconds=0.0;
};

struct MeasuredLapComparison final
{
    bool Valid=false;
    int LapNumber=0;
    int ReferenceLapNumber=0;
    int GateCount=0;
    double LapSeconds=0.0;
    double ReferenceLapSeconds=0.0;
    double DeltaSeconds=0.0;
    double IncludedFeeSeconds=0.0;
    int FasterGates=0;
    int SlowerGates=0;
    int MatchedGates=0;
    int LargestLossGate=-1;
    int LargestGainGate=-1;
    double LargestLossSeconds=0.0;
    double LargestGainSeconds=0.0;
    std::array<MeasuredGateComparison,MeasuredLapMaxGates> Rows{};
};

inline bool ValidMeasuredLapClock(double value) noexcept
{
    return std::isfinite(value)&&value>0.0&&value<=MeasuredLapClockLimit;
}

inline bool ValidMeasuredLapGate(MeasuredLapGate gate,
    MeasuredLapGate previous) noexcept
{
    if(!ValidMeasuredLapClock(gate.CumulativeSeconds)||
       !std::isfinite(gate.CumulativeFeeSeconds)||
       gate.CumulativeFeeSeconds<0.0||
       !std::isfinite(previous.CumulativeSeconds)||
       !std::isfinite(previous.CumulativeFeeSeconds)||
       previous.CumulativeSeconds<0.0||
       previous.CumulativeFeeSeconds<0.0||
       previous.CumulativeFeeSeconds>previous.CumulativeSeconds)
        return false;
    const double duration=gate.CumulativeSeconds-previous.CumulativeSeconds;
    const double fees=gate.CumulativeFeeSeconds-previous.CumulativeFeeSeconds;
    return duration>0.0&&fees>=0.0&&
        fees<=duration+MeasuredLapTolerance&&
        gate.CumulativeFeeSeconds<=gate.CumulativeSeconds;
}

inline bool ValidMeasuredLapTrace(const MeasuredLapTrace& trace) noexcept
{
    if(!trace.Complete||!trace.AllGatesActivelyTimed||trace.LapNumber<=0||
       trace.GateCount<1||trace.GateCount>MeasuredLapMaxGates||
       !ValidMeasuredLapClock(trace.LapSeconds))return false;
    MeasuredLapGate previous{};
    for(int i=0;i<trace.GateCount;++i)
    {
        const auto gate=trace.Gates[static_cast<std::size_t>(i)];
        if(!ValidMeasuredLapGate(gate,previous))return false;
        previous=gate;
    }
    return previous.CumulativeSeconds==trace.LapSeconds&&
        trace.FeeFree==(previous.CumulativeFeeSeconds==0.0);
}

namespace measured_lap_detail
{
// Internal helper: callers establish complete reference and valid actual
// prefix. Keeping this separate avoids repeatedly walking 256 gates when
// producing a full lap report or consuming already-validated live events.
inline MeasuredGateComparison CompareValidatedGate(
    const MeasuredLapTrace& actual,const MeasuredLapTrace& reference,
    int gateIndex) noexcept
{
    if(actual.LapNumber<=reference.LapNumber||reference.LapNumber<=0||
       !actual.AllGatesActivelyTimed||!reference.Complete||
       !reference.AllGatesActivelyTimed||!reference.FeeFree||
       actual.GateCount<1||actual.GateCount>MeasuredLapMaxGates||
       reference.GateCount<actual.GateCount||
       reference.GateCount>MeasuredLapMaxGates||
       gateIndex<0||gateIndex>=actual.GateCount)return {};
    const auto i=static_cast<std::size_t>(gateIndex);
    const auto a=actual.Gates[i],b=reference.Gates[i];
    const auto ap=i>0?actual.Gates[i-1]:MeasuredLapGate{};
    const auto bp=i>0?reference.Gates[i-1]:MeasuredLapGate{};
    if(!ValidMeasuredLapGate(a,ap)||!ValidMeasuredLapGate(b,bp)||
       b.CumulativeFeeSeconds!=0.0)return {};
    const double actualSector=a.CumulativeSeconds-ap.CumulativeSeconds;
    const double referenceSector=b.CumulativeSeconds-bp.CumulativeSeconds;
    return {true,actual.LapNumber,reference.LapNumber,gateIndex,
        a.CumulativeSeconds,b.CumulativeSeconds,
        a.CumulativeSeconds-b.CumulativeSeconds,
        actualSector,referenceSector,actualSector-referenceSector,
        std::min(actualSector,a.CumulativeFeeSeconds-ap.CumulativeFeeSeconds),
        a.CumulativeFeeSeconds};
}

} // namespace measured_lap_detail

inline MeasuredGateComparison CompareMeasuredLapGate(
    const MeasuredLapTrace& actual,const MeasuredLapTrace& reference,
    int gateIndex) noexcept
{
    // Public callers can construct their own traces. The selected pair
    // alone is insufficient evidence: an earlier corrupt gate or later
    // invalid reference gate must not create a credible-looking delta.
    if (!ValidMeasuredLapTrace(reference) || !reference.FeeFree ||
        actual.LapNumber <= reference.LapNumber ||
        !actual.AllGatesActivelyTimed || actual.GateCount < 1 ||
        actual.GateCount > reference.GateCount ||
        gateIndex < 0 || gateIndex >= actual.GateCount)
        return {};
    if (actual.Complete)
    {
        if (actual.GateCount != reference.GateCount ||
            !ValidMeasuredLapTrace(actual))
            return {};
    }
    else
    {
        MeasuredLapGate previous{};
        for (int i = 0; i < actual.GateCount; ++i)
        {
            const auto gate = actual.Gates[static_cast<std::size_t>(i)];
            if (!ValidMeasuredLapGate(gate,previous)) return {};
            previous = gate;
        }
    }
    return measured_lap_detail::CompareValidatedGate(actual,reference,gateIndex);
}

inline MeasuredLapComparison CompareMeasuredLaps(
    const MeasuredLapTrace& actual,const MeasuredLapTrace& reference) noexcept
{
    if(!ValidMeasuredLapTrace(actual)||!ValidMeasuredLapTrace(reference)||
       !reference.FeeFree||actual.LapNumber<=reference.LapNumber||
       actual.GateCount!=reference.GateCount)return {};
    MeasuredLapComparison out{};
    out.LapNumber=actual.LapNumber;
    out.ReferenceLapNumber=reference.LapNumber;
    out.GateCount=actual.GateCount;
    out.LapSeconds=actual.LapSeconds;
    out.ReferenceLapSeconds=reference.LapSeconds;
    out.DeltaSeconds=actual.LapSeconds-reference.LapSeconds;
    out.IncludedFeeSeconds=actual.Gates[
        static_cast<std::size_t>(actual.GateCount-1)].CumulativeFeeSeconds;
    double totalDelta=0.0;
    for(int i=0;i<actual.GateCount;++i)
    {
        const auto row=measured_lap_detail::CompareValidatedGate(actual,reference,i);
        if(!row.Valid)return {};
        out.Rows[static_cast<std::size_t>(i)]=row;
        totalDelta+=row.SectorDeltaSeconds;
        if(row.SectorDeltaSeconds>MeasuredLapTolerance)
        {
            ++out.SlowerGates;
            if(row.SectorDeltaSeconds>out.LargestLossSeconds)
            {
                out.LargestLossSeconds=row.SectorDeltaSeconds;
                out.LargestLossGate=i;
            }
        }
        else if(row.SectorDeltaSeconds< -MeasuredLapTolerance)
        {
            ++out.FasterGates;
            if(-row.SectorDeltaSeconds>out.LargestGainSeconds)
            {
                out.LargestGainSeconds=-row.SectorDeltaSeconds;
                out.LargestGainGate=i;
            }
        }
        else ++out.MatchedGates;
    }
    if(std::abs(totalDelta-out.DeltaSeconds)>MeasuredLapTolerance||
       out.FasterGates+out.SlowerGates+out.MatchedGates!=out.GateCount)
        return {};
    out.Valid=true;
    return out;
}

// Fixed memory. Active-clock evidence is sourced only from a successful
// RaceProgress::Advance since the last accepted checkpoint. No sampling,
// interpolation, time estimation, heap allocation or persisted PB changes.
class MeasuredLapSplitReference final
{
public:
    void Reset(int gateCount) noexcept
    {
        count_=std::clamp(gateCount,1,MeasuredLapMaxGates);
        current_={};
        best_={};
        last_={};
        latestGate_={};
        Begin(1);
    }

    // Ordered physical checkpoints alone advance this observation.
    // A corrupt, zero-time or penalty-only segment invalidates the current
    // comparison prefix, but does not poison the next physical lap.
    void Observe(int lapNumber,int gateIndex,double elapsedLapSeconds,
        double cumulativeFeeSeconds,bool activeSectorClock) noexcept
    {
        if(lapNumber!=current_.LapNumber||gateIndex!=current_.GateCount||
           gateIndex<0||gateIndex>=count_)
        {
            current_.AllGatesActivelyTimed=false;
            latestGate_={};
            return;
        }
        const auto i=static_cast<std::size_t>(gateIndex);
        const auto previous=i>0?current_.Gates[i-1]:MeasuredLapGate{};
        const MeasuredLapGate gate{elapsedLapSeconds,cumulativeFeeSeconds};
        current_.Gates[i]=gate;
        ++current_.GateCount;
        current_.AllGatesActivelyTimed=current_.AllGatesActivelyTimed&&
            activeSectorClock&&ValidMeasuredLapGate(gate,previous);
        latestGate_={};
        if(current_.AllGatesActivelyTimed&&best_.Complete)
            latestGate_=measured_lap_detail::CompareValidatedGate(current_,best_,gateIndex);
    }

    void Complete(int lapNumber,double lapSeconds,double includedFeeSeconds) noexcept
    {
        // Last comparison always belongs to the immediately completed
        // physical lap. Do not show stale analytics after invalid laps.
        last_={};
        current_.LapSeconds=lapSeconds;
        current_.Complete=lapNumber==current_.LapNumber&&
            current_.GateCount==count_&&current_.AllGatesActivelyTimed&&
            std::isfinite(includedFeeSeconds)&&includedFeeSeconds>=0.0&&
            current_.Gates[static_cast<std::size_t>(count_-1)].CumulativeFeeSeconds==
                includedFeeSeconds;
        current_.FeeFree=includedFeeSeconds==0.0;
        if(ValidMeasuredLapTrace(current_))
        {
            // Compare BEFORE replacing the reference, so a new best does
            // not accidentally produce a meaningless self-comparison.
            if(best_.Complete)last_=CompareMeasuredLaps(current_,best_);
            if(current_.FeeFree&&
               (!best_.Complete||current_.LapSeconds<best_.LapSeconds))
                best_=current_;
        }
        Begin(lapNumber<std::numeric_limits<int>::max()?lapNumber+1:0);
    }

    const MeasuredLapTrace& BestReference() const noexcept { return best_; }
    // Read-only exact accepted prefix; never an extrapolated gate or saved record.
    const MeasuredLapTrace& CurrentTrace() const noexcept { return current_; }
    const MeasuredLapComparison& LastComparison() const noexcept { return last_; }

    // Last accepted gate only. This does not estimate an in-between
    // position or compare an unfinished sector with a completed one.
    MeasuredGateComparison LatestGate() const noexcept { return latestGate_; }

private:
    void Begin(int lapNumber) noexcept
    {
        current_={};
        current_.LapNumber=lapNumber;
        current_.AllGatesActivelyTimed=lapNumber>0;
        latestGate_={};
    }
    int count_=4;
    MeasuredLapTrace current_{};
    MeasuredLapTrace best_{};
    MeasuredLapComparison last_{};
    MeasuredGateComparison latestGate_{};
};
} // namespace latebrake
