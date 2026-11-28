#include "Racing/DrivingCoachIntegrity.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <string>

namespace
{
using namespace latebrake;
void Require(bool ok,const std::string& detail)
{
    if(!ok)
    {
        std::cerr << "DrivingCoachIntegrity: " << detail << '\n';
        std::exit(EXIT_FAILURE);
    }
}
void ObserveLoop(DrivingSessionStats& stats,int count,double dt,double baseSpeed,
    bool alternateBraking)
{
    for(int i=0;i<count;++i)
    {
        const double speed=baseSpeed+static_cast<double>(i%7)*2.0;
        Require(stats.Observe({dt,speed,speed*0.25,
            alternateBraking && (i%12)<4}),"accepted sample");
    }
}

void EmptyAndSmallSessions()
{
    DrivingSessionStats stats;
    auto result=VerifyDrivingCoachIntegrity(stats);
    Require(!result.Valid && !result.AdviceAllowed &&
        result.Status==DrivingCoachIntegrityStatus::NoSamples,
        "empty sessions never show accepted coaching advice");
    Require(stats.Observe({0.01,900.0,100.0,false}),"one valid frame");
    result=VerifyDrivingCoachIntegrity(stats);
    Require(result.Valid && !result.AdviceAllowed &&
        result.Status==DrivingCoachIntegrityStatus::LimitedSamples &&
        !result.HasIndependentDistribution,"small source agrees but lacks coverage");
    ObserveLoop(stats,32,0.1,1000.0,true);
    result=VerifyDrivingCoachIntegrity(stats);
    Require(result.Valid && !result.AdviceAllowed &&
        result.Status==DrivingCoachIntegrityStatus::LimitedSamples &&
        result.HasIndependentDistribution,
        "distribution alone does not prove broad observation");
    ObserveLoop(stats,20,0.1,1200.0,false);
    result=VerifyDrivingCoachIntegrity(stats);
    Require(result.Valid && result.AdviceAllowed &&
        result.Status==DrivingCoachIntegrityStatus::VerifiedSourceAgreement &&
        result.HasIndependentDistribution,
        "adequate independent source counters agree");
    Require(result.SourceSamples==53 && result.CoachSamples==53 &&
        result.SourceBrakeApplications==3,
        "accepted and brake episode counters remain consistent");
}

void ContradictionsAreWithheld()
{
    DrivingSessionStats stats;
    ObserveLoop(stats,120,0.1,1800.0,true);
    const auto good=stats.GetCoachReview();
    Require(VerifyDrivingCoachIntegrity(stats,good).AdviceAllowed,
        "verified source fixture");
    const auto altered=[&](auto change,DrivingCoachIntegrityStatus wanted)
    {
        auto injected=good;
        change(injected);
        const auto report=VerifyDrivingCoachIntegrity(stats,injected);
        Require(!report.Valid && !report.AdviceAllowed &&
            report.Status==wanted, "cross-source tamper rejected " +
            std::to_string(static_cast<int>(wanted)) + " actual " +
            std::to_string(static_cast<int>(report.Status)));
    };
    altered([](DrivingCoachReadout& r){r.AcceptedSamples++;},
        DrivingCoachIntegrityStatus::InvalidCoachReadout);
    altered([](DrivingCoachReadout& r){r.AcceptedBrakeEpisodes++;},
        DrivingCoachIntegrityStatus::CrossSourceMismatch);
    altered([](DrivingCoachReadout& r){r.ObservedSeconds+=0.25;},
        DrivingCoachIntegrityStatus::InvalidCoachReadout);
    altered([](DrivingCoachReadout& r){r.PeakSpeedKmh+=10.0;},
        DrivingCoachIntegrityStatus::CrossSourceMismatch);
    altered([](DrivingCoachReadout& r){r.HighSlipSeconds+=0.25;},
        DrivingCoachIntegrityStatus::CrossSourceMismatch);
    altered([](DrivingCoachReadout& r){r.NoGameplayAuthority=false;},
        DrivingCoachIntegrityStatus::InvalidCoachReadout);
    altered([](DrivingCoachReadout& r){r.DistanceKm=std::numeric_limits<double>::quiet_NaN();},
        DrivingCoachIntegrityStatus::InvalidCoachReadout);
    altered([](DrivingCoachReadout& r){r.BrakeSeconds+=1.0;},
        DrivingCoachIntegrityStatus::CrossSourceMismatch);
    altered([](DrivingCoachReadout& r){r.SpeedBands[0].Seconds+=1.0;},
        DrivingCoachIntegrityStatus::InvalidCoachReadout);
    altered([](DrivingCoachReadout& r){r.AcceptedSlipEpisodes=r.CompletedSlipEpisodes-1;},
        DrivingCoachIntegrityStatus::InvalidCoachReadout);
    auto final=VerifyDrivingCoachIntegrity(stats);
    Require(final.AdviceAllowed && final.Valid,
        "tamper was only in copied readouts and did not mutate live stats");
}

void InputSuspensionAndRecoveries()
{
    DrivingSessionStats stats;
    ObserveLoop(stats,80,0.1,1200.0,true);
    const auto before=stats.GetCoachReview();
    Require(VerifyDrivingCoachIntegrity(stats).AdviceAllowed,
        "source qualified before invalid frame");
    stats.Suspend();
    Require(stats.GetCoachReview().Suspensions==before.Suspensions+1,
        "explicit pause marks continuity boundary");
    Require(!stats.Observe({0.3,1000.0,0.0,false}),
        "invalid long frame is rejected");
    Require(!stats.Observe({0.1,std::numeric_limits<double>::infinity(),0.0,false}),
        "invalid infinity rejected");
    Require(stats.GetCoachReview().RefusedSamples==2,
        "source refusal recorded");
    auto result=VerifyDrivingCoachIntegrity(stats);
    Require(result.AdviceAllowed && result.SourceSamples==80,
        "rejections cannot reduce accepted cumulative observation");
    ObserveLoop(stats,30,0.1,900.0,true);
    result=VerifyDrivingCoachIntegrity(stats);
    Require(result.AdviceAllowed && result.SourceSamples==110,
        "resume continues audit without fake data across gaps");
    stats.Reset();
    Require(!VerifyDrivingCoachIntegrity(stats).Valid &&
        stats.GetCoachReview().AcceptedSamples==0,
        "new session resets every evidence counter");
}

void DeterministicRandomRegression()
{
    DrivingSessionStats stats;
    std::mt19937 engine{0x1BCAFE};
    std::uniform_real_distribution<double> dt(0.005,0.25);
    std::uniform_real_distribution<double> speed(0.0,10000.0);
    std::uniform_real_distribution<double> lateral(-10000.0,10000.0);
    for(int i=0;i<25000;++i)
    {
        if(i%977==0)stats.Suspend();
        const bool braking=(engine()%5)<2;
        Require(stats.Observe({dt(engine),speed(engine),lateral(engine),braking}),
            "bounded random source acceptance");
        if(i%331==0)
        {
            const auto result=VerifyDrivingCoachIntegrity(stats);
            Require(result.Valid &&
                result.SourceSamples==static_cast<int>(result.CoachSamples),
                "random source counter agreement across motion regimes");
        }
    }
    Require(VerifyDrivingCoachIntegrity(stats).AdviceAllowed,
        "large diverse source session remains internally consistent");
}
}
int main()
{
    EmptyAndSmallSessions();
    ContradictionsAreWithheld();
    InputSuspensionAndRecoveries();
    DeterministicRandomRegression();
    std::cout << "Driving coach cross-source integrity tests passed.\n";
    return EXIT_SUCCESS;
}
