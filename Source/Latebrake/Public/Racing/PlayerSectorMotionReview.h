#pragma once

#include "Track/CircuitLayout.h"
#include "Track/TrackLimitsFootprint.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Run-local, read-only motion review for the original four-gate circuit.
// It consumes the actual accepted race clock and actual body observations.
// Geometry is a projected visual footprint, not a wheel-contact sensor.
// Neither a sample nor any derived comparison can accept a gate,
// estimate unobserved lap time, award credits or change physics.
inline constexpr int SectorMotionHistoryCapacity=32;
inline constexpr int SectorMotionMaximumLaps=100;
inline constexpr double SectorMotionMaximumClock=604800.0;
inline constexpr double SectorMotionMaximumDelta=0.10;
inline constexpr double SectorMotionClockTolerance=1e-6;
inline constexpr double SectorMotionMaximumSpeed=15000.0;
inline constexpr double SectorMotionMaximumPosition=100000.0;
inline constexpr double SectorMotionPositionResidual=175.0;
inline constexpr double SectorMotionMinimumCoverage=0.85;
inline constexpr double SectorMotionMinimumDuration=0.5;
inline constexpr double SectorMotionMinimumComparisonDelta=0.10;

struct SectorMotionFrame final
{
    std::uint64_t Actor=0;
    std::uint64_t EngineFrame=0;
    std::uint64_t SessionRevision=0;
    std::uint64_t TeleportRevision=0;
    int Ordinal=0;
    double ElapsedSeconds=0.0;
    double PenaltySeconds=0.0;
    double AdvancedSeconds=0.0;
    CircuitPoint Position{};
    double WorldZ=0.0;
    double VelocityX=0.0;
    double VelocityY=0.0;
    double VelocityZ=0.0;
    double HeadingDegrees=0.0;
    bool PhysicalBodyObserved=false;
    bool ClockAdvanced=false;
};

struct SectorMotionEndpoint final
{
    bool Valid=false;
    double Speed=0.0;
    double ForwardSpeed=0.0;
    double LateralSpeed=0.0;
    bool FullyOffProjectedRoad=false;
    bool PartlyOffProjectedRoad=false;
    bool LowSpeed=false;
    bool HighSlip=false;
    bool ReverseMotion=false;
};

inline bool ValidSectorMotionClock(double elapsed,double penalty) noexcept
{
    return std::isfinite(elapsed) && elapsed>=0.0 && elapsed<=SectorMotionMaximumClock &&
        std::isfinite(penalty) && penalty>=0.0 && penalty<=elapsed;
}

inline double SectorMotionActiveClock(const SectorMotionFrame& frame) noexcept
{
    return frame.ElapsedSeconds-frame.PenaltySeconds;
}

