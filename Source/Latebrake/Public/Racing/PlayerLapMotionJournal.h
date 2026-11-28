#pragma once

#include "Racing/PlayerSectorMotionReview.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// Full-attempt lap rollup of already accepted sector receipts. The
// existing 32-sector diagnostic ring remains useful for detailed review,
// but cannot represent the entire ten-lap practice. Keep one compact
// summary per physical lap; never infer missing movement from a clock.
enum class LapMotionQuality : std::uint8_t
{
    MissingTiming, InsufficientCoverage, Interrupted, Penalized, Qualified
};

inline const char* LapMotionQualityName(LapMotionQuality quality) noexcept
{
    switch(quality)
    {
    case LapMotionQuality::Qualified:return "QUALIFIED";
    case LapMotionQuality::Penalized:return "PENALIZED";
    case LapMotionQuality::Interrupted:return "INTERRUPTED";
    case LapMotionQuality::InsufficientCoverage:return "LOW COVERAGE";
    default:return "MISSING TIMING";
    }
}

inline bool LapMotionClose(double a,double b) noexcept
{
    return std::isfinite(a) && std::isfinite(b) &&
        std::abs(a-b)<=SectorMotionClockTolerance;
}

inline bool ValidLapMotionMetrics(const SectorMotionMetrics& m,double active) noexcept
{
    if(!std::isfinite(active) || active<=0 ||
        !std::isfinite(m.ObservedSeconds) || m.ObservedSeconds<0 ||
        m.ObservedSeconds>active+SectorMotionClockTolerance ||
        // An interval requires two accepted endpoints. Cuts can only reduce
        // the number of intervals that a sample sequence contributes.
        (m.Intervals && m.Intervals>=m.Samples) ||
        m.RecoveryBreaks>m.Breaks || m.PositionBreaks>m.Breaks-m.RecoveryBreaks ||
        m.Breaks>m.Samples ||
        m.ObservedSeconds>static_cast<double>(m.Intervals)*
            (SectorMotionMaximumDelta+SectorMotionClockTolerance)+SectorMotionClockTolerance ||
        !std::isfinite(m.SpeedIntegral) || m.SpeedIntegral<0 ||
        !std::isfinite(m.PlanarDistanceCm) || m.PlanarDistanceCm<0 ||
        !std::isfinite(m.PeakSpeed) || m.PeakSpeed<0 ||
        m.PeakSpeed>SectorMotionMaximumSpeed ||
        (m.ObservedSeconds==0 && (m.Intervals || m.SpeedIntegral!=0 || m.PlanarDistanceCm!=0)) ||
        (m.ObservedSeconds>0 && !m.Intervals) ||
        m.SpeedIntegral>m.PeakSpeed*m.ObservedSeconds+SectorMotionClockTolerance)
        return false;
    const double durations[]={m.LowSpeedSeconds,m.HighSlipSeconds,m.ReverseSeconds,
        m.PartialRoadSeconds,m.FullOffRoadSeconds};
    for(double seconds:durations)
        if(!std::isfinite(seconds) || seconds<0 ||
            seconds>m.ObservedSeconds+SectorMotionClockTolerance)return false;
    const double speeds[]={m.MinimumSpeed,m.FirstSpeed,m.LastSpeed};
    for(double speed:speeds)
        if(!std::isfinite(speed) || speed<0 || speed>m.PeakSpeed)return false;
    if(m.MinimumSpeed>m.FirstSpeed || m.MinimumSpeed>m.LastSpeed ||
        m.SpeedIntegral+SectorMotionClockTolerance<m.MinimumSpeed*m.ObservedSeconds ||
        !std::isfinite(m.LateralIntegral) || m.LateralIntegral<0 ||
        m.LateralIntegral>m.SpeedIntegral+SectorMotionClockTolerance)
        return false;
    const double episodes[]={m.LongestLowSpeedSeconds,m.LongestSlipSeconds};
    const double totals[]={m.LowSpeedSeconds,m.HighSlipSeconds};
    for(std::size_t i=0;i<2;++i)
        if(!std::isfinite(episodes[i]) || episodes[i]<0 ||
            episodes[i]>totals[i]+SectorMotionClockTolerance)return false;
    const double changes[]={m.AccelerationIntegral,m.DecelerationIntegral};
    for(double change:changes)
        if(!std::isfinite(change) || change<0 ||
            change>static_cast<double>(m.Intervals)*m.PeakSpeed)return false;
    if(!m.Samples && (m.PeakSpeed!=0 || m.Breaks!=0))return false;
    // The continuity check allows a bounded position residual per interval.
    // Use that same conservative envelope rather than treating the path as
    // exact wheel travel or accepting arbitrarily large recorded distance.
    const double distanceBound=SectorMotionMaximumSpeed*m.ObservedSeconds+
        SectorMotionPositionResidual*static_cast<double>(m.Intervals);
    return m.PlanarDistanceCm<=distanceBound+SectorMotionClockTolerance &&
        m.FullOffRoadSeconds<=m.PartialRoadSeconds+SectorMotionClockTolerance;
}

