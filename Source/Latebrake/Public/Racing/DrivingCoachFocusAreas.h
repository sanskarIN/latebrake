#pragma once

#include "Racing/DrivingCoachReview.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// Shortlist of *observed* windows worth reviewing. This is not driver
// ranking or a claim that braking/slip is inherently bad; circuit shape,
// traffic, wet grip and deliberate maneuvers are not modelled here.
inline constexpr std::size_t DrivingCoachMaximumFocusAreas=6;
inline constexpr double DrivingCoachMinimumFocusWindowSeconds=3.50;
inline constexpr std::uint64_t DrivingCoachMinimumFocusWindowSamples=20;

enum class DrivingCoachFocusKind : std::uint8_t
{
    None,
    HighSampledSlip,
    LargeSampledBrakeShare,
    LowerObservedMeanSpeed,
    ChangesBetweenAdjacentWindows
};

inline const char* DrivingCoachFocusLabel(DrivingCoachFocusKind kind) noexcept
{
    switch(kind)
    {
    case DrivingCoachFocusKind::HighSampledSlip:
        return "REVIEW LATERAL MOTION IN THIS SAMPLE WINDOW";
    case DrivingCoachFocusKind::LargeSampledBrakeShare:
        return "REVIEW BRAKING TIME IN THIS SAMPLE WINDOW";
    case DrivingCoachFocusKind::LowerObservedMeanSpeed:
        return "REVIEW LOW MEAN SPEED (MAY INCLUDE SLOW CORNERS)";
    case DrivingCoachFocusKind::ChangesBetweenAdjacentWindows:
        return "REVIEW CHANGES BETWEEN ADJACENT SAMPLE WINDOWS";
    default:return "NO QUALIFIED FOCUS WINDOW";
    }
}

struct DrivingCoachFocusArea final
{
    bool Valid=false;
    DrivingCoachFocusKind Kind=DrivingCoachFocusKind::None;
    std::uint64_t Sequence=0;
    std::uint64_t ContinuityEpoch=0;
    double StartedSeconds=0.0;
    double EndedSeconds=0.0;
    double ObservedSeconds=0.0;
    double MeanSpeedKmh=0.0;
    double BrakingFraction=0.0;
    double SlipFraction=0.0;
    double Severity=0.0;
    std::uint64_t Samples=0;
    bool BasedOnPhysicalEvidenceOnly=true;
    bool OfficialSector=false;
    bool RaceOutcomeChanged=false;
};

struct DrivingCoachFocusReview final
{
    bool Valid=false;
    std::array<DrivingCoachFocusArea,DrivingCoachMaximumFocusAreas> Areas{};
    std::size_t Count=0;
    std::uint64_t EligibleWindows=0;
    std::uint64_t SkippedIncompleteWindows=0;
    std::uint64_t SkippedShortWindows=0;
    std::uint64_t SkippedLowEvidenceWindows=0;
    double QualifyingSeconds=0.0;
    bool NoRewards=true;
    bool NoOfficialTiming=true;
};

