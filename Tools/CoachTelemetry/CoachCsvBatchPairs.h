#pragma once

#include "CoachCsvBatch.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake::coachcsv
{
// Every pair of local source files is reviewed separately. A coarse all-file
// status must not conceal one incompatible or duplicated observation. This
// is NOT a similarity score for real players or a race ranking.
inline constexpr std::size_t MaximumCoachBatchPairs=
    MaximumCoachBatchFiles*(MaximumCoachBatchFiles-1)/2;

enum class CoachBatchPairStatus : std::uint8_t
{
    Unavailable,
    PossibleRepeatedInput,
    MissingClaimedContext,
    DifferentClaimedContext,
    InsufficientEvidence,
    CompatibleClaimsOnly
};

inline const char* CoachBatchPairStatusLabel(CoachBatchPairStatus status) noexcept
{
    switch(status)
    {
    case CoachBatchPairStatus::PossibleRepeatedInput:
        return "REPEATED INPUT HASH: MAY BE REUSED";
    case CoachBatchPairStatus::MissingClaimedContext:
        return "OPTIONAL USER-CLAIMED CONTEXT MISSING";
    case CoachBatchPairStatus::DifferentClaimedContext:
        return "USER-CLAIMED CONTEXT DIFFERENT";
    case CoachBatchPairStatus::InsufficientEvidence:
        return "INSUFFICIENT OR INTERRUPTED SAMPLING";
    case CoachBatchPairStatus::CompatibleClaimsOnly:
        return "MATCHING CLAIMS AND SUFFICIENT SAMPLES ONLY";
    default:
        return "PAIR UNAVAILABLE";
    }
}

struct CoachBatchPairEntry final
{
    bool Valid=false;
    unsigned First=0;
    unsigned Second=0;
    CoachBatchPairStatus Status=CoachBatchPairStatus::Unavailable;
    // Ratio of shorter to longer sampled duration, never race pace.
    double ObservedDurationCoverageRatio=0.0;
    bool BothExplicitlyEnded=false;
    bool BothHaveSameClaimedContext=false;
    bool IndependentSessionsVerified=false;
    bool OfficialRaceComparison=false;
};

struct CoachBatchPairReview final
{
    bool Valid=false;
    std::size_t SourceCount=0;
    unsigned PairCount=0;
    unsigned RepeatedInputPairs=0;
    unsigned MissingClaimPairs=0;
    unsigned DifferentClaimPairs=0;
    unsigned InsufficientPairs=0;
    unsigned CompatibleClaimPairs=0;
    std::array<CoachBatchPairEntry,MaximumCoachBatchPairs> Pairs{};
    bool AuthenticityEstablished=false;
    bool OfficialPerformanceRanked=false;
    bool CanModifyProfile=false;
};

inline bool ValidCoachBatchPair(const CoachBatchPairEntry& pair,
    std::size_t sourceCount) noexcept
{
    if(!pair.Valid || pair.First>=pair.Second || pair.Second>=sourceCount ||
       pair.Status==CoachBatchPairStatus::Unavailable ||
       !std::isfinite(pair.ObservedDurationCoverageRatio) ||
       pair.ObservedDurationCoverageRatio<=0.0 ||
       pair.ObservedDurationCoverageRatio>1.0 ||
       pair.IndependentSessionsVerified || pair.OfficialRaceComparison)
        return false;
    if(pair.Status==CoachBatchPairStatus::CompatibleClaimsOnly &&
       (!pair.BothHaveSameClaimedContext || !pair.BothExplicitlyEnded))
        return false;
    if(pair.Status==CoachBatchPairStatus::DifferentClaimedContext &&
       pair.BothHaveSameClaimedContext)
        return false;
    return true;
}

inline CoachBatchPairReview BuildCoachBatchPairReview(
    const CoachBatchReview& batch) noexcept
{
    CoachBatchPairReview result;
    if(!ValidCoachCsvBatchReview(batch))return result;
    result.SourceCount=batch.Count;
    for(std::size_t i=0;i<batch.Count;++i)
    {
        const auto& first=batch.Entries[i];
        for(std::size_t j=i+1;j<batch.Count;++j)
        {
            const auto& second=batch.Entries[j];
            if(result.PairCount>=result.Pairs.size())return {};
            CoachBatchPairEntry pair;
            pair.First=static_cast<unsigned>(i);
            pair.Second=static_cast<unsigned>(j);
            pair.BothExplicitlyEnded=
                first.EndMarkerPresent && second.EndMarkerPresent;
            pair.BothHaveSameClaimedContext=
                first.HasClaimedContext && second.HasClaimedContext &&
                SameClaimedContext(first.ClaimedContext,second.ClaimedContext);
            const double longer=std::max(first.ObservedSeconds,
                                          second.ObservedSeconds);
            pair.ObservedDurationCoverageRatio=
                std::min(first.ObservedSeconds,second.ObservedSeconds)/longer;
            if(first.Fingerprint==second.Fingerprint)
            {
                pair.Status=CoachBatchPairStatus::PossibleRepeatedInput;
                ++result.RepeatedInputPairs;
            }
            else if(!first.HasClaimedContext || !second.HasClaimedContext)
            {
                pair.Status=CoachBatchPairStatus::MissingClaimedContext;
                ++result.MissingClaimPairs;
            }
            else if(!pair.BothHaveSameClaimedContext)
            {
                pair.Status=CoachBatchPairStatus::DifferentClaimedContext;
                ++result.DifferentClaimPairs;
            }
            else if(!first.StructurallyUseful || !second.StructurallyUseful ||
                    !pair.BothExplicitlyEnded || first.Suspensions>0 ||
                    second.Suspensions>0 ||
                    pair.ObservedDurationCoverageRatio<0.3)
            {
                pair.Status=CoachBatchPairStatus::InsufficientEvidence;
                ++result.InsufficientPairs;
            }
            else
            {
                pair.Status=CoachBatchPairStatus::CompatibleClaimsOnly;
                ++result.CompatibleClaimPairs;
            }
            pair.Valid=true;
            if(!ValidCoachBatchPair(pair,batch.Count))return {};
            result.Pairs[result.PairCount++]=pair;
        }
    }
    result.Valid=true;
    return result;
}

inline bool ValidCoachBatchPairReview(const CoachBatchPairReview& review,
    const CoachBatchReview& batch) noexcept
{
    if(!review.Valid || !ValidCoachCsvBatchReview(batch) ||
       review.SourceCount!=batch.Count ||
       review.PairCount!=batch.Count*(batch.Count-1)/2 ||
       review.PairCount>MaximumCoachBatchPairs ||
       review.AuthenticityEstablished ||
       review.OfficialPerformanceRanked || review.CanModifyProfile ||
       review.RepeatedInputPairs!=batch.RepeatedFingerprintPairs ||
       review.RepeatedInputPairs+review.MissingClaimPairs+
       review.DifferentClaimPairs+review.InsufficientPairs+
       review.CompatibleClaimPairs!=review.PairCount)
        return false;
    unsigned expectedFirst=0;
    unsigned expectedSecond=1;
    unsigned repeated=0,missing=0,different=0,insufficient=0,compatible=0;
    for(unsigned index=0;index<review.PairCount;++index)
    {
        const auto& pair=review.Pairs[index];
        if(!ValidCoachBatchPair(pair,review.SourceCount) ||
           pair.First!=expectedFirst || pair.Second!=expectedSecond)
            return false;
        switch(pair.Status)
        {
        case CoachBatchPairStatus::PossibleRepeatedInput:++repeated;break;
        case CoachBatchPairStatus::MissingClaimedContext:++missing;break;
        case CoachBatchPairStatus::DifferentClaimedContext:++different;break;
        case CoachBatchPairStatus::InsufficientEvidence:++insufficient;break;
        case CoachBatchPairStatus::CompatibleClaimsOnly:++compatible;break;
        default:return false;
        }
        if(++expectedSecond==review.SourceCount)
        {
            ++expectedFirst;
            expectedSecond=expectedFirst+1;
        }
    }
    return repeated==review.RepeatedInputPairs &&
        missing==review.MissingClaimPairs &&
        different==review.DifferentClaimPairs &&
        insufficient==review.InsufficientPairs &&
        compatible==review.CompatibleClaimPairs;
}
} // namespace latebrake::coachcsv
