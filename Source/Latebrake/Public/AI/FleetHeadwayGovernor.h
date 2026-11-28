#pragma once

#include "AI/FleetPassingLane.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake
{
// Arcade opponent longitudinal pacing, not a vehicle dynamics solver.
// It can only restrict the motor command already admitted by the
// physical traffic supervisor. It never releases a brake, chooses a
// steering direction, grants route progress, or changes race rewards.
inline constexpr double FleetHeadwayStandstillCm=180.0;
inline constexpr double FleetHeadwayTimeSeconds=0.80;
inline constexpr double FleetHeadwayPreviewSeconds=0.65;
inline constexpr double FleetHeadwaySpeedBandCmS=300.0;
inline constexpr double FleetHeadwaySideMarginCm=35.0;
inline constexpr double FleetHeadwayMaxGapCm=3200.0;
inline constexpr double FleetHeadwayMaxYawDegrees=20.0;
inline constexpr double FleetHeadwayMaxLateralCmS=450.0;
inline constexpr double FleetHeadwayReleasePerSecond=1.25;
inline constexpr double FleetHeadwayMinForwardCmS=0.0;

enum class FleetHeadwayStatus : std::uint8_t
{
    Unavailable,
    UnsupportedGeometry,
    Clear,
    Following,
    Releasing,
    Interrupted
};

struct FleetHeadwayCandidate final
{
    bool Valid=false;
    int Slot=-1;
    std::uint64_t Actor=0;
    double GapCm=0.0;
    double DesiredGapCm=0.0;
    double ProjectedGapCm=0.0;
    double LeaderForwardCmS=0.0;
    double TargetForwardCmS=0.0;
    double ThrottleCeiling=1.0;
};

struct FleetHeadwayAssessment final
{
    FleetHeadwayStatus Status=FleetHeadwayStatus::Unavailable;
    std::array<std::uint64_t,StartGridCapacity> Roster{};
    FleetHeadwayCandidate Leader{};
    double TargetCeiling=1.0;
    int Candidates=0;

    bool Qualified() const noexcept
    {
        return Status==FleetHeadwayStatus::Clear ||
            Status==FleetHeadwayStatus::Following;
    }
};

// Continuous lateral intersection over a short preview. This includes
// measured cut-ins without assuming that a wheel instantly follows a
// steering request. The longitudinal gap is evaluated over the entire
// preview, deliberately conservative when the lateral crossing is late.
inline bool FleetHeadwayLateralConflict(double offset,double relativeVelocity,
    double extent) noexcept
{
    if(!std::isfinite(offset) || !std::isfinite(relativeVelocity) ||
        !std::isfinite(extent) || extent<0.0)
        return false;
    double first=0.0,last=FleetHeadwayPreviewSeconds;
    return ClipFleetPassAxis(offset,relativeVelocity,extent,first,last);
}

inline bool FleetHeadwayBodyOnStraight(const FleetPassLocalBody& body,
    const RecoveryObstacle& actual,const FleetIncomingStraight& straight) noexcept
{
    return body.Token!=0 && body.Along>=-straight.LengthCm &&
        body.Along<=0.0 && body.YawError<=FleetHeadwayMaxYawDegrees &&
        body.Forward>=FleetHeadwayMinForwardCmS &&
        std::abs(body.Lateral)<=FleetHeadwayMaxLateralCmS &&
        FleetLanePoseFitsSlab(actual.Pose,straight.Slab) &&
        FleetLanePoseFitsSlab({actual.Pose.Position,straight.Heading},straight.Slab);
}

// Use bumper gap, not center-to-center separation. The desired gap grows
// with measured self speed. The target combines a time-gap controller
// with a relative-speed braking estimate discounted by observed grip.
// These are tuning parameters for virtual arcade cars, not certified
// stopping distances or an instruction to drive a real vehicle.
inline FleetHeadwayCandidate EvaluateFleetHeadwayCandidate(
    const FleetPassLocalBody& self,const FleetPassLocalBody& other,
    double brakeScale) noexcept
{
    FleetHeadwayCandidate out;
    if(!std::isfinite(brakeScale) || brakeScale<0.15 || brakeScale>1.0 ||
        self.Slot<=HumanGridSlot || self.Slot>=StartGridCapacity || !self.Token ||
        !std::isfinite(self.Along) || !std::isfinite(other.Along) ||
        !std::isfinite(self.Side) || !std::isfinite(other.Side) ||
        !std::isfinite(self.Forward) || !std::isfinite(other.Forward) ||
        !std::isfinite(self.Lateral) || !std::isfinite(other.Lateral) ||
        !std::isfinite(self.HalfAlong) || !std::isfinite(other.HalfAlong) ||
        !std::isfinite(self.HalfSide) || !std::isfinite(other.HalfSide) ||
        self.HalfAlong<=0.0 || other.HalfAlong<=0.0 ||
        self.HalfSide<=0.0 || other.HalfSide<=0.0 ||
        self.Forward<0.0 || other.Forward<0.0 ||
        self.Forward>FleetTrafficMaxSpeedCmS || other.Forward>FleetTrafficMaxSpeedCmS ||
        other.Slot<0 || other.Slot>=StartGridCapacity || other.Slot==self.Slot ||
        !other.Token || other.Token==self.Token)
        return out;
    const double centerGap=other.Along-self.Along;
    if(centerGap<=0.0 || centerGap>FleetHeadwayMaxGapCm+
        self.HalfAlong+other.HalfAlong)
        return out;
    if(!FleetHeadwayLateralConflict(other.Side-self.Side,
        other.Lateral-self.Lateral,self.HalfSide+other.HalfSide+FleetHeadwaySideMarginCm))
        return out;
    const double gap=centerGap-self.HalfAlong-other.HalfAlong;
    const double closing=std::max(0.0,self.Forward-other.Forward);
    const double projected=gap-closing*FleetHeadwayPreviewSeconds;
    const double desired=FleetHeadwayStandstillCm+self.Forward*FleetHeadwayTimeSeconds;
    const double spacingTarget=std::max(0.0,other.Forward+
        (projected-desired)/FleetHeadwayTimeSeconds);
    const double deceleration=FleetForecastDecelerationCmS2*brakeScale;
    const double brakingTarget=std::sqrt(other.Forward*other.Forward+
        2.0*deceleration*std::max(0.0,projected-FleetHeadwayStandstillCm));
    const double target=std::min(spacingTarget,brakingTarget);
    // At or above target speed coast; below it progressively restore
    // motor demand. The existing traffic stack owns actual braking.
    const double cap=projected<=FleetHeadwayStandstillCm?0.0:
        std::clamp((target-self.Forward)/FleetHeadwaySpeedBandCmS,0.0,1.0);
    return {true,other.Slot,other.Token,gap,desired,projected,other.Forward,target,cap};
}

inline bool PreferFleetHeadwayCandidate(const FleetHeadwayCandidate& candidate,
    const FleetHeadwayCandidate& current) noexcept
{
    if(!candidate.Valid)return false;
    if(!current.Valid)return true;
    if(candidate.ThrottleCeiling!=current.ThrottleCeiling)
        return candidate.ThrottleCeiling<current.ThrottleCeiling;
    if(candidate.ProjectedGapCm!=current.ProjectedGapCm)
        return candidate.ProjectedGapCm<current.ProjectedGapCm;
    return candidate.Slot<current.Slot;
}

inline FleetHeadwayAssessment AssessFleetHeadway(const FleetPassSample& sample) noexcept
{
    FleetHeadwayAssessment out;
    if(!sample.Traffic.BrakeObservationQualified ||
        !std::isfinite(sample.Traffic.ConservativeBrakeScale) ||
        sample.Traffic.ConservativeBrakeScale<0.15 ||
        sample.Traffic.ConservativeBrakeScale>1.0)
        return out;
    const auto geometry=InspectFleetPassGeometry(sample);
    if(!geometry.Valid)return out;
    out.Roster=geometry.Roster;
    if(!FleetHeadwayBodyOnStraight(geometry.Self,sample.Traffic.Self,geometry.Straight))
    {
        out.Status=FleetHeadwayStatus::UnsupportedGeometry;
        return out;
    }
    out.Status=FleetHeadwayStatus::Clear;
    for(std::size_t i=0;i<geometry.Others.size();++i)
    {
        const auto& other=geometry.Others[i];
        if(!FleetHeadwayBodyOnStraight(other,sample.Traffic.Blockers[i],geometry.Straight))
            continue; // Existing collision guards still see these actors.
        const auto candidate=EvaluateFleetHeadwayCandidate(geometry.Self,other,
            sample.Traffic.ConservativeBrakeScale);
        if(!candidate.Valid)continue;
        ++out.Candidates;
        if(PreferFleetHeadwayCandidate(candidate,out.Leader))out.Leader=candidate;
    }
    if(out.Leader.Valid)
    {
        out.TargetCeiling=out.Leader.ThrottleCeiling;
        if(out.TargetCeiling<1.0)out.Status=FleetHeadwayStatus::Following;
    }
    return out;
}

struct FleetHeadwayReadout final
{
    FleetHeadwayStatus Status=FleetHeadwayStatus::Unavailable;
    FleetHeadwayAssessment Assessment{};
    PilotCommand Command{};
    double AppliedCeiling=1.0;
    bool Restricted=false;
};

// Retain only restrictive motor history. Recovery or pause discards
// that history; no previous clear frame can authorize a new command.
// A nonfresh observation cannot relax a ceiling or contribute time.
class FleetHeadwayGovernor final
{
public:
    void Reset() noexcept
    {
        *this={};
    }

    void Suspend() noexcept
    {
        HaveCeiling=false;
        Ceiling=1.0;
        Identity={};
        GateOrdinal=-1;
        Last={};
    }

    FleetHeadwayReadout Readout() const noexcept {return Last;}

    FleetHeadwayReadout Apply(PilotCommand prior,const FleetPassSample& sample) noexcept
    {
        Last={};
        Last.Command=prior;
        if(!ValidFleetPassCommand(prior))
        {
            Suspend();
            Last.Command={};
            return Last;
        }
        if(!sample.Stamp.Valid() ||
            (LastStamp.Valid() && !FleetPassFresh(LastStamp,sample.Stamp)))
        {
            // Never release propulsion from duplicated or foreign frames.
            Last.Status=FleetHeadwayStatus::Interrupted;
            Last.AppliedCeiling=0.0;
            Last.Restricted=prior.Throttle>0.0;
            Last.Command.Throttle=0.0;
            return Last;
        }
        const bool continuous=LastStamp.Valid() &&
            FleetPassContinuous(LastStamp,sample.Stamp);
        LastStamp=sample.Stamp;
        const auto assessment=AssessFleetHeadway(sample);
        if(prior.Arrived)
        {
            Suspend();
            Last.Command=prior;
            return Last;
        }
        if(!assessment.Qualified())
        {
            // Off-road, corners and unqualified observations remain under
            // the original traffic/route policy. No stale cap survives.
            Suspend();
            Last.Assessment=assessment;
            Last.Status=assessment.Status;
            Last.Command=prior;
            return Last;
        }
        if(!continuous || !HaveCeiling || Identity!=assessment.Roster ||
            GateOrdinal!=sample.GateOrdinal)
        {
            // Restart from current measured spacing; never blend across a
            // new physical roster, discontinuity or route segment.
            Ceiling=assessment.TargetCeiling;
            Identity=assessment.Roster;
            GateOrdinal=sample.GateOrdinal;
            HaveCeiling=true;
        }
        else
        {
            Ceiling=std::min(assessment.TargetCeiling,
                Ceiling+FleetHeadwayReleasePerSecond*sample.DeltaSeconds);
        }
        Last.Assessment=assessment;
        Last.Status=assessment.Status;
        if(Ceiling<assessment.TargetCeiling)
            Last.Status=FleetHeadwayStatus::Releasing;
        Last.AppliedCeiling=Ceiling;
        Last.Command=prior;
        Last.Command.Throttle=prior.Brake?0.0:std::min(prior.Throttle,Ceiling);
        Last.Restricted=Last.Command.Throttle<prior.Throttle;
        return Last;
    }

private:
    FleetPassStamp LastStamp{};
    std::array<std::uint64_t,StartGridCapacity> Identity{};
    int GateOrdinal=-1;
    bool HaveCeiling=false;
    double Ceiling=1.0;
    FleetHeadwayReadout Last{};
};
} // namespace latebrake
