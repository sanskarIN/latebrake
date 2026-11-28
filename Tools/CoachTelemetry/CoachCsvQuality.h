#pragma once

#include "CoachCsvPolicy.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake::coachcsv
{
// Structural properties of *editable* QA files. These diagnostics never
// infer authenticity, fair play, actual track contact, or an official lap.
// A constant speed band is a low-diversity observation, not cheating proof.
inline constexpr std::size_t MaximumCsvQualityNotices=12;

enum class CoachCsvQualityNotice : std::uint8_t
{
    MissingEndMarker,
    NoClaimedContext,
    TooFewSamples,
    VeryShortObservation,
    InterruptedObservation,
    PredominantlyStationary,
    ExtremelyConcentratedSpeedBand,
    NarrowSampledSpeedRange,
    RepeatedHeavyBraking,
    SustainedHighLateralMotion,
    NoDetectedSampleLimitations
};

inline const char* CoachCsvQualityNoticeLabel(
    CoachCsvQualityNotice notice) noexcept
{
    switch(notice)
    {
    case CoachCsvQualityNotice::MissingEndMarker:
        return "EXPLICIT END MARKER ABSENT: FILE MAY BE TRUNCATED";
    case CoachCsvQualityNotice::NoClaimedContext:
        return "NO OPTIONAL USER-CLAIMED CIRCUIT/EVENT/VEHICLE/MODE";
    case CoachCsvQualityNotice::TooFewSamples:
        return "FEWER THAN 120 ACCEPTED CHASSIS SAMPLES";
    case CoachCsvQualityNotice::VeryShortObservation:
        return "LESS THAN 15 SECONDS OF CONTINUOUSLY SAMPLED TIME";
    case CoachCsvQualityNotice::InterruptedObservation:
        return "OBSERVATION CONTAINS PAUSES OR SUSPENSIONS";
    case CoachCsvQualityNotice::PredominantlyStationary:
        return "PREDOMINANTLY STATIONARY OBSERVATION";
    case CoachCsvQualityNotice::ExtremelyConcentratedSpeedBand:
        return "SAMPLED SPEED FALLS ALMOST ENTIRELY IN ONE BROAD BAND";
    case CoachCsvQualityNotice::NarrowSampledSpeedRange:
        return "ONLY ONE SPEED BAND REPRESENTED: CONTEXT IS LIMITED";
    case CoachCsvQualityNotice::RepeatedHeavyBraking:
        return "BRAKE SIGNAL OBSERVED IN OVER HALF OF SAMPLED TIME";
    case CoachCsvQualityNotice::SustainedHighLateralMotion:
        return "HIGH LATERAL MOTION OCCUPIED SUBSTANTIAL SAMPLE TIME";
    case CoachCsvQualityNotice::NoDetectedSampleLimitations:
        return "NO ADDITIONAL STRUCTURAL SAMPLE LIMITATION DETECTED";
    default:return "UNKNOWN CSV QUALITY NOTICE";
    }
}

enum class CoachCsvQualityCategory : std::uint8_t
{
    Invalid,
    Sparse,
    Interrupted,
    DescriptiveOnly
};

inline const char* CoachCsvQualityCategoryLabel(
    CoachCsvQualityCategory category) noexcept
{
    switch(category)
    {
    case CoachCsvQualityCategory::Invalid:return "INVALID SAMPLE SOURCE";
    case CoachCsvQualityCategory::Sparse:return "INSUFFICIENT OBSERVED CHASSIS COVERAGE";
    case CoachCsvQualityCategory::Interrupted:return "INTERRUPTED CHASSIS OBSERVATION";
    case CoachCsvQualityCategory::DescriptiveOnly:return "STRUCTURALLY USEFUL DESCRIPTIVE SAMPLES";
    default:return "QUALITY UNAVAILABLE";
    }
}

struct CoachCsvQualityReview final
{
    bool Valid=false;
    CoachCsvQualityCategory Category=CoachCsvQualityCategory::Invalid;
    std::array<CoachCsvQualityNotice,MaximumCsvQualityNotices> Notices{};
    unsigned NoticeCount=0;
    bool ExplicitEnd=false;
    bool ClaimedContext=false;
    bool HasUninterruptedObservation=false;
    bool HasEnoughFrames=false;
    bool HasEnoughObservedTime=false;
    std::uint64_t SampleCount=0;
    std::uint64_t SuspensionCount=0;
    double ObservedSeconds=0.0;
    double MovingShare=0.0;
    double HighSlipShare=0.0;
    double BrakingShare=0.0;
    double SpeedBandConcentration=0.0;
    double SpeedBandSimpsonDiversity=0.0;
    unsigned OccupiedSpeedBands=0;
    int DominantSpeedBand=-1;
    double DominantSpeedBandSeconds=0.0;
    bool SourceAuthenticated=false;
    bool PhysicsVerified=false;
    bool UnrealBuildVerified=false;
    bool RaceTimingVerified=false;
    bool IntegrityOfInstalledSaveVerified=false;
    bool CanProveRaceImprovement=false;
    bool CanAlterGameProgress=false;
};

inline bool ValidCoachCsvQualityNotice(
    CoachCsvQualityNotice notice) noexcept
{
    return notice>=CoachCsvQualityNotice::MissingEndMarker &&
        notice<=CoachCsvQualityNotice::NoDetectedSampleLimitations;
}

inline bool HasCoachCsvQualityNotice(const CoachCsvQualityReview& report,
    CoachCsvQualityNotice notice) noexcept
{
    for(unsigned i=0;i<report.NoticeCount;++i)
        if(report.Notices[i]==notice)return true;
    return false;
}

inline CoachCsvQualityReview BuildCoachCsvQualityReview(
    const CsvAudit& audit,const DrivingSessionStats& stats) noexcept
{
    CoachCsvQualityReview out;
    if(!audit.Valid || !audit.Integrity.Valid ||
       audit.SourceIsAuthenticated || audit.NativeUnrealVerified ||
       audit.PhysicsAuthorityChanged || audit.SaveDataRead ||
       audit.SaveDataWritten ||
       !VerifyDrivingCoachIntegrity(stats).Valid ||
       !ValidDrivingCoachReadout(stats.GetCoachReview()))return out;
    const auto distribution=stats.GetSpeedDistribution();
    if(!distribution.Valid || distribution.TotalFrames<2 ||
       static_cast<std::uint64_t>(distribution.TotalFrames)!=audit.AcceptedRows ||
       distribution.Rows.size()!=SampledSpeedBandCount)return out;
    const auto coach=stats.GetCoachReview();
    const double time=coach.ObservedSeconds;
    if(!std::isfinite(time) || time<=0.0)return out;
    out.SampleCount=audit.AcceptedRows;
    out.SuspensionCount=audit.SuspensionRows;
    out.ObservedSeconds=time;
    out.MovingShare=coach.MovingFraction;
    out.HighSlipShare=coach.HighSlipFraction;
    out.BrakingShare=coach.BrakeFraction;
    out.ExplicitEnd=audit.ExplicitEnd;
    out.ClaimedContext=ValidSessionContext(audit.Context);
    out.HasUninterruptedObservation=audit.SuspensionRows==0 &&
        coach.Suspensions==0;
    out.HasEnoughFrames=coach.AcceptedSamples>=120;
    out.HasEnoughObservedTime=time>=15.0;
    double squares=0.0;
    double largest=-1.0;
    for(std::size_t i=0;i<distribution.Rows.size();++i)
    {
        const auto& row=distribution.Rows[i];
        if(row.Seconds<=0.0)continue;
        ++out.OccupiedSpeedBands;
        const double fraction=row.Seconds/time;
        if(!std::isfinite(fraction) || fraction<0.0 || fraction>1.0+1e-7)
            return {};
        squares+=fraction*fraction;
        if(row.Seconds>largest)
        {
            largest=row.Seconds;
            out.DominantSpeedBand=static_cast<int>(i);
            out.DominantSpeedBandSeconds=row.Seconds;
        }
    }
    if(out.OccupiedSpeedBands==0 || out.DominantSpeedBand<0)return {};
    out.SpeedBandConcentration=largest/time;
    out.SpeedBandSimpsonDiversity=std::clamp(1.0-squares,0.0,1.0);
    const auto add=[&](CoachCsvQualityNotice notice) noexcept
    {
        if(out.NoticeCount>=out.Notices.size() ||
           HasCoachCsvQualityNotice(out,notice))return false;
        out.Notices[out.NoticeCount++]=notice;
        return true;
    };
    if(!out.ExplicitEnd && !add(CoachCsvQualityNotice::MissingEndMarker))return {};
    if(!out.ClaimedContext && !add(CoachCsvQualityNotice::NoClaimedContext))return {};
    if(!out.HasEnoughFrames && !add(CoachCsvQualityNotice::TooFewSamples))return {};
    if(!out.HasEnoughObservedTime &&
       !add(CoachCsvQualityNotice::VeryShortObservation))return {};
    if(!out.HasUninterruptedObservation &&
       !add(CoachCsvQualityNotice::InterruptedObservation))return {};
    if(out.MovingShare<0.30 &&
       !add(CoachCsvQualityNotice::PredominantlyStationary))return {};
    if(out.SpeedBandConcentration>=0.98 &&
       !add(CoachCsvQualityNotice::ExtremelyConcentratedSpeedBand))return {};
    if(out.OccupiedSpeedBands==1 &&
       !add(CoachCsvQualityNotice::NarrowSampledSpeedRange))return {};
    if(out.BrakingShare>0.50 &&
       !add(CoachCsvQualityNotice::RepeatedHeavyBraking))return {};
    if(out.HighSlipShare>=0.30 &&
       !add(CoachCsvQualityNotice::SustainedHighLateralMotion))return {};
    if(out.NoticeCount==0 &&
       !add(CoachCsvQualityNotice::NoDetectedSampleLimitations))return {};
    out.Category=(!out.HasEnoughFrames || !out.HasEnoughObservedTime ||
        out.MovingShare<0.30) ? CoachCsvQualityCategory::Sparse :
        !out.HasUninterruptedObservation ?
            CoachCsvQualityCategory::Interrupted :
            CoachCsvQualityCategory::DescriptiveOnly;
    out.Valid=true;
    return out;
}

inline bool ValidCoachCsvQualityReview(const CoachCsvQualityReview& value,
    const CsvAudit& audit,const DrivingSessionStats& stats) noexcept
{
    if(!value.Valid || value.Category==CoachCsvQualityCategory::Invalid ||
       value.NoticeCount==0 ||
       value.NoticeCount>MaximumCsvQualityNotices ||
       value.SampleCount!=audit.AcceptedRows ||
       value.SuspensionCount!=audit.SuspensionRows ||
       value.SourceAuthenticated || value.PhysicsVerified ||
       value.UnrealBuildVerified || value.RaceTimingVerified ||
       value.IntegrityOfInstalledSaveVerified ||
       value.CanProveRaceImprovement || value.CanAlterGameProgress ||
       !std::isfinite(value.ObservedSeconds) ||
       value.ObservedSeconds<=0.0 ||
       !std::isfinite(value.MovingShare) ||
       value.MovingShare<0.0 || value.MovingShare>1.0 ||
       !std::isfinite(value.SpeedBandConcentration) ||
       value.SpeedBandConcentration<0.0 ||
       value.SpeedBandConcentration>1.0 ||
       !std::isfinite(value.SpeedBandSimpsonDiversity) ||
       value.SpeedBandSimpsonDiversity<0.0 ||
       value.SpeedBandSimpsonDiversity>1.0 ||
       value.OccupiedSpeedBands==0 ||
       value.OccupiedSpeedBands>SampledSpeedBandCount ||
       value.DominantSpeedBand<0 ||
       value.DominantSpeedBand>=static_cast<int>(SampledSpeedBandCount))return false;
    for(unsigned i=0;i<value.NoticeCount;++i)
    {
        if(!ValidCoachCsvQualityNotice(value.Notices[i]))return false;
        for(unsigned j=0;j<i;++j)
            if(value.Notices[i]==value.Notices[j])return false;
    }
    const auto recomputed=BuildCoachCsvQualityReview(audit,stats);
    if(!recomputed.Valid || recomputed.NoticeCount!=value.NoticeCount ||
       recomputed.Category!=value.Category ||
       recomputed.ExplicitEnd!=value.ExplicitEnd ||
       recomputed.ClaimedContext!=value.ClaimedContext ||
       recomputed.OccupiedSpeedBands!=value.OccupiedSpeedBands ||
       recomputed.DominantSpeedBand!=value.DominantSpeedBand ||
       std::abs(recomputed.SpeedBandConcentration-
           value.SpeedBandConcentration)>1e-8 ||
       std::abs(recomputed.SpeedBandSimpsonDiversity-
           value.SpeedBandSimpsonDiversity)>1e-8)return false;
    for(unsigned i=0;i<value.NoticeCount;++i)
        if(value.Notices[i]!=recomputed.Notices[i])return false;
    return true;
}
} // namespace latebrake::coachcsv
