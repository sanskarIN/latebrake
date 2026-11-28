#include "Racing/DrivingCoachPracticeGoals.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;

void Require(bool check,const char* message)
{
    if(!check)
    {
        std::cerr<<"DrivingCoachPracticeGoals: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double a,double b)
{
    return std::abs(a-b)<=1e-7*std::max({1.0,std::abs(a),std::abs(b)});
}

DrivingCoachReadout MakeRun(double seconds,double speed,
    double slipProportion,double brakingProportion,bool brakeFlapping=false)
{
    DrivingCoachAccumulator accumulator;
    const int count=static_cast<int>(std::round(seconds/0.05));
    for(int i=0;i<count;++i)
    {
        const bool slip=i<static_cast<int>(slipProportion*count);
        const bool brake=brakeFlapping ?
            i%2==0 : i<static_cast<int>(brakingProportion*count);
        Require(accumulator.Observe({0.05,speed,slip?speed*0.6:0.0,brake}),
            "fixture sample accepted");
    }
    return accumulator.Review();
}

void NoGoalOnUnverifiedEvidence()
{
    Require(!BuildDrivingCoachGoals({}).Valid,
        "empty source cannot produce a motivational target");
    auto readout=MakeRun(0.5,1500.0,0,0);
    const auto plan=BuildDrivingCoachGoals(readout);
    Require(plan.Valid && plan.Count==1 &&
        plan.Goals[0].Kind==DrivingCoachGoalKind::GatherMoreEvidence,
        "short session creates observation target only");
    Require(Near(plan.Goals[0].Baseline,0.5) &&
        Near(plan.Goals[0].Target,10.0) &&
        plan.Goals[0].Unit==DrivingCoachGoalUnit::Seconds,
        "more-evidence goal measures source duration, not fictitious skill");
    Require(ValidDrivingCoachGoalPlan(plan) &&
        !plan.OffersSavedRewards && !plan.TriggersInputAssistance,
        "limited evidence cannot open financial or steering authority");
    auto corrupt=readout;
    corrupt.NoGameplayAuthority=false;
    Require(!BuildDrivingCoachGoals(corrupt).Valid,
        "malformed authority declarations fail closed");
}

void LateralSlipPractice()
{
    auto readout=MakeRun(20.0,2000.0,0.5,0.1);
    const auto plan=BuildDrivingCoachGoals(readout);
    Require(ValidDrivingCoachGoalPlan(plan),"source-consistent slip goal plan");
    Require(plan.Goals[0].Kind==DrivingCoachGoalKind::ReduceHighLateralSlip,
        "slip target prioritized when half of sample time has lateral slip");
    Require(Near(plan.Goals[0].Baseline,50.0) &&
        Near(plan.Goals[0].Target,40.0),
        "target is relative to measured baseline rather than zero-slip fiction");
    const auto improved=MakeRun(20.0,2000.0,0.3,0.1);
    const auto progress=ReviewDrivingCoachGoalProgress(plan.Goals[0],improved);
    Require(progress.Valid &&
        progress.Outcome==DrivingCoachGoalOutcome::ObservedTargetMet &&
        Near(progress.ObservedValue,30.0) &&
        progress.DifferenceFromBaseline<0.0,
        "goal progress is an offline sample comparison, not race scoring");
    Require(!progress.OfficialImprovementClaimed &&
        !progress.OfficialLapComparison && !progress.RewardsGranted,
        "improvement in sampled proportion is not a certified lap improvement");
    const auto worse=MakeRun(20.0,2000.0,0.8,0.1);
    Require(ReviewDrivingCoachGoalProgress(plan.Goals[0],worse).Outcome==
        DrivingCoachGoalOutcome::ObservedTargetNotMet,
        "worse lateral sample share does not meet reduction target");
}

void BrakeEpisodePractice()
{
    const auto tapped=MakeRun(20.0,2000.0,0.0,0.5,true);
    const auto plan=BuildDrivingCoachGoals(tapped);
    Require(ValidDrivingCoachGoalPlan(plan) && plan.Count>=1,
        "repeated observed tapping yields a practice plan");
    Require(plan.Goals[0].Kind==DrivingCoachGoalKind::ReduceBrakeReapplication,
        "frequent brake edges are treated separately from brake duration");
    Require(plan.Goals[0].Unit==DrivingCoachGoalUnit::EpisodesPerMinute &&
        Near(plan.Goals[0].Baseline,600.0) &&
        Near(plan.Goals[0].Target,480.0),
        "rate uses observed time rather than fixed frame assumptions");
    const auto held=MakeRun(20.0,2000.0,0.0,0.55,false);
    const auto progress=ReviewDrivingCoachGoalProgress(plan.Goals[0],held);
    Require(progress.Outcome==DrivingCoachGoalOutcome::ObservedTargetMet &&
        progress.ObservedValue<10.0,
        "a held brake is one episode, not one per frame");
    const auto holdPlan=BuildDrivingCoachGoals(held);
    Require(holdPlan.Goals[0].Kind==DrivingCoachGoalKind::ReduceBrakeTimeShare &&
        holdPlan.Goals[0].Unit==DrivingCoachGoalUnit::PercentSampleTime &&
        holdPlan.Goals[0].Baseline>50.0,
        "heavy brake duration is a distinct advisory when tapping is absent");
}

void PlanPriorityAndCapacity()
{
    const auto busy=MakeRun(20.0,2000.0,0.5,0.5,true);
    const auto goals=BuildDrivingCoachGoals(busy);
    Require(goals.Count==3 && ValidDrivingCoachGoalPlan(goals),
        "slip, reapplication, brake duration fit exactly three bounded goals");
    Require(goals.Goals[0].Kind==DrivingCoachGoalKind::ReduceHighLateralSlip &&
        goals.Goals[1].Kind==DrivingCoachGoalKind::ReduceBrakeReapplication &&
        goals.Goals[2].Kind==DrivingCoachGoalKind::ReduceBrakeTimeShare,
        "ordered goals reflect distinct verified sample observations");
    const auto neutral=BuildDrivingCoachGoals(MakeRun(20.0,2000.0,0,0));
    Require(neutral.Count==1 &&
        neutral.Goals[0].Kind==DrivingCoachGoalKind::MaintainSmoothSampledMotion,
        "stable sample produces optional maintenance, not mandatory intervention");
    Require(Near(neutral.Goals[0].Baseline,0.0) &&
        Near(neutral.Goals[0].Target,0.0),
        "zero observed high slip is not turned into a negative target");
    auto invalid=goals;
    invalid.Count=4;
    Require(!ValidDrivingCoachGoalPlan(invalid),
        "no out-of-bounds goals accepted");
    invalid=goals;
    invalid.OffersSavedRewards=true;
    Require(!ValidDrivingCoachGoalPlan(invalid),
        "rewards flag cannot be added to a practice plan");
    invalid=goals;
    invalid.Goals[1].Kind=invalid.Goals[0].Kind;
    Require(!ValidDrivingCoachGoalPlan(invalid),
        "duplicate goal kinds are rejected");
}

void ProgressEvidenceGates()
{
    const auto baseline=MakeRun(20.0,2000.0,0.5,0.2);
    const auto plan=BuildDrivingCoachGoals(baseline);
    Require(plan.Count>0,"reference baseline has a meaningful goal");
    const auto tiny=MakeRun(1.0,2000.0,0.0,0.1);
    auto progress=ReviewDrivingCoachGoalProgress(plan.Goals[0],tiny);
    Require(progress.Valid &&
        progress.Outcome==DrivingCoachGoalOutcome::NotEnoughSamples &&
        progress.ObservedSamples>0,
        "very short perfect-looking segment cannot satisfy a practice goal");
    auto malformed=tiny;
    malformed.Valid=false;
    Require(!ReviewDrivingCoachGoalProgress(plan.Goals[0],malformed).Valid,
        "invalid observations cannot create comparison evidence");
    auto goal=plan.Goals[0];
    goal.MinimumEvidenceSeconds=1000.0;
    Require(!ReviewDrivingCoachGoalProgress(goal,baseline).Valid,
        "unrealistically huge minimum evidence boundary refused");
    goal=plan.Goals[0];
    goal.Target=std::numeric_limits<double>::quiet_NaN();
    Require(!ReviewDrivingCoachGoalProgress(goal,baseline).Valid,
        "NaN target cannot become a practice result");
}

void IndependentMeasureChecks()
{
    const auto sample=MakeRun(10.0,2000.0,0.4,0.3);
    Require(Near(DrivingCoachGoalMeasure(
        DrivingCoachGoalKind::GatherMoreEvidence,sample),10.0),
        "gather goal uses observed duration");
    Require(Near(DrivingCoachGoalMeasure(
        DrivingCoachGoalKind::ReduceHighLateralSlip,sample),40.0),
        "slip share measured in percentage points");
    Require(Near(DrivingCoachGoalMeasure(
        DrivingCoachGoalKind::ReduceBrakeTimeShare,sample),30.0),
        "brake share measured in percentage points");
    Require(Near(DrivingCoachGoalMeasure(
        DrivingCoachGoalKind::ReduceBrakeReapplication,sample),6.0),
        "single brake episode over ten seconds is six per minute");
    for(const auto value:{DrivingCoachGoalKind::GatherMoreEvidence,
        DrivingCoachGoalKind::ReduceHighLateralSlip,
        DrivingCoachGoalKind::ReduceBrakeReapplication,
        DrivingCoachGoalKind::ReduceBrakeTimeShare,
        DrivingCoachGoalKind::MaintainSmoothSampledMotion})
        Require(DrivingCoachGoalLabel(value)[0]!='\0',
            "every practice goal has an accessible text label");
    for(const auto outcome:{DrivingCoachGoalOutcome::NotComparable,
        DrivingCoachGoalOutcome::NotEnoughSamples,
        DrivingCoachGoalOutcome::ObservedTargetMet,
        DrivingCoachGoalOutcome::ObservedTargetNotMet})
        Require(DrivingCoachGoalOutcomeLabel(outcome)[0]!='\0',
            "every practice outcome has an accessible text label");
}

void RandomizedPlanProperties()
{
    std::mt19937_64 rng(0x4D41524B4552ULL);
    std::uniform_int_distribution<int> slip(0,100);
    std::uniform_int_distribution<int> brake(0,100);
    for(int round=0;round<600;++round)
    {
        const double slipRate=static_cast<double>(slip(rng))/100.0;
        const double brakeRate=static_cast<double>(brake(rng))/100.0;
        const auto source=MakeRun(10.0,2000.0,slipRate,brakeRate);
        const auto plan=BuildDrivingCoachGoals(source);
        Require(plan.Valid && ValidDrivingCoachGoalPlan(plan) &&
            plan.Count>=1 && plan.Count<=DrivingCoachMaximumGoals,
            "all seeded valid sessions produce bounded valid targets");
        for(std::size_t i=0;i<plan.Count;++i)
        {
            const auto& goal=plan.Goals[i];
            Require(ValidDrivingCoachGoal(goal) &&
                goal.Target>=0.0 && goal.Baseline>=0.0 &&
                !goal.HasGameplayAuthority,
                "randomized goals never invent negative targets or race authority");
            auto progress=ReviewDrivingCoachGoalProgress(goal,source);
            Require(progress.Valid && !progress.RewardsGranted &&
                std::isfinite(progress.ObservedValue),
                "progress against same source is a finite read-only result");
        }
    }
}
}

int main()
{
    NoGoalOnUnverifiedEvidence();
    LateralSlipPractice();
    BrakeEpisodePractice();
    PlanPriorityAndCapacity();
    ProgressEvidenceGates();
    IndependentMeasureChecks();
    RandomizedPlanProperties();
    std::cout<<"DrivingCoachPracticeGoals: evidence-constrained goals passed.\n";
    return EXIT_SUCCESS;
}