inline bool ValidDrivingCoachFocusArea(const DrivingCoachFocusArea& area) noexcept
{
    if(!area.Valid || area.Kind==DrivingCoachFocusKind::None ||
        area.Sequence==0 || area.ContinuityEpoch==0 || area.Samples==0 ||
        !area.BasedOnPhysicalEvidenceOnly || area.OfficialSector ||
        area.RaceOutcomeChanged ||
        !std::isfinite(area.StartedSeconds) ||
        !std::isfinite(area.EndedSeconds) ||
        !std::isfinite(area.ObservedSeconds) ||
        !std::isfinite(area.MeanSpeedKmh) ||
        !std::isfinite(area.BrakingFraction) ||
        !std::isfinite(area.SlipFraction) ||
        !std::isfinite(area.Severity) ||
        area.StartedSeconds<0.0 || area.EndedSeconds<=area.StartedSeconds ||
        area.ObservedSeconds<DrivingCoachMinimumFocusWindowSeconds ||
        area.ObservedSeconds>DrivingCoachWindowSeconds+1e-7 ||
        area.MeanSpeedKmh<0.0 || area.MeanSpeedKmh>360.0 ||
        area.BrakingFraction<0.0 || area.BrakingFraction>1.0 ||
        area.SlipFraction<0.0 || area.SlipFraction>1.0 ||
        area.Severity<0.0 || area.Severity>100.0)
        return false;
    switch(area.Kind)
    {
    case DrivingCoachFocusKind::HighSampledSlip:
        return area.SlipFraction>=0.14;
    case DrivingCoachFocusKind::LargeSampledBrakeShare:
        return area.BrakingFraction>=0.40;
    case DrivingCoachFocusKind::LowerObservedMeanSpeed:
        return area.MeanSpeedKmh<=35.0;
    case DrivingCoachFocusKind::ChangesBetweenAdjacentWindows:
        return area.Severity>=15.0;
    default:return false;
    }
}

inline bool DrivingCoachFocusPreferred(
    const DrivingCoachFocusArea& candidate,
    const DrivingCoachFocusArea& current) noexcept
{
    if(candidate.Severity!=current.Severity)
        return candidate.Severity>current.Severity;
    if(candidate.ObservedSeconds!=current.ObservedSeconds)
        return candidate.ObservedSeconds>current.ObservedSeconds;
    if(candidate.Sequence!=current.Sequence)
        return candidate.Sequence>current.Sequence;
    return static_cast<int>(candidate.Kind)<static_cast<int>(current.Kind);
}

inline DrivingCoachFocusArea ClassifyDrivingCoachWindow(
    const DrivingCoachWindow& window,
    const DrivingCoachWindow* previous) noexcept
{
    if(!ValidDrivingCoachWindow(window) || !window.Complete ||
        window.ObservedSeconds<DrivingCoachMinimumFocusWindowSeconds ||
        window.Samples<DrivingCoachMinimumFocusWindowSamples)
        return {};
    DrivingCoachFocusArea out;
    out.Valid=true;
    out.Sequence=window.Sequence;
    out.ContinuityEpoch=window.ContinuityEpoch;
    out.StartedSeconds=window.StartedSeconds;
    out.EndedSeconds=window.EndedSeconds;
    out.ObservedSeconds=window.ObservedSeconds;
    out.Samples=window.Samples;
    out.MeanSpeedKmh=window.MeanSpeedKmh;
    out.BrakingFraction=window.BrakingFraction;
    out.SlipFraction=window.SlipFraction;
    const double slip=window.SlipFraction;
    const double braking=window.BrakingFraction;
    if(slip>=0.14)
    {
        out.Kind=DrivingCoachFocusKind::HighSampledSlip;
        out.Severity=std::clamp(100.0*slip,0.0,100.0);
    }
    if(braking>=0.40)
    {
        const double severity=std::clamp(85.0*braking,0.0,100.0);
        if(out.Kind==DrivingCoachFocusKind::None || severity>out.Severity)
        {
            out.Kind=DrivingCoachFocusKind::LargeSampledBrakeShare;
            out.Severity=severity;
        }
    }
    // Low mean speed is informative only in fully observed windows; never
    // infer stall, position, offroad or substandard circuit performance.
    if(window.MeanSpeedKmh<=35.0)
    {
        const double severity=(35.0-window.MeanSpeedKmh)*0.70+15.0;
        if(out.Kind==DrivingCoachFocusKind::None || severity>out.Severity)
        {
            out.Kind=DrivingCoachFocusKind::LowerObservedMeanSpeed;
            out.Severity=severity;
        }
    }
    // A pair must be from the exact same uninterrupted period. Sample
    // speeds are not compared across recovery, focus loss or paused time.
    if(previous && ValidDrivingCoachWindow(*previous) && previous->Complete &&
        previous->ContinuityEpoch==window.ContinuityEpoch &&
        previous->Sequence+1==window.Sequence &&
        previous->ObservedSeconds>=DrivingCoachMinimumFocusWindowSeconds &&
        previous->Samples>=DrivingCoachMinimumFocusWindowSamples &&
        window.MeanSpeedKmh>=40.0 && previous->MeanSpeedKmh>=40.0)
    {
        const double delta=std::abs(window.MeanSpeedKmh-
            previous->MeanSpeedKmh)/std::max(window.MeanSpeedKmh,
                previous->MeanSpeedKmh);
        if(delta>=0.20)
        {
            const double severity=std::clamp(100.0*delta,0.0,100.0);
            if(out.Kind==DrivingCoachFocusKind::None || severity>out.Severity)
            {
                out.Kind=DrivingCoachFocusKind::ChangesBetweenAdjacentWindows;
                out.Severity=severity;
            }
        }
    }
    if(out.Kind==DrivingCoachFocusKind::None) return {};
    return ValidDrivingCoachFocusArea(out) ? out : DrivingCoachFocusArea{};
}