inline SectorMotionEndpoint MeasureSectorMotionEndpoint(const SectorMotionFrame& f) noexcept
{
    if(!f.PhysicalBodyObserved || !f.ClockAdvanced || !f.Actor || !f.EngineFrame ||
        f.Ordinal<0 || f.Ordinal>=CircuitGateCount*SectorMotionMaximumLaps ||
        !ValidSectorMotionClock(f.ElapsedSeconds,f.PenaltySeconds) ||
        !std::isfinite(f.AdvancedSeconds) || f.AdvancedSeconds<=0.0 ||
        f.AdvancedSeconds>SectorMotionMaximumDelta ||
        SectorMotionActiveClock(f)+SectorMotionClockTolerance<f.AdvancedSeconds ||
        !std::isfinite(f.Position.X) || !std::isfinite(f.Position.Y) ||
        !std::isfinite(f.WorldZ) || std::abs(f.Position.X)>SectorMotionMaximumPosition ||
        std::abs(f.Position.Y)>SectorMotionMaximumPosition ||
        std::abs(f.WorldZ)>SectorMotionMaximumPosition ||
        !std::isfinite(f.VelocityX) || !std::isfinite(f.VelocityY) ||
        !std::isfinite(f.VelocityZ) || !std::isfinite(f.HeadingDegrees))
        return {};
    const double speed=std::hypot(f.VelocityX,f.VelocityY);
    if(std::hypot(speed,f.VelocityZ)>SectorMotionMaximumSpeed)
        return {};
    const auto footprint=ObserveStarterWheelFootprint(f.Position,f.HeadingDegrees);
    if(!footprint.Valid) return {};
    const double yaw=std::remainder(f.HeadingDegrees,360.0)*0.01745329251994329577;
    const double forward=f.VelocityX*std::cos(yaw)+f.VelocityY*std::sin(yaw);
    const double lateral=f.VelocityY*std::cos(yaw)-f.VelocityX*std::sin(yaw);
    SectorMotionEndpoint out;
    out.Valid=true;
    out.Speed=speed;
    out.ForwardSpeed=forward;
    out.LateralSpeed=lateral;
    out.FullyOffProjectedRoad=footprint.FullyOffPavement;
    out.PartlyOffProjectedRoad=footprint.WheelsNearPavement<4;
    out.LowSpeed=speed<100.0;
    out.HighSlip=speed>=500.0 && std::abs(lateral)>0.40*speed;
    out.ReverseMotion=forward< -100.0;
    return out;
}

enum class SectorMotionBreak
{
    None,
    MissingFrame,
    SessionChanged,
    Recovery,
    Penalty,
    ClockMismatch,
    PositionJump,
    InvalidSample,
    Suspended
};

inline SectorMotionBreak SectorMotionContinuity(const SectorMotionFrame& a,
    const SectorMotionFrame& b,const SectorMotionEndpoint& first,
    const SectorMotionEndpoint& last) noexcept
{
    if(!first.Valid || !last.Valid || a.Actor!=b.Actor || a.Ordinal!=b.Ordinal)
        return SectorMotionBreak::InvalidSample;
    if(b.EngineFrame<=a.EngineFrame || b.EngineFrame-a.EngineFrame!=1)
        return SectorMotionBreak::MissingFrame;
    if(a.SessionRevision!=b.SessionRevision) return SectorMotionBreak::SessionChanged;
    if(a.TeleportRevision!=b.TeleportRevision) return SectorMotionBreak::Recovery;
    if(a.PenaltySeconds!=b.PenaltySeconds) return SectorMotionBreak::Penalty;
    const double dt=SectorMotionActiveClock(b)-SectorMotionActiveClock(a);
    if(!std::isfinite(dt) || dt<=0.0 ||
        dt>SectorMotionMaximumDelta+SectorMotionClockTolerance ||
        std::abs(dt-b.AdvancedSeconds)>SectorMotionClockTolerance)
        return SectorMotionBreak::ClockMismatch;
    const double distance=std::hypot(std::hypot(b.Position.X-a.Position.X,
        b.Position.Y-a.Position.Y),b.WorldZ-a.WorldZ);
    const double speed=std::max(std::hypot(first.Speed,a.VelocityZ),
        std::hypot(last.Speed,b.VelocityZ));
    if(!std::isfinite(distance) || distance>speed*dt+SectorMotionPositionResidual)
        return SectorMotionBreak::PositionJump;
    return SectorMotionBreak::None;
}

struct SectorMotionMetrics final
{
    std::uint64_t Samples=0;
    std::uint64_t Intervals=0;
    std::uint64_t Breaks=0;
    std::uint64_t Rejected=0;
    std::uint64_t RecoveryBreaks=0;
    std::uint64_t PositionBreaks=0;
    double ObservedSeconds=0.0;
    double SpeedIntegral=0.0;
    double LateralIntegral=0.0;
    double PlanarDistanceCm=0.0;
    double PeakSpeed=0.0;
    double MinimumSpeed=0.0;
    double FirstSpeed=0.0;
    double LastSpeed=0.0;
    double LowSpeedSeconds=0.0;
    double HighSlipSeconds=0.0;
    double ReverseSeconds=0.0;
    double PartialRoadSeconds=0.0;
    double FullOffRoadSeconds=0.0;
    double DecelerationIntegral=0.0;
    double AccelerationIntegral=0.0;
    double LongestLowSpeedSeconds=0.0;
    double LongestSlipSeconds=0.0;

