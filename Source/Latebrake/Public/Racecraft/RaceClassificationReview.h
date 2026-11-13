#pragma once
#include "Racecraft/VerifiedGateCrossing.h"
#include "Racecraft/RacecraftFrames.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
enum class ClassificationState : std::uint8_t
{
    Unqualified, Racing, ClassifiedFinish,
    ProvisionalFinish, Retired, Disqualified,
    DidNotStart, InvalidSession
};
struct ClassificationSettings final
{
    double MaximumTimestampSkewSeconds=0.12;
    double MinimumLegalFinishTimeSeconds=8.0;
    double MaximumOfficialTimeSeconds=86400.0;
    double MaximumReportedTrackGapCm=40000.0;
    double MaximumForwardProgressPerFrameCm=2200.0;
    double MaximumClassifiedSpeedCmS=MaximumPhysicalSpeedCmS;
    double MinimumGapBetweenFinishObservationsSeconds=0.5;
    double MaximumClockDifferenceSeconds=0.12;
    double MinimumTrackLengthCm=10000.0;
    int RequiredLaps=3;
    int RequiredGates=OriginalSegmentCount;
    int MinimumClassifiedStarters=2;
    int MaximumClassifiedStarters=PhysicalFleetCapacity;
    bool RequireExactOriginalCircuit=true;
    bool RequireAllSixPhysicalCars=false;
    bool NeverOverrideOfficialClassification=true;
    bool RequirePhysicalFinishGate=true;
    bool RequireConsistentSession=true;
};
struct ClassifiedActor final
{
    RaceIdentity Identity{};
    ClassificationState State=ClassificationState::Unqualified;
    int OfficialPosition=0;
    int CompletedLaps=0;
    int LastCreditedGate=0;
    int ValidatedGateCount=0;
    int FinishOrderOrdinal=0;
    double UnwrappedProgressCm=0.0;
    double CurrentLapProgressCm=0.0;
    double LastGateTimeSeconds=0.0;
    double OfficialFinishSeconds=0.0;
    double RaceClockSeconds=0.0;
    double MeasuredSpeedCmS=0.0;
    double LastPhysicalFrameSeconds=0.0;
    bool OriginalEnginePhysical=true;
    bool FinishingPlaneQualified=false;
    bool OriginalRaceProgressVerified=false;
    bool OfficialGameModeConfirmed=false;
    bool RetiredByEngine=false;
};
struct ClassificationReviewInput final
{
    std::array<ClassifiedActor,PhysicalFleetCapacity> Cars{};
    std::uint64_t Session=0;
    std::uint32_t Revision=0;
    double OfficialRaceClockSeconds=0.0;
    int Count=0;
    int OfficialRaceStarterCount=0;
    bool OriginalMapVerified=false;
    bool OriginalRaceGameMode=true;
    bool SessionFinished=false;
};
struct ClassificationEntry final
{
    int Slot=-1;
    int AnalyticalRank=0;
    int OfficialPosition=0;
    int CompletedLaps=0;
    int LastGate=0;
    double UnwrappedProgressCm=0.0;
    double TimeToLeaderAtCurrentSpeedSeconds=0.0;
    double DistanceBehindLeaderCm=0.0;
    double LastGateGapSeconds=0.0;
    bool ActuallyFinished=false;
    bool InOfficialResult=false;
    bool HasVerifiedPhysicalCar=false;
    bool ProvisionalOnly=true;
};
struct ClassificationReport final
{
    std::array<ClassificationEntry,PhysicalFleetCapacity> BySlot{};
    std::array<int,PhysicalFleetCapacity> SlotByAnalyticalRank{};
    int Count=0;
    int ClassifiedFinishers=0;
    int ProvisionalFinishers=0;
    int RetiredCars=0;
    int EnginesNotQualified=0;
    int OfficialPositionsVerified=0;
    int CorrectOfficialOrders=0;
    int OrderDiscrepancies=0;
    double LeadingProgressCm=0.0;
    double LastQualifiedProgressCm=0.0;
    double MaximumGapCm=0.0;
    double ClockSeconds=0.0;
    bool FinishedSession=false;
    bool AllOriginalActorsQualified=false;
    bool RaceAwardsChanged=false;
    bool OfficialClassificationModified=false;
    bool Valid=false;
};
inline bool ValidClassificationSettings(
    ClassificationSettings p) noexcept
{
    return Finite(p.MaximumTimestampSkewSeconds) &&
        p.MaximumTimestampSkewSeconds>=0.0 &&
        Finite(p.MinimumLegalFinishTimeSeconds) &&
        p.MinimumLegalFinishTimeSeconds>0.0 &&
        Finite(p.MaximumOfficialTimeSeconds) &&
        p.MaximumOfficialTimeSeconds>p.MinimumLegalFinishTimeSeconds &&
        Finite(p.MaximumReportedTrackGapCm) &&
        p.MaximumReportedTrackGapCm>0.0 &&
        Finite(p.MaximumForwardProgressPerFrameCm) &&
        p.MaximumForwardProgressPerFrameCm>0.0 &&
        Finite(p.MaximumClassifiedSpeedCmS) &&
        p.MaximumClassifiedSpeedCmS>0.0 &&
        Finite(p.MinimumTrackLengthCm) &&
        p.MinimumTrackLengthCm>0.0 &&
        p.RequiredLaps>=1 &&
        p.RequiredLaps<=500 &&
        p.RequiredGates==OriginalSegmentCount &&
        p.MinimumClassifiedStarters>=1 &&
        p.MaximumClassifiedStarters<=PhysicalFleetCapacity &&
        p.MaximumClassifiedStarters>=p.MinimumClassifiedStarters;
}
inline bool ValidClassifiedActor(
    ClassifiedActor c,ClassificationSettings p) noexcept
{
    return PhysicalActorIdentityValid(c.Identity) &&
        c.OriginalEnginePhysical &&
        c.OriginalRaceProgressVerified &&
        c.OfficialGameModeConfirmed &&
        c.OfficialPosition>=1 &&
        c.OfficialPosition<=PhysicalFleetCapacity &&
        c.CompletedLaps>=0 &&
        c.CompletedLaps<=p.RequiredLaps &&
        c.LastCreditedGate>=0 &&
        c.LastCreditedGate<p.RequiredGates &&
        c.ValidatedGateCount>=0 &&
        Finite(c.UnwrappedProgressCm) &&
        Finite(c.CurrentLapProgressCm) &&
        Finite(c.LastGateTimeSeconds) &&
        Finite(c.OfficialFinishSeconds) &&
        Finite(c.RaceClockSeconds) &&
        Finite(c.MeasuredSpeedCmS) &&
        Finite(c.LastPhysicalFrameSeconds) &&
        c.UnwrappedProgressCm>=0.0 &&
        c.RaceClockSeconds>=0.0 &&
        c.RaceClockSeconds<=p.MaximumOfficialTimeSeconds &&
        c.MeasuredSpeedCmS>=0.0 &&
        c.MeasuredSpeedCmS<=p.MaximumClassifiedSpeedCmS &&
        (!c.FinishingPlaneQualified ||
         c.CompletedLaps==p.RequiredLaps);
}
inline bool ValidClassificationInput(
    ClassificationReviewInput input,
    ClassificationSettings p) noexcept
{
    if (!input.OriginalMapVerified ||
        !input.OriginalRaceGameMode ||
        input.Session==0 ||
        input.Count<p.MinimumClassifiedStarters ||
        input.Count>p.MaximumClassifiedStarters ||
        input.OfficialRaceStarterCount!=input.Count ||
        !Finite(input.OfficialRaceClockSeconds) ||
        input.OfficialRaceClockSeconds<0.0)
        return false;
    for (int i=0;i<input.Count;++i)
    {
        const auto& a=input.Cars[
            static_cast<std::size_t>(i)];
        if (!ValidClassifiedActor(a,p) ||
            a.Identity.Session!=input.Session ||
            a.Identity.SessionRevision!=input.Revision ||
            a.Identity.Slot!=i ||
            std::abs(a.RaceClockSeconds-
                     input.OfficialRaceClockSeconds)>
                p.MaximumTimestampSkewSeconds)
            return false;
        for (int j=0;j<i;++j)
        {
            const auto& b=input.Cars[
                static_cast<std::size_t>(j)];
            if (a.Identity.Actor==b.Identity.Actor ||
                a.OfficialPosition==b.OfficialPosition)
                return false;
        }
    }
    return true;
}
inline bool ClassificationPrecedes(
    ClassifiedActor a,ClassifiedActor b) noexcept
{
    const bool af=a.FinishingPlaneQualified;
    const bool bf=b.FinishingPlaneQualified;
    if (af!=bf)return af;
    if (af &&bf &&
        a.OfficialFinishSeconds!=b.OfficialFinishSeconds)
        return a.OfficialFinishSeconds<
            b.OfficialFinishSeconds;
    if (a.CompletedLaps!=b.CompletedLaps)
        return a.CompletedLaps>b.CompletedLaps;
    if (a.ValidatedGateCount!=b.ValidatedGateCount)
        return a.ValidatedGateCount>b.ValidatedGateCount;
    if (a.UnwrappedProgressCm!=b.UnwrappedProgressCm)
        return a.UnwrappedProgressCm>b.UnwrappedProgressCm;
    return a.Identity.Slot<b.Identity.Slot;
}
inline ClassificationReport EvaluateRaceClassification(
    ClassificationReviewInput input,
    ClassificationSettings settings={}) noexcept
{
    ClassificationReport result;
    if (!ValidClassificationSettings(settings) ||
        !ValidClassificationInput(input,settings))
        return result;
    result.Count=input.Count;
    result.ClockSeconds=input.OfficialRaceClockSeconds;
    result.FinishedSession=input.SessionFinished;
    result.AllOriginalActorsQualified=true;
    std::array<int,PhysicalFleetCapacity> indices{};
    for (int i=0;i<input.Count;++i)indices[
        static_cast<std::size_t>(i)]=i;
    for (int i=1;i<input.Count;++i)
    {
        const int element=indices[static_cast<std::size_t>(i)];
        int j=i;
        while(j>0 &&
              ClassificationPrecedes(
                input.Cars[static_cast<std::size_t>(element)],
                input.Cars[static_cast<std::size_t>(
                    indices[static_cast<std::size_t>(j-1)])]))
        {
            indices[static_cast<std::size_t>(j)]=
                indices[static_cast<std::size_t>(j-1)];
            --j;
        }
        indices[static_cast<std::size_t>(j)]=element;
    }
    result.SlotByAnalyticalRank=indices;
    const auto& leader=input.Cars[
        static_cast<std::size_t>(indices[0])];
    result.LeadingProgressCm=leader.UnwrappedProgressCm;
    result.LastQualifiedProgressCm=
        input.Cars[static_cast<std::size_t>(
            indices[static_cast<std::size_t>(input.Count-1)])].
            UnwrappedProgressCm;
    for (int rank=0;rank<input.Count;++rank)
    {
        const int slot=indices[static_cast<std::size_t>(rank)];
        const auto& car=input.Cars[
            static_cast<std::size_t>(slot)];
        ClassificationEntry entry;
        entry.Slot=slot;
        entry.AnalyticalRank=rank+1;
        entry.OfficialPosition=car.OfficialPosition;
        entry.CompletedLaps=car.CompletedLaps;
        entry.LastGate=car.LastCreditedGate;
        entry.UnwrappedProgressCm=car.UnwrappedProgressCm;
        entry.DistanceBehindLeaderCm=std::max(
            0.0,result.LeadingProgressCm-
                car.UnwrappedProgressCm);
        entry.TimeToLeaderAtCurrentSpeedSeconds=
            SafeDivision(entry.DistanceBehindLeaderCm,
                         std::max(1.0,car.MeasuredSpeedCmS));
        entry.LastGateGapSeconds=car.LastGateTimeSeconds-
            leader.LastGateTimeSeconds;
        entry.ActuallyFinished=
            car.FinishingPlaneQualified;
        entry.InOfficialResult=input.SessionFinished &&
            car.FinishingPlaneQualified;
        entry.HasVerifiedPhysicalCar=true;
        entry.ProvisionalOnly=true;
        result.BySlot[static_cast<std::size_t>(slot)]=entry;
        result.MaximumGapCm=std::max(
            result.MaximumGapCm,entry.DistanceBehindLeaderCm);
        result.ClassifiedFinishers+=
            entry.ActuallyFinished?1:0;
        result.ProvisionalFinishers+=
            entry.ActuallyFinished?0:1;
        result.RetiredCars+=car.RetiredByEngine?1:0;
        ++result.OfficialPositionsVerified;
        if (entry.AnalyticalRank==entry.OfficialPosition)
            ++result.CorrectOfficialOrders;
        else ++result.OrderDiscrepancies;
    }
    result.RaceAwardsChanged=false;
    result.OfficialClassificationModified=false;
    result.Valid=true;
    return result;
}
struct ClassificationHistory final
{
    std::uint64_t AcceptedFrames=0;
    std::uint64_t RejectedFrames=0;
    std::uint64_t AnalyticalLeadChanges=0;
    std::uint64_t OfficialOrderDiscrepancyFrames=0;
    std::array<int,PhysicalFleetCapacity> PreviousRanks{};
    int LastLeadingSlot=-1;
    double LongestObservedLeaderGapCm=0.0;
    double LastClockSeconds=0.0;
    bool ModifiedRaceResult=false;
};
class RaceClassificationMonitor final
{
public:
    bool Configure(ClassificationSettings settings) noexcept
    {
        if (!ValidClassificationSettings(settings))
            return false;
        Settings=settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        History={};
        Last={};
        Session=0;
        Revision=0;
        HasPrevious=false;
    }
    ClassificationReport Observe(
        ClassificationReviewInput input) noexcept
    {
        const auto review=EvaluateRaceClassification(
            input,Settings);
        if (!review.Valid)
        {
            ++History.RejectedFrames;
            return {};
        }
        if (HasPrevious &&
            (input.Session!=Session ||
             input.Revision!=Revision ||
             input.OfficialRaceClockSeconds<
                History.LastClockSeconds))
        {
            ++History.RejectedFrames;
            HasPrevious=false;
            return {};
        }
        Session=input.Session;
        Revision=input.Revision;
        const int leader=review.SlotByAnalyticalRank[0];
        if (HasPrevious &&leader!=History.LastLeadingSlot)
            ++History.AnalyticalLeadChanges;
        History.LastLeadingSlot=leader;
        History.LastClockSeconds=
            input.OfficialRaceClockSeconds;
        History.LongestObservedLeaderGapCm=std::max(
            History.LongestObservedLeaderGapCm,
            review.MaximumGapCm);
        if (review.OrderDiscrepancies>0)
            ++History.OfficialOrderDiscrepancyFrames;
        for (int i=0;i<review.Count;++i)
        {
            const int slot=review.SlotByAnalyticalRank[
                static_cast<std::size_t>(i)];
            History.PreviousRanks[
                static_cast<std::size_t>(slot)]=i+1;
        }
        History.ModifiedRaceResult=false;
        HasPrevious=true;
        Last=review;
        ++History.AcceptedFrames;
        return Last;
    }
    ClassificationReport Snapshot() const noexcept{return Last;}
    ClassificationHistory Statistics() const noexcept{return History;}
private:
    ClassificationSettings Settings{};
    ClassificationHistory History{};
    ClassificationReport Last{};
    std::uint64_t Session=0;
    std::uint32_t Revision=0;
    bool HasPrevious=false;
};
} // namespace latebrake::racecraft