struct LapMotionTotals final
{
    int Gates=0;
    int TimedGates=0;
    int SampledGates=0;
    int QualifiedGates=0;
    int InterruptedGates=0;
    int PenalizedGates=0;
    double OfficialSeconds=0;
    double ActiveSeconds=0;
    double FeeSeconds=0;
    double ObservedSeconds=0;
    double SpeedIntegral=0;
    double PeakSpeed=0;
    double DistanceCm=0;
    double LowSpeedSeconds=0;
    double HighSlipSeconds=0;
    double ReverseSeconds=0;
    double PartialRoadSeconds=0;
    double FullOffRoadSeconds=0;

    double Coverage() const noexcept
    {return ActiveSeconds>0?std::clamp(ObservedSeconds/ActiveSeconds,0.0,1.0):0;}

    double MeanKmh() const noexcept
    {return ObservedSeconds>0?SpeedIntegral/ObservedSeconds*0.036:0;}

    double PeakKmh() const noexcept {return PeakSpeed*0.036;}
    double DistanceMeters() const noexcept {return DistanceCm/100.0;}

    void Include(const LapMotionTotals& other) noexcept
    {
        Gates+=other.Gates;
        TimedGates+=other.TimedGates;
        SampledGates+=other.SampledGates;
        QualifiedGates+=other.QualifiedGates;
        InterruptedGates+=other.InterruptedGates;
        PenalizedGates+=other.PenalizedGates;
        OfficialSeconds+=other.OfficialSeconds;
        ActiveSeconds+=other.ActiveSeconds;
        FeeSeconds+=other.FeeSeconds;
        ObservedSeconds+=other.ObservedSeconds;
        SpeedIntegral+=other.SpeedIntegral;
        PeakSpeed=std::max(PeakSpeed,other.PeakSpeed);
        DistanceCm+=other.DistanceCm;
        LowSpeedSeconds+=other.LowSpeedSeconds;
        HighSlipSeconds+=other.HighSlipSeconds;
        ReverseSeconds+=other.ReverseSeconds;
        PartialRoadSeconds+=other.PartialRoadSeconds;
        FullOffRoadSeconds+=other.FullOffRoadSeconds;
    }
};

struct PlayerLapMotionRow final
{
    bool Present=false;
    int Lap=0;
    bool StandingStart=false;
    LapMotionQuality Quality=LapMotionQuality::MissingTiming;
    LapMotionTotals Totals{};

    bool Comparable() const noexcept
    {return Present && !StandingStart && Quality==LapMotionQuality::Qualified;}
};