    double AverageKmh() const noexcept
    {
        return ObservedSeconds>0.0?SpeedIntegral/ObservedSeconds*0.036:0.0;
    }

    double PeakKmh() const noexcept {return PeakSpeed*0.036;}

    double MeanAbsLateralKmh() const noexcept
    {
        return ObservedSeconds>0.0?LateralIntegral/ObservedSeconds*0.036:0.0;
    }

    double DistanceMeters() const noexcept {return PlanarDistanceCm/100.0;}
};

struct SectorMotionReceipt final
{
    int AcceptedOrdinal=0;
    int GateIndex=-1;
    double ElapsedSeconds=0.0;
    double PenaltySeconds=0.0;
    double OfficialSplitSeconds=0.0;
};

enum class SectorMotionQuality
{
    Unavailable,
    InsufficientCoverage,
    Interrupted,
    Penalized,
    Qualified
};

struct SectorMotionRow final
{
    bool ValidTiming=false;
    int Ordinal=0;
    int Lap=0;
    int Gate=-1;
    bool StandingStart=false;
    double ActiveSeconds=0.0;
    double FeeSeconds=0.0;
    double OfficialSeconds=0.0;
    double Coverage=0.0;
    SectorMotionQuality Quality=SectorMotionQuality::Unavailable;
    SectorMotionMetrics Motion{};

    bool Comparable() const noexcept {return Quality==SectorMotionQuality::Qualified;}
};

inline const char* SectorMotionQualityName(SectorMotionQuality quality) noexcept
{
    switch(quality)
    {
        case SectorMotionQuality::Qualified:return "QUALIFIED";
        case SectorMotionQuality::InsufficientCoverage:return "LOW COVERAGE";
        case SectorMotionQuality::Interrupted:return "INTERRUPTED";
        case SectorMotionQuality::Penalized:return "PENALIZED";
        default:return "UNAVAILABLE";
    }
}

struct SectorMotionComparison final
{
    bool Valid=false;
    int Gate=-1;
    int ActualLap=0;
    int ReferenceLap=0;
    double TimeDeltaSeconds=0.0;
    double MeanSpeedDeltaKmh=0.0;
    double PeakSpeedDeltaKmh=0.0;
    double SlipDeltaSeconds=0.0;
    double LowSpeedDeltaSeconds=0.0;
    double OffRoadDeltaSeconds=0.0;
    double DistanceDeltaMeters=0.0;
    bool MeaningfulTimeDelta=false;
};

inline SectorMotionComparison CompareSectorMotionRows(const SectorMotionRow& actual,
    const SectorMotionRow& reference) noexcept
{
    if(!actual.Comparable() || !reference.Comparable() || actual.StandingStart ||
        reference.StandingStart || actual.Gate!=reference.Gate ||
        actual.Gate<0 || actual.Gate>=CircuitGateCount ||
        actual.Ordinal<=reference.Ordinal || actual.Lap<=reference.Lap)
        return {};
    const double delta=actual.ActiveSeconds-reference.ActiveSeconds;
    if(!std::isfinite(delta)) return {};
    return {true,actual.Gate,actual.Lap,reference.Lap,delta,
        actual.Motion.AverageKmh()-reference.Motion.AverageKmh(),
        actual.Motion.PeakKmh()-reference.Motion.PeakKmh(),
        actual.Motion.HighSlipSeconds-reference.Motion.HighSlipSeconds,
        actual.Motion.LowSpeedSeconds-reference.Motion.LowSpeedSeconds,
        actual.Motion.FullOffRoadSeconds-reference.Motion.FullOffRoadSeconds,
        actual.Motion.DistanceMeters()-reference.Motion.DistanceMeters(),
        std::abs(delta)>=SectorMotionMinimumComparisonDelta};
}

