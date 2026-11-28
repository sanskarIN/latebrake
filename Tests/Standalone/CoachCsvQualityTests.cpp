#include "../../Tools/CoachTelemetry/CoachCsvQuality.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>

namespace
{
using namespace latebrake;
using namespace latebrake::coachcsv;
void Require(bool ok,const char* why)
{
    if(!ok)
    {
        std::cerr << "CoachCsvQuality: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
struct Fixture final
{
    DrivingSessionStats Stats;
    CsvAudit Audit;
    explicit Fixture(const std::string& text)
    {
        std::istringstream input(text);
        Audit=AnalyzeCsv(input,Stats);
        Require(Audit.Valid,"source fixture accepted before separate quality review");
    }
};
std::string Trace(int count=220,int mode=0,
    bool explicitEnd=true,bool context=true,bool interruptions=false)
{
    std::ostringstream out;
    out << Header << '\n';
    if(context)out << "context,0,1,2,race\n";
    for(int i=0;i<count;++i)
    {
        if(interruptions && i>0 && i%47==0)
            out << "suspend,,,,\n";
        const int speed=mode==1?0:
            mode==2?1800:
            mode==3?1000+(i%7)*1300:
            1500+(i%5)*600;
        const int lateral=mode==4?1000:100;
        const int brake=mode==5?1:i%19<6?1:0;
        out << "sample,0.10," << speed << ',' << lateral << ',' << brake << '\n';
    }
    if(explicitEnd)out << "end,,,,\n";
    return out.str();
}
void ValidStructuredReportIsClearlyNotNativeProof()
{
    Fixture f(Trace());
    const auto quality=BuildCoachCsvQualityReview(f.Audit,f.Stats);
    Require(ValidCoachCsvQualityReview(quality,f.Audit,f.Stats),
        "read-only quality report can verify its own structural consistency");
    Require(quality.Valid && quality.ExplicitEnd && quality.ClaimedContext &&
        quality.SampleCount==220 && quality.SuspensionCount==0 &&
        quality.HasEnoughFrames && quality.HasEnoughObservedTime &&
        quality.HasUninterruptedObservation &&
        quality.Category==CoachCsvQualityCategory::DescriptiveOnly &&
        !quality.PhysicsVerified && !quality.RaceTimingVerified &&
        !quality.CanProveRaceImprovement && !quality.CanAlterGameProgress,
        "many samples do not establish engine or competitive authenticity");
    Require(quality.SpeedBandConcentration>0 &&
        quality.SpeedBandConcentration<=1.0 &&
        quality.SpeedBandSimpsonDiversity>0 &&
        quality.SpeedBandSimpsonDiversity<1.0,
        "source speed partition yields bounded descriptive diversity");
}
void SparseAndUnfinishedLogsExplainUncertainty()
{
    Fixture few(Trace(20,0,false,false));
    const auto review=BuildCoachCsvQualityReview(few.Audit,few.Stats);
    Require(ValidCoachCsvQualityReview(review,few.Audit,few.Stats) &&
        review.Category==CoachCsvQualityCategory::Sparse &&
        HasCoachCsvQualityNotice(review,CoachCsvQualityNotice::TooFewSamples) &&
        HasCoachCsvQualityNotice(review,CoachCsvQualityNotice::VeryShortObservation) &&
        HasCoachCsvQualityNotice(review,CoachCsvQualityNotice::MissingEndMarker) &&
        HasCoachCsvQualityNotice(review,CoachCsvQualityNotice::NoClaimedContext),
        "truncated, context-free sample clearly discloses independent weaknesses");
    Fixture one(Trace(1));
    Require(!BuildCoachCsvQualityReview(one.Audit,one.Stats).Valid,
        "one frame lacks an independent speed distribution for stronger report");
}
void InterruptedAndStationarySourceDisclosesLimitations()
{
    Fixture interrupted(Trace(220,0,true,true,true));
    const auto a=BuildCoachCsvQualityReview(interrupted.Audit,interrupted.Stats);
    Require(ValidCoachCsvQualityReview(a,interrupted.Audit,interrupted.Stats) &&
        a.Category==CoachCsvQualityCategory::Interrupted &&
        a.SuspensionCount==4 && !a.HasUninterruptedObservation &&
        HasCoachCsvQualityNotice(a,CoachCsvQualityNotice::InterruptedObservation),
        "explicit discontinuity cannot become a continuous source session");
    Fixture stationary(Trace(220,1));
    const auto b=BuildCoachCsvQualityReview(stationary.Audit,stationary.Stats);
    Require(ValidCoachCsvQualityReview(b,stationary.Audit,stationary.Stats) &&
        b.Category==CoachCsvQualityCategory::Sparse &&
        HasCoachCsvQualityNotice(b,CoachCsvQualityNotice::PredominantlyStationary) &&
        HasCoachCsvQualityNotice(b,CoachCsvQualityNotice::NarrowSampledSpeedRange),
        "standing still is visible but never assigned an illegitimate penalty");
}
void OneSpeedBandIsNotEvidenceOfFraud()
{
    Fixture steady(Trace(200,2));
    const auto review=BuildCoachCsvQualityReview(steady.Audit,steady.Stats);
    Require(ValidCoachCsvQualityReview(review,steady.Audit,steady.Stats) &&
        review.OccupiedSpeedBands==1 &&
        std::abs(review.SpeedBandConcentration-1.0)<1e-9 &&
        std::abs(review.SpeedBandSimpsonDiversity)<1e-9 &&
        HasCoachCsvQualityNotice(review,
            CoachCsvQualityNotice::ExtremelyConcentratedSpeedBand) &&
        HasCoachCsvQualityNotice(review,
            CoachCsvQualityNotice::NarrowSampledSpeedRange) &&
        !review.CanProveRaceImprovement,
        "constant speed is descriptive limited coverage, not cheat detection");
}
void RepeatedBrakingAndLateralSlipAreAdvisoryOnly()
{
    Fixture braking(Trace(240,5));
    const auto a=BuildCoachCsvQualityReview(braking.Audit,braking.Stats);
    Require(ValidCoachCsvQualityReview(a,braking.Audit,braking.Stats) &&
        HasCoachCsvQualityNotice(a,CoachCsvQualityNotice::RepeatedHeavyBraking),
        "source brake exposure is described without changing driving controls");
    Fixture slip(Trace(240,4));
    const auto b=BuildCoachCsvQualityReview(slip.Audit,slip.Stats);
    Require(ValidCoachCsvQualityReview(b,slip.Audit,slip.Stats) &&
        HasCoachCsvQualityNotice(b,CoachCsvQualityNotice::SustainedHighLateralMotion) &&
        b.HighSlipShare>=0.30 && !b.CanAlterGameProgress,
        "source lateral motion does not authorize a penalty or tuning change");
}
void SpoofedReportsFailValidation()
{
    Fixture original(Trace());
    const auto baseline=BuildCoachCsvQualityReview(original.Audit,original.Stats);
    Require(ValidCoachCsvQualityReview(baseline,original.Audit,original.Stats),
        "honest baseline report self consistent");
    auto modified=baseline;
    modified.SourceAuthenticated=true;
    Require(!ValidCoachCsvQualityReview(modified,original.Audit,original.Stats),
        "forged installed-game provenance cannot pass quality gate");
    modified=baseline;
    modified.SpeedBandConcentration+=0.2;
    Require(!ValidCoachCsvQualityReview(modified,original.Audit,original.Stats),
        "fabricated dominant band concentration rejected");
    modified=baseline;
    modified.SampleCount+=12;
    Require(!ValidCoachCsvQualityReview(modified,original.Audit,original.Stats),
        "sample count inflation cannot pass audit concordance");
    modified=baseline;
    modified.Notices[0]=static_cast<CoachCsvQualityNotice>(255);
    Require(!ValidCoachCsvQualityReview(modified,original.Audit,original.Stats),
        "invalid notice enum cannot appear in trusted tool output");
}
void SeededRandomizedSampleDistribution()
{
    std::mt19937 random{0x3B19D5};
    for(int iteration=0;iteration<80;++iteration)
    {
        std::ostringstream text;
        text << Header << "\ncontext,3,0,4,practice\n";
        const int size=160+static_cast<int>(random()%120u);
        for(int i=0;i<size;++i)
        {
            const int velocity=static_cast<int>(random()%9000u);
            const int lateral=static_cast<int>(random()%1500u)-750;
            const int brake=static_cast<int>(random()%2u);
            text << "sample,0.1," << velocity << ',' << lateral << ',' << brake << '\n';
        }
        text << "end,,,,\n";
        Fixture sample(text.str());
        const auto report=BuildCoachCsvQualityReview(sample.Audit,sample.Stats);
        Require(ValidCoachCsvQualityReview(report,sample.Audit,sample.Stats) &&
            report.SpeedBandSimpsonDiversity>=0.0 &&
            report.SpeedBandSimpsonDiversity<=1.0 &&
            report.SpeedBandConcentration>=1.0/SampledSpeedBandCount &&
            report.SpeedBandConcentration<=1.0 &&
            report.SampleCount==static_cast<std::uint64_t>(size),
            "randomized observed speed partitions conserve input sample count");
    }
}
}
int main()
{
    ValidStructuredReportIsClearlyNotNativeProof();
    SparseAndUnfinishedLogsExplainUncertainty();
    InterruptedAndStationarySourceDisclosesLimitations();
    OneSpeedBandIsNotEvidenceOfFraud();
    RepeatedBrakingAndLateralSlipAreAdvisoryOnly();
    SpoofedReportsFailValidation();
    SeededRandomizedSampleDistribution();
    std::cout << "Coach CSV quality evidence tests passed.\n";
    return EXIT_SUCCESS;
}
