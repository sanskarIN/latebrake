#include "../../Tools/CoachTelemetry/CoachCsvBatch.h"

#include <array>
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
void Require(bool condition,const char* reason)
{
    if(!condition)
    {
        std::cerr << "CoachCsvBatch: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
std::string Data(int samples,int baseSpeed,int context=0,bool end=true,
    bool suspend=false)
{
    std::ostringstream stream;
    stream << Header << '\n';
    if(context>=0)
        stream << "context," << context << ",1,2,race\n";
    for(int i=0;i<samples;++i)
    {
        if(suspend && i==samples/2)stream << "suspend,,,,\n";
        const int speed=baseSpeed==0 ? 0 : std::max(0,baseSpeed+100*(i%8));
        stream << "sample,0.1," << speed << "," << 100*(i%4) << ","
            << (i%12==0 ? 1 : 0) << '\n';
    }
    if(end)stream << "end,,,,\n";
    return stream.str();
}
struct BatchFixture final
{
    std::array<DrivingSessionStats,MaximumCoachBatchFiles> Stats{};
    std::array<CsvAudit,MaximumCoachBatchFiles> Audits{};
    std::size_t Count=0;
    void Add(const std::string& body)
    {
        Require(Count<MaximumCoachBatchFiles,"bounded fixture");
        std::istringstream stream(body);
        Audits[Count]=AnalyzeCsv(stream,Stats[Count]);
        Require(Audits[Count].Valid,"self-contained source fixture valid");
        ++Count;
    }
    CoachBatchReview Review() const noexcept
    {
        return BuildCoachCsvBatch(Audits.data(),Stats.data(),Count);
    }
};
void TwoConsistentFilesHaveDescriptiveOnlyReport()
{
    BatchFixture group;
    group.Add(Data(250,1300));
    group.Add(Data(260,1700));
    const auto report=group.Review();
    Require(ValidCoachCsvBatchReview(report) &&
        report.Count==2 && report.AllHaveContext &&
        report.AllClaimSameContext && report.AllProvideEndMarker &&
        report.Status==CoachBatchReviewStatus::DescriptiveOnly &&
        report.StructurallyUsefulFiles==2 &&
        report.SparseFiles==0 && report.MissingEndFiles==0 &&
        report.RepeatedFingerprintPairs==0,
        "two eligible local sample logs produce bounded, descriptive QA");
    Require(report.MinimumMeanSpeedKmh<report.MaximumMeanSpeedKmh &&
        report.MinimumObservedSeconds<=report.MaximumObservedSeconds &&
        report.EstablishesIndependentSessions==false &&
        report.EstablishesAuthenticSourceFiles==false &&
        report.EstablishesIdenticalRaceConditions==false &&
        report.RanksOfficialRacePerformance==false &&
        report.CanModifyGameProfile==false,
        "sample differences can never prove official winning or real source identity");
}
void MissingOrMixedClaimsNeverQualifiedAsSameConditions()
{
    BatchFixture missing;
    missing.Add(Data(220,1500));
    missing.Add(Data(220,1600,-1));
    auto report=missing.Review();
    Require(ValidCoachCsvBatchReview(report) &&
        report.Status==CoachBatchReviewStatus::MissingContext &&
        !report.AllHaveContext && !report.AllClaimSameContext,
        "missing optional metadata prevents claimed-context alignment");
    BatchFixture mixed;
    mixed.Add(Data(220,1500));
    mixed.Add(Data(220,1600,1));
    report=mixed.Review();
    Require(ValidCoachCsvBatchReview(report) &&
        report.Status==CoachBatchReviewStatus::MixedContext &&
        report.AllHaveContext && !report.AllClaimSameContext,
        "distinct circuit claim prevents misleading cross-file comparison");
}
void DuplicateFingerprintsAreFlaggedAsPossibleInputReuse()
{
    BatchFixture same;
    same.Add(Data(200,1400));
    same.Add(Data(200,1400));
    same.Add(Data(200,1400));
    const auto report=same.Review();
    Require(ValidCoachCsvBatchReview(report) &&
        report.Status==CoachBatchReviewStatus::RepeatedInputFingerprint &&
        report.RepeatedFingerprintPairs==3,
        "reused local CSV bytes count possible repetitions not verified fraud");
    Require(!report.EstablishesIndependentSessions &&
        !report.RanksOfficialRacePerformance,
        "matching hashes cannot establish real engine origin");
}
void SparseAndStationaryFilesAreNotRanked()
{
    BatchFixture shortRun;
    shortRun.Add(Data(230,1600));
    shortRun.Add(Data(40,1400));
    auto report=shortRun.Review();
    Require(ValidCoachCsvBatchReview(report) &&
        report.Status==CoachBatchReviewStatus::InsufficientSampling &&
        report.SparseFiles==1,
        "short source not presented as a competitive ranking");
    BatchFixture slow;
    slow.Add(Data(220,1600));
    slow.Add(Data(230,0));
    report=slow.Review();
    Require(ValidCoachCsvBatchReview(report) &&
        report.Status==CoachBatchReviewStatus::LimitedChassisMotion &&
        !report.RanksOfficialRacePerformance,
        "mostly stationary source lacks descriptive moving evidence");
}
void FilesWithoutEndAndWithSuspensionsRemainTraceable()
{
    BatchFixture interrupted;
    interrupted.Add(Data(230,1600));
    interrupted.Add(Data(230,1900,0,false,true));
    const auto report=interrupted.Review();
    Require(ValidCoachCsvBatchReview(report) &&
        report.InterruptedFiles==1 && report.MissingEndFiles==1 &&
        report.StructurallyUsefulFiles==1 && !report.AllProvideEndMarker,
        "loss of end marker and source continuity are disclosed separately");
}
void RejectWrongArityInvalidAuditAndFakeMetrics()
{
    BatchFixture fixture;
    fixture.Add(Data(200,1300));
    fixture.Add(Data(200,1700));
    Require(!BuildCoachCsvBatch(nullptr,fixture.Stats.data(),2).Valid &&
        !BuildCoachCsvBatch(fixture.Audits.data(),nullptr,2).Valid &&
        !BuildCoachCsvBatch(fixture.Audits.data(),fixture.Stats.data(),1).Valid &&
        !BuildCoachCsvBatch(fixture.Audits.data(),fixture.Stats.data(),13).Valid,
        "the offline batch is explicitly bounded to 2-12 files");
    auto broken=fixture.Audits;
    broken[1].SourceIsAuthenticated=true;
    Require(!BuildCoachCsvBatch(broken.data(),fixture.Stats.data(),2).Valid,
        "fabricated authenticated source cannot pass structural analysis");
    auto report=fixture.Review();
    Require(ValidCoachCsvBatchReview(report),"honest baseline");
    report.RanksOfficialRacePerformance=true;
    Require(!ValidCoachCsvBatchReview(report),
        "offline batch cannot label sample range a race leaderboard");
    report=fixture.Review();
    report.Entries[0].MeanSpeedKmh=std::numeric_limits<double>::infinity();
    Require(!ValidCoachCsvBatchReview(report),
        "infinite fabricated average speed rejected");
}
void MaximumBatchRetainsEveryFileWithoutOverflow()
{
    BatchFixture fixture;
    for(int i=0;i<static_cast<int>(MaximumCoachBatchFiles);++i)
        fixture.Add(Data(160+i,1600+i*27));
    const auto report=fixture.Review();
    Require(ValidCoachCsvBatchReview(report) &&
        report.Count==MaximumCoachBatchFiles &&
        report.AllClaimSameContext &&
        report.RepeatedFingerprintPairs==0 &&
        report.MinimumObservedSeconds>0,
        "full bounded batch uses every independent structural source exactly once");
    std::uint64_t sourceCount=0;
    for(std::size_t i=0;i<report.Count;++i)
    {
        Require(ValidCoachBatchEntry(report.Entries[i]) &&
            report.Entries[i].AcceptedSamples==
                static_cast<std::uint64_t>(160+i),
            "each source retains its own sample count and provenance disclaimer");
        sourceCount+=report.Entries[i].AcceptedSamples;
    }
    Require(sourceCount==static_cast<std::uint64_t>(
        MaximumCoachBatchFiles*160+
        MaximumCoachBatchFiles*(MaximumCoachBatchFiles-1)/2),
        "file-level accepted sample counts match independent arithmetic oracle");
}
void SeededInputVariationChangesStudyBounds()
{
    std::mt19937 generator{0x55BA12D};
    for(int trial=0;trial<70;++trial)
    {
        BatchFixture fixture;
        const int count=2+static_cast<int>(generator()%8);
        for(int i=0;i<count;++i)
        {
            const int duration=160+static_cast<int>(generator()%50);
            const int speed=500+static_cast<int>(generator()%2600);
            fixture.Add(Data(duration,speed));
        }
        const auto report=fixture.Review();
        Require(ValidCoachCsvBatchReview(report) &&
            report.Count==static_cast<std::size_t>(count),
            "seeded diverse cohorts keep bounded source representation");
        double low=std::numeric_limits<double>::infinity();
        double high=0.0;
        for(int i=0;i<count;++i)
        {
            const double observed=fixture.Stats[static_cast<std::size_t>(i)].
                GetCoachReview().MeanSpeedKmh;
            low=std::min(low,observed);
            high=std::max(high,observed);
        }
        Require(std::abs(report.MinimumMeanSpeedKmh-low)<1e-6 &&
            std::abs(report.MaximumMeanSpeedKmh-high)<1e-6,
            "batch min/max reconciles independently observed sample means");
    }
}
}
int main()
{
    TwoConsistentFilesHaveDescriptiveOnlyReport();
    MissingOrMixedClaimsNeverQualifiedAsSameConditions();
    DuplicateFingerprintsAreFlaggedAsPossibleInputReuse();
    SparseAndStationaryFilesAreNotRanked();
    FilesWithoutEndAndWithSuspensionsRemainTraceable();
    RejectWrongArityInvalidAuditAndFakeMetrics();
    MaximumBatchRetainsEveryFileWithoutOverflow();
    SeededInputVariationChangesStudyBounds();
    std::cout << "Offline QA coach CSV batch summary tests passed.\n";
    return EXIT_SUCCESS;
}