struct SectorMotionGateSummary final
{
    int Gate=-1;
    int Timed=0;
    int Qualified=0;
    int Interrupted=0;
    int Penalized=0;
    int Insufficient=0;
    int StandingStarts=0;
    double QualifiedSeconds=0.0;
    double QualifiedObservedSeconds=0.0;
    double QualifiedSpeedIntegral=0.0;
    SectorMotionRow Best{};
    SectorMotionRow Latest{};
    SectorMotionComparison LatestComparison{};

    double QualifiedMeanKmh() const noexcept
    {
        return QualifiedObservedSeconds>0.0?
            QualifiedSpeedIntegral/QualifiedObservedSeconds*0.036:0.0;
    }
};

struct SectorMotionReview final
{
    bool Valid=false;
    bool Finished=false;
    int AcceptedGates=0;
    int TimedSectors=0;
    int QualifiedSectors=0;
    int MissingTiming=0;
    int RequiredLaps=0;
    int RetainedRows=0;
    double ActiveSeconds=0.0;
    double CurrentSectorSeconds=0.0;
    SectorMotionMetrics Current{};
    SectorMotionRow Last{};
    SectorMotionComparison LastComparison{};
    std::array<SectorMotionGateSummary,CircuitGateCount> Gates{};
};

struct SectorMotionHistory final
{
    bool Valid=false;
    int AcceptedGates=0;
    int Count=0;
    double ActiveSeconds=0.0;
    std::array<SectorMotionRow,SectorMotionHistoryCapacity> Rows{};
};

class PlayerSectorMotionJournal final
{
public:
    void Reset(int gateCount,int laps) noexcept
    {
        *this={};
        Configured=gateCount==CircuitGateCount && laps>=1 &&
            laps<=SectorMotionMaximumLaps;
        RequiredLaps=Configured?laps:0;
        for(int i=0;i<CircuitGateCount;++i)
            Gates[static_cast<std::size_t>(i)].Gate=i;
    }

    bool Observe(const SectorMotionFrame& frame) noexcept
    {
        if(!Configured || Accepted>=TotalGates() || frame.Ordinal!=Accepted)
            return false;
        // Stale and foreign observations cannot close a newer interval.
        if((BoundActor && frame.Actor!=BoundActor) ||
            (LastAttemptedFrame && frame.EngineFrame<=LastAttemptedFrame))
            return false;
        if(frame.EngineFrame) LastAttemptedFrame=frame.EngineFrame;
        const auto endpoint=MeasureSectorMotionEndpoint(frame);
        if(!endpoint.Valid || frame.ElapsedSeconds<BoundaryElapsed ||
            frame.PenaltySeconds<BoundaryPenalty ||
            SectorMotionActiveClock(frame)+SectorMotionClockTolerance<BoundaryActive() ||
            (Previous.EngineFrame && (frame.ElapsedSeconds<Previous.ElapsedSeconds ||
                frame.PenaltySeconds<Previous.PenaltySeconds ||
                SectorMotionActiveClock(frame)+SectorMotionClockTolerance<
                    SectorMotionActiveClock(Previous))))
        {
            ++Current.Rejected;
            Cut(SectorMotionBreak::InvalidSample);
            return false;
        }
        if(!BoundActor) BoundActor=frame.Actor;
        if(HavePrevious)
        {
            const auto reason=SectorMotionContinuity(Previous,frame,PreviousEndpoint,endpoint);
            if(reason==SectorMotionBreak::None)
                AddInterval(Previous,frame,PreviousEndpoint,endpoint);
            else Cut(reason);
        }
        AddSample(endpoint);
        Previous=frame;
        PreviousEndpoint=endpoint;
        HavePrevious=true;
        BreakOpen=false;
        return true;
    }

