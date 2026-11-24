#pragma once

#include "Telemetry/TelemetryDeltaComparison.h"
#include "Telemetry/TelemetryVehicleHealth.h"
#include "Telemetry/TelemetryFuelStrategy.h"
#include "Telemetry/TelemetryOpponents.h"
#include "Telemetry/TelemetryBraking.h"
#include "Telemetry/TelemetryCornering.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class RaceEngineerCoachingTopic : std::uint8_t
{
    None,
    BrakeEarlier,
    BrakeLaterCarefully,
    ReduceWheelLock,
    SmoothBrakeRelease,
    ImproveCornerEntry,
    ImproveApexConsistency,
    StabilizeCornerExit,
    ReduceExcessiveSlip,
    ReduceUndersteer,
    ReduceOversteer,
    ReduceTrackCutting,
    StayClearOfOpponent,
    ManageTireHeat,
    ManageBrakeHeat,
    MonitorEngineHeat,
    ConserveFuel,
    ReviewPitPlan,
    RecheckSensorData,
    MaintainSaferFollowing,
    ReduceThrottleBrakeOverlap,
    PreserveRaceFocus,
    CompareSector,
    VerifyGroundContact
};

enum class RaceEngineerCoachingPriority : std::uint8_t
{
    None,
    Opportunity,
    Performance,
    Consistency,
    Mechanical,
    Safety,
    Critical
};

enum class CoachingPresentation : std::uint8_t
{
    Silenced,
    AfterSession,
    AfterLap,
    NextStraight,
    AdvisoryNow
};

inline constexpr int CoachingTopicCount = 23;
inline constexpr int MaximumCoachTips = 16;

struct RaceEngineerCoachingSettings final
{
    double MinimumEvidenceConfidence = 0.70;
    double MinimumHintIntervalSeconds = 5.0;
    double SafetyHintCooldownSeconds = 2.0;
    double PerformanceHintCooldownSeconds = 12.0;
    double MaximumHintLifetimeSeconds = 20.0;
    double MaximumCornerSlipRadians = 0.24;
    double MaximumBrakeLockSeconds = 0.20;
    double MaximumOffTrackSeconds = 0.10;
    double MinimumOpportunityTimeGainSeconds = 0.08;
    double CriticalTireHeatC = 145.0;
    double CriticalBrakeHeatC = 790.0;
    double CriticalEngineHeatC = 132.0;
    double MinimumSafeOpponentDistanceCm = 500.0;
    double MinimumFuelReserveFraction = 0.06;
    double MinimumLapSectorComparisonSeconds = 0.03;
    double MaximumAcceptableLineErrorCm = 420.0;
    int MaximumActiveTips = MaximumCoachTips;
    bool MutePerformanceUnderCaution = true;
    bool MutePerformanceDuringContactRisk = true;
    bool RequireNativeEvidence = true;
};

inline bool ValidCoachingSettings(
    const RaceEngineerCoachingSettings& s) noexcept
{
    return Finite(s.MinimumEvidenceConfidence) &&
           s.MinimumEvidenceConfidence >= 0.0 &&
           s.MinimumEvidenceConfidence <= 1.0 &&
           Finite(s.MinimumHintIntervalSeconds) &&
           s.MinimumHintIntervalSeconds >= 0.0 &&
           Finite(s.SafetyHintCooldownSeconds) &&
           s.SafetyHintCooldownSeconds >= 0.0 &&
           Finite(s.PerformanceHintCooldownSeconds) &&
           s.PerformanceHintCooldownSeconds >= 0.0 &&
           Finite(s.MaximumHintLifetimeSeconds) &&
           s.MaximumHintLifetimeSeconds > 0.0 &&
           Finite(s.MaximumCornerSlipRadians) &&
           s.MaximumCornerSlipRadians > 0.0 &&
           Finite(s.MaximumBrakeLockSeconds) &&
           s.MaximumBrakeLockSeconds > 0.0 &&
           Finite(s.MaximumOffTrackSeconds) &&
           s.MaximumOffTrackSeconds > 0.0 &&
           Finite(s.MinimumOpportunityTimeGainSeconds) &&
           s.MinimumOpportunityTimeGainSeconds > 0.0 &&
           Finite(s.CriticalTireHeatC) &&
           s.CriticalTireHeatC > 0.0 &&
           Finite(s.CriticalBrakeHeatC) &&
           s.CriticalBrakeHeatC > 0.0 &&
           Finite(s.CriticalEngineHeatC) &&
           s.CriticalEngineHeatC > 0.0 &&
           Finite(s.MinimumSafeOpponentDistanceCm) &&
           s.MinimumSafeOpponentDistanceCm > 0.0 &&
           Finite(s.MinimumFuelReserveFraction) &&
           s.MinimumFuelReserveFraction >= 0.0 &&
           Finite(s.MinimumLapSectorComparisonSeconds) &&
           s.MinimumLapSectorComparisonSeconds > 0.0 &&
           Finite(s.MaximumAcceptableLineErrorCm) &&
           s.MaximumAcceptableLineErrorCm > 0.0 &&
           s.MaximumActiveTips > 0 &&
           s.MaximumActiveTips <= MaximumCoachTips;
}