inline DrivingCoachFocusReview BuildDrivingCoachFocusAreas(
    const DrivingCoachReadout& readout) noexcept
{
    DrivingCoachFocusReview result;
    if(!ValidDrivingCoachReadout(readout))return result;
    result.Valid=true;
    // Retained windows are newest-first; walk earliest-to-latest so that
    // temporal comparison is only between actual adjacent source records.
    const DrivingCoachWindow* previous=nullptr;
    for(std::size_t offset=readout.RetainedWindows;offset-- >0;)
    {
        const auto& window=readout.Windows[offset];
        if(!window.Complete)
        {
            ++result.SkippedIncompleteWindows;
            previous=nullptr;
            continue;
        }
        if(window.ObservedSeconds<DrivingCoachMinimumFocusWindowSeconds)
        {
            ++result.SkippedShortWindows;
            previous=nullptr;
            continue;
        }
        if(window.Samples<DrivingCoachMinimumFocusWindowSamples)
        {
            ++result.SkippedLowEvidenceWindows;
            previous=nullptr;
            continue;
        }
        ++result.EligibleWindows;
        result.QualifyingSeconds+=window.ObservedSeconds;
        const auto choice=ClassifyDrivingCoachWindow(window,previous);
        previous=&window;
        if(!choice.Valid) continue;
        std::size_t index=0;
        while(index<result.Count &&
            !DrivingCoachFocusPreferred(choice,result.Areas[index]))++index;
        if(index>=result.Areas.size()) continue;
        const std::size_t end=std::min(result.Count,
            result.Areas.size()-1);
        for(std::size_t at=end;at>index;--at)
            result.Areas[at]=result.Areas[at-1];
        result.Areas[index]=choice;
        if(result.Count<result.Areas.size())++result.Count;
    }
    return result;
}

inline bool ValidDrivingCoachFocusReview(
    const DrivingCoachFocusReview& review) noexcept
{
    if(!review.Valid || !review.NoRewards || !review.NoOfficialTiming ||
        review.Count>review.Areas.size() ||
        !std::isfinite(review.QualifyingSeconds) ||
        review.QualifyingSeconds<0.0 ||
        review.QualifyingSeconds>DrivingCoachWindowCapacity*
            DrivingCoachWindowSeconds+1e-6 ||
        review.Count>review.EligibleWindows)
        return false;
    for(std::size_t i=0;i<review.Count;++i)
    {
        if(!ValidDrivingCoachFocusArea(review.Areas[i]))return false;
        if(i>0 && DrivingCoachFocusPreferred(review.Areas[i],review.Areas[i-1]))
            return false;
        for(std::size_t j=0;j<i;++j)
            if(review.Areas[j].Sequence==review.Areas[i].Sequence)return false;
    }
    return true;
}
} // namespace latebrake
