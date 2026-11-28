#pragma once

#include "Racing/DrivingCoachReview.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// Optional practice suggestions derived from accepted vehicle samples.
// Their targets are relative to a previous observation, not assertions
// about real driver ability, cornering safety, optimal lap time or rank.
inline constexpr std::size_t DrivingCoachMaximumGoals=3;
inline constexpr double DrivingCoachMinimumGoalSeconds=10.0;
inline constexpr std::uint64_t DrivingCoachMinimumGoalSamples=100;

enum class DrivingCoachGoalKind : std::uint8_t
{
    GatherMoreEvidence,
    ReduceHighLateralSlip,
    ReduceBrakeReapplication,
    ReduceBrakeTimeShare,
    MaintainSmoothSampledMotion
};

enum class DrivingCoachGoalUnit : std::uint8_t
{
    PercentSampleTime,
    EpisodesPerMinute,
    Seconds,
    None
};

enum class DrivingCoachGoalOutcome : std::uint8_t
{
    NotComparable,
    NotEnoughSamples,
    ObservedTargetMet,
    ObservedTargetNotMet
};

inline const char* DrivingCoachGoalLabel(DrivingCoachGoalKind goal) noexcept
{
    switch(goal)
    {
    case DrivingCoachGoalKind::GatherMoreEvidence:
        return "COLLECT MORE CONTINUOUS SAMPLED DRIVING";
    case DrivingCoachGoalKind::ReduceHighLateralSlip:
        return "PRACTICE LOWER LATERAL SLIP EXPOSURE";
    case DrivingCoachGoalKind::ReduceBrakeReapplication:
        return "PRACTICE FEWER SEPARATE BRAKE PRESSES";
    case DrivingCoachGoalKind::ReduceBrakeTimeShare:
        return "REVIEW WHERE BRAKING TIME ACCUMULATES";
    case DrivingCoachGoalKind::MaintainSmoothSampledMotion:
        return "REVIEW WHETHER STABLE SAMPLED MOTION PERSISTS";
    default:return "PRACTICE GOAL UNAVAILABLE";
    }
}

inline const char* DrivingCoachGoalOutcomeLabel(DrivingCoachGoalOutcome outcome) noexcept
{
    switch(outcome)
    {
    case DrivingCoachGoalOutcome::NotComparable:
        return "NOT COMPARABLE WITH AN UNVERIFIED SESSION";
    case DrivingCoachGoalOutcome::NotEnoughSamples:
        return "MORE ACCEPTED OBSERVATION REQUIRED";
    case DrivingCoachGoalOutcome::ObservedTargetMet:
        return "TARGET MET IN SAMPLED TELEMETRY ONLY";
    case DrivingCoachGoalOutcome::ObservedTargetNotMet:
        return "TARGET NOT MET IN SAMPLED TELEMETRY";
    default:return "PRACTICE PROGRESS UNAVAILABLE";
    }
}

struct DrivingCoachGoal final
{
    bool Valid=false;
    DrivingCoachGoalKind Kind=DrivingCoachGoalKind::GatherMoreEvidence;
    DrivingCoachGoalUnit Unit=DrivingCoachGoalUnit::None;
    double Baseline=0.0;
    double Target=0.0;
    double MinimumEvidenceSeconds=DrivingCoachMinimumGoalSeconds;
    std::uint64_t MinimumEvidenceSamples=DrivingCoachMinimumGoalSamples;
    bool LowerIsBetter=true;
    bool IsOptional=true;
    bool HasGameplayAuthority=false;
};

struct DrivingCoachGoalPlan final
{
    bool Valid=false;
    DrivingCoachEvidence BaselineEvidence=DrivingCoachEvidence::Unavailable;
    double BaselineObservedSeconds=0.0;
    std::uint64_t BaselineSamples=0;
    std::array<DrivingCoachGoal,DrivingCoachMaximumGoals> Goals{};
    std::size_t Count=0;
    bool SourceIsRunLocal=true;
    bool OffersSavedRewards=false;
    bool TriggersInputAssistance=false;
};

struct DrivingCoachGoalProgress final
{
    bool Valid=false;
    DrivingCoachGoalKind Kind=DrivingCoachGoalKind::GatherMoreEvidence;
    DrivingCoachGoalOutcome Outcome=DrivingCoachGoalOutcome::NotComparable;
    double ObservedValue=0.0;
    double BaselineValue=0.0;
    double TargetValue=0.0;
    double DifferenceFromBaseline=0.0;
    double DifferenceFromTarget=0.0;
    double ObservedSeconds=0.0;
    std::uint64_t ObservedSamples=0;
    bool OfficialImprovementClaimed=false;
    bool OfficialLapComparison=false;
    bool RewardsGranted=false;
};

