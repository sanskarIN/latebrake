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
inline constexpr int PhysicalSectorCount=4;
inline constexpr int TimingCheckpointCapacity=16;
enum class SectorTimingFault : std::uint8_t
{
    None, MissingEngineGate, WrongSession, ReplayedFrame,
    OutOfOrderGate, ImpossibleSectorSpeed, CrossedWrongWay,
    MissedSector, InvalidTime, IncompleteLap,
    IllegalLapCredit, StaleClock, UnverifiedOriginalRoad
};
struct SectorTimingParameters final
{
    double CircuitLengthCm=250000.0;
    double MaximumPhysicalSpeedCmS=18000.0;
    double MinimumMovingSpeedCmS=50.0;
    double MinimumGateSpacingCm=1200.0;
    double MaximumGateSpacingCm=80000.0;
    double MaximumGateContactRadiusCm=550.0;
    double MaximumFrameSeconds=0.10;
    double MaximumClockDriftSeconds=0.04;
    double MaximumRaceDurationSeconds=86400.0;
    double MinimumSectorDurationSeconds=0.50;
    double MaximumSectorDurationSeconds=1200.0;
    double MinimumLapDurationSeconds=10.0;
    double MaximumLapDurationSeconds=3600.0;
    double MaximumClockJumpSeconds=0.25;
    double MaximumDistanceDiscontinuityCm=10000.0;
    double MinimumForwardGateDot=0.15;
    double MaximumHeadingErrorRadians=1.3;
    double MinimumSourceConfidence=0.80;
    double MaximumLapDistanceToleranceCm=600.0;
    double MaximumGateSequenceGap=1.0;
    double MaximumAcceptedLapCount=500.0;
    double MinimumGatePlaneMotionCm=0.1;
    double TimingResolutionSeconds=0.001;
    double MaximumUnwrappedPositionCm=1e9;
    double MaximumSectorTimeImprovementFraction=0.95;
    double PenaltyDisqualificationSeconds=300.0;
    bool RequireOriginalCollisionGate=true;
    bool RequireActualUnrealClock=true;
    bool NeverAwardOfficialLap=true;
    bool RejectTeleportRecovery=true;
};
struct SectorTimingGate final
{
    std::uint64_t OriginalGateComponent=0;
    int GateOrdinal=-1;
    int SectorIndex=-1;
    Vec2 OriginalGateCenterCm{};
    Vec2 GateForwardNormal{};
    double DistanceAlongLapCm=0.0;
    double HalfWidthCm=400.0;
    double TriggerRadiusCm=450.0;
    double EngineClockSeconds=0.0;
    bool CollisionBearingGate=true;
    bool OriginalRaceMap=true;
    bool PhysicalTriggerVerified=true;
};
struct SectorTimingCrossing final
{
    RaceIdentity Identity{};
    int GateOrdinal=-1;
    Vec2 PreviousPositionCm{};
    Vec2 CurrentPositionCm{};
    Vec2 MeasuredVelocityCmS{};
    double PreviousRaceClockSeconds=0.0;
    double RaceClockSeconds=0.0;
    double DeltaSeconds=0.0;
    double DistanceAlongLapCm=0.0;
    double UnwrappedDistanceCm=0.0;
    double ChassisHeadingRadians=0.0;
    double MeasuredGateConfidence=1.0;
    bool EngineRaceGreen=false;
    bool ChaosBodyQualified=false;
    bool OriginalMapConfirmed=false;
    bool CollisionTriggerQualified=false;
    bool RecoveryTeleportActive=false;
};
struct SectorSplitRecord final
{
    int SectorIndex=-1;
    int GateOrdinal=-1;
    double SectorStartClockSeconds=0.0;
    double SectorEndClockSeconds=0.0;
    double SectorDurationSeconds=0.0;
    double CumulativeLapDurationSeconds=0.0;
    double DistanceCompletedCm=0.0;
    double MeanSectorSpeedCmS=0.0;
    double MinimumPossibleDurationSeconds=0.0;
    double TimeSincePreviousCrossingSeconds=0.0;
    double MinimumSourceConfidence=1.0;
    bool OriginalGateCrossedForward=false;
    bool PhysicallyConsistent=true;
    bool PersonalBestCandidate=false;
    bool OfficialLapTimeAwarded=false;
    bool Valid=false;
};
struct LapTimingEvidence final
{
    RaceIdentity Identity{};
    std::array<SectorSplitRecord,PhysicalSectorCount> Sectors{};
    double LapStartClockSeconds=0.0;
    double LapFinishClockSeconds=0.0;
    double ProposedLapTimeSeconds=0.0;
    double FastestMeasuredSectorSeconds=0.0;
    double SlowestMeasuredSectorSeconds=0.0;
    double TotalVerifiedDistanceCm=0.0;
    double MinimumSectorConfidence=0.0;
    int CompletedSectorCount=0;
    int LapOrdinal=0;
    bool AllPhysicalGatesVisited=false;
    bool ActualEngineLapCreditIssued=false;
    bool CandidateForOfficialReview=false;
    bool Valid=false;
};
inline bool ValidSectorTimingParameters(
    SectorTimingParameters p) noexcept
{
    return Finite(p.CircuitLengthCm) &&
        p.CircuitLengthCm>5000.0 &&
        Finite(p.MaximumPhysicalSpeedCmS) &&
        p.MaximumPhysicalSpeedCmS>0.0 &&
        p.MaximumPhysicalSpeedCmS<=MaximumPhysicalSpeedCmS &&
        Finite(p.MinimumMovingSpeedCmS) &&
        p.MinimumMovingSpeedCmS>=0.0 &&
        Finite(p.MinimumGateSpacingCm) &&
        p.MinimumGateSpacingCm>0.0 &&
        Finite(p.MaximumGateSpacingCm) &&
        p.MaximumGateSpacingCm>p.MinimumGateSpacingCm &&
        Finite(p.MaximumGateContactRadiusCm) &&
        p.MaximumGateContactRadiusCm>0.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MaximumClockDriftSeconds) &&
        p.MaximumClockDriftSeconds>=0.0 &&
        Finite(p.MaximumRaceDurationSeconds) &&
        p.MaximumRaceDurationSeconds>0.0 &&
        Finite(p.MinimumSectorDurationSeconds) &&
        p.MinimumSectorDurationSeconds>0.0 &&
        Finite(p.MaximumSectorDurationSeconds) &&
        p.MaximumSectorDurationSeconds>
            p.MinimumSectorDurationSeconds &&
        Finite(p.MinimumLapDurationSeconds) &&
        p.MinimumLapDurationSeconds>0.0 &&
        Finite(p.MaximumLapDurationSeconds) &&
        p.MaximumLapDurationSeconds>p.MinimumLapDurationSeconds &&
        Finite(p.MaximumClockJumpSeconds) &&
        p.MaximumClockJumpSeconds>0.0 &&
        Finite(p.MaximumDistanceDiscontinuityCm) &&
        p.MaximumDistanceDiscontinuityCm>0.0 &&
        Finite(p.MinimumForwardGateDot) &&
        p.MinimumForwardGateDot>0.0 &&
        p.MinimumForwardGateDot<=1.0 &&
        Finite(p.MaximumHeadingErrorRadians) &&
        p.MaximumHeadingErrorRadians>0.0 &&
        Finite(p.MinimumSourceConfidence) &&
        p.MinimumSourceConfidence>0.0 &&
        p.MinimumSourceConfidence<=1.0 &&
        Finite(p.MaximumLapDistanceToleranceCm) &&
        p.MaximumLapDistanceToleranceCm>0.0 &&
        Finite(p.MaximumGateSequenceGap) &&
        p.MaximumGateSequenceGap>=1.0 &&
        Finite(p.MaximumAcceptedLapCount) &&
        p.MaximumAcceptedLapCount>=1.0 &&
        Finite(p.MinimumGatePlaneMotionCm) &&
        p.MinimumGatePlaneMotionCm>0.0 &&
        Finite(p.TimingResolutionSeconds) &&
        p.TimingResolutionSeconds>0.0 &&
        Finite(p.MaximumUnwrappedPositionCm) &&
        p.MaximumUnwrappedPositionCm>0.0 &&
        Finite(p.MaximumSectorTimeImprovementFraction) &&
        p.MaximumSectorTimeImprovementFraction>=0.0 &&
        p.MaximumSectorTimeImprovementFraction<1.0 &&
        Finite(p.PenaltyDisqualificationSeconds) &&
        p.PenaltyDisqualificationSeconds>0.0;
}
inline bool ValidSectorTimingGate(
    SectorTimingGate g,SectorTimingParameters p) noexcept
{
    return g.OriginalGateComponent!=0 &&
        g.GateOrdinal>=0 &&
        g.GateOrdinal<TimingCheckpointCapacity &&
        g.SectorIndex>=0 &&
        g.SectorIndex<PhysicalSectorCount &&
        Finite(g.OriginalGateCenterCm) &&
        Finite(g.GateForwardNormal) &&
        std::abs(Length(g.GateForwardNormal)-1.0)<0.01 &&
        Finite(g.DistanceAlongLapCm) &&
        g.DistanceAlongLapCm>=0.0 &&
        g.DistanceAlongLapCm<p.CircuitLengthCm &&
        Finite(g.HalfWidthCm) &&
        g.HalfWidthCm>0.0 &&
        Finite(g.TriggerRadiusCm) &&
        g.TriggerRadiusCm>0.0 &&
        g.TriggerRadiusCm<=p.MaximumGateContactRadiusCm &&
        Finite(g.EngineClockSeconds) &&
        g.EngineClockSeconds>=0.0 &&
        (!p.RequireOriginalCollisionGate ||
         (g.CollisionBearingGate &&
          g.OriginalRaceMap &&
          g.PhysicalTriggerVerified));
}
inline bool ValidSectorTimingCrossing(
    SectorTimingCrossing c,
    const SectorTimingGate& g,
    SectorTimingParameters p) noexcept
{
    return ValidSectorTimingParameters(p) &&
        ValidSectorTimingGate(g,p) &&
        PhysicalActorIdentityValid(c.Identity) &&
        c.Identity.Frame>0 &&
        c.GateOrdinal==g.GateOrdinal &&
        c.EngineRaceGreen &&
        c.ChaosBodyQualified &&
        c.OriginalMapConfirmed &&
        c.CollisionTriggerQualified &&
        (!p.RejectTeleportRecovery ||
         !c.RecoveryTeleportActive) &&
        Finite(c.PreviousPositionCm) &&
        Finite(c.CurrentPositionCm) &&
        Finite(c.MeasuredVelocityCmS) &&
        Length(c.MeasuredVelocityCmS)<=p.MaximumPhysicalSpeedCmS &&
        Finite(c.PreviousRaceClockSeconds) &&
        c.PreviousRaceClockSeconds>=0.0 &&
        Finite(c.RaceClockSeconds) &&
        c.RaceClockSeconds>c.PreviousRaceClockSeconds &&
        c.RaceClockSeconds<=p.MaximumRaceDurationSeconds &&
        Finite(c.DeltaSeconds) &&
        c.DeltaSeconds>0.0 &&
        c.DeltaSeconds<=p.MaximumFrameSeconds &&
        std::abs(c.RaceClockSeconds-
                 c.PreviousRaceClockSeconds-
                 c.DeltaSeconds)<=p.MaximumClockJumpSeconds &&
        // Gate metadata is captured at course configuration, not
        // regenerated at each live physics-frame trigger. Reject only
        // gate metadata which claims to be newer than this crossing.
        g.EngineClockSeconds<=
            c.RaceClockSeconds+p.MaximumClockDriftSeconds &&
        Finite(c.DistanceAlongLapCm) &&
        c.DistanceAlongLapCm>=0.0 &&
        c.DistanceAlongLapCm<p.CircuitLengthCm &&
        std::abs(c.DistanceAlongLapCm-g.DistanceAlongLapCm)<=
            p.MaximumGateContactRadiusCm &&
        Finite(c.UnwrappedDistanceCm) &&
        c.UnwrappedDistanceCm>=0.0 &&
        c.UnwrappedDistanceCm<=p.MaximumUnwrappedPositionCm &&
        // A physically observed gate must also match the measured
        // progress on the original circuit, including lap wraparound.
        std::min(
            std::abs(std::fmod(c.UnwrappedDistanceCm,
                p.CircuitLengthCm)-g.DistanceAlongLapCm),
            p.CircuitLengthCm-std::abs(
                std::fmod(c.UnwrappedDistanceCm,
                    p.CircuitLengthCm)-g.DistanceAlongLapCm))<=
            p.MaximumLapDistanceToleranceCm &&
        Finite(c.ChassisHeadingRadians) &&
        Finite(c.MeasuredGateConfidence) &&
        c.MeasuredGateConfidence>=p.MinimumSourceConfidence &&
        c.MeasuredGateConfidence<=1.0;
}
inline double GateSignedDistanceCm(
    Vec2 point,SectorTimingGate gate) noexcept
{
    return Dot(point-gate.OriginalGateCenterCm,
               gate.GateForwardNormal);
}
inline bool PhysicalForwardGateCrossing(
    SectorTimingCrossing c,SectorTimingGate gate,
    SectorTimingParameters p) noexcept
{
    if (!ValidSectorTimingCrossing(c,gate,p))
        return false;
    const double before=GateSignedDistanceCm(
        c.PreviousPositionCm,gate);
    const double after=GateSignedDistanceCm(
        c.CurrentPositionCm,gate);
    const double travel=after-before;
    if (before>0.0 || after<0.0 ||
        travel<p.MinimumGatePlaneMotionCm)
        return false;
    const Vec2 lateral={
        -gate.GateForwardNormal.Y,
        gate.GateForwardNormal.X};
    const double offset=std::abs(Dot(
        c.CurrentPositionCm-
            gate.OriginalGateCenterCm,lateral));
    const double forwardDot=Dot(
        Normalized(c.MeasuredVelocityCmS),
        gate.GateForwardNormal);
    return offset<=gate.HalfWidthCm &&
        std::abs(after)<=gate.TriggerRadiusCm &&
        forwardDot>=p.MinimumForwardGateDot &&
        std::abs(WrappedAngle(
            c.ChassisHeadingRadians-
            AngleOf(gate.GateForwardNormal)))<=
                p.MaximumHeadingErrorRadians;
}
struct SectorTimingCourse final
{
    std::array<SectorTimingGate,TimingCheckpointCapacity> Gates{};
    int GateCount=0;
    int SectorCount=PhysicalSectorCount;
    double CircuitLengthCm=0.0;
    bool OriginalRaceMapVerified=false;
    bool RealGateComponentsConfirmed=false;
};
inline bool ValidSectorTimingCourse(
    const SectorTimingCourse& course,
    SectorTimingParameters p) noexcept
{
    if (!ValidSectorTimingParameters(p) ||
        !course.OriginalRaceMapVerified ||
        !course.RealGateComponentsConfirmed ||
        course.GateCount<PhysicalSectorCount ||
        course.GateCount>TimingCheckpointCapacity ||
        course.SectorCount!=PhysicalSectorCount ||
        !Finite(course.CircuitLengthCm) ||
        std::abs(course.CircuitLengthCm-
                 p.CircuitLengthCm)>1.0)
        return false;
    double previous=-p.MinimumGateSpacingCm;
    std::array<bool,PhysicalSectorCount> sectors{};
    int lastSector=-1;
    for(int i=0;i<course.GateCount;++i)
    {
        const auto& gate=course.Gates[
            static_cast<std::size_t>(i)];
        if (!ValidSectorTimingGate(gate,p) ||
            gate.GateOrdinal!=i ||
            gate.DistanceAlongLapCm-previous<
                p.MinimumGateSpacingCm ||
            (i>0 && gate.DistanceAlongLapCm-previous>
                p.MaximumGateSpacingCm) ||
            gate.SectorIndex<lastSector)
            return false;
        sectors[static_cast<std::size_t>(
            gate.SectorIndex)]=true;
        lastSector=gate.SectorIndex;
        for(int earlier=0;earlier<i;++earlier)
            if(course.Gates[
                static_cast<std::size_t>(earlier)].
                    OriginalGateComponent==
                    gate.OriginalGateComponent)
                return false;
        previous=gate.DistanceAlongLapCm;
    }
    // The last gate must have a real remaining road segment to the
    // start/finish line. A last intermediate sector is not a finish.
    const double wrapGap=p.CircuitLengthCm-
        previous+course.Gates[0].DistanceAlongLapCm;
    return course.Gates[0].SectorIndex==0 &&
        lastSector==PhysicalSectorCount-1 &&
        wrapGap>=p.MinimumGateSpacingCm &&
        wrapGap<=p.MaximumGateSpacingCm &&
        std::all_of(sectors.begin(),sectors.end(),
            [](bool present){return present;});
}