inline LapMotionQuality ClassifyLapMotion(const LapMotionTotals& totals) noexcept
{
    if(totals.Gates!=CircuitGateCount || totals.TimedGates!=CircuitGateCount)
        return LapMotionQuality::MissingTiming;
    if(totals.FeeSeconds>SectorMotionClockTolerance)
        return LapMotionQuality::Penalized;
    if(totals.InterruptedGates)
        return LapMotionQuality::Interrupted;
    if(totals.QualifiedGates!=CircuitGateCount ||
        totals.Coverage()<SectorMotionMinimumCoverage)
        return LapMotionQuality::InsufficientCoverage;
    return LapMotionQuality::Qualified;
}

struct PlayerLapMotionReview final
{
    bool Valid=false;
    bool Finished=false;
    int RequiredLaps=0;
    int CompletedLaps=0;
    int AcceptedGates=0;
    int QualifiedLaps=0;
    int ComparableLaps=0;
    int PenalizedLaps=0;
    int InterruptedLaps=0;
    int LowCoverageLaps=0;
    int MissingTimingLaps=0;
    LapMotionTotals Completed{};
    LapMotionTotals Current{};
    PlayerLapMotionRow BestRolling{};
    PlayerLapMotionRow SlowestRolling{};
    std::array<PlayerLapMotionRow,SectorMotionMaximumLaps> Rows{};

    PlayerLapMotionRow At(int lap) const noexcept
    {
        return Valid && lap>=1 && lap<=CompletedLaps &&
            lap<=SectorMotionMaximumLaps?Rows[static_cast<std::size_t>(lap-1)]:PlayerLapMotionRow{};
    }
};

class PlayerLapMotionJournal final
{
public:
    void Reset(int gates,int laps) noexcept
    {
        *this={};
        Configured=gates==CircuitGateCount && laps>=1 && laps<=SectorMotionMaximumLaps;
        Required=Configured?laps:0;
    }

    // Receipts come from the actual ordered gate callback. Missing motion
    // does not erase official timing. It only reduces evidence coverage.
    bool Accept(const SectorMotionReceipt& receipt,const SectorMotionRow& sector) noexcept
    {
        if(!Configured || Accepted>=Required*CircuitGateCount ||
            receipt.AcceptedOrdinal!=Accepted+1 || receipt.GateIndex!=Accepted%CircuitGateCount ||
            !ValidSectorMotionClock(receipt.ElapsedSeconds,receipt.PenaltySeconds) ||
            receipt.ElapsedSeconds<Elapsed || receipt.PenaltySeconds<Penalty ||
            receipt.ElapsedSeconds-receipt.PenaltySeconds<Elapsed-Penalty)
            return false;
        LapMotionTotals item;
        item.Gates=1;
        item.OfficialSeconds=receipt.ElapsedSeconds-Elapsed;
        item.FeeSeconds=receipt.PenaltySeconds-Penalty;
        item.ActiveSeconds=item.OfficialSeconds-item.FeeSeconds;
        if(item.ActiveSeconds>0 && LapMotionClose(item.OfficialSeconds,receipt.OfficialSplitSeconds))
            item.TimedGates=1;
        if(item.TimedGates && sector.ValidTiming &&
            sector.Ordinal==receipt.AcceptedOrdinal && sector.Gate==receipt.GateIndex &&
            sector.Lap==Accepted/CircuitGateCount+1 &&
            sector.StandingStart==(Accepted==0) &&
            LapMotionClose(sector.OfficialSeconds,item.OfficialSeconds) &&
            LapMotionClose(sector.ActiveSeconds,item.ActiveSeconds) &&
            LapMotionClose(sector.FeeSeconds,item.FeeSeconds) &&
            ValidLapMotionMetrics(sector.Motion,item.ActiveSeconds) &&
            LapMotionClose(sector.Coverage,
                std::clamp(sector.Motion.ObservedSeconds/item.ActiveSeconds,0.0,1.0)))
        {
            const auto& m=sector.Motion;
            item.SampledGates=m.ObservedSeconds>0?1:0;
            item.ObservedSeconds=m.ObservedSeconds;
            item.SpeedIntegral=m.SpeedIntegral;
            item.PeakSpeed=m.PeakSpeed;
            item.DistanceCm=m.PlanarDistanceCm;
            item.LowSpeedSeconds=m.LowSpeedSeconds;
            item.HighSlipSeconds=m.HighSlipSeconds;
            item.ReverseSeconds=m.ReverseSeconds;
            item.PartialRoadSeconds=m.PartialRoadSeconds;
            item.FullOffRoadSeconds=m.FullOffRoadSeconds;
            item.InterruptedGates=(m.Breaks || m.Rejected ||
                sector.Quality==SectorMotionQuality::Interrupted)?1:0;
            item.QualifiedGates=sector.Quality==SectorMotionQuality::Qualified &&
                !item.InterruptedGates && item.FeeSeconds<=SectorMotionClockTolerance &&
                item.ActiveSeconds>=SectorMotionMinimumDuration && m.Intervals>0 &&
                sector.Coverage>=SectorMotionMinimumCoverage?1:0;
        }
        item.PenalizedGates=item.FeeSeconds>SectorMotionClockTolerance?1:0;
        Pending.Include(item);
        Elapsed=receipt.ElapsedSeconds;
        Penalty=receipt.PenaltySeconds;
        ++Accepted;
        if(Accepted%CircuitGateCount==0)CommitLap();
        return true;
    }