inline bool DrivingCoachGoalHasMetric(DrivingCoachGoalKind kind) noexcept
{
    switch(kind)
    {
    case DrivingCoachGoalKind::ReduceHighLateralSlip:
    case DrivingCoachGoalKind::ReduceBrakeReapplication:
    case DrivingCoachGoalKind::ReduceBrakeTimeShare:
    case DrivingCoachGoalKind::MaintainSmoothSampledMotion:return true;
    default:return false;
    }
}

inline double DrivingCoachGoalMeasure(DrivingCoachGoalKind kind,
    const DrivingCoachReadout& observation) noexcept
{
    switch(kind)
    {
    case DrivingCoachGoalKind::GatherMoreEvidence:
        return observation.ObservedSeconds;
    case DrivingCoachGoalKind::ReduceHighLateralSlip:
        return 100.0*observation.HighSlipFraction;
    case DrivingCoachGoalKind::ReduceBrakeReapplication:
        return observation.ObservedSeconds>0.0 ?
            60.0*static_cast<double>(observation.AcceptedBrakeEpisodes)/
                observation.ObservedSeconds : 0.0;
    case DrivingCoachGoalKind::ReduceBrakeTimeShare:
        return 100.0*observation.BrakeFraction;
    case DrivingCoachGoalKind::MaintainSmoothSampledMotion:
        return 100.0*observation.HighSlipFraction;
    default:return 0.0;
    }
}

inline bool ValidDrivingCoachGoal(const DrivingCoachGoal& goal) noexcept
{
    if(!goal.Valid || !goal.IsOptional || goal.HasGameplayAuthority ||
        !std::isfinite(goal.Baseline) || !std::isfinite(goal.Target) ||
        !std::isfinite(goal.MinimumEvidenceSeconds) ||
        goal.MinimumEvidenceSeconds<5.0 ||
        goal.MinimumEvidenceSeconds>600.0 ||
        goal.MinimumEvidenceSamples<20 ||
        goal.MinimumEvidenceSamples>100000 ||
        goal.Baseline<0.0 || goal.Target<0.0 ||
        goal.Baseline>10000.0 || goal.Target>10000.0)
        return false;
    if(goal.Kind==DrivingCoachGoalKind::GatherMoreEvidence)
        return goal.Unit==DrivingCoachGoalUnit::Seconds &&
            !goal.LowerIsBetter && goal.Target>=5.0 &&
            goal.Baseline<goal.Target;
    if(!DrivingCoachGoalHasMetric(goal.Kind))return false;
    if(!goal.LowerIsBetter || goal.Target>goal.Baseline)
        return false;
    switch(goal.Kind)
    {
    case DrivingCoachGoalKind::ReduceHighLateralSlip:
    case DrivingCoachGoalKind::ReduceBrakeTimeShare:
    case DrivingCoachGoalKind::MaintainSmoothSampledMotion:
        return goal.Unit==DrivingCoachGoalUnit::PercentSampleTime &&
            goal.Baseline<=100.0 && goal.Target<=100.0;
    case DrivingCoachGoalKind::ReduceBrakeReapplication:
        return goal.Unit==DrivingCoachGoalUnit::EpisodesPerMinute;
    default:return false;
    }
}

inline DrivingCoachGoal MakeDrivingCoachGoal(DrivingCoachGoalKind kind,
    double baseline,double target) noexcept
{
    DrivingCoachGoal out;
    out.Valid=true;
    out.Kind=kind;
    out.Baseline=baseline;
    out.Target=target;
    out.Unit=kind==DrivingCoachGoalKind::GatherMoreEvidence ?
        DrivingCoachGoalUnit::Seconds :
        kind==DrivingCoachGoalKind::ReduceBrakeReapplication ?
            DrivingCoachGoalUnit::EpisodesPerMinute :
            DrivingCoachGoalUnit::PercentSampleTime;
    out.LowerIsBetter=kind!=DrivingCoachGoalKind::GatherMoreEvidence;
    if(!ValidDrivingCoachGoal(out))return {};
    return out;
}

