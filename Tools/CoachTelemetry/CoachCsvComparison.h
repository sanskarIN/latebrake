#pragma once

#include "CoachCsvPolicy.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake::coachcsv
{
// Compare two explicitly user-supplied samples with the same *claimed*
// circuit/event/car/mode. No identity, build digest, road traversal or exact
// track/weather/AI settings are authenticated by this offline utility.
inline constexpr double MinimumComparisonObservedSeconds=15.0;
inline constexpr std::uint64_t MinimumComparisonSamples=120;
inline constexpr double MinimumComparisonTimeRatio=0.60;
inline constexpr double MaximumComparisonTimeRatio=1.80;

enum class ComparisonStatus : std::uint8_t
{
    ComparableObservations,
    InvalidInput,
    MissingContext,
    DifferentContext,
    InsufficientCoverage,
    DisproportionateObservation,
    InsufficientMovingEvidence
};

inline const char* ComparisonStatusLabel(ComparisonStatus status) noexcept
{
    switch(status)
    {
    case ComparisonStatus::ComparableObservations:
        return "SAMPLE DESCRIPTIONS COMPARABLE; OUTCOMES NOT CERTIFIED";
    case ComparisonStatus::InvalidInput:
        return "SOURCE OBSERVATION INVALID";
    case ComparisonStatus::MissingContext:
        return "BOTH FILES REQUIRE MATCHING USER-SUPPLIED CONTEXT";
    case ComparisonStatus::DifferentContext:
        return "CIRCUIT/EVENT/VEHICLE/MODE METADATA DIFFER";
    case ComparisonStatus::InsufficientCoverage:
        return "MORE OBSERVATION REQUIRED IN BOTH FILES";
    case ComparisonStatus::DisproportionateObservation:
        return "OBSERVED TIME COVERAGE DIFFERS TOO MUCH";
    case ComparisonStatus::InsufficientMovingEvidence:
        return "MOVING CHASSIS COVERAGE INSUFFICIENT";
    default:return "COMPARISON NOT AVAILABLE";
    }
}

enum class ComparisonDirection : std::uint8_t
{
    Similar,
    HigherInCandidate,
    LowerInCandidate
};

inline const char* ComparisonDirectionLabel(ComparisonDirection direction) noexcept
{
    switch(direction)
    {
    case ComparisonDirection::Similar:return "SIMILAR SAMPLED VALUE";
    case ComparisonDirection::HigherInCandidate:
        return "HIGHER SAMPLED VALUE IN SECOND FILE";
    case ComparisonDirection::LowerInCandidate:
        return "LOWER SAMPLED VALUE IN SECOND FILE";
    default:return "UNAVAILABLE";
    }
}

struct ComparisonMetric final
{
    bool Valid=false;
    double BaselineValue=0.0;
    double CandidateValue=0.0;
    double AbsoluteDifference=0.0;
    double RelativeDifferencePercent=0.0;
    ComparisonDirection Direction=ComparisonDirection::Similar;
};

struct CoachCsvComparison final
{
    bool Valid=false;
    ComparisonStatus Status=ComparisonStatus::InvalidInput;
    CsvSessionContext ClaimedContext{};
    double BaselineObservedSeconds=0.0;
    double CandidateObservedSeconds=0.0;
    std::uint64_t BaselineSamples=0;
    std::uint64_t CandidateSamples=0;
    ComparisonMetric MeanSpeedKmh{};
    ComparisonMetric BrakingTimePercent{};
    ComparisonMetric HighSlipTimePercent{};
    ComparisonMetric MovingTimePercent{};
    ComparisonMetric BrakeApplicationsPerMinute{};
    ComparisonMetric StrongSpeedChangesPerMinute{};
    bool SameAuthenticatedTrack=false;
    bool SameAuthenticatedCar=false;
    bool VerifiedCompetitiveImprovement=false;
    bool OfficialLapEvidence=false;
    bool AltersCareerRecords=false;
};

inline ComparisonMetric CompareSampledMetric(double before,double after,
    double meaningfulAbsoluteDifference) noexcept
{
    ComparisonMetric result;
    if(!std::isfinite(before) || !std::isfinite(after) ||
        !std::isfinite(meaningfulAbsoluteDifference) ||
        before<0.0 || after<0.0 || meaningfulAbsoluteDifference<=0.0)
        return result;
    result.Valid=true;
    result.BaselineValue=before;
    result.CandidateValue=after;
    result.AbsoluteDifference=after-before;
    result.RelativeDifferencePercent=100.0*(after-before)/
        std::max(0.01,std::abs(before));
    if(!std::isfinite(result.RelativeDifferencePercent) ||
       !std::isfinite(result.AbsoluteDifference))return {};
    result.Direction=result.AbsoluteDifference>meaningfulAbsoluteDifference ?
        ComparisonDirection::HigherInCandidate :
        result.AbsoluteDifference< -meaningfulAbsoluteDifference ?
            ComparisonDirection::LowerInCandidate :
            ComparisonDirection::Similar;
    return result;
}

inline bool SameClaimedContext(CsvSessionContext a,CsvSessionContext b) noexcept
{
    return ValidSessionContext(a) && ValidSessionContext(b) &&
        a.CircuitId==b.CircuitId && a.EventId==b.EventId &&
        a.VehicleId==b.VehicleId && a.Mode==b.Mode;
}

inline CoachCsvComparison CompareCoachCsvSessions(
    const CsvAudit& baseline,const DrivingSessionStats& baselineStats,
    const CsvAudit& candidate,const DrivingSessionStats& candidateStats) noexcept
{
    CoachCsvComparison result;
    if(!baseline.Valid || !candidate.Valid ||
       baseline.SourceIsAuthenticated || candidate.SourceIsAuthenticated ||
       baseline.SaveDataRead || candidate.SaveDataRead ||
       baseline.SaveDataWritten || candidate.SaveDataWritten ||
       !VerifyDrivingCoachIntegrity(baselineStats).Valid ||
       !VerifyDrivingCoachIntegrity(candidateStats).Valid ||
       !ValidDrivingCoachReadout(baselineStats.GetCoachReview()) ||
       !ValidDrivingCoachReadout(candidateStats.GetCoachReview()))
    {
        result.Status=ComparisonStatus::InvalidInput;
        return result;
    }
    if(!ValidSessionContext(baseline.Context) ||
       !ValidSessionContext(candidate.Context))
    {
        result.Status=ComparisonStatus::MissingContext;
        return result;
    }
    if(!SameClaimedContext(baseline.Context,candidate.Context))
    {
        result.Status=ComparisonStatus::DifferentContext;
        return result;
    }
    result.ClaimedContext=baseline.Context;
    const auto before=baselineStats.GetCoachReview();
    const auto after=candidateStats.GetCoachReview();
    result.BaselineObservedSeconds=before.ObservedSeconds;
    result.CandidateObservedSeconds=after.ObservedSeconds;
    result.BaselineSamples=before.AcceptedSamples;
    result.CandidateSamples=after.AcceptedSamples;
    if(before.ObservedSeconds<MinimumComparisonObservedSeconds ||
       after.ObservedSeconds<MinimumComparisonObservedSeconds ||
       before.AcceptedSamples<MinimumComparisonSamples ||
       after.AcceptedSamples<MinimumComparisonSamples ||
       before.Evidence==DrivingCoachEvidence::Limited ||
       after.Evidence==DrivingCoachEvidence::Limited)
    {
        result.Status=ComparisonStatus::InsufficientCoverage;
        return result;
    }
    const double durationRatio=after.ObservedSeconds/before.ObservedSeconds;
    if(durationRatio<MinimumComparisonTimeRatio ||
       durationRatio>MaximumComparisonTimeRatio)
    {
        result.Status=ComparisonStatus::DisproportionateObservation;
        return result;
    }
    // A pause or stopped car is still counted, but a mostly-stationary
    // session must not masquerade as a comparable racing run.
    if(before.MovingFraction<0.30 || after.MovingFraction<0.30)
    {
        result.Status=ComparisonStatus::InsufficientMovingEvidence;
        return result;
    }
    result.MeanSpeedKmh=CompareSampledMetric(
        before.MeanSpeedKmh,after.MeanSpeedKmh,3.0);
    result.BrakingTimePercent=CompareSampledMetric(
        before.BrakeFraction*100.0,after.BrakeFraction*100.0,2.0);
    result.HighSlipTimePercent=CompareSampledMetric(
        before.HighSlipFraction*100.0,after.HighSlipFraction*100.0,2.0);
    result.MovingTimePercent=CompareSampledMetric(
        before.MovingFraction*100.0,after.MovingFraction*100.0,2.0);
    const auto perMinute=[](std::uint64_t count,double seconds) noexcept
    {
        return 60.0*static_cast<double>(count)/seconds;
    };
    result.BrakeApplicationsPerMinute=CompareSampledMetric(
        perMinute(before.AcceptedBrakeEpisodes,before.ObservedSeconds),
        perMinute(after.AcceptedBrakeEpisodes,after.ObservedSeconds),1.0);
    result.StrongSpeedChangesPerMinute=CompareSampledMetric(
        perMinute(before.ObservedStrongSpeedChanges,before.ObservedSeconds),
        perMinute(after.ObservedStrongSpeedChanges,after.ObservedSeconds),2.0);
    if(!result.MeanSpeedKmh.Valid || !result.BrakingTimePercent.Valid ||
       !result.HighSlipTimePercent.Valid || !result.MovingTimePercent.Valid ||
       !result.BrakeApplicationsPerMinute.Valid ||
       !result.StrongSpeedChangesPerMinute.Valid)
    {
        result.Status=ComparisonStatus::InvalidInput;
        return result;
    }
    result.Valid=true;
    result.Status=ComparisonStatus::ComparableObservations;
    return result;
}

inline bool ValidCoachCsvComparison(const CoachCsvComparison& report) noexcept
{
    if(!report.Valid || report.Status!=ComparisonStatus::ComparableObservations ||
       !SameClaimedContext(report.ClaimedContext,report.ClaimedContext) ||
       report.SameAuthenticatedTrack || report.SameAuthenticatedCar ||
       report.VerifiedCompetitiveImprovement || report.OfficialLapEvidence ||
       report.AltersCareerRecords ||
       !std::isfinite(report.BaselineObservedSeconds) ||
       !std::isfinite(report.CandidateObservedSeconds) ||
       report.BaselineObservedSeconds<MinimumComparisonObservedSeconds ||
       report.CandidateObservedSeconds<MinimumComparisonObservedSeconds ||
       report.BaselineSamples<MinimumComparisonSamples ||
       report.CandidateSamples<MinimumComparisonSamples)
        return false;
    const ComparisonMetric metrics[]{report.MeanSpeedKmh,
        report.BrakingTimePercent,report.HighSlipTimePercent,
        report.MovingTimePercent,report.BrakeApplicationsPerMinute,
        report.StrongSpeedChangesPerMinute};
    for(const auto& entry:metrics)
    {
        if(!entry.Valid || !std::isfinite(entry.BaselineValue) ||
           !std::isfinite(entry.CandidateValue) ||
           !std::isfinite(entry.AbsoluteDifference) ||
           !std::isfinite(entry.RelativeDifferencePercent) ||
           entry.BaselineValue<0.0 || entry.CandidateValue<0.0 ||
           std::abs(entry.CandidateValue-entry.BaselineValue-
               entry.AbsoluteDifference)>1e-6)
            return false;
    }
    return true;
}
} // namespace latebrake::coachcsv
