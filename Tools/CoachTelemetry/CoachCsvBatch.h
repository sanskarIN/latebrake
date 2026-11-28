#pragma once

#include "CoachCsvQuality.h"
#include "CoachCsvComparison.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::coachcsv
{
// Accept up to twelve separately validated, user-supplied CSV files. This
// is a bounded local QA study, not a leaderboard or a proof that those files
// came from separate native driving sessions or even the actual game.
inline constexpr std::size_t MaximumCoachBatchFiles=12;

enum class CoachBatchReviewStatus : std::uint8_t
{
    Unavailable,
    MissingContext,
    MixedContext,
    RepeatedInputFingerprint,
    InsufficientSampling,
    LimitedChassisMotion,
    DescriptiveOnly
};

inline const char* CoachBatchReviewStatusLabel(
    CoachBatchReviewStatus status) noexcept
{
    switch(status)
    {
    case CoachBatchReviewStatus::Unavailable:return "BATCH SOURCE UNAVAILABLE";
    case CoachBatchReviewStatus::MissingContext:
        return "BATCH CONTAINS FILES WITHOUT OPTIONAL USER-CLAIMED CONTEXT";
    case CoachBatchReviewStatus::MixedContext:
        return "BATCH FILES DECLARE DIFFERENT CIRCUIT/EVENT/CAR/MODE";
    case CoachBatchReviewStatus::RepeatedInputFingerprint:
        return "FILES SHARE INPUT FINGERPRINTS: REPEATED SOURCE MAY EXIST";
    case CoachBatchReviewStatus::InsufficientSampling:
        return "SOME FILES HAVE INSUFFICIENT CHASSIS SAMPLE COVERAGE";
    case CoachBatchReviewStatus::LimitedChassisMotion:
        return "SOME FILES LACK ADEQUATE MOVING CHASSIS EVIDENCE";
    case CoachBatchReviewStatus::DescriptiveOnly:
        return "OBSERVED METRICS MAY BE DESCRIBED, NOT RANKED AS RACES";
    default:return "BATCH SOURCE NOT REVIEWED";
    }
}

struct CoachBatchEntry final
{
    bool Valid=false;
    std::uint64_t Fingerprint=0;
    std::uint64_t Bytes=0;
    std::uint64_t AcceptedSamples=0;
    std::uint64_t Suspensions=0;
    bool HasClaimedContext=false;
    CsvSessionContext ClaimedContext{};
    bool EndMarkerPresent=false;
    bool StructurallyUseful=false;
    double ObservedSeconds=0.0;
    double MeanSpeedKmh=0.0;
    double BrakingShare=0.0;
    double HighSlipShare=0.0;
    double MovingShare=0.0;
    double SpeedDiversity=0.0;
    CoachCsvQualityCategory Quality=CoachCsvQualityCategory::Invalid;
};

struct CoachBatchReview final
{
    bool Valid=false;
    CoachBatchReviewStatus Status=CoachBatchReviewStatus::Unavailable;
    std::size_t Count=0;
    CsvSessionContext ClaimedContext{};
    bool AllHaveContext=false;
    bool AllClaimSameContext=false;
    bool AllProvideEndMarker=false;
    unsigned StructurallyUsefulFiles=0;
    unsigned InterruptedFiles=0;
    unsigned SparseFiles=0;
    unsigned MissingEndFiles=0;
    unsigned RepeatedFingerprintPairs=0;
    double MinimumObservedSeconds=0.0;
    double MaximumObservedSeconds=0.0;
    double MinimumMeanSpeedKmh=0.0;
    double MaximumMeanSpeedKmh=0.0;
    double MinimumBrakingShare=0.0;
    double MaximumBrakingShare=0.0;
    double MinimumHighSlipShare=0.0;
    double MaximumHighSlipShare=0.0;
    std::array<CoachBatchEntry,MaximumCoachBatchFiles> Entries{};
    bool EstablishesIndependentSessions=false;
    bool EstablishesAuthenticSourceFiles=false;
    bool EstablishesIdenticalRaceConditions=false;
    bool RanksOfficialRacePerformance=false;
    bool CanModifyGameProfile=false;
};

inline bool ValidCoachBatchEntry(CoachBatchEntry entry) noexcept
{
    return entry.Valid && entry.Bytes>0 &&
        entry.AcceptedSamples>0 &&
        std::isfinite(entry.ObservedSeconds) && entry.ObservedSeconds>0.0 &&
        std::isfinite(entry.MeanSpeedKmh) && entry.MeanSpeedKmh>=0.0 &&
        entry.MeanSpeedKmh<=360.0 &&
        std::isfinite(entry.BrakingShare) &&
        entry.BrakingShare>=0.0 && entry.BrakingShare<=1.0 &&
        std::isfinite(entry.HighSlipShare) &&
        entry.HighSlipShare>=0.0 && entry.HighSlipShare<=1.0 &&
        std::isfinite(entry.MovingShare) &&
        entry.MovingShare>=0.0 && entry.MovingShare<=1.0 &&
        std::isfinite(entry.SpeedDiversity) &&
        entry.SpeedDiversity>=0.0 && entry.SpeedDiversity<=1.0 &&
        entry.Quality!=CoachCsvQualityCategory::Invalid &&
        (!entry.HasClaimedContext || ValidSessionContext(entry.ClaimedContext)) &&
        !entry.ClaimedContext.SourceAuthenticated;
}

inline CoachBatchReview BuildCoachCsvBatch(
    const CsvAudit* audits,const DrivingSessionStats* stats,
    std::size_t count) noexcept
{
    CoachBatchReview result;
    if(!audits || !stats || count<2 || count>MaximumCoachBatchFiles)
        return result;
    result.Count=count;
    bool allContext=true;
    bool sameContext=true;
    bool allEnd=true;
    bool enoughSample=true;
    bool enoughMovement=true;
    for(std::size_t i=0;i<count;++i)
    {
        const auto quality=BuildCoachCsvQualityReview(audits[i],stats[i]);
        if(!ValidCoachCsvQualityReview(quality,audits[i],stats[i]))return {};
        const auto read=stats[i].GetCoachReview();
        CoachBatchEntry entry;
        entry.Valid=true;
        entry.Fingerprint=audits[i].Fnv64;
        entry.Bytes=audits[i].TotalBytes;
        entry.AcceptedSamples=audits[i].AcceptedRows;
        entry.Suspensions=audits[i].SuspensionRows;
        entry.HasClaimedContext=ValidSessionContext(audits[i].Context);
        entry.ClaimedContext=audits[i].Context;
        entry.EndMarkerPresent=audits[i].ExplicitEnd;
        entry.StructurallyUseful=quality.Category==
            CoachCsvQualityCategory::DescriptiveOnly;
        entry.ObservedSeconds=read.ObservedSeconds;
        entry.MeanSpeedKmh=read.MeanSpeedKmh;
        entry.BrakingShare=read.BrakeFraction;
        entry.HighSlipShare=read.HighSlipFraction;
        entry.MovingShare=read.MovingFraction;
        entry.SpeedDiversity=quality.SpeedBandSimpsonDiversity;
        entry.Quality=quality.Category;
        if(!ValidCoachBatchEntry(entry))return {};
        result.Entries[i]=entry;
        allContext=allContext && entry.HasClaimedContext;
        allEnd=allEnd && entry.EndMarkerPresent;
        enoughSample=enoughSample && quality.HasEnoughFrames &&
            quality.HasEnoughObservedTime;
        enoughMovement=enoughMovement && entry.MovingShare>=0.30;
        if(i==0)
        {
            result.ClaimedContext=audits[i].Context;
            result.MinimumObservedSeconds=result.MaximumObservedSeconds=
                entry.ObservedSeconds;
            result.MinimumMeanSpeedKmh=result.MaximumMeanSpeedKmh=
                entry.MeanSpeedKmh;
            result.MinimumBrakingShare=result.MaximumBrakingShare=
                entry.BrakingShare;
            result.MinimumHighSlipShare=result.MaximumHighSlipShare=
                entry.HighSlipShare;
        }
        else
        {
            if(!SameClaimedContext(audits[0].Context,audits[i].Context))
                sameContext=false;
            result.MinimumObservedSeconds=std::min(
                result.MinimumObservedSeconds,entry.ObservedSeconds);
            result.MaximumObservedSeconds=std::max(
                result.MaximumObservedSeconds,entry.ObservedSeconds);
            result.MinimumMeanSpeedKmh=std::min(
                result.MinimumMeanSpeedKmh,entry.MeanSpeedKmh);
            result.MaximumMeanSpeedKmh=std::max(
                result.MaximumMeanSpeedKmh,entry.MeanSpeedKmh);
            result.MinimumBrakingShare=std::min(
                result.MinimumBrakingShare,entry.BrakingShare);
            result.MaximumBrakingShare=std::max(
                result.MaximumBrakingShare,entry.BrakingShare);
            result.MinimumHighSlipShare=std::min(
                result.MinimumHighSlipShare,entry.HighSlipShare);
            result.MaximumHighSlipShare=std::max(
                result.MaximumHighSlipShare,entry.HighSlipShare);
        }
        if(entry.StructurallyUseful)++result.StructurallyUsefulFiles;
        if(entry.Quality==CoachCsvQualityCategory::Interrupted)
            ++result.InterruptedFiles;
        if(entry.Quality==CoachCsvQualityCategory::Sparse)
            ++result.SparseFiles;
        if(!entry.EndMarkerPresent)++result.MissingEndFiles;
        for(std::size_t older=0;older<i;++older)
            if(entry.Fingerprint==result.Entries[older].Fingerprint)
                ++result.RepeatedFingerprintPairs;
    }
    result.AllHaveContext=allContext;
    result.AllClaimSameContext=allContext && sameContext;
    result.AllProvideEndMarker=allEnd;
    result.Status=!allContext ? CoachBatchReviewStatus::MissingContext :
        !sameContext ? CoachBatchReviewStatus::MixedContext :
        result.RepeatedFingerprintPairs>0 ?
            CoachBatchReviewStatus::RepeatedInputFingerprint :
        !enoughSample ? CoachBatchReviewStatus::InsufficientSampling :
        !enoughMovement ? CoachBatchReviewStatus::LimitedChassisMotion :
            CoachBatchReviewStatus::DescriptiveOnly;
    result.Valid=true;
    return result;
}

inline bool ValidCoachCsvBatchReview(const CoachBatchReview& review) noexcept
{
    if(!review.Valid || review.Count<2 ||
       review.Count>MaximumCoachBatchFiles ||
       review.Status==CoachBatchReviewStatus::Unavailable ||
       review.StructurallyUsefulFiles>review.Count ||
       review.InterruptedFiles>review.Count ||
       review.SparseFiles>review.Count ||
       review.MissingEndFiles>review.Count ||
       review.RepeatedFingerprintPairs>review.Count*(review.Count-1)/2 ||
       review.EstablishesIndependentSessions ||
       review.EstablishesAuthenticSourceFiles ||
       review.EstablishesIdenticalRaceConditions ||
       review.RanksOfficialRacePerformance ||
       review.CanModifyGameProfile ||
       !std::isfinite(review.MinimumObservedSeconds) ||
       review.MinimumObservedSeconds<=0.0 ||
       !std::isfinite(review.MaximumObservedSeconds) ||
       review.MaximumObservedSeconds<review.MinimumObservedSeconds ||
       !std::isfinite(review.MinimumMeanSpeedKmh) ||
       review.MinimumMeanSpeedKmh<0.0 ||
       !std::isfinite(review.MaximumMeanSpeedKmh) ||
       review.MaximumMeanSpeedKmh<review.MinimumMeanSpeedKmh ||
       review.MaximumMeanSpeedKmh>360.0)return false;
    for(std::size_t i=0;i<review.Count;++i)
        if(!ValidCoachBatchEntry(review.Entries[i]))return false;
    if(review.AllClaimSameContext &&
       (!review.AllHaveContext || !ValidSessionContext(review.ClaimedContext)))
        return false;
    return true;
}
} // namespace latebrake::coachcsv
