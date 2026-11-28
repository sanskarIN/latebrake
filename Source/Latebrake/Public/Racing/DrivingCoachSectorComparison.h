#pragma once

#include "Racing/DrivingCoachSectorOverview.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// An observer of the same *gate interval* in distinct laps. This is only
// descriptive chassis exposure: the actual RaceProgress clock, penalties,
// road validity, pit service, track conditions and AI are not normalized.
inline constexpr double CoachSectorMinimumComparisonSeconds=1.0;
inline constexpr std::uint64_t CoachSectorMinimumComparisonFrames=12;

enum class CoachSectorDifference : std::uint8_t
{
    Similar,
    HigherInNewer,
    LowerInNewer
};

inline const char* CoachSectorDifferenceLabel(CoachSectorDifference kind) noexcept
{
    switch(kind)
    {
    case CoachSectorDifference::Similar:return "SIMILAR SAMPLE EXPOSURE";
    case CoachSectorDifference::HigherInNewer:return "HIGHER NEWER SAMPLE VALUE";
    case CoachSectorDifference::LowerInNewer:return "LOWER NEWER SAMPLE VALUE";
    default:return "NO SAMPLE DIFFERENCE";
    }
}

struct CoachSectorMetricChange final
{
    bool Valid=false;
    double EarlierValue=0.0;
    double NewerValue=0.0;
    double Difference=0.0;
    double RelativePercent=0.0;
    CoachSectorDifference Direction=CoachSectorDifference::Similar;
};

inline CoachSectorMetricChange MeasureCoachSectorChange(
    double earlier,double newer,double threshold) noexcept
{
    if(!std::isfinite(earlier) || !std::isfinite(newer) ||
        !std::isfinite(threshold) || earlier<0.0 || newer<0.0 ||
        threshold<=0.0)return {};
    const double difference=newer-earlier;
    const double percent=100.0*difference/std::max(0.01,earlier);
    if(!std::isfinite(difference) || !std::isfinite(percent))return {};
    return {true,earlier,newer,difference,percent,
        difference>threshold ? CoachSectorDifference::HigherInNewer :
        difference< -threshold ? CoachSectorDifference::LowerInNewer :
        CoachSectorDifference::Similar};
}

inline bool ValidCoachSectorMetricChange(CoachSectorMetricChange measure) noexcept
{
    return measure.Valid &&
        std::isfinite(measure.EarlierValue) && measure.EarlierValue>=0.0 &&
        std::isfinite(measure.NewerValue) && measure.NewerValue>=0.0 &&
        std::isfinite(measure.Difference) &&
        std::isfinite(measure.RelativePercent) &&
        std::abs(measure.NewerValue-measure.EarlierValue-
            measure.Difference)<1e-6 &&
        measure.Direction>=CoachSectorDifference::Similar &&
        measure.Direction<=CoachSectorDifference::LowerInNewer;
}

struct CoachSectorLapPair final
{
    bool Valid=false;
    int GateIndex=-1;
    int EarlierLap=-1;
    int NewerLap=-1;
    std::uint64_t EarlierSequence=0;
    std::uint64_t NewerSequence=0;
    double EarlierCoverage=0.0;
    double NewerCoverage=0.0;
    CoachSectorMetricChange MeanSpeedKmh{};
    CoachSectorMetricChange BrakingPercent{};
    CoachSectorMetricChange HighSlipPercent{};
    CoachSectorMetricChange ObservedDistanceKm{};
    bool OfficialLapComparison=false;
    bool GuaranteedSameTrackConditions=false;
    bool CertifiedImprovement=false;
    bool MayAwardCareerCredits=false;
};

inline bool ComparableCoachSectorSample(
    DrivingCoachSectorSample sample,int gateCount) noexcept
{
    return ValidDrivingCoachSectorSample(sample,gateCount) &&
        sample.ChassisCoverageFraction>=CoachSectorComparisonMinimumCoverage &&
        sample.ChassisSampleSeconds>=CoachSectorMinimumComparisonSeconds &&
        sample.SampledFrames>=CoachSectorMinimumComparisonFrames;
}