    void Suspend() noexcept
    {
        if(Configured && Accepted<TotalGates()) Cut(SectorMotionBreak::Suspended);
    }

    // Only the independently accepted RaceProgress checkpoint calls
    // this method. Do not infer completion from position or time alone.
    bool AcceptGate(const SectorMotionReceipt& receipt) noexcept
    {
        if(!Configured || Accepted>=TotalGates() ||
            receipt.AcceptedOrdinal!=Accepted+1 ||
            receipt.GateIndex!=Accepted%CircuitGateCount ||
            !ValidSectorMotionClock(receipt.ElapsedSeconds,receipt.PenaltySeconds) ||
            receipt.ElapsedSeconds<BoundaryElapsed ||
            receipt.PenaltySeconds<BoundaryPenalty ||
            receipt.ElapsedSeconds-receipt.PenaltySeconds+SectorMotionClockTolerance<BoundaryActive())
            return false;

        SectorMotionRow row;
        row.Ordinal=receipt.AcceptedOrdinal;
        row.Gate=receipt.GateIndex;
        row.Lap=Accepted/CircuitGateCount+1;
        row.StandingStart=Accepted==0;
        row.OfficialSeconds=receipt.OfficialSplitSeconds;
        row.FeeSeconds=receipt.PenaltySeconds-BoundaryPenalty;
        row.ActiveSeconds=receipt.ElapsedSeconds-receipt.PenaltySeconds-BoundaryActive();
        row.Motion=Current;
        row.ValidTiming=std::isfinite(row.OfficialSeconds) && row.OfficialSeconds>0.0 &&
            row.ActiveSeconds>0.0 && std::abs(row.OfficialSeconds-row.FeeSeconds-
                row.ActiveSeconds)<=SectorMotionClockTolerance &&
            // A continuity cut does not erase the timestamp of the last accepted
            // sample. Future motion must not qualify an earlier checkpoint.
            (!Previous.EngineFrame || (Previous.ElapsedSeconds<=receipt.ElapsedSeconds &&
                Previous.PenaltySeconds<=receipt.PenaltySeconds &&
                SectorMotionActiveClock(Previous)<=receipt.ElapsedSeconds-
                    receipt.PenaltySeconds+SectorMotionClockTolerance)) &&
            Current.ObservedSeconds<=row.ActiveSeconds+SectorMotionClockTolerance;
        if(row.ValidTiming)
        {
            row.Coverage=std::clamp(Current.ObservedSeconds/row.ActiveSeconds,0.0,1.0);
            if(row.FeeSeconds>SectorMotionClockTolerance)
                row.Quality=SectorMotionQuality::Penalized;
            else if(Current.Breaks || Current.Rejected)
                row.Quality=SectorMotionQuality::Interrupted;
            else if(row.ActiveSeconds<SectorMotionMinimumDuration ||
                Current.Intervals==0 || row.Coverage<SectorMotionMinimumCoverage)
                row.Quality=SectorMotionQuality::InsufficientCoverage;
            else row.Quality=SectorMotionQuality::Qualified;
        }
        auto& summary=Gates[static_cast<std::size_t>(row.Gate)];
        LastComparison=CompareSectorMotionRows(row,summary.Best);
        summary.LatestComparison=LastComparison;
        summary.Latest=row;
        if(row.StandingStart) ++summary.StandingStarts;
        if(row.ValidTiming)
        {
            ++Timed;
            ++summary.Timed;
            switch(row.Quality)
            {
                case SectorMotionQuality::Qualified:
                    ++Qualified;
                    ++summary.Qualified;
                    if(!row.StandingStart)
                    {
                        summary.QualifiedSeconds+=row.ActiveSeconds;
                        summary.QualifiedObservedSeconds+=row.Motion.ObservedSeconds;
                        summary.QualifiedSpeedIntegral+=row.Motion.SpeedIntegral;
                        if(!summary.Best.Comparable() || row.ActiveSeconds<summary.Best.ActiveSeconds)
                            summary.Best=row;
                    }
                    break;
                case SectorMotionQuality::Interrupted:++summary.Interrupted;break;
                case SectorMotionQuality::Penalized:++summary.Penalized;break;
                case SectorMotionQuality::InsufficientCoverage:++summary.Insufficient;break;
                default:break;
            }
        }
        else ++MissingTiming;
        History[Next]=row;
        Next=(Next+1)%History.size();
        if(Size<History.size()) ++Size;
        ++Accepted;
        BoundaryElapsed=receipt.ElapsedSeconds;
        BoundaryPenalty=receipt.PenaltySeconds;
        Last=row;
        Current={};
        HavePrevious=false;
        BreakOpen=false;
        Previous={};
        PreviousEndpoint={};
        LowRun=0.0;
        SlipRun=0.0;
        return row.ValidTiming;
    }

