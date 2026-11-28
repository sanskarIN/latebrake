#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Bounded game-AI courtesy brake for a nearby moving player entering
// the rival's forward footprint. It uses measured planar relative
// motion, not a collision solver or a steering path. In particular,
// similar longitudinal speeds do not rule out a lateral merge.
struct RivalNearMergeObservation final
{
    double ForwardGapCm = 0.0;
    double LateralGapCm = 0.0;
    double RelativeForwardCmPerSecond = 0.0; // player minus rival
    double RelativeLateralCmPerSecond = 0.0; // player minus rival
    double RivalForwardCmPerSecond = 0.0;
};

struct RivalNearMergeDecision final
{
    bool Valid = false;
    bool Brake = false;
    double FirstOverlapSeconds = 0.0;
    double LastOverlapSeconds = 0.0;
};

namespace rival_near_merge_detail
{
inline constexpr double HorizonSeconds = 0.40;
inline constexpr double ForwardFootprintCm = 280.0;
inline constexpr double LateralFootprintCm = 175.0;
inline constexpr double ExistingLaneCorridorCm = 225.0;
inline constexpr double MaximumLocalGapCm = 2500.0;
inline constexpr double MaximumRelativeSpeedCmPerSecond = 30000.0;

struct TimeInterval final
{
    double Begin = 0.0;
    double End = HorizonSeconds;
};

// Clip a time interval to a slab under constant observed velocity.
// Inputs have already been bounded, avoiding division or overflow
// on malformed world coordinates. Stationary axes only constrain
// the interval if they are outside the requested slab.
inline bool ClipAxis(double position,double velocity,double low,double high,
    TimeInterval& interval) noexcept
{
    if (velocity==0.0)
        return position>=low && position<=high;
    // First reject a slab that cannot be touched during the bounded
    // interval, even if dividing by a subnormal velocity would overflow.
    const double atBegin=position+velocity*interval.Begin;
    const double atEnd=position+velocity*interval.End;
    if (std::max(atBegin,atEnd)<low || std::min(atBegin,atEnd)>high)
        return false;
    if (velocity>0.0)
    {
        if (atBegin<low)
            interval.Begin=std::max(interval.Begin,(low-position)/velocity);
        if (atEnd>high)
            interval.End=std::min(interval.End,(high-position)/velocity);
    }
    else
    {
        if (atBegin>high)
            interval.Begin=std::max(interval.Begin,(high-position)/velocity);
        if (atEnd<low)
            interval.End=std::min(interval.End,(low-position)/velocity);
    }
    return std::isfinite(interval.Begin) && std::isfinite(interval.End) &&
        interval.Begin<=interval.End;
}
} // namespace rival_near_merge_detail

inline RivalNearMergeDecision EvaluateRivalNearMerge(
    RivalNearMergeObservation sample) noexcept
{
    using namespace rival_near_merge_detail;
    RivalNearMergeDecision result;
    if (!std::isfinite(sample.ForwardGapCm) ||
        !std::isfinite(sample.LateralGapCm) ||
        !std::isfinite(sample.RelativeForwardCmPerSecond) ||
        !std::isfinite(sample.RelativeLateralCmPerSecond) ||
        !std::isfinite(sample.RivalForwardCmPerSecond) ||
        std::abs(sample.RelativeForwardCmPerSecond)>MaximumRelativeSpeedCmPerSecond ||
        std::abs(sample.RelativeLateralCmPerSecond)>MaximumRelativeSpeedCmPerSecond ||
        std::abs(sample.RivalForwardCmPerSecond)>15000.0)
        return result;

    result.Valid=true;
    const double lateral=std::abs(sample.LateralGapCm);
    // Existing following logic owns the current lane. This extension
    // only considers a player outside that lane and moving INWARD.
    // It cannot brake for a player behind, a distant crossing, parallel
    // lanes, outward motion or a sideways/reversing rival.
    if (sample.ForwardGapCm<0.0 ||
        sample.ForwardGapCm>MaximumLocalGapCm ||
        lateral<ExistingLaneCorridorCm || lateral>MaximumLocalGapCm ||
        sample.RivalForwardCmPerSecond<=1.0 ||
        (sample.LateralGapCm>0.0
            ? sample.RelativeLateralCmPerSecond>=0.0
            : sample.RelativeLateralCmPerSecond<=0.0))
        return result;

    TimeInterval intersection;
    if (!ClipAxis(sample.ForwardGapCm,sample.RelativeForwardCmPerSecond,
            0.0,ForwardFootprintCm,intersection) ||
        !ClipAxis(sample.LateralGapCm,sample.RelativeLateralCmPerSecond,
            -LateralFootprintCm,LateralFootprintCm,intersection))
        return result;
    if (intersection.Begin<0.0 || intersection.End>HorizonSeconds ||
        intersection.End<intersection.Begin)
        return result;

    result.Brake=true;
    result.FirstOverlapSeconds=intersection.Begin;
    result.LastOverlapSeconds=intersection.End;
    return result;
}
} // namespace latebrake