inline bool ValidCoachSectorLapPair(const CoachSectorLapPair& pair) noexcept
{
    return pair.Valid &&
        pair.GateIndex>=0 && pair.GateIndex<DrivingCoachMaximumGates &&
        pair.EarlierLap>=0 && pair.NewerLap>pair.EarlierLap &&
        pair.EarlierSequence>0 && pair.NewerSequence>pair.EarlierSequence &&
        std::isfinite(pair.EarlierCoverage) &&
        std::isfinite(pair.NewerCoverage) &&
        pair.EarlierCoverage>=CoachSectorComparisonMinimumCoverage &&
        pair.EarlierCoverage<=1.0 &&
        pair.NewerCoverage>=CoachSectorComparisonMinimumCoverage &&
        pair.NewerCoverage<=1.0 &&
        ValidCoachSectorMetricChange(pair.MeanSpeedKmh) &&
        ValidCoachSectorMetricChange(pair.BrakingPercent) &&
        ValidCoachSectorMetricChange(pair.HighSlipPercent) &&
        ValidCoachSectorMetricChange(pair.ObservedDistanceKm) &&
        !pair.OfficialLapComparison &&
        !pair.GuaranteedSameTrackConditions &&
        !pair.CertifiedImprovement &&
        !pair.MayAwardCareerCredits;
}

inline CoachSectorLapPair CompareCoachSectorSamples(
    DrivingCoachSectorSample earlier,DrivingCoachSectorSample newer,
    int gateCount) noexcept
{
    CoachSectorLapPair out;
    if(gateCount<1 || gateCount>DrivingCoachMaximumGates ||
       !ComparableCoachSectorSample(earlier,gateCount) ||
       !ComparableCoachSectorSample(newer,gateCount) ||
       earlier.SectorIndex!=newer.SectorIndex ||
       earlier.LapIndex>=newer.LapIndex ||
       earlier.Sequence>=newer.Sequence ||
       earlier.EndClockSeconds>=newer.StartClockSeconds)
        return out;
    out.GateIndex=earlier.SectorIndex;
    out.EarlierLap=earlier.LapIndex;
    out.NewerLap=newer.LapIndex;
    out.EarlierSequence=earlier.Sequence;
    out.NewerSequence=newer.Sequence;
    out.EarlierCoverage=earlier.ChassisCoverageFraction;
    out.NewerCoverage=newer.ChassisCoverageFraction;
    out.MeanSpeedKmh=MeasureCoachSectorChange(
        earlier.MeanSpeedKmh,newer.MeanSpeedKmh,3.0);
    out.BrakingPercent=MeasureCoachSectorChange(
        earlier.BrakingFraction*100.0,newer.BrakingFraction*100.0,4.0);
    out.HighSlipPercent=MeasureCoachSectorChange(
        earlier.HighSlipFraction*100.0,newer.HighSlipFraction*100.0,4.0);
    out.ObservedDistanceKm=MeasureCoachSectorChange(
        earlier.SampledDistanceKm,newer.SampledDistanceKm,0.015);
    out.Valid=true;
    return ValidCoachSectorLapPair(out)?out:CoachSectorLapPair{};
}

struct CoachSectorPairOverview final
{
    bool Valid=false;
    int GateCount=0;
    unsigned ComparableGates=0;
    unsigned ChangedSpeedGates=0;
    unsigned ChangedSlipGates=0;
    unsigned ChangedBrakingGates=0;
    int LargestSpeedChangeGate=-1;
    double LargestAbsoluteSpeedChange=0.0;
    std::array<CoachSectorLapPair,DrivingCoachMaximumGates> Gates{};
    bool NotAnOfficialRaceReview=true;
    bool DoesNotCertifySkillImprovement=true;
};

