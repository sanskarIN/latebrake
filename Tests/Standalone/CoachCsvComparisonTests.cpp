#include "../../Tools/CoachTelemetry/CoachCsvComparison.h"

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
void Require(bool v,const char* message)
{
    if(!v)
    {
        std::cerr << "CoachCsvComparison: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
std::string SampleCsv(int count=250,double speed=2000.0,
    double lateral=0.0,bool brake=false,std::string context="context,0,1,2,race")
{
    std::ostringstream s;
    s << Header << '\n';
    if(!context.empty())s << context << '\n';
    for(int i=0;i<count;++i)
        s << "sample,0.1," << speed+(i%5)*2 << ',' << lateral << ',' <<
            static_cast<unsigned>(brake && (i%9)<5) << '\n';
    s << "end,,,,\n";
    return s.str();
}
struct Fixture final
{
    DrivingSessionStats Stats;
    CsvAudit Audit{};
    explicit Fixture(const std::string& csv)
    {
        std::istringstream source(csv);
        Audit=AnalyzeCsv(source,Stats);
        Require(Audit.Valid,"valid source fixture");
    }
};
void MatchingMetadataAndMeasurements()
{
    Fixture initial(SampleCsv());
    Fixture next(SampleCsv(250,1900.0,1000.0,true));
    Require(ValidSessionContext(initial.Audit.Context) &&
        SameClaimedContext(initial.Audit.Context,next.Audit.Context),
        "matching explicit user-supplied context accepted");
    const auto review=CompareCoachCsvSessions(
        initial.Audit,initial.Stats,next.Audit,next.Stats);
    Require(ValidCoachCsvComparison(review),
        "sample-only comparison qualified");
    Require(review.MeanSpeedKmh.Direction==ComparisonDirection::LowerInCandidate &&
        review.HighSlipTimePercent.Direction==ComparisonDirection::HigherInCandidate &&
        review.BrakingTimePercent.Direction==ComparisonDirection::HigherInCandidate,
        "deltas come from measured source exposure, never subjective ratings");
    Require(review.BrakeApplicationsPerMinute.Valid &&
        review.StrongSpeedChangesPerMinute.Valid &&
        !review.SameAuthenticatedCar && !review.SameAuthenticatedTrack &&
        !review.OfficialLapEvidence && !review.VerifiedCompetitiveImprovement,
        "untrusted metadata cannot establish official best lap or skill");
}
void MissingAndDifferentContexts()
{
    Fixture initial(SampleCsv());
    Fixture missing(SampleCsv(250,1800,0,false,""));
    auto result=CompareCoachCsvSessions(initial.Audit,initial.Stats,
        missing.Audit,missing.Stats);
    Require(!result.Valid && result.Status==ComparisonStatus::MissingContext,
        "metadata required for cross-file comparison");
    for(const auto& tag:{"context,1,1,2,race", "context,0,2,2,race",
        "context,0,1,3,race", "context,0,1,2,practice"})
    {
        Fixture altered(SampleCsv(250,1800,0,false,tag));
        result=CompareCoachCsvSessions(initial.Audit,initial.Stats,
            altered.Audit,altered.Stats);
        Require(!result.Valid && result.Status==ComparisonStatus::DifferentContext,
            "circuit, event, car and mode must agree exactly");
    }
}
void InsufficientAndActivityBoundaries()
{
    Fixture initial(SampleCsv(250));
    Fixture shortRun(SampleCsv(100));
    auto comparison=CompareCoachCsvSessions(initial.Audit,initial.Stats,
        shortRun.Audit,shortRun.Stats);
    Require(!comparison.Valid &&
        comparison.Status==ComparisonStatus::InsufficientCoverage,
        "ten second observation too small to compare");
    Fixture tooLong(SampleCsv(900));
    comparison=CompareCoachCsvSessions(initial.Audit,initial.Stats,
        tooLong.Audit,tooLong.Stats);
    Require(!comparison.Valid &&
        comparison.Status==ComparisonStatus::DisproportionateObservation,
        "observed time mismatch must not masquerade as improvement");
    Fixture stationary(SampleCsv(250,0.0));
    comparison=CompareCoachCsvSessions(initial.Audit,initial.Stats,
        stationary.Audit,stationary.Stats);
    Require(!comparison.Valid &&
        comparison.Status==ComparisonStatus::InsufficientMovingEvidence,
        "mostly stationary telemetry has insufficient race motion");
    Fixture nearby(SampleCsv(220,2100));
    comparison=CompareCoachCsvSessions(initial.Audit,initial.Stats,
        nearby.Audit,nearby.Stats);
    Require(ValidCoachCsvComparison(comparison),
        "similar measured intervals support descriptive comparison");
}
void StrictContextParsing()
{
    for(const auto& good: {"context,0,0,0,practice", "context,3,3,5,race",
        "context,2,1,3,time"})
    {
        Fixture f(SampleCsv(10,1000,0,false,good));
        Require(ValidSessionContext(f.Audit.Context),"known bounded context");
    }
    for(const auto& bad: {"context,4,1,2,race", "context,-1,1,2,race",
        "context,0,4,2,race", "context,0,1,6,race",
        "context,0,01,2,race", "context,0,1,2,invalid",
        "context,0,1,2,Race", "context,0,1,2,race,extra"})
    {
        const auto data=SampleCsv(10,1000,0,false,bad);
        std::istringstream stream(data);
        DrivingSessionStats stats;
        const auto audit=AnalyzeCsv(stream,stats);
        Require(!audit.Valid &&
            (audit.Error==CsvError::InvalidContext ||
             audit.Error==CsvError::MissingFields),
             "malformed claimed context cannot pass silently");
    }
    auto dupe=std::string(Header)+"\ncontext,0,1,2,race\ncontext,0,1,2,race\n";
    std::istringstream stream(dupe);
    DrivingSessionStats stats;
    Require(AnalyzeCsv(stream,stats).Error==CsvError::DuplicateContext,
        "duplicate context rejected");
    const auto late=std::string(Header)+"\nsample,0.1,1200,0,0\ncontext,0,1,2,race\n";
    std::istringstream lateStream(late);
    Require(AnalyzeCsv(lateStream,stats).Error==CsvError::LateContext,
        "context after observation rejected");
}
void ForgedReportCannotAuthorizeComparison()
{
    Fixture initial(SampleCsv());
    Fixture later(SampleCsv(260));
    auto first=initial.Audit;
    first.SourceIsAuthenticated=true;
    Require(!CompareCoachCsvSessions(first,initial.Stats,later.Audit,later.Stats).Valid,
        "forged source authority must fail closed");
    first=initial.Audit;
    first.SaveDataRead=true;
    Require(!CompareCoachCsvSessions(first,initial.Stats,later.Audit,later.Stats).Valid,
        "offline input cannot claim save reads");
    auto result=CompareCoachCsvSessions(
        initial.Audit,initial.Stats,later.Audit,later.Stats);
    Require(ValidCoachCsvComparison(result),"original valid baseline");
    result.OfficialLapEvidence=true;
    Require(!ValidCoachCsvComparison(result),"forged official lap evidence rejected");
    result.OfficialLapEvidence=false;
    result.MeanSpeedKmh.CandidateValue=std::numeric_limits<double>::infinity();
    Require(!ValidCoachCsvComparison(result),"nonfinite spoofed metric rejected");
}
void RandomizedDeltasHaveStableDirection()
{
    std::mt19937 rng{0x3B9E019};
    std::uniform_real_distribution<double> values(0.0,100.0);
    for(int i=0;i<50000;++i)
    {
        const double a=values(rng),b=values(rng);
        const auto metric=CompareSampledMetric(a,b,1.0);
        Require(metric.Valid &&
            std::abs(metric.AbsoluteDifference-(b-a))<1e-9,
            "signed metric delta matches an independent arithmetic oracle");
        Require((b-a>1.0) ==
            (metric.Direction==ComparisonDirection::HigherInCandidate),
            "positive noticeable difference classified correctly");
        Require((b-a< -1.0) ==
            (metric.Direction==ComparisonDirection::LowerInCandidate),
            "negative noticeable difference classified correctly");
    }
    Require(!CompareSampledMetric(-1.0,3.0,1.0).Valid &&
        !CompareSampledMetric(1.0,std::numeric_limits<double>::quiet_NaN(),1.0).Valid &&
        !CompareSampledMetric(1.0,2.0,0.0).Valid,
        "invalid arithmetic evidence never labeled as improvement");
}
}
int main()
{
    MatchingMetadataAndMeasurements();
    MissingAndDifferentContexts();
    InsufficientAndActivityBoundaries();
    StrictContextParsing();
    ForgedReportCannotAuthorizeComparison();
    RandomizedDeltasHaveStableDirection();
    std::cout << "Coach CSV two-run comparison tests passed.\n";
    return EXIT_SUCCESS;
}
