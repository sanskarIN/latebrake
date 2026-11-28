#pragma once

#include "Racing/DrivingCoachSectorJournal.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Aggregate only witnessed, validated between-gate chassis observations.
// No record here is an official checkpoint split or a skill judgment.
inline constexpr double CoachSectorComparisonMinimumCoverage=0.90;
inline constexpr double CoachSectorContextMinimumSeconds=0.5;
inline constexpr std::uint64_t CoachSectorMinimumFrames=5;

enum class CoachSectorObservationQuality : std::uint8_t
{
    Unavailable,
    Sparse,
    PartiallySampled,
    Representative
};

inline const char* CoachSectorObservationQualityLabel(
    CoachSectorObservationQuality value) noexcept
{
    switch(value)
    {
    case CoachSectorObservationQuality::Unavailable:return "NO GATE-BOUND SAMPLE";
    case CoachSectorObservationQuality::Sparse:return "FEW CHASSIS SAMPLES";
    case CoachSectorObservationQuality::PartiallySampled:return "PARTIAL CHASSIS COVERAGE";
    case CoachSectorObservationQuality::Representative:return "REPEATED, WELL-SAMPLED CHASSIS INTERVAL";
    default:return "SAMPLE QUALITY UNAVAILABLE";
    }
}

enum class CoachSectorAdvisoryFocus : std::uint8_t
{
    ObserveMore,
    ReviewLateralControl,
    ReviewBrakingDistribution,
    ReviewLowMotion,
    StableSampledMotion
};

inline const char* CoachSectorAdvisoryFocusLabel(CoachSectorAdvisoryFocus focus) noexcept
{
    switch(focus)
    {
    case CoachSectorAdvisoryFocus::ObserveMore:
        return "COLLECT MORE CONTINUOUS CHASSIS SAMPLES";
    case CoachSectorAdvisoryFocus::ReviewLateralControl:
        return "REVIEW RECURRENT LATERAL SLIP IN THIS GATE INTERVAL";
    case CoachSectorAdvisoryFocus::ReviewBrakingDistribution:
        return "REVIEW BRAKING DISTRIBUTION IN THIS GATE INTERVAL";
    case CoachSectorAdvisoryFocus::ReviewLowMotion:
        return "OBSERVED A LOW SHARE OF CHASSIS MOVEMENT";
    case CoachSectorAdvisoryFocus::StableSampledMotion:
        return "OBSERVED CHASSIS MOTION LOOKS STEADY";
    default:return "NO VERIFIED COACH ADVICE";
    }
}

struct CoachSectorAggregate final
{
    bool Valid=false;
    int SectorIndex=-1;
    int LastObservedLap=-1;
    unsigned ObservedIntervals=0;
    unsigned RepresentativeIntervals=0;
    double SampledSeconds=0.0;
    double EstimatedSampledDistanceKm=0.0;
    double ActiveClockSecondsCovered=0.0;
    double WeightedMeanSpeedKmh=0.0;
    double WeightedBrakingFraction=0.0;
    double WeightedHighSlipFraction=0.0;
    double WeightedChassisCoverageFraction=0.0;
    double MinimumSampledSpeedKmh=0.0;
    double MaximumSampledSpeedKmh=0.0;
    double LargestObservedBrakingFraction=0.0;
    double LargestObservedSlipFraction=0.0;
    std::uint64_t TotalChassisFrames=0;
    CoachSectorObservationQuality Quality=CoachSectorObservationQuality::Unavailable;
    CoachSectorAdvisoryFocus Focus=CoachSectorAdvisoryFocus::ObserveMore;
    bool EquatesToOfficialSectorTime=false;
};

struct CoachSectorOverview final
{
    bool Valid=false;
    int GateCount=0;
    unsigned ObservedGates=0;
    unsigned RepresentativeGates=0;
    int SuggestedReviewGate=-1;
    int LeastCoveredGate=-1;
    double OverallSampledSeconds=0.0;
    double OverallSampledDistanceKm=0.0;
    double OverallBrakingFraction=0.0;
    double OverallSlipFraction=0.0;
    double OverallCoverageFraction=0.0;
    std::uint64_t TotalObservedIntervals=0;
    std::array<CoachSectorAggregate,DrivingCoachMaximumGates> Gates{};
    bool NoRaceAuthority=true;
    bool NoCompetitiveRanking=true;
    bool NoNativeSourceAuthenticity=true;
};