inline CoachSectorPairOverview BuildCoachSectorPairOverview(
    const DrivingCoachSectorJournal& journal) noexcept
{
    const auto review=journal.Review();
    CoachSectorPairOverview out;
    if(!review.Valid || review.GateCount<1 ||
       review.GateCount>DrivingCoachMaximumGates)return out;
    out.GateCount=review.GateCount;
    // A bounded ring, scanned newest-first; compare only the two newest
    // sufficiently covered observations in *different* actual lap ordinals.
    std::array<DrivingCoachSectorSample,DrivingCoachMaximumGates> newest{};
    std::array<DrivingCoachSectorSample,DrivingCoachMaximumGates> older{};
    for(std::size_t i=0;i<review.RetainedSectorSamples;++i)
    {
        DrivingCoachSectorSample sample;
        if(!journal.ReadNewest(i,sample) ||
           !ValidDrivingCoachSectorSample(sample,out.GateCount))return {};
        if(!ComparableCoachSectorSample(sample,out.GateCount))continue;
        const auto index=static_cast<std::size_t>(sample.SectorIndex);
        if(!newest[index].Valid)
        {
            newest[index]=sample;
            continue;
        }
        if(!older[index].Valid && sample.LapIndex!=newest[index].LapIndex)
            older[index]=sample;
    }
    for(int gate=0;gate<out.GateCount;++gate)
    {
        const auto index=static_cast<std::size_t>(gate);
        if(!newest[index].Valid || !older[index].Valid)continue;
        auto pair=CompareCoachSectorSamples(older[index],newest[index],out.GateCount);
        if(!ValidCoachSectorLapPair(pair))return {};
        out.Gates[index]=pair;
        ++out.ComparableGates;
        if(pair.MeanSpeedKmh.Direction!=CoachSectorDifference::Similar)
            ++out.ChangedSpeedGates;
        if(pair.HighSlipPercent.Direction!=CoachSectorDifference::Similar)
            ++out.ChangedSlipGates;
        if(pair.BrakingPercent.Direction!=CoachSectorDifference::Similar)
            ++out.ChangedBrakingGates;
        const double speedMagnitude=std::abs(pair.MeanSpeedKmh.Difference);
        if(speedMagnitude>out.LargestAbsoluteSpeedChange)
        {
            out.LargestAbsoluteSpeedChange=speedMagnitude;
            out.LargestSpeedChangeGate=gate;
        }
    }
    out.Valid=out.ComparableGates>0;
    return out;
}

inline bool ValidCoachSectorPairOverview(
    const CoachSectorPairOverview& review) noexcept
{
    if(!review.Valid || review.GateCount<1 ||
       review.GateCount>DrivingCoachMaximumGates ||
       review.ComparableGates==0 ||
       review.ComparableGates>static_cast<unsigned>(review.GateCount) ||
       review.ChangedSpeedGates>review.ComparableGates ||
       review.ChangedSlipGates>review.ComparableGates ||
       review.ChangedBrakingGates>review.ComparableGates ||
       review.LargestSpeedChangeGate>=review.GateCount ||
       !std::isfinite(review.LargestAbsoluteSpeedChange) ||
       review.LargestAbsoluteSpeedChange<0.0 ||
       !review.NotAnOfficialRaceReview ||
       !review.DoesNotCertifySkillImprovement)return false;
    unsigned count=0,speed=0,slip=0,brake=0;
    for(int i=0;i<review.GateCount;++i)
    {
        const auto& pair=review.Gates[static_cast<std::size_t>(i)];
        if(!pair.Valid)continue;
        if(!ValidCoachSectorLapPair(pair) || pair.GateIndex!=i)return false;
        ++count;
        if(pair.MeanSpeedKmh.Direction!=CoachSectorDifference::Similar)++speed;
        if(pair.HighSlipPercent.Direction!=CoachSectorDifference::Similar)++slip;
        if(pair.BrakingPercent.Direction!=CoachSectorDifference::Similar)++brake;
    }
    return count==review.ComparableGates &&
        speed==review.ChangedSpeedGates &&
        slip==review.ChangedSlipGates &&
        brake==review.ChangedBrakingGates;
}
} // namespace latebrake