    bool TryReadRecent(std::size_t offset,SectorMotionRow& row) const noexcept
    {
        row={};
        if(offset>=Size) return false;
        row=History[(Next+History.size()-1-offset)%History.size()];
        return true;
    }

    SectorMotionHistory ReadFinishedHistory(int authoritativeOrdinal,
        double elapsed,double penalty) const noexcept
    {
        SectorMotionHistory out;
        const auto review=Read(authoritativeOrdinal,elapsed,penalty,true);
        if(!review.Valid || !review.Finished || review.RetainedRows<=0) return out;
        out.Valid=true;
        out.AcceptedGates=review.AcceptedGates;
        out.ActiveSeconds=review.ActiveSeconds;
        for(std::size_t i=0;i<Size;++i)
        {
            if(!TryReadRecent(i,out.Rows[i])) return {};
            ++out.Count;
        }
        return out;
    }

    SectorMotionReview Read(int authoritativeOrdinal,double elapsed,double penalty,
        bool finished) const noexcept
    {
        SectorMotionReview out;
        if(!Configured || authoritativeOrdinal!=Accepted ||
            !ValidSectorMotionClock(elapsed,penalty) ||
            elapsed<BoundaryElapsed || penalty<BoundaryPenalty ||
            elapsed-penalty+SectorMotionClockTolerance<BoundaryActive() ||
            finished!=(Accepted==TotalGates()) ||
            // Live reads must not attach samples to an older authoritative clock.
            (!finished && Previous.EngineFrame &&
                (elapsed<Previous.ElapsedSeconds || penalty<Previous.PenaltySeconds ||
                 elapsed-penalty+SectorMotionClockTolerance<SectorMotionActiveClock(Previous))) ||
            (finished && (std::abs(elapsed-BoundaryElapsed)>SectorMotionClockTolerance ||
                std::abs(penalty-BoundaryPenalty)>SectorMotionClockTolerance)))
            return out;
        out.Valid=true;
        out.Finished=finished;
        out.AcceptedGates=Accepted;
        out.RequiredLaps=RequiredLaps;
        out.TimedSectors=Timed;
        out.QualifiedSectors=Qualified;
        out.MissingTiming=MissingTiming;
        out.RetainedRows=static_cast<int>(Size);
        // A tolerated floating-point difference must not mutate a frozen result.
        out.ActiveSeconds=finished?BoundaryActive():elapsed-penalty;
        out.CurrentSectorSeconds=finished?0.0:std::max(0.0,out.ActiveSeconds-BoundaryActive());
        out.Current=Current;
        out.Last=Last;
        out.LastComparison=LastComparison;
        out.Gates=Gates;
        return out;
    }

private:
    int TotalGates() const noexcept {return RequiredLaps*CircuitGateCount;}
    double BoundaryActive() const noexcept {return BoundaryElapsed-BoundaryPenalty;}

