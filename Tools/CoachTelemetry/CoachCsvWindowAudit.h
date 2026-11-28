#pragma once

#include "CoachCsvQuality.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake::coachcsv
{
// Retained windows of a user-edited trace. Every interval comes directly
// from an accepted chassis sample, not a fabricated video or official lap.
// Ring eviction and suspension are disclosed: absence of an old window
// is not evidence that the old driving did not occur.
struct CoachCsvWindowRow final
{
    bool Valid=false;
    std::uint64_t Sequence=0;
    std::uint64_t ContinuityEpoch=0;
    bool Complete=false;
    double StartedSeconds=0.0;
    double EndedSeconds=0.0;
    double ObservedSeconds=0.0;
    double MeanSpeedKmh=0.0;
    double BrakingFraction=0.0;
    double HighSlipFraction=0.0;
    double EstimatedDistanceKm=0.0;
    std::uint64_t AcceptedSamples=0;
};

inline bool ValidCoachCsvWindowRow(const CoachCsvWindowRow& row) noexcept
{
    return row.Valid && row.Sequence!=0 && row.ContinuityEpoch!=0 &&
        row.AcceptedSamples!=0 &&
        std::isfinite(row.StartedSeconds) && row.StartedSeconds>=0.0 &&
        std::isfinite(row.EndedSeconds) &&
        row.EndedSeconds>row.StartedSeconds &&
        std::isfinite(row.ObservedSeconds) &&
        row.ObservedSeconds>0.0 &&
        row.ObservedSeconds<=DrivingCoachWindowSeconds+1.e-8 &&
        std::abs(row.EndedSeconds-row.StartedSeconds-row.ObservedSeconds)<=
            1.e-7*std::max(1.0,row.ObservedSeconds) &&
        std::isfinite(row.MeanSpeedKmh) &&
        row.MeanSpeedKmh>=0.0 && row.MeanSpeedKmh<=360.0+1.e-8 &&
        std::isfinite(row.BrakingFraction) &&
        row.BrakingFraction>=0.0 && row.BrakingFraction<=1.0 &&
        std::isfinite(row.HighSlipFraction) &&
        row.HighSlipFraction>=0.0 && row.HighSlipFraction<=1.0 &&
        std::isfinite(row.EstimatedDistanceKm) &&
        row.EstimatedDistanceKm>=0.0 &&
        row.EstimatedDistanceKm<=0.5+1.e-8 &&
        std::abs(row.EstimatedDistanceKm-
            row.ObservedSeconds*row.MeanSpeedKmh/3600.0)<=
                1.e-8*std::max(1.0,row.EstimatedDistanceKm);
}

struct CoachCsvWindowAudit final
{
    bool Valid=false;
    bool IncludesUnfinishedWindows=false;
    bool SomeOldWindowsNotRetained=false;
    bool ContainsContinuityBreaks=false;
    bool WindowsAreNewestFirst=true;
    bool SourceAuthenticityVerified=false;
    bool AuthenticLapTimingVerified=false;
    bool DrivesGamePhysics=false;
    bool WritesSaveData=false;
    std::uint64_t SourceSamples=0;
    std::uint64_t RetainedSamples=0;
    std::uint64_t OriginalSourceFingerprint=0;
    unsigned RetainedCount=0;
    unsigned CompleteCount=0;
    unsigned PartialCount=0;
    unsigned ObservedEpochChanges=0;
    double RetainedSeconds=0.0;
    double SourceObservedSeconds=0.0;
    double RetainedCoverageFraction=0.0;
    double EarliestRetainedSeconds=0.0;
    double LatestRetainedSeconds=0.0;
    std::array<CoachCsvWindowRow,DrivingCoachWindowCapacity> Rows{};
};

inline bool ValidCoachCsvWindowAudit(const CoachCsvWindowAudit& report) noexcept
{
    if(!report.Valid || !report.WindowsAreNewestFirst ||
       report.SourceAuthenticityVerified || report.AuthenticLapTimingVerified ||
       report.DrivesGamePhysics || report.WritesSaveData ||
       report.SourceSamples==0 || report.RetainedSamples==0 ||
       report.RetainedSamples>report.SourceSamples ||
       report.RetainedCount==0 ||
       report.RetainedCount>DrivingCoachWindowCapacity ||
       report.CompleteCount+report.PartialCount!=report.RetainedCount ||
       report.ObservedEpochChanges>=report.RetainedCount ||
       report.IncludesUnfinishedWindows!=(report.PartialCount>0) ||
       report.SomeOldWindowsNotRetained!=
            (report.RetainedSamples<report.SourceSamples) ||
       !std::isfinite(report.RetainedSeconds) ||
       !std::isfinite(report.SourceObservedSeconds) ||
       !std::isfinite(report.RetainedCoverageFraction) ||
       report.RetainedSeconds<=0.0 ||
       report.SourceObservedSeconds<report.RetainedSeconds-1.e-6 ||
       report.RetainedCoverageFraction<=0.0 ||
       report.RetainedCoverageFraction>1.0 ||
       !std::isfinite(report.EarliestRetainedSeconds) ||
       !std::isfinite(report.LatestRetainedSeconds) ||
       report.EarliestRetainedSeconds<0.0 ||
       report.LatestRetainedSeconds<=report.EarliestRetainedSeconds ||
       std::abs(report.RetainedCoverageFraction-
           report.RetainedSeconds/report.SourceObservedSeconds)>1.e-7)
        return false;
    std::uint64_t sumSamples=0;
    unsigned complete=0,partial=0,epochChanges=0;
    double sumSeconds=0.0;
    for(unsigned i=0;i<report.RetainedCount;++i)
    {
        const auto& row=report.Rows[i];
        if(!ValidCoachCsvWindowRow(row) ||
           row.AcceptedSamples>report.SourceSamples-sumSamples)
            return false;
        if(i>0)
        {
            const auto& newer=report.Rows[i-1u];
            if(newer.Sequence<=row.Sequence ||
               newer.ContinuityEpoch<row.ContinuityEpoch ||
               newer.StartedSeconds+1.e-7<row.EndedSeconds)
                return false;
            if(newer.ContinuityEpoch!=row.ContinuityEpoch)
                ++epochChanges;
        }
        sumSamples+=row.AcceptedSamples;
        sumSeconds+=row.ObservedSeconds;
        if(row.Complete)++complete;
        else ++partial;
    }
    return complete==report.CompleteCount &&
        partial==report.PartialCount &&
        epochChanges==report.ObservedEpochChanges &&
        report.ContainsContinuityBreaks==(epochChanges>0) &&
        sumSamples==report.RetainedSamples &&
        std::abs(sumSeconds-report.RetainedSeconds)<1.e-7 &&
        std::abs(report.EarliestRetainedSeconds-
            report.Rows[report.RetainedCount-1].StartedSeconds)<1.e-7 &&
        std::abs(report.LatestRetainedSeconds-
            report.Rows[0].EndedSeconds)<1.e-7;
}

inline CoachCsvWindowAudit BuildCoachCsvWindowAudit(
    const CsvAudit& source,const DrivingSessionStats& stats) noexcept
{
    CoachCsvWindowAudit report;
    const auto quality=BuildCoachCsvQualityReview(source,stats);
    if(!ValidCoachCsvQualityReview(quality,source,stats))return report;
    const auto review=stats.GetCoachReview();
    if(!review.Valid || review.RetainedWindows==0 ||
       review.RetainedWindows>DrivingCoachWindowCapacity ||
       review.AcceptedSamples!=source.AcceptedRows ||
       review.AcceptedSamples==0)return report;
    report.SourceSamples=review.AcceptedSamples;
    report.SourceObservedSeconds=review.ObservedSeconds;
    report.OriginalSourceFingerprint=source.Fnv64;
    report.RetainedCount=static_cast<unsigned>(review.RetainedWindows);
    for(unsigned i=0;i<report.RetainedCount;++i)
    {
        const auto& input=review.Windows[i];
        CoachCsvWindowRow row;
        row.Valid=input.Valid;
        row.Sequence=input.Sequence;
        row.ContinuityEpoch=input.ContinuityEpoch;
        row.Complete=input.Complete;
        row.StartedSeconds=input.StartedSeconds;
        row.EndedSeconds=input.EndedSeconds;
        row.ObservedSeconds=input.ObservedSeconds;
        row.MeanSpeedKmh=input.MeanSpeedKmh;
        row.BrakingFraction=input.BrakingFraction;
        row.HighSlipFraction=input.SlipFraction;
        row.EstimatedDistanceKm=input.DistanceKm;
        row.AcceptedSamples=input.Samples;
        if(!ValidCoachCsvWindowRow(row) ||
           row.AcceptedSamples>report.SourceSamples-report.RetainedSamples)
            return {};
        if(i>0)
        {
            const auto& newer=report.Rows[i-1];
            if(newer.Sequence<=row.Sequence ||
               newer.ContinuityEpoch<row.ContinuityEpoch ||
               newer.StartedSeconds+1.e-7<row.EndedSeconds)
                return {};
            if(newer.ContinuityEpoch!=row.ContinuityEpoch)
                ++report.ObservedEpochChanges;
        }
        report.Rows[i]=row;
        report.RetainedSamples+=row.AcceptedSamples;
        report.RetainedSeconds+=row.ObservedSeconds;
        if(row.Complete)++report.CompleteCount;
        else ++report.PartialCount;
    }
    report.EarliestRetainedSeconds=
        report.Rows[report.RetainedCount-1].StartedSeconds;
    report.LatestRetainedSeconds=report.Rows[0].EndedSeconds;
    report.IncludesUnfinishedWindows=report.PartialCount>0;
    report.SomeOldWindowsNotRetained=
        report.RetainedSamples<report.SourceSamples;
    report.ContainsContinuityBreaks=report.ObservedEpochChanges>0;
    report.RetainedCoverageFraction=report.RetainedSeconds/
        report.SourceObservedSeconds;
    if(report.RetainedCoverageFraction>1.0 &&
       report.RetainedCoverageFraction<=1.0+1.e-8)
        report.RetainedCoverageFraction=1.0;
    report.Valid=true;
    if(!ValidCoachCsvWindowAudit(report))return {};
    return report;
}
} // namespace latebrake::coachcsv
