#pragma once

#include "Track/WorldScaleVariantRacePace.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Future-circuit START/ORDERED-GATE geometry ADVISORY ONLY. None of
// these structs are bound to actual UE5.8 actor spawns, real vehicle
// collision, official checkpoints or paid race progression.
inline constexpr unsigned WorldScaleVariantGridCars=6u;
inline constexpr unsigned WorldScaleVariantGateCount=4u;
inline constexpr double WorldScaleVariantGridHalfLengthCm=205.0;
inline constexpr double WorldScaleVariantGridHalfWidthCm=95.0;
inline constexpr double WorldScaleVariantGateHalfSpanCm=375.0;

struct WorldScaleVariantGridSettings final
{
    double FirstRowBehindStartCm=1000.0;
    double RowSpacingCm=650.0;
    double LateralSlotCm=175.0;
    double MinimumBodyGapCm=55.0;
    double VehicleHalfLengthCm=WorldScaleVariantGridHalfLengthCm;
    double VehicleHalfWidthCm=WorldScaleVariantGridHalfWidthCm;
    bool OriginalRoadOnly=true;
    bool RequireNonOverlappingCarFootprints=true;
    bool NoActiveUnrealSpawns=true;
};

struct WorldScaleVariantGridCar final
{
    CircuitPose Pose{};
    double UnwrappedStationCm=0.0;
    int GridSlot=-1;
    std::uint64_t SourceToken=0;
    bool AllBodyCornersOnRoad=false;
};

struct WorldScaleVariantGridPlan final
{
    bool Valid=false;
    WorldScaleVariantId SourceTrack=WorldScaleVariantId::OriginalLoop;
    std::array<WorldScaleVariantGridCar,WorldScaleVariantGridCars> Cars{};
    double MinimumObservedCenterGapCm=0.0;
    double MinimumObservedBodyClearanceCm=0.0;
    bool OriginalPhysicalRoadUnchanged=true;
    bool OfficialRaceGridUnchanged=true;
};

struct WorldScaleVariantGate final
{
    CircuitPose Center{};
    CircuitPoint LeftExtent{};
    CircuitPoint RightExtent{};
    double StationCm=0.0;
    unsigned GateIndex=0;
    bool WholeGateOnAuthoredRoad=false;
};

struct WorldScaleVariantGatePlan final
{
    bool Valid=false;
    WorldScaleVariantId SourceTrack=WorldScaleVariantId::OriginalLoop;
    std::array<WorldScaleVariantGate,WorldScaleVariantGateCount> Gates{};
    bool AdvisoryOnly=true;
    bool DoesNotIssueOfficialLapCredit=true;
};

struct WorldScaleVariantGateObservation final
{
    CircuitPoint Previous{};
    CircuitPoint Current{};
    double DeltaSeconds=0.0;
    double MaximumObservedSpeedCmps=0.0;
    std::uint64_t PhysicalActorToken=0;
    unsigned ExpectedGate=0;
    bool OriginalRoadContactVerified=false;
    bool ActiveRaceVerified=false;
};

struct WorldScaleVariantGateEvidence final
{
    bool ValidSource=false;
    bool CrossedForward=false;
    double PreviousSignedDistanceCm=0.0;
    double CurrentSignedDistanceCm=0.0;
    double CrossingLateralOffsetCm=0.0;
    bool OfficialLapAwarded=false;
};

inline bool ValidWorldScaleVariantGridSettings(
    WorldScaleVariantGridSettings p) noexcept
{
    return std::isfinite(p.FirstRowBehindStartCm) &&
        p.FirstRowBehindStartCm>=750.0 &&
        p.FirstRowBehindStartCm<=1750.0 &&
        std::isfinite(p.RowSpacingCm) &&
        p.RowSpacingCm>=550.0 &&
        p.RowSpacingCm<=900.0 &&
        std::isfinite(p.LateralSlotCm) &&
        p.LateralSlotCm>=150.0 && p.LateralSlotCm<=200.0 &&
        std::isfinite(p.MinimumBodyGapCm) &&
        p.MinimumBodyGapCm>=30.0 &&
        p.MinimumBodyGapCm<=100.0 &&
        std::isfinite(p.VehicleHalfLengthCm) &&
        p.VehicleHalfLengthCm>=180.0 &&
        p.VehicleHalfLengthCm<=230.0 &&
        std::isfinite(p.VehicleHalfWidthCm) &&
        p.VehicleHalfWidthCm>=80.0 &&
        p.VehicleHalfWidthCm<=110.0 &&
        p.LateralSlotCm+p.VehicleHalfWidthCm+60.0<=
            WorldScaleVariantRoadHalfWidthCm &&
        2.0*(p.LateralSlotCm-p.VehicleHalfWidthCm)>=
            p.MinimumBodyGapCm &&
        p.RowSpacingCm-2.0*p.VehicleHalfLengthCm>=
            p.MinimumBodyGapCm &&
        p.OriginalRoadOnly &&
        p.RequireNonOverlappingCarFootprints &&
        p.NoActiveUnrealSpawns;
}