struct RaceEngineerCoachingEvidence final
{
    RaceEngineerCoachingTopic Topic = RaceEngineerCoachingTopic::None;
    RaceEngineerCoachingPriority Priority = RaceEngineerCoachingPriority::None;
    CoachingPresentation Presentation =
        CoachingPresentation::AfterLap;
    double Confidence = 0.0;
    double Magnitude = 0.0;
    double Reference = 0.0;
    double ObservedSeconds = 0.0;
    double SuggestedReviewUrgency = 0.0;
    double EstimatedPotentialTimeGainSeconds = 0.0;
    int TrackSector = -1;
    int RelatedOpponentSlot = -1;
    std::uint64_t SourceSession = 0;
    std::uint64_t SourceActor = 0;
    bool NativeObserved = false;
    bool IsSafetyRelated = false;
    bool Qualified = false;
};

struct CoachingTip final
{
    RaceEngineerCoachingEvidence Evidence{};
    std::uint64_t Sequence = 0;
    double FirstObservedSeconds = 0.0;
    double LastObservedSeconds = 0.0;
    double LastShownSeconds = 0.0;
    double LongestObservedSeconds = 0.0;
    double ConfidenceWeightedMagnitude = 0.0;
    double MaximumPotentialGainSeconds = 0.0;
    std::uint64_t Observations = 0;
    int ActivePriority = 0;
    bool Active = false;
    bool Visible = false;
    bool Dismissed = false;
    bool Provisional = true;
};

struct CoachingFrameInput final
{
    BrakeLiveMetrics Brake{};
    CornerLiveMetrics Corner{};
    GripTelemetry Grip{};
    VehicleHealthSnapshot Health{};
    FuelRaceProjection Fuel{};
    FleetTelemetrySnapshot Opponents{};
    DeltaSample LapDelta{};
    TelemetryFrame SourceFrame{};
    double EvidenceConfidence = 0.0;
    double MonotonicSeconds = 0.0;
    bool DrivingInCaution = false;
    bool RacePaused = false;
    bool RaceFinished = false;
    bool NativeObserved = false;
};

inline RaceEngineerCoachingEvidence MakeCoachingEvidence(
    RaceEngineerCoachingTopic topic,
    RaceEngineerCoachingPriority priority,
    CoachingPresentation presentation,
    double magnitude, double reference,
    double confidence, double nowSeconds,
    bool nativeObserved, bool isSafetyRelated) noexcept
{
    RaceEngineerCoachingEvidence result;
    if (topic == RaceEngineerCoachingTopic::None ||
        priority == RaceEngineerCoachingPriority::None ||
        !Finite(magnitude) ||
        !Finite(reference) ||
        !Finite(confidence) ||
        !Finite(nowSeconds) ||
        nowSeconds < 0.0 ||
        confidence < 0.0 || confidence > 1.0)
    {
        return result;
    }
    result.Topic = topic;
    result.Priority = priority;
    result.Presentation = presentation;
    result.Magnitude = magnitude;
    result.Reference = reference;
    result.Confidence = confidence;
    result.ObservedSeconds = nowSeconds;
    result.SuggestedReviewUrgency =
        std::min(1.0, std::abs(magnitude) /
            std::max(std::abs(reference), 0.01));
    result.NativeObserved = nativeObserved;
    result.IsSafetyRelated = isSafetyRelated;
    result.Qualified = true;
    return result;
}

inline int CoachingPriorityRank(
    const RaceEngineerCoachingEvidence& e) noexcept
{
    if (!e.Qualified)
    {
        return -1;
    }
    const double weighted =
        e.Confidence * 100.0 +
        std::clamp(e.SuggestedReviewUrgency, 0.0, 1.0) * 100.0;
    return static_cast<int>(e.Priority) * 1000 +
        static_cast<int>(weighted);
}

class CoachingCandidateBuilder final
{
public:
    void Reset() noexcept
    {
        Candidates.fill({});
        Count = 0;
    }

