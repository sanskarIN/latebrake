#include "../../Tools/CoachTelemetry/CoachCsvBatchPairs.h"

#include <cstdlib>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

namespace
{
using namespace latebrake;
using namespace latebrake::coachcsv;
void Check(bool yes,const char* message)
{
    if(!yes)
    {
        std::cerr << "CoachCsvBatchPairs: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
std::string Make(int samples,int car=1,bool context=true,
                 bool end=true,bool interrupted=false)
{
    std::ostringstream stream;
    stream << Header << '\n';
    if(context)stream << "context,1,2," << car << ",race\n";
    for(int i=0;i<samples;++i)
    {
        if(interrupted && i==samples/2)stream << "suspend,,,,\n";
        stream << "sample,0.1," << 1400+10*(i%19) << ","
            << 60+10*(i%5) << "," << (i%13==0?1:0) << '\n';
    }
    if(end)stream << "end,,,,\n";
    return stream.str();
}
struct Fixture
{
    std::array<CsvAudit,MaximumCoachBatchFiles> Audits{};
    std::array<DrivingSessionStats,MaximumCoachBatchFiles> Stats{};
    std::size_t Count=0;
    void Add(const std::string& source)
    {
        Check(Count<MaximumCoachBatchFiles,"bounded fixture");
        std::istringstream in(source);
        Audits[Count]=AnalyzeCsv(in,Stats[Count]);
        Check(Audits[Count].Valid,"synthetic CSV fixture valid");
        ++Count;
    }
    CoachBatchReview Batch() const noexcept
    {
        return BuildCoachCsvBatch(Audits.data(),Stats.data(),Count);
    }
};
void TwoDescriptiveSourcesRemainOnlyClaims()
{
    Fixture f;
    f.Add(Make(210));f.Add(Make(251));
    const auto batch=f.Batch();
    const auto result=BuildCoachBatchPairReview(batch);
    Check(ValidCoachBatchPairReview(result,batch),"two sources pass strict pair consistency");
    Check(result.PairCount==1 && result.CompatibleClaimPairs==1 &&
          result.RepeatedInputPairs==0 &&
          result.Pairs[0].BothExplicitlyEnded &&
          result.Pairs[0].BothHaveSameClaimedContext &&
          result.Pairs[0].Status==CoachBatchPairStatus::CompatibleClaimsOnly,
          "matching claimed context never authenticates independent sessions");
    Check(!result.AuthenticityEstablished && !result.OfficialPerformanceRanked &&
          !result.CanModifyProfile &&
          !result.Pairs[0].IndependentSessionsVerified,
          "explicit absence of source/race authority");
}
void PairDecisionPriorityAndMissingContext()
{
    Fixture f;
    f.Add(Make(230));
    f.Add(Make(230)); // Exact replay: same hash, higher-priority repeated input.
    f.Add(Make(230,1,false));
    f.Add(Make(230,3));
    f.Add(Make(12,1,true,false));
    const auto batch=f.Batch();
    const auto result=BuildCoachBatchPairReview(batch);
    Check(ValidCoachBatchPairReview(result,batch) &&
          result.PairCount==10 && result.RepeatedInputPairs==1,
          "every distinct synthetic file pair has exactly one category");
    Check(result.Pairs[0].First==0 && result.Pairs[0].Second==1 &&
          result.Pairs[0].Status==CoachBatchPairStatus::PossibleRepeatedInput,
          "identical files are not treated as independent racing sessions");
    Check(result.Pairs[1].Status==CoachBatchPairStatus::MissingClaimedContext &&
          result.Pairs[2].Status==CoachBatchPairStatus::DifferentClaimedContext &&
          result.Pairs[3].Status==CoachBatchPairStatus::InsufficientEvidence,
          "missing, different, or short sources remain visible as separate problems");
    Check(result.CompatibleClaimPairs==0,
          "no unqualified source pair can masquerade as complete comparison");
}
void InterruptedAndUnfinishedCannotQualify()
{
    Fixture f;
    f.Add(Make(210));
    f.Add(Make(220,1,true,false,true));
    const auto batch=f.Batch();
    const auto result=BuildCoachBatchPairReview(batch);
    Check(ValidCoachBatchPairReview(result,batch) &&
          result.InsufficientPairs==1 &&
          !result.Pairs[0].BothExplicitlyEnded,
          "interruptions and missing end flags prevent comparable claims");
}
void MaximumSourceCountProducesBounded66Pairs()
{
    Fixture f;
    for(int i=0;i<12;++i)f.Add(Make(160+i));
    const auto batch=f.Batch();
    const auto result=BuildCoachBatchPairReview(batch);
    Check(ValidCoachBatchPairReview(result,batch),"all 66 pairs validated");
    Check(result.PairCount==66 && result.SourceCount==12 &&
          result.CompatibleClaimPairs==66 &&
          result.Pairs[0].First==0 && result.Pairs[0].Second==1 &&
          result.Pairs[65].First==10 && result.Pairs[65].Second==11,
          "stable lexicographic pair indexing without phantom comparisons");
    const auto forged=result;
    auto bad=forged;
    bad.Pairs[50].First=0;
    Check(!ValidCoachBatchPairReview(bad,batch),
          "out-of-order pair receipt rejected");
    bad=forged;
    bad.Pairs[3].ObservedDurationCoverageRatio=2.0;
    Check(!ValidCoachBatchPairReview(bad,batch),
          "impossible sample duration ratio rejected");
    bad=forged;
    bad.Pairs[10].OfficialRaceComparison=true;
    Check(!ValidCoachBatchPairReview(bad,batch),
          "fabricated real ranking flag rejected");
    bad=forged;
    ++bad.CompatibleClaimPairs;
    Check(!ValidCoachBatchPairReview(bad,batch),
          "counter mismatch fails rather than silently skewing source review");
}
void RandomizedPairsReconcileWithIndependentLoop()
{
    std::mt19937 gen{0x51CECAFE};
    for(int trial=0;trial<95;++trial)
    {
        Fixture f;
        const int count=2+static_cast<int>(gen()%11);
        for(int i=0;i<count;++i)
        {
            const int samples=120+static_cast<int>(gen()%90);
            const int car=static_cast<int>(gen()%6);
            f.Add(Make(samples,car,gen()%7!=0,gen()%7!=0,gen()%8==0));
        }
        const auto batch=f.Batch();
        const auto review=BuildCoachBatchPairReview(batch);
        Check(ValidCoachBatchPairReview(review,batch) &&
              review.PairCount==static_cast<unsigned>(count*(count-1)/2),
              "randomly varied bounded sources always account for every pair");
        for(unsigned index=0;index<review.PairCount;++index)
        {
            const auto& pair=review.Pairs[index];
            const auto& a=batch.Entries[pair.First];
            const auto& b=batch.Entries[pair.Second];
            const double expected=std::min(a.ObservedSeconds,b.ObservedSeconds)/
                std::max(a.ObservedSeconds,b.ObservedSeconds);
            Check(std::abs(pair.ObservedDurationCoverageRatio-expected)<1.0e-12,
                  "duration ratio agrees with independent file-level oracle");
            Check(pair.Status!=CoachBatchPairStatus::CompatibleClaimsOnly ||
                  (a.StructurallyUseful && b.StructurallyUseful &&
                   a.EndMarkerPresent && b.EndMarkerPresent &&
                   pair.BothHaveSameClaimedContext),
                  "compatible claim category requires every coverage condition");
        }
    }
}
}
int main()
{
    TwoDescriptiveSourcesRemainOnlyClaims();
    PairDecisionPriorityAndMissingContext();
    InterruptedAndUnfinishedCannotQualify();
    MaximumSourceCountProducesBounded66Pairs();
    RandomizedPairsReconcileWithIndependentLoop();
    std::cout << "Batch pairwise source classification tests passed.\n";
    return EXIT_SUCCESS;
}