inline CircuitPoint WorldScaleVariantLateral(
    const RoundedCenterlinePose& center,double offsetCm) noexcept
{
    const double heading=center.Pose.HeadingDegrees*
        WorldScalePaceRadPerDegree;
    return {
        center.Pose.Position.X-std::sin(heading)*offsetCm,
        center.Pose.Position.Y+std::cos(heading)*offsetCm
    };
}

// Require the actual WORLDSTATIC-road union under the entire flat
// footprint, not just its centroid. This is a source-only XY check:
// native Chaos tire contact/ground Z still requires independent QA.
inline bool WorldScaleVariantBodyOnRoad(
    const WorldScaleVariantRecipe& p,
    CircuitPose pose,double halfLengthCm,double halfWidthCm) noexcept
{
    if(!ValidWorldScaleVariantRecipe(p) ||
       !std::isfinite(pose.Position.X) ||
       !std::isfinite(pose.Position.Y) ||
       !std::isfinite(pose.HeadingDegrees) ||
       !std::isfinite(halfLengthCm) ||
       !std::isfinite(halfWidthCm) ||
       halfLengthCm<0.0 || halfWidthCm<0.0)
        return false;
    const double angle=pose.HeadingDegrees*
        WorldScalePaceRadPerDegree;
    const double tx=std::cos(angle),ty=std::sin(angle);
    const double nx=-ty,ny=tx;
    for(double longitudinal : {-halfLengthCm,0.0,halfLengthCm})
    {
        for(double lateral : {-halfWidthCm,0.0,halfWidthCm})
        {
            const CircuitPoint at{
                pose.Position.X+longitudinal*tx+lateral*nx,
                pose.Position.Y+longitudinal*ty+lateral*ny
            };
            if(!IsOnWorldScaleVariantRoad(p,at))
                return false;
        }
    }
    return true;
}

// SAT separating-axis clearance between two source-only XY chassis
// rectangles. A circular bounding radius falsely rejects perfectly
// legal side-by-side grid slots; four actual OBB axes preserve safety.
inline double WorldScaleVariantGridBodyClearance(
    CircuitPose a,CircuitPose b,
    double halfLength,double halfWidth) noexcept
{
    const double angleA=a.HeadingDegrees*WorldScalePaceRadPerDegree;
    const double angleB=b.HeadingDegrees*WorldScalePaceRadPerDegree;
    const std::array<CircuitPoint,4> axes{{
        {std::cos(angleA),std::sin(angleA)},
        {-std::sin(angleA),std::cos(angleA)},
        {std::cos(angleB),std::sin(angleB)},
        {-std::sin(angleB),std::cos(angleB)}
    }};
    const CircuitPoint delta{
        b.Position.X-a.Position.X,b.Position.Y-a.Position.Y
    };
    double best=-std::numeric_limits<double>::infinity();
    for(const auto& axis:axes)
    {
        const double offset=std::abs(
            delta.X*axis.X+delta.Y*axis.Y);
        const double radiusA=halfLength*std::abs(
            axis.X*std::cos(angleA)+axis.Y*std::sin(angleA))+
            halfWidth*std::abs(
                -axis.X*std::sin(angleA)+axis.Y*std::cos(angleA));
        const double radiusB=halfLength*std::abs(
            axis.X*std::cos(angleB)+axis.Y*std::sin(angleB))+
            halfWidth*std::abs(
                -axis.X*std::sin(angleB)+axis.Y*std::cos(angleB));
        if(!std::isfinite(offset) ||
           !std::isfinite(radiusA) ||
           !std::isfinite(radiusB))return
                -std::numeric_limits<double>::infinity();
        best=std::max(best,offset-radiusA-radiusB);
    }
    return best;
}

inline WorldScaleVariantGridPlan BuildWorldScaleVariantGrid(
    const WorldScaleVariantRecipe& p,
    WorldScaleVariantGridSettings s = {}) noexcept
{
    if(!ValidWorldScaleVariantRecipe(p) ||
       !ValidWorldScaleVariantGridSettings(s))
        return {};
    WorldScaleVariantGridPlan result{};
    result.SourceTrack=p.Id;
    double minCenter=std::numeric_limits<double>::infinity();
    double minBody=std::numeric_limits<double>::infinity();
    for(unsigned slot=0;slot<WorldScaleVariantGridCars;++slot)
    {
        const unsigned row=slot/2u;
        const int side=(slot%2u==0u)?-1:1;
        const double station=-s.FirstRowBehindStartCm-
            row*s.RowSpacingCm;
        const auto center=WorldScaleVariantCenterlineAt(p,station);
        if(!center.Valid)return {};
        const CircuitPoint position=WorldScaleVariantLateral(
            center,side*s.LateralSlotCm);
        CircuitPose pose{position,center.Pose.HeadingDegrees};
        if(!WorldScaleVariantBodyOnRoad(
                p,pose,s.VehicleHalfLengthCm,
                s.VehicleHalfWidthCm))
            return {};
        const auto index=static_cast<std::size_t>(slot);
        result.Cars[index]={
            pose,station,static_cast<int>(slot),
            1000000u+static_cast<std::uint64_t>(p.Id)*100u+slot,
            true
        };
        // Conservative disc exclusion includes complete 2D footprint
        // radius; real oriented 3D collision needs Chaos afterward.
        for(unsigned prior=0;prior<slot;++prior)
        {
            const auto& other=result.Cars[prior];
            const double gap=std::hypot(
                pose.Position.X-other.Pose.Position.X,
                pose.Position.Y-other.Pose.Position.Y);
            const double clearance=
                WorldScaleVariantGridBodyClearance(
                    pose,other.Pose,
                    s.VehicleHalfLengthCm,s.VehicleHalfWidthCm);
            if(!std::isfinite(gap) ||
               clearance<s.MinimumBodyGapCm)
                return {};
            minCenter=std::min(minCenter,gap);
            minBody=std::min(minBody,clearance);
        }
    }
    result.MinimumObservedCenterGapCm=minCenter;
    result.MinimumObservedBodyClearanceCm=minBody;
    result.Valid=true;
    return result;
}