    bool Add(RaceEngineerCoachingEvidence evidence) noexcept
    {
        if (!evidence.Qualified ||
            Count >= static_cast<int>(Candidates.size()))
        {
            return false;
        }
        for (int i = 0; i < Count; ++i)
        {
            RaceEngineerCoachingEvidence& existing =
                Candidates[static_cast<std::size_t>(i)];
            if (existing.Topic != evidence.Topic)
            {
                continue;
            }
            if (CoachingPriorityRank(evidence) >
                CoachingPriorityRank(existing))
            {
                existing = evidence;
            }
            return true;
        }
        Candidates[static_cast<std::size_t>(Count++)] = evidence;
        return true;
    }

    int Evaluate(
        const CoachingFrameInput& input,
        const RaceEngineerCoachingSettings& settings) noexcept
    {
        Reset();
        if (!ValidCoachingSettings(settings) ||
            !Finite(input.MonotonicSeconds) ||
            input.MonotonicSeconds < 0.0 ||
            input.EvidenceConfidence <
                settings.MinimumEvidenceConfidence)
        {
            return 0;
        }
        const auto add = [this, &input](
            RaceEngineerCoachingTopic topic,
            RaceEngineerCoachingPriority priority,
            CoachingPresentation presentation,
            double magnitude, double reference,
            bool safety = false)
        {
            RaceEngineerCoachingEvidence e = MakeCoachingEvidence(
                topic, priority, presentation,
                magnitude, reference,
                input.EvidenceConfidence,
                input.MonotonicSeconds,
                input.NativeObserved, safety);
            e.SourceSession = input.SourceFrame.Identity.SessionToken;
            e.SourceActor = input.SourceFrame.Identity.ActorToken;
            e.TrackSector = input.SourceFrame.Race.SectorIndex;
            Add(e);
        };
        if (input.Health.Qualified)
        {
            if (input.Health.EngineTemperatureC >=
                settings.CriticalEngineHeatC)
            {
                add(RaceEngineerCoachingTopic::MonitorEngineHeat,
                    RaceEngineerCoachingPriority::Critical,
                    CoachingPresentation::AdvisoryNow,
                    input.Health.EngineTemperatureC,
                    settings.CriticalEngineHeatC, true);
            }
            if (input.Health.BrakeTemperatureC >=
                settings.CriticalBrakeHeatC)
            {
                add(RaceEngineerCoachingTopic::ManageBrakeHeat,
                    RaceEngineerCoachingPriority::Safety,
                    CoachingPresentation::NextStraight,
                    input.Health.BrakeTemperatureC,
                    settings.CriticalBrakeHeatC, true);
            }
        }
        if (input.Opponents.Valid &&
            input.Opponents.PredictedContacts > 0)
        {
            add(RaceEngineerCoachingTopic::StayClearOfOpponent,
                RaceEngineerCoachingPriority::Safety,
                CoachingPresentation::AdvisoryNow,
                static_cast<double>(
                    input.Opponents.PredictedContacts),
                1.0, true);
        }
        if (input.Grip.Valid)
        {
            if (input.Grip.State == GripState::Sliding ||
                input.Grip.State == GripState::Saturated)
            {
                add(RaceEngineerCoachingTopic::ReduceExcessiveSlip,
                    RaceEngineerCoachingPriority::Safety,
                    CoachingPresentation::NextStraight,
                    input.Grip.MaximumCombinedSaturation,
                    1.0, true);
            }
            if (input.Grip.SuspectedUndersteer)
            {
                add(RaceEngineerCoachingTopic::ReduceUndersteer,
                    RaceEngineerCoachingPriority::Consistency,
                    CoachingPresentation::AfterLap,
                    input.Grip.MaximumCombinedSaturation, 1.0);
            }
            if (input.Grip.SuspectedOversteer)
            {
                add(RaceEngineerCoachingTopic::ReduceOversteer,
                    RaceEngineerCoachingPriority::Safety,
                    CoachingPresentation::NextStraight,
                    input.Grip.MaximumCombinedSaturation, 1.0, true);
            }
            if (input.Grip.MeanTireTemperatureC >=
                settings.CriticalTireHeatC)
            {
                add(RaceEngineerCoachingTopic::ManageTireHeat,
                    RaceEngineerCoachingPriority::Mechanical,
                    CoachingPresentation::NextStraight,
                    input.Grip.MeanTireTemperatureC,
                    settings.CriticalTireHeatC, true);
            }
        }
        const bool suppressPerformance =
            input.RacePaused || input.RaceFinished ||
            (settings.MutePerformanceUnderCaution &&
             input.DrivingInCaution) ||
            (settings.MutePerformanceDuringContactRisk &&
             input.Opponents.Valid &&
             input.Opponents.PredictedContacts > 0);
        if (!suppressPerformance)
        {
            if (input.Brake.Valid &&
                input.Brake.CurrentWheelLockFraction > 0.0)
            {
                add(RaceEngineerCoachingTopic::ReduceWheelLock,
                    RaceEngineerCoachingPriority::Consistency,
                    CoachingPresentation::AfterLap,
                    input.Brake.CurrentWheelLockFraction, 0.0);
            }
            if (input.Corner.Valid &&
                input.Corner.CurrentLineErrorCm >
                    settings.MaximumAcceptableLineErrorCm)
            {
                add(RaceEngineerCoachingTopic::ImproveApexConsistency,
                    RaceEngineerCoachingPriority::Consistency,
                    CoachingPresentation::AfterLap,
                    input.Corner.CurrentLineErrorCm,
                    settings.MaximumAcceptableLineErrorCm);
            }
            if (input.LapDelta.Qualified &&
                input.LapDelta.DeltaSeconds >=
                    settings.MinimumOpportunityTimeGainSeconds)
            {
                add(RaceEngineerCoachingTopic::CompareSector,
                    RaceEngineerCoachingPriority::Opportunity,
                    CoachingPresentation::AfterLap,
                    input.LapDelta.DeltaSeconds,
                    settings.MinimumOpportunityTimeGainSeconds);
            }
            if (input.Fuel.Qualified &&
                input.Fuel.FuelSavingLikelyNeeded)
            {
                add(RaceEngineerCoachingTopic::ConserveFuel,
                    RaceEngineerCoachingPriority::Performance,
                    CoachingPresentation::AfterLap,
                    input.Fuel.ReserveShortfallFraction,
                    settings.MinimumFuelReserveFraction);
            }
        }
        if (input.Grip.Valid &&
            input.Grip.ContactFraction < 0.5)
        {
            add(RaceEngineerCoachingTopic::VerifyGroundContact,
                RaceEngineerCoachingPriority::Mechanical,
                CoachingPresentation::AdvisoryNow,
                1.0 - input.Grip.ContactFraction,
                0.5, true);
        }
        return Count;
    }