inline bool ValidCoachSectorAggregate(const CoachSectorAggregate& row,
    int expectedIndex) noexcept
{
    return row.Valid && row.SectorIndex==expectedIndex &&
        row.LastObservedLap>=0 && row.LastObservedLap<16000 &&
        row.ObservedIntervals>0 &&
        row.ObservedIntervals<=DrivingCoachSectorHistoryCapacity &&
        row.RepresentativeIntervals<=row.ObservedIntervals &&
        std::isfinite(row.SampledSeconds) && row.SampledSeconds>0.0 &&
        std::isfinite(row.EstimatedSampledDistanceKm) &&
        row.EstimatedSampledDistanceKm>=0.0 &&
        row.EstimatedSampledDistanceKm<=row.SampledSeconds*0.1+1e-5 &&
        std::isfinite(row.ActiveClockSecondsCovered) &&
        row.ActiveClockSecondsCovered>0.0 &&
        std::isfinite(row.WeightedMeanSpeedKmh) &&
        row.WeightedMeanSpeedKmh>=0.0 && row.WeightedMeanSpeedKmh<=360.0 &&
        std::abs(row.WeightedMeanSpeedKmh-
            row.EstimatedSampledDistanceKm*3600.0/row.SampledSeconds)<1e-4 &&
        std::isfinite(row.WeightedBrakingFraction) &&
        row.WeightedBrakingFraction>=0.0 && row.WeightedBrakingFraction<=1.0 &&
        std::isfinite(row.WeightedHighSlipFraction) &&
        row.WeightedHighSlipFraction>=0.0 &&
        row.WeightedHighSlipFraction<=1.0 &&
        std::isfinite(row.WeightedChassisCoverageFraction) &&
        row.WeightedChassisCoverageFraction>=0.0 &&
        row.WeightedChassisCoverageFraction<=1.0 &&
        std::isfinite(row.MinimumSampledSpeedKmh) &&
        row.MinimumSampledSpeedKmh>=0.0 &&
        row.MinimumSampledSpeedKmh<=row.MaximumSampledSpeedKmh &&
        row.MaximumSampledSpeedKmh<=360.0 &&
        std::isfinite(row.LargestObservedBrakingFraction) &&
        std::isfinite(row.LargestObservedSlipFraction) &&
        row.LargestObservedBrakingFraction>=0.0 &&
        row.LargestObservedBrakingFraction<=1.0 &&
        row.LargestObservedSlipFraction>=0.0 &&
        row.LargestObservedSlipFraction<=1.0 &&
        row.TotalChassisFrames>=row.ObservedIntervals &&
        !row.EquatesToOfficialSectorTime;
}

inline CoachSectorObservationQuality CoachSectorAssessQuality(
    const CoachSectorAggregate& row) noexcept
{
    if(!row.Valid)return CoachSectorObservationQuality::Unavailable;
    if(row.ObservedIntervals<2 || row.TotalChassisFrames<20 ||
       row.SampledSeconds<4.0)
        return CoachSectorObservationQuality::Sparse;
    if(row.RepresentativeIntervals<2 ||
       row.WeightedChassisCoverageFraction<CoachSectorComparisonMinimumCoverage)
        return CoachSectorObservationQuality::PartiallySampled;
    return CoachSectorObservationQuality::Representative;
}

inline CoachSectorAdvisoryFocus CoachSectorSelectFocus(
    const CoachSectorAggregate& row) noexcept
{
    if(row.Quality!=CoachSectorObservationQuality::Representative)
        return CoachSectorAdvisoryFocus::ObserveMore;
    if(row.WeightedHighSlipFraction>=0.18)
        return CoachSectorAdvisoryFocus::ReviewLateralControl;
    if(row.WeightedBrakingFraction>=0.40)
        return CoachSectorAdvisoryFocus::ReviewBrakingDistribution;
    if(row.WeightedMeanSpeedKmh<25.0)
        return CoachSectorAdvisoryFocus::ReviewLowMotion;
    return CoachSectorAdvisoryFocus::StableSampledMotion;
}