inline DrivingCoachGoalPlan BuildDrivingCoachGoals(
    const DrivingCoachReadout& observation) noexcept
{
    DrivingCoachGoalPlan plan;
    if(!ValidDrivingCoachReadout(observation))return plan;
    plan.Valid=true;
    plan.BaselineEvidence=observation.Evidence;
    plan.BaselineObservedSeconds=observation.ObservedSeconds;
    plan.BaselineSamples=observation.AcceptedSamples;
    const auto append=[&](DrivingCoachGoal goal)
    {
        if(goal.Valid && plan.Count<plan.Goals.size())
            plan.Goals[plan.Count++]=goal;
    };
    if(observation.Evidence==DrivingCoachEvidence::Limited ||
        observation.ObservedSeconds<DrivingCoachMinimumGoalSeconds ||
        observation.AcceptedSamples<DrivingCoachMinimumGoalSamples)
    {
        append(MakeDrivingCoachGoal(
            DrivingCoachGoalKind::GatherMoreEvidence,
            std::min(9.99,observation.ObservedSeconds),10.0));
        return plan;
    }
    const double slip=DrivingCoachGoalMeasure(
        DrivingCoachGoalKind::ReduceHighLateralSlip,observation);
    const double brakeFrequency=DrivingCoachGoalMeasure(
        DrivingCoachGoalKind::ReduceBrakeReapplication,observation);
    const double brakeShare=DrivingCoachGoalMeasure(
        DrivingCoachGoalKind::ReduceBrakeTimeShare,observation);
    if(slip>=10.0 && observation.HighSlipSeconds>=1.0)
        append(MakeDrivingCoachGoal(
            DrivingCoachGoalKind::ReduceHighLateralSlip,
            slip,std::max(2.0,0.80*slip)));
    if(brakeFrequency>=12.0 && observation.AcceptedBrakeEpisodes>=8)
        append(MakeDrivingCoachGoal(
            DrivingCoachGoalKind::ReduceBrakeReapplication,
            brakeFrequency,0.8*brakeFrequency));
    if(brakeShare>=35.0 && observation.BrakeSeconds>=3.0)
        append(MakeDrivingCoachGoal(
            DrivingCoachGoalKind::ReduceBrakeTimeShare,
            brakeShare,0.85*brakeShare));
    if(plan.Count==0)
        append(MakeDrivingCoachGoal(
            DrivingCoachGoalKind::MaintainSmoothSampledMotion,
            slip,std::max(0.0,slip-1.0)));
    return plan;
}

inline bool ValidDrivingCoachGoalPlan(const DrivingCoachGoalPlan& plan) noexcept
{
    if(!plan.Valid || plan.Count==0 || plan.Count>plan.Goals.size() ||
        !plan.SourceIsRunLocal || plan.OffersSavedRewards ||
        plan.TriggersInputAssistance ||
        !std::isfinite(plan.BaselineObservedSeconds) ||
        plan.BaselineObservedSeconds<=0.0 || plan.BaselineSamples==0)
        return false;
    for(std::size_t i=0;i<plan.Count;++i)
    {
        if(!ValidDrivingCoachGoal(plan.Goals[i]))return false;
        for(std::size_t j=0;j<i;++j)
            if(plan.Goals[j].Kind==plan.Goals[i].Kind)return false;
    }
    return true;
}

inline DrivingCoachGoalProgress ReviewDrivingCoachGoalProgress(
    DrivingCoachGoal goal,const DrivingCoachReadout& candidate) noexcept
{
    DrivingCoachGoalProgress result;
    if(!ValidDrivingCoachGoal(goal) || !ValidDrivingCoachReadout(candidate))
        return result;
    result.Valid=true;
    result.Kind=goal.Kind;
    result.BaselineValue=goal.Baseline;
    result.TargetValue=goal.Target;
    result.ObservedValue=DrivingCoachGoalMeasure(goal.Kind,candidate);
    result.DifferenceFromBaseline=result.ObservedValue-goal.Baseline;
    result.DifferenceFromTarget=result.ObservedValue-goal.Target;
    result.ObservedSeconds=candidate.ObservedSeconds;
    result.ObservedSamples=candidate.AcceptedSamples;
    if(candidate.ObservedSeconds<goal.MinimumEvidenceSeconds ||
        candidate.AcceptedSamples<goal.MinimumEvidenceSamples ||
        candidate.Evidence==DrivingCoachEvidence::Limited)
    {
        result.Outcome=DrivingCoachGoalOutcome::NotEnoughSamples;
        return result;
    }
    result.Outcome=goal.LowerIsBetter ?
        result.ObservedValue<=goal.Target ?
            DrivingCoachGoalOutcome::ObservedTargetMet :
            DrivingCoachGoalOutcome::ObservedTargetNotMet :
        result.ObservedValue>=goal.Target ?
            DrivingCoachGoalOutcome::ObservedTargetMet :
            DrivingCoachGoalOutcome::ObservedTargetNotMet;
    return result;
}
} // namespace latebrake