    RaceEngineerCoachingEvidence At(int index) const noexcept
    {
        return index >= 0 && index < Count ?
            Candidates[static_cast<std::size_t>(index)] :
            RaceEngineerCoachingEvidence{};
    }

    RaceEngineerCoachingEvidence Best() const noexcept
    {
        RaceEngineerCoachingEvidence best;
        for (int i = 0; i < Count; ++i)
        {
            const RaceEngineerCoachingEvidence& current =
                Candidates[static_cast<std::size_t>(i)];
            if (!best.Qualified ||
                CoachingPriorityRank(current) >
                    CoachingPriorityRank(best))
            {
                best = current;
            }
        }
        return best;
    }

    int Size() const noexcept { return Count; }

private:
    std::array<RaceEngineerCoachingEvidence, MaximumCoachTips> Candidates{};
    int Count = 0;
};

class RaceEngineerCoach final
{
public:
    bool Configure(RaceEngineerCoachingSettings settings) noexcept
    {
        if (!ValidCoachingSettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        Tips.fill({});
        LastEmitted.fill(-1.0e12);
        TopicEmissions.fill(0);
        Builder.Reset();
        ActiveCount = 0;
        Sequence = 0;
        Session = 0;
        Actor = 0;
        LastTip = {};
        LastTime = 0.0;
        Rejected = 0;
        Accepted = 0;
    }

    bool Observe(CoachingFrameInput input) noexcept
    {
        if (!Finite(input.MonotonicSeconds) ||
            input.MonotonicSeconds < 0.0 ||
            input.SourceFrame.Identity.SessionToken == 0 ||
            input.SourceFrame.Identity.ActorToken == 0 ||
            (Settings.RequireNativeEvidence &&
             !input.NativeObserved) ||
            input.EvidenceConfidence <
                Settings.MinimumEvidenceConfidence)
        {
            Rejected = SaturatingIncrement(Rejected);
            return false;
        }
        if (Session != 0 &&
            (Session != input.SourceFrame.Identity.SessionToken ||
             Actor != input.SourceFrame.Identity.ActorToken))
        {
            Reset();
        }
        if (input.MonotonicSeconds < LastTime)
        {
            Rejected = SaturatingIncrement(Rejected);
            return false;
        }
        Session = input.SourceFrame.Identity.SessionToken;
        Actor = input.SourceFrame.Identity.ActorToken;
        LastTime = input.MonotonicSeconds;
        ActiveCount = Builder.Evaluate(input, Settings);
        std::array<RaceEngineerCoachingEvidence, MaximumCoachTips> sorted{};
        for (int i = 0; i < ActiveCount; ++i)
        {
            sorted[static_cast<std::size_t>(i)] = Builder.At(i);
        }
        for (int i = 1; i < ActiveCount; ++i)
        {
            const RaceEngineerCoachingEvidence selected =
                sorted[static_cast<std::size_t>(i)];
            int j = i;
            while (j > 0 &&
                CoachingPriorityRank(
                    sorted[static_cast<std::size_t>(j - 1)]) <
                CoachingPriorityRank(selected))
            {
                sorted[static_cast<std::size_t>(j)] =
                    sorted[static_cast<std::size_t>(j - 1)];
                --j;
            }
            sorted[static_cast<std::size_t>(j)] = selected;
        }
        Tips.fill({});
        for (int i = 0; i < ActiveCount &&
             i < Settings.MaximumActiveTips; ++i)
        {
            const RaceEngineerCoachingEvidence e =
                sorted[static_cast<std::size_t>(i)];
            const int topic = static_cast<int>(e.Topic);
            if (topic <= 0 || topic >= CoachingTopicCount)
            {
                continue;
            }
            const double cooldown = e.IsSafetyRelated ?
                Settings.SafetyHintCooldownSeconds :
                Settings.PerformanceHintCooldownSeconds;
            const bool visible = input.MonotonicSeconds -
                LastEmitted[static_cast<std::size_t>(topic)] >=
                std::max(Settings.MinimumHintIntervalSeconds, cooldown);
            CoachingTip tip;
            tip.Evidence = e;
            tip.FirstObservedSeconds = input.MonotonicSeconds;
            tip.LastObservedSeconds = input.MonotonicSeconds;
            tip.Active = true;
            tip.Visible = visible;
            tip.Provisional = e.Presentation !=
                CoachingPresentation::AdvisoryNow;
            tip.ActivePriority = CoachingPriorityRank(e);
            tip.ConfidenceWeightedMagnitude =
                e.Confidence * e.Magnitude;
            tip.Observations = 1;
            if (visible)
            {
                ++Sequence;
                tip.Sequence = Sequence;
                tip.LastShownSeconds = input.MonotonicSeconds;
                LastEmitted[static_cast<std::size_t>(topic)] =
                    input.MonotonicSeconds;
                TopicEmissions[static_cast<std::size_t>(topic)] += 1;
                if (!LastTip.Active || tip.ActivePriority >
                    LastTip.ActivePriority)
                {
                    LastTip = tip;
                }
            }
            Tips[static_cast<std::size_t>(i)] = tip;
        }
        Accepted = SaturatingIncrement(Accepted);
        return true;
    }

    CoachingTip PrimaryTip() const noexcept
    {
        for (int i = 0; i < ActiveCount; ++i)
        {
            const CoachingTip& tip =
                Tips[static_cast<std::size_t>(i)];
            if (tip.Active && tip.Visible)
            {
                return tip;
            }
        }
        return {};
    }

    CoachingTip Tip(int index) const noexcept
    {
        return index >= 0 && index < ActiveCount ?
            Tips[static_cast<std::size_t>(index)] :
            CoachingTip{};
    }
    int ActiveTips() const noexcept { return ActiveCount; }
    CoachingTip LastEmittedTip() const noexcept { return LastTip; }
    std::uint64_t EmissionsFor(RaceEngineerCoachingTopic topic) const noexcept
    {
        const int index = static_cast<int>(topic);
        return index >= 0 && index < CoachingTopicCount ?
            TopicEmissions[static_cast<std::size_t>(index)] : 0;
    }
    std::uint64_t AcceptedFrames() const noexcept { return Accepted; }
    std::uint64_t RejectedFrames() const noexcept { return Rejected; }

private:
    RaceEngineerCoachingSettings Settings{};
    CoachingCandidateBuilder Builder;
    std::array<CoachingTip, MaximumCoachTips> Tips{};
    std::array<double, CoachingTopicCount> LastEmitted{};
    std::array<std::uint64_t, CoachingTopicCount> TopicEmissions{};
    CoachingTip LastTip{};
    int ActiveCount = 0;
    std::uint64_t Sequence = 0;
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    double LastTime = 0.0;
    std::uint64_t Accepted = 0;
    std::uint64_t Rejected = 0;
};
} // namespace latebrake::engineering