inline bool ValidCoachSectorOverview(const CoachSectorOverview& overview) noexcept
{
    if(!overview.Valid || overview.GateCount<1 ||
       overview.GateCount>DrivingCoachMaximumGates ||
       overview.ObservedGates>static_cast<unsigned>(overview.GateCount) ||
       overview.RepresentativeGates>overview.ObservedGates ||
       overview.TotalObservedIntervals==0 ||
       overview.TotalObservedIntervals>DrivingCoachSectorHistoryCapacity ||
       !overview.NoRaceAuthority || !overview.NoCompetitiveRanking ||
       !overview.NoNativeSourceAuthenticity ||
       !std::isfinite(overview.OverallSampledSeconds) ||
       overview.OverallSampledSeconds<=0.0 ||
       !std::isfinite(overview.OverallSampledDistanceKm) ||
       overview.OverallSampledDistanceKm<0.0 ||
       !std::isfinite(overview.OverallBrakingFraction) ||
       overview.OverallBrakingFraction<0.0 ||
       overview.OverallBrakingFraction>1.0 ||
       !std::isfinite(overview.OverallSlipFraction) ||
       overview.OverallSlipFraction<0.0 ||
       overview.OverallSlipFraction>1.0 ||
       !std::isfinite(overview.OverallCoverageFraction) ||
       overview.OverallCoverageFraction<0.0 ||
       overview.OverallCoverageFraction>1.0 ||
       overview.SuggestedReviewGate>=overview.GateCount ||
       overview.LeastCoveredGate>=overview.GateCount)return false;
    unsigned observed=0,representative=0;
    std::uint64_t intervals=0;
    for(int i=0;i<overview.GateCount;++i)
    {
        const auto& row=overview.Gates[static_cast<std::size_t>(i)];
        if(!row.Valid)continue;
        if(!ValidCoachSectorAggregate(row,i))return false;
        ++observed;
        if(row.Quality==CoachSectorObservationQuality::Representative)
            ++representative;
        intervals+=row.ObservedIntervals;
    }
    return observed==overview.ObservedGates &&
        representative==overview.RepresentativeGates &&
        intervals==overview.TotalObservedIntervals;
}