    PlayerLapMotionReview Read(int ordinal,double elapsed,double penalty,bool finished) const noexcept
    {
        if(!Configured || ordinal!=Accepted ||
            !ValidSectorMotionClock(elapsed,penalty) || elapsed<Elapsed || penalty<Penalty ||
            elapsed-penalty+SectorMotionClockTolerance<Elapsed-Penalty ||
            finished!=(Accepted==Required*CircuitGateCount) ||
            (finished && (!LapMotionClose(elapsed,Elapsed) || !LapMotionClose(penalty,Penalty))))
            return {};
        auto out=Stored;
        out.Valid=true;
        out.Finished=finished;
        out.RequiredLaps=Required;
        out.AcceptedGates=Accepted;
        out.Current=Pending;
        return out;
    }

private:
    void CommitLap() noexcept
    {
        PlayerLapMotionRow row;
        row.Present=true;
        row.Lap=Accepted/CircuitGateCount;
        row.StandingStart=row.Lap==1;
        row.Totals=Pending;
        row.Quality=ClassifyLapMotion(Pending);
        Stored.Rows[static_cast<std::size_t>(row.Lap-1)]=row;
        Stored.CompletedLaps=row.Lap;
        Stored.Completed.Include(Pending);
        switch(row.Quality)
        {
        case LapMotionQuality::Qualified:++Stored.QualifiedLaps;break;
        case LapMotionQuality::Penalized:++Stored.PenalizedLaps;break;
        case LapMotionQuality::Interrupted:++Stored.InterruptedLaps;break;
        case LapMotionQuality::InsufficientCoverage:++Stored.LowCoverageLaps;break;
        default:++Stored.MissingTimingLaps;break;
        }
        if(row.Comparable())
        {
            ++Stored.ComparableLaps;
            if(!Stored.BestRolling.Present ||
                row.Totals.ActiveSeconds<Stored.BestRolling.Totals.ActiveSeconds)
                Stored.BestRolling=row;
            if(!Stored.SlowestRolling.Present ||
                row.Totals.ActiveSeconds>Stored.SlowestRolling.Totals.ActiveSeconds)
                Stored.SlowestRolling=row;
        }
        Pending={};
    }

    bool Configured=false;
    int Required=0;
    int Accepted=0;
    double Elapsed=0,Penalty=0;
    LapMotionTotals Pending{};
    PlayerLapMotionReview Stored{};
};
} // namespace latebrake