class PhysicalSectorTimingSupervisor final
{
public:
    bool Configure(
        const SectorTimingCourse& course,
        SectorTimingParameters p) noexcept
    {
        if (!ValidSectorTimingCourse(course,p))
            return false;
        Settings=p;
        Course=course;
        Reset();
        Configured=true;
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Source={};
        LapRecords={};
        GateIndex=0;
        LapOrdinal=0;
        LastGateClock=0.0;
        LastGateConfidence=1.0;
        LapStartClock=0.0;
        LapStartDistanceCm=0.0;
        LastUnwrappedDistanceCm=0.0;
        AcceptedGateCrossings=0;
        RejectedGateCrossings=0;
        CompletedCandidateLaps=0;
        HasSource=false;
    }
    SectorTimingFault Submit(
        const SectorTimingCrossing& sample) noexcept
    {
        if (!Configured)
        {
            ++RejectedGateCrossings;
            return SectorTimingFault::MissingEngineGate;
        }
        const auto& gate=Course.Gates[
            static_cast<std::size_t>(GateIndex)];
        if(!ValidSectorTimingCrossing(sample,gate,Settings))
        {
            ++RejectedGateCrossings;
            return SectorTimingFault::UnverifiedOriginalRoad;
        }
        const bool sameEpoch=HasSource &&
            Source.Session==sample.Identity.Session &&
            Source.SessionRevision==
                sample.Identity.SessionRevision &&
            Source.Actor==sample.Identity.Actor;
        if (sameEpoch && sample.Identity.Frame<=Source.Frame)
        {
            ++RejectedGateCrossings;
            return SectorTimingFault::ReplayedFrame;
        }
        if (HasSource && !sameEpoch)
        {
            ++RejectedGateCrossings;
            return SectorTimingFault::WrongSession;
        }
        if (!PhysicalForwardGateCrossing(sample,gate,Settings))
        {
            ++RejectedGateCrossings;
            return SectorTimingFault::CrossedWrongWay;
        }
        const double elapsed=HasSource ?
            sample.RaceClockSeconds-LastGateClock:0.0;
        const double progress=HasSource ?
            sample.UnwrappedDistanceCm-
                LastUnwrappedDistanceCm:0.0;
        if(HasSource &&
           (progress<0.0 ||
            elapsed<Settings.MinimumSectorDurationSeconds ||
            elapsed>Settings.MaximumSectorDurationSeconds ||
            progress>Settings.MaximumPhysicalSpeedCmS*elapsed+
                Settings.MaximumLapDistanceToleranceCm))
        {
            ++RejectedGateCrossings;
            return SectorTimingFault::ImpossibleSectorSpeed;
        }
        const bool finishingLap=HasSource && GateIndex==0;
        // First pass through gate zero starts evidence collection. Every
        // subsequent crossing closes the distance after the PREVIOUS gate.
        // This includes the final sector, closed only at gate zero again.
        if (!HasSource)
        {
            LapStartClock=sample.RaceClockSeconds;
            LapStartDistanceCm=sample.UnwrappedDistanceCm;
            LapRecords={};
            ++LapOrdinal;
        }
        else
        {
            const int preceding=GateIndex==0 ?
                Course.GateCount-1:GateIndex-1;
            const int sector=Course.Gates[
                static_cast<std::size_t>(preceding)].SectorIndex;
            SectorSplitRecord split=LapRecords[
                static_cast<std::size_t>(sector)];
            if (!split.Valid)
            {
                split.SectorIndex=sector;
                split.SectorStartClockSeconds=LastGateClock;
                split.MinimumSourceConfidence=
                    LastGateConfidence;
            }
            split.GateOrdinal=GateIndex;
            split.SectorEndClockSeconds=sample.RaceClockSeconds;
            split.SectorDurationSeconds+=elapsed;
            split.DistanceCompletedCm+=progress;
            split.CumulativeLapDurationSeconds=
                sample.RaceClockSeconds-LapStartClock;
            split.MeanSectorSpeedCmS=SafeDivision(
                split.DistanceCompletedCm,
                split.SectorDurationSeconds);
            split.MinimumPossibleDurationSeconds=SafeDivision(
                split.DistanceCompletedCm,
                Settings.MaximumPhysicalSpeedCmS);
            split.TimeSincePreviousCrossingSeconds=elapsed;
            split.MinimumSourceConfidence=std::min(
                split.MinimumSourceConfidence,
                sample.MeasuredGateConfidence);
            split.OriginalGateCrossedForward=true;
            split.PhysicallyConsistent=
                split.SectorDurationSeconds>=
                    Settings.MinimumSectorDurationSeconds &&
                split.SectorDurationSeconds<=
                    Settings.MaximumSectorDurationSeconds &&
                split.MeanSectorSpeedCmS>=0.0 &&
                split.MeanSectorSpeedCmS<=
                    Settings.MaximumPhysicalSpeedCmS &&
                split.DistanceCompletedCm>=0.0;
            split.Valid=split.PhysicallyConsistent &&
                Finite(split.MeanSectorSpeedCmS);
            if (!split.Valid)
            {
                ++RejectedGateCrossings;
                return SectorTimingFault::ImpossibleSectorSpeed;
            }
            LapRecords[static_cast<std::size_t>(sector)]=split;
        }
        Source=sample.Identity;
        HasSource=true;
        LastGateClock=sample.RaceClockSeconds;
        LastGateConfidence=sample.MeasuredGateConfidence;
        LastUnwrappedDistanceCm=sample.UnwrappedDistanceCm;
        ++AcceptedGateCrossings;
        if (finishingLap)
        {
            BuildCandidateLap(sample);
            // The finish crossing is also the next lap's start. Preserve
            // source continuity without copying previous sector evidence.
            LapStartClock=sample.RaceClockSeconds;
            LapStartDistanceCm=sample.UnwrappedDistanceCm;
            LapRecords={};
            ++LapOrdinal;
        }
        GateIndex=(GateIndex+1)%Course.GateCount;
        return SectorTimingFault::None;
    }
    void BuildCandidateLap(
        const SectorTimingCrossing& finish) noexcept
    {
        LapTimingEvidence result;
        result.Identity=finish.Identity;
        result.LapOrdinal=LapOrdinal;
        result.Sectors=LapRecords;
        result.LapStartClockSeconds=LapStartClock;
        result.LapFinishClockSeconds=finish.RaceClockSeconds;
        result.ProposedLapTimeSeconds=
            result.LapFinishClockSeconds-
            result.LapStartClockSeconds;
        result.MinimumSectorConfidence=1.0;
        result.FastestMeasuredSectorSeconds=
            std::numeric_limits<double>::infinity();
        for(const auto& split:result.Sectors)
        {
            if(!split.Valid)continue;
            ++result.CompletedSectorCount;
            result.TotalVerifiedDistanceCm+=
                split.DistanceCompletedCm;
            result.MinimumSectorConfidence=std::min(
                result.MinimumSectorConfidence,
                split.MinimumSourceConfidence);
            result.FastestMeasuredSectorSeconds=std::min(
                result.FastestMeasuredSectorSeconds,
                split.SectorDurationSeconds);
            result.SlowestMeasuredSectorSeconds=std::max(
                result.SlowestMeasuredSectorSeconds,
                split.SectorDurationSeconds);
        }
        const double unwrappedLapDistance=
            finish.UnwrappedDistanceCm-
            LapStartDistanceCm;
        result.AllPhysicalGatesVisited=
            AcceptedGateCrossings>=
                static_cast<std::uint64_t>(Course.GateCount+1) &&
            result.CompletedSectorCount==PhysicalSectorCount;
        const bool completeDistance=
            std::abs(unwrappedLapDistance-
                     Settings.CircuitLengthCm)<=
                Settings.MaximumLapDistanceToleranceCm &&
            std::abs(result.TotalVerifiedDistanceCm-
                     Settings.CircuitLengthCm)<=
                Settings.MaximumLapDistanceToleranceCm;
        result.ActualEngineLapCreditIssued=false;
        result.CandidateForOfficialReview=
            result.AllPhysicalGatesVisited &&
            completeDistance &&
            result.MinimumSectorConfidence>=
                Settings.MinimumSourceConfidence &&
            result.ProposedLapTimeSeconds>=
                Settings.MinimumLapDurationSeconds &&
            result.ProposedLapTimeSeconds<=
                Settings.MaximumLapDurationSeconds;
        result.Valid=result.CandidateForOfficialReview;
        Last=result;
        if(result.Valid)++CompletedCandidateLaps;
    }
    LapTimingEvidence Snapshot() const noexcept{return Last;}
    std::uint64_t AcceptedGates() const noexcept
    {return AcceptedGateCrossings;}
    std::uint64_t RejectedGates() const noexcept
    {return RejectedGateCrossings;}
    std::uint64_t CandidateLaps() const noexcept
    {return CompletedCandidateLaps;}
    int NextGateOrdinal() const noexcept{return GateIndex;}
private:
    SectorTimingCourse Course{};
    SectorTimingParameters Settings{};
    LapTimingEvidence Last{};
    std::array<SectorSplitRecord,PhysicalSectorCount> LapRecords{};
    RaceIdentity Source{};
    int GateIndex=0;
    int LapOrdinal=0;
    double LastGateClock=0.0;
    double LastGateConfidence=1.0;
    double LapStartClock=0.0;
    double LapStartDistanceCm=0.0;
    double LastUnwrappedDistanceCm=0.0;
    std::uint64_t AcceptedGateCrossings=0;
    std::uint64_t RejectedGateCrossings=0;
    std::uint64_t CompletedCandidateLaps=0;
    bool HasSource=false;
    bool Configured=false;
};
} // namespace latebrake::racecraft