inline WorldScaleVariantGatePlan BuildWorldScaleVariantGates(
    const WorldScaleVariantRecipe& p) noexcept
{
    if(!ValidWorldScaleVariantRecipe(p))return {};
    WorldScaleVariantGatePlan result{};
    result.SourceTrack=p.Id;
    for(unsigned i=0;i<WorldScaleVariantGateCount;++i)
    {
        const double distance=p.LapLengthCm*
            static_cast<double>(i)/WorldScaleVariantGateCount;
        const auto center=WorldScaleVariantCenterlineAt(p,distance);
        if(!center.Valid)return {};
        const auto left=WorldScaleVariantLateral(
            center,WorldScaleVariantGateHalfSpanCm);
        const auto right=WorldScaleVariantLateral(
            center,-WorldScaleVariantGateHalfSpanCm);
        if(!IsOnWorldScaleVariantRoad(p,center.Pose.Position) ||
           !IsOnWorldScaleVariantRoad(p,left) ||
           !IsOnWorldScaleVariantRoad(p,right))
            return {};
        result.Gates[i]={
            center.Pose,left,right,distance,i,true
        };
    }
    result.Valid=true;
    return result;
}

// A route plane-crossing observation that explicitly refuses fake lap
// credit and wrong-way movement. The native game still owns its own
// authenticated gate, progress, race phase and actor snapshot logic.
inline WorldScaleVariantGateEvidence ObserveWorldScaleVariantGate(
    const WorldScaleVariantRecipe& p,
    const WorldScaleVariantGatePlan& gates,
    const WorldScaleVariantGateObservation& observed) noexcept
{
    if(!ValidWorldScaleVariantRecipe(p) ||
       !gates.Valid || gates.SourceTrack!=p.Id ||
       observed.ExpectedGate>=WorldScaleVariantGateCount ||
       !observed.OriginalRoadContactVerified ||
       !observed.ActiveRaceVerified ||
       observed.PhysicalActorToken==0 ||
       !std::isfinite(observed.DeltaSeconds) ||
       observed.DeltaSeconds<=0.0 ||
       observed.DeltaSeconds>0.10 ||
       !std::isfinite(observed.MaximumObservedSpeedCmps) ||
       observed.MaximumObservedSpeedCmps<0.0 ||
       observed.MaximumObservedSpeedCmps>15000.0 ||
       !std::isfinite(observed.Previous.X) ||
       !std::isfinite(observed.Previous.Y) ||
       !std::isfinite(observed.Current.X) ||
       !std::isfinite(observed.Current.Y))
        return {};
    const auto& gate=gates.Gates[observed.ExpectedGate];
    const double dx=observed.Current.X-observed.Previous.X;
    const double dy=observed.Current.Y-observed.Previous.Y;
    const double moved=std::hypot(dx,dy);
    if(!std::isfinite(moved) ||
       moved>observed.MaximumObservedSpeedCmps*
             observed.DeltaSeconds+5.0 ||
       !IsOnWorldScaleVariantRoad(p,observed.Previous) ||
       !IsOnWorldScaleVariantRoad(p,observed.Current))
        return {};
    const double radians=gate.Center.HeadingDegrees*
        WorldScalePaceRadPerDegree;
    const double fx=std::cos(radians),fy=std::sin(radians);
    const double nx=-fy,ny=fx;
    const double beforeX=observed.Previous.X-
        gate.Center.Position.X;
    const double beforeY=observed.Previous.Y-
        gate.Center.Position.Y;
    const double afterX=observed.Current.X-
        gate.Center.Position.X;
    const double afterY=observed.Current.Y-
        gate.Center.Position.Y;
    const double before=beforeX*fx+beforeY*fy;
    const double after=afterX*fx+afterY*fy;
    const bool forward=before < -0.01 && after >= 0.01;
    const double alpha=forward ?
        std::clamp(-before/(after-before),0.0,1.0):0.0;
    const double lateral=(beforeX+alpha*dx)*nx+
        (beforeY+alpha*dy)*ny;
    const bool within=std::isfinite(lateral) &&
        std::abs(lateral)<=WorldScaleVariantGateHalfSpanCm;
    return {true,forward && within,before,after,lateral,false};
}
} // namespace latebrake