inline CoachSectorOverview BuildDrivingCoachSectorOverview(
    const DrivingCoachSectorJournal& journal) noexcept
{
    const auto source=journal.Review();
    CoachSectorOverview out;
    if(!source.Valid || source.GateCount<1 ||
       source.GateCount>DrivingCoachMaximumGates)return out;
    out.GateCount=source.GateCount;
    double totalBraking=0.0,totalSlipping=0.0,totalClock=0.0;
    std::array<double,DrivingCoachMaximumGates> brake{};
    std::array<double,DrivingCoachMaximumGates> slip{};
    std::array<double,DrivingCoachMaximumGates> weightedCoverage{};
    std::array<double,DrivingCoachMaximumGates> weightedClock{};
    for(std::size_t i=source.RetainedSectorSamples;i-- >0u;)
    {
        DrivingCoachSectorSample segment;
        if(!journal.ReadNewest(i,segment) ||
           !ValidDrivingCoachSectorSample(segment,out.GateCount))return {};
        auto& row=out.Gates[static_cast<std::size_t>(segment.SectorIndex)];
        if(!row.Valid)
        {
            row.Valid=true;
            row.SectorIndex=segment.SectorIndex;
            row.MinimumSampledSpeedKmh=segment.MeanSpeedKmh;
            row.MaximumSampledSpeedKmh=segment.MeanSpeedKmh;
        }
        row.LastObservedLap=std::max(row.LastObservedLap,segment.LapIndex);
        ++row.ObservedIntervals;
        if(segment.ChassisCoverageFraction>=CoachSectorComparisonMinimumCoverage &&
           segment.ChassisSampleSeconds>=CoachSectorContextMinimumSeconds &&
           segment.SampledFrames>=CoachSectorMinimumFrames)
            ++row.RepresentativeIntervals;
        row.SampledSeconds+=segment.ChassisSampleSeconds;
        row.EstimatedSampledDistanceKm+=segment.SampledDistanceKm;
        row.ActiveClockSecondsCovered+=segment.ClockIntervalSeconds;
        row.TotalChassisFrames+=segment.SampledFrames;
        row.MinimumSampledSpeedKmh=std::min(
            row.MinimumSampledSpeedKmh,segment.MeanSpeedKmh);
        row.MaximumSampledSpeedKmh=std::max(
            row.MaximumSampledSpeedKmh,segment.MeanSpeedKmh);
        row.LargestObservedBrakingFraction=std::max(
            row.LargestObservedBrakingFraction,segment.BrakingFraction);
        row.LargestObservedSlipFraction=std::max(
            row.LargestObservedSlipFraction,segment.HighSlipFraction);
        const auto gate=static_cast<std::size_t>(segment.SectorIndex);
        brake[gate]+=segment.BrakingSeconds;
        slip[gate]+=segment.HighSlipSeconds;
        weightedCoverage[gate]+=segment.ChassisCoverageFraction*
            segment.ClockIntervalSeconds;
        weightedClock[gate]+=segment.ClockIntervalSeconds;
        out.OverallSampledSeconds+=segment.ChassisSampleSeconds;
        out.OverallSampledDistanceKm+=segment.SampledDistanceKm;
        totalBraking+=segment.BrakingSeconds;
        totalSlipping+=segment.HighSlipSeconds;
        totalClock+=segment.ClockIntervalSeconds;
        ++out.TotalObservedIntervals;
    }
    double suggestedSeverity=-1.0;
    double lowestCoverage=2.0;
    for(int i=0;i<out.GateCount;++i)
    {
        auto& row=out.Gates[static_cast<std::size_t>(i)];
        if(!row.Valid)continue;
        row.WeightedMeanSpeedKmh=
            row.EstimatedSampledDistanceKm*3600.0/row.SampledSeconds;
        row.WeightedBrakingFraction=std::clamp(
            brake[static_cast<std::size_t>(i)]/row.SampledSeconds,0.0,1.0);
        row.WeightedHighSlipFraction=std::clamp(
            slip[static_cast<std::size_t>(i)]/row.SampledSeconds,0.0,1.0);
        row.WeightedChassisCoverageFraction=std::clamp(
            weightedCoverage[static_cast<std::size_t>(i)]/
            weightedClock[static_cast<std::size_t>(i)],0.0,1.0);
        row.Quality=CoachSectorAssessQuality(row);
        row.Focus=CoachSectorSelectFocus(row);
        if(!ValidCoachSectorAggregate(row,i))return {};
        ++out.ObservedGates;
        if(row.Quality==CoachSectorObservationQuality::Representative)
        {
            ++out.RepresentativeGates;
            const double severity=std::max(row.WeightedHighSlipFraction,
                0.6*row.WeightedBrakingFraction);
            if(row.Focus!=CoachSectorAdvisoryFocus::StableSampledMotion &&
               severity>suggestedSeverity)
            {
                suggestedSeverity=severity;
                out.SuggestedReviewGate=i;
            }
        }
        if(row.WeightedChassisCoverageFraction<lowestCoverage)
        {
            lowestCoverage=row.WeightedChassisCoverageFraction;
            out.LeastCoveredGate=i;
        }
    }
    if(out.OverallSampledSeconds<=0.0 || totalClock<=0.0)return {};
    out.OverallBrakingFraction=std::clamp(
        totalBraking/out.OverallSampledSeconds,0.0,1.0);
    out.OverallSlipFraction=std::clamp(
        totalSlipping/out.OverallSampledSeconds,0.0,1.0);
    out.OverallCoverageFraction=std::clamp(
        out.OverallSampledSeconds/totalClock,0.0,1.0);
    out.Valid=true;
    return ValidCoachSectorOverview(out)?out:CoachSectorOverview{};
}
} // namespace latebrake