    void Cut(SectorMotionBreak reason) noexcept
    {
        if(Current.Samples && !BreakOpen)
        {
            ++Current.Breaks;
            if(reason==SectorMotionBreak::Recovery) ++Current.RecoveryBreaks;
            if(reason==SectorMotionBreak::PositionJump) ++Current.PositionBreaks;
            BreakOpen=true;
        }
        HavePrevious=false;
        LowRun=0.0;
        SlipRun=0.0;
    }

    void AddSample(const SectorMotionEndpoint& e) noexcept
    {
        if(!Current.Samples)
        {
            Current.MinimumSpeed=e.Speed;
            Current.FirstSpeed=e.Speed;
        }
        ++Current.Samples;
        Current.MinimumSpeed=std::min(Current.MinimumSpeed,e.Speed);
        Current.PeakSpeed=std::max(Current.PeakSpeed,e.Speed);
        Current.LastSpeed=e.Speed;
    }

    void AddInterval(const SectorMotionFrame& a,const SectorMotionFrame& b,
        const SectorMotionEndpoint& first,const SectorMotionEndpoint& last) noexcept
    {
        const double dt=SectorMotionActiveClock(b)-SectorMotionActiveClock(a);
        ++Current.Intervals;
        Current.ObservedSeconds+=dt;
        Current.SpeedIntegral+=(first.Speed+last.Speed)*0.5*dt;
        Current.LateralIntegral+=(std::abs(first.LateralSpeed)+
            std::abs(last.LateralSpeed))*0.5*dt;
        Current.PlanarDistanceCm+=std::hypot(b.Position.X-a.Position.X,b.Position.Y-a.Position.Y);
        const double dv=last.Speed-first.Speed;
        if(dv>0.0) Current.AccelerationIntegral+=dv;
        else Current.DecelerationIntegral-=dv;
        if(first.LowSpeed && last.LowSpeed)
        {
            Current.LowSpeedSeconds+=dt;
            LowRun+=dt;
            Current.LongestLowSpeedSeconds=std::max(Current.LongestLowSpeedSeconds,LowRun);
        }
        else LowRun=0.0;
        if(first.HighSlip && last.HighSlip)
        {
            Current.HighSlipSeconds+=dt;
            SlipRun+=dt;
            Current.LongestSlipSeconds=std::max(Current.LongestSlipSeconds,SlipRun);
        }
        else SlipRun=0.0;
        if(first.ReverseMotion && last.ReverseMotion) Current.ReverseSeconds+=dt;
        if(first.PartlyOffProjectedRoad && last.PartlyOffProjectedRoad)
            Current.PartialRoadSeconds+=dt;
        if(first.FullyOffProjectedRoad && last.FullyOffProjectedRoad)
            Current.FullOffRoadSeconds+=dt;
    }

    bool Configured=false;
    int RequiredLaps=0;
    int Accepted=0;
    int Timed=0;
    int Qualified=0;
    int MissingTiming=0;
    std::uint64_t BoundActor=0;
    std::uint64_t LastAttemptedFrame=0;
    double BoundaryElapsed=0.0;
    double BoundaryPenalty=0.0;
    bool HavePrevious=false;
    bool BreakOpen=false;
    double LowRun=0.0;
    double SlipRun=0.0;
    SectorMotionFrame Previous{};
    SectorMotionEndpoint PreviousEndpoint{};
    SectorMotionMetrics Current{};
    SectorMotionRow Last{};
    SectorMotionComparison LastComparison{};
    std::array<SectorMotionGateSummary,CircuitGateCount> Gates{};
    std::array<SectorMotionRow,SectorMotionHistoryCapacity> History{};
    std::size_t Next=0;
    std::size_t Size=0;
};
} // namespace latebrake
