#pragma once

#include "Track/TrackLimitsSteward.h"
#include "Vehicles/ChassisContactReadout.h"
#include "Progression/RecordValidation.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Actual game chassis samples. Ground support comes from the most recent
// accepted four WorldStatic probes; it is not an individual tire sensor.
// A short unsupported or discontinuous sample breaks exposure instead of
// creating a violation while a car is airborne, tumbling or being moved.
struct TrackLimitsMotionSample final
{
    CircuitPoint Position{};
    double HeadingDegrees=0.0;
    double PlanarSpeedKmH=0.0;
    double VerticalSpeedCmPerSecond=0.0;
    double DeltaSeconds=0.0;
    ChassisContactReadout Ground{};
};

enum class TrackLimitsMotionStatus
{
    Invalid,
    Baseline,
    Discontinuous,
    Accepted
};

struct TrackLimitsMotionDecision final
{
    TrackLimitsMotionStatus Status=TrackLimitsMotionStatus::Invalid;
    double MovingSpeedKmH=0.0;
    double DisplacementCm=0.0;
    bool Accepted() const noexcept
    {
        return Status==TrackLimitsMotionStatus::Accepted;
    }
};

inline bool HasTrackLimitsGroundEvidence(const ChassisContactReadout& ground) noexcept
{
    if (!ground.Live) return false;
    // Only exact supported states of DescribeChassisContact are eligible.
    // Unknown, airborne and two-probe unstable contact do not count.
    return (ground.Status==ChassisContactStatus::Full &&
            ground.QualifiedProbes==4 && ground.DriveTractionPercent==100) ||
        (ground.Status==ChassisContactStatus::Partial &&
            ground.QualifiedProbes==3 && ground.DriveTractionPercent==75);
}

// A conservative continuity gate for a source game, not a real vehicle
// measurement system. Require monotonic active clock, bounded travel
// and supported motion from consecutive samples before accumulating
// off-pavement exposure. Position-derived speed can only reduce the
// reported speed for adjudication; it never creates a moving violation.
class TrackLimitsMotionAdmission final
{
public:
    static constexpr double MinimumFrameSeconds=0.001;
    static constexpr double MaximumFrameSeconds=0.10;
    static constexpr double MaximumSpeedKmH=360.0;
    static constexpr double MaximumVerticalSpeedCmPerSecond=250.0;
    static constexpr double MaximumFrameTravelCm=1200.0;
    static constexpr double PositionToleranceCm=50.0;
    static constexpr double ClockToleranceSeconds=1.0e-7;
    static constexpr double MaximumYawChangeDegrees=135.0;

    void Reset() noexcept
    {
        HavePrevious=false;
        Previous={};
        PreviousActiveSeconds=0.0;
    }

    TrackLimitsMotionDecision Observe(const TrackLimitsMotionSample& sample,
        double activeSeconds) noexcept
    {
        if (!ValidSample(sample,activeSeconds))
        {
            Reset();
            return {};
        }
        if (!HavePrevious)
        {
            Arm(sample,activeSeconds);
            return {TrackLimitsMotionStatus::Baseline,0.0,0.0};
        }

        const double dx=sample.Position.X-Previous.Position.X;
        const double dy=sample.Position.Y-Previous.Position.Y;
        const double displacement=std::hypot(dx,dy);
        const double elapsed=activeSeconds-PreviousActiveSeconds;
        const double yawDelta=std::abs(std::remainder(
            sample.HeadingDegrees-Previous.HeadingDegrees,360.0));
        const double permittedTravel=std::min(MaximumFrameTravelCm,
            std::max(sample.PlanarSpeedKmH,Previous.PlanarSpeedKmH)/0.036*
                sample.DeltaSeconds+PositionToleranceCm);

        if (!std::isfinite(displacement) || displacement>permittedTravel ||
            !std::isfinite(elapsed) || elapsed<=0.0 ||
            std::abs(elapsed-sample.DeltaSeconds)>ClockToleranceSeconds ||
            !std::isfinite(yawDelta) || yawDelta>MaximumYawChangeDegrees)
        {
            // Rebase at the new observation, never bridge a failed pair.
            Arm(sample,activeSeconds);
            return {TrackLimitsMotionStatus::Discontinuous,0.0,0.0};
        }

        const double measuredSpeed=0.036*displacement/sample.DeltaSeconds;
        if (!std::isfinite(measuredSpeed))
        {
            Reset();
            return {};
        }
        const double moving=std::min(sample.PlanarSpeedKmH,measuredSpeed);
        Arm(sample,activeSeconds);
        return {TrackLimitsMotionStatus::Accepted,moving,displacement};
    }

private:
    static bool ValidSample(const TrackLimitsMotionSample& sample,
        double activeSeconds) noexcept
    {
        return IsValidFinishedRaceTime(activeSeconds) &&
            std::isfinite(sample.DeltaSeconds) &&
            sample.DeltaSeconds>=MinimumFrameSeconds &&
            sample.DeltaSeconds<=MaximumFrameSeconds &&
            std::isfinite(sample.PlanarSpeedKmH) &&
            sample.PlanarSpeedKmH>=0.0 && sample.PlanarSpeedKmH<=MaximumSpeedKmH &&
            std::isfinite(sample.VerticalSpeedCmPerSecond) &&
            std::abs(sample.VerticalSpeedCmPerSecond)<=MaximumVerticalSpeedCmPerSecond &&
            std::isfinite(sample.HeadingDegrees) &&
            std::abs(sample.HeadingDegrees)<=36000.0 &&
            ObserveStarterWheelFootprint(sample.Position,sample.HeadingDegrees).Valid &&
            HasTrackLimitsGroundEvidence(sample.Ground);
    }

    void Arm(const TrackLimitsMotionSample& sample,double activeSeconds) noexcept
    {
        Previous=sample;
        PreviousActiveSeconds=activeSeconds;
        HavePrevious=true;
    }

    TrackLimitsMotionSample Previous{};
    double PreviousActiveSeconds=0.0;
    bool HavePrevious=false;
};
} // namespace latebrake
