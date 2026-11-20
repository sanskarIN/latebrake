#pragma once

#include "Track/WorldScaleBankedRoadPlan.h"
#include "Track/WorldScaleVariantRacePace.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

namespace latebrake
{
// Deliberately an independent FUTURE-CIRCUIT advisory. The original
// authored road is still flat in the live Unreal map; no Chaos control,
// official time, AI throttle, grid ordering, or gameplay record is issued.
inline constexpr double BankedPaceGravityCmps2=980.665;
inline constexpr double BankedPaceNumericalTolerance=1.0e-6;
inline constexpr unsigned BankedPaceCapacity=WorldScalePaceStations;

struct BankedPaceSettings final
{
    double NominalGripG=0.9;
    double GripScale=1.0;
    double MaximumSpeedCmps=WorldScalePaceMaxSpeedCmps;
    double BrakingEfficiency=0.72;
    double DriveEfficiency=0.40;
    double LateralBudgetFraction=0.72;
    double MinimumAcceptedSpeedCmps=250.0;
    bool UseOriginalSourceRoad=true;
    bool NoUnrealActuation=true;
    bool NoOfficialLapCredit=true;
};

struct BankedPaceKnot final
{
    double DistanceCm=0.0;
    double WorldHeightCm=0.0;
    double RoadGradeRadians=0.0;
    double RoadBankRadians=0.0;
    double AbsCurvaturePerCm=0.0;
    double EffectiveGripAccelerationCmps2=0.0;
    double MaximumLateralAccelerationCmps2=0.0;
    double LateralDemandAtCapCmps2=0.0;
    double GuaranteedBrakingCmps2=0.0;
    double GuaranteedDriveCmps2=0.0;
    double LocalSpeedCapCmps=0.0;
    double TargetSpeedCmps=0.0;
    bool OriginalRoadVerified=false;
};

struct BankedPacePlan final
{
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::array<BankedPaceKnot,BankedPaceCapacity> Nodes{};
    double StepCm=0.0;
    double MinimumTargetSpeedCmps=0.0;
    double MaximumTargetSpeedCmps=0.0;
    double MinimumLocalSpeedCapCmps=0.0;
    double EstimatedGeometricLapSeconds=0.0;
    double GreatestAbsoluteBankRadians=0.0;
    double GreatestAbsoluteGradeRadians=0.0;
    double MinimumGuaranteedBrakeCmps2=0.0;
    unsigned RelaxationPasses=0;
    bool AllNodesOnOriginalRoad=false;
    bool SourceOnly=true;
    bool OfficialTimingUnchanged=true;
};

inline bool ValidBankedPaceSettings(BankedPaceSettings s) noexcept
{
    return std::isfinite(s.NominalGripG) &&
        s.NominalGripG>=0.35 &&
        s.NominalGripG<=2.50 &&
        std::isfinite(s.GripScale) &&
        s.GripScale>=0.35 &&
        s.GripScale<=1.0 &&
        std::isfinite(s.MaximumSpeedCmps) &&
        s.MaximumSpeedCmps>=1500.0 &&
        s.MaximumSpeedCmps<=WorldScalePaceMaxSpeedCmps &&
        std::isfinite(s.BrakingEfficiency) &&
        s.BrakingEfficiency>=0.3 &&
        s.BrakingEfficiency<=0.9 &&
        std::isfinite(s.DriveEfficiency) &&
        s.DriveEfficiency>=0.1 &&
        s.DriveEfficiency<=0.75 &&
        std::isfinite(s.LateralBudgetFraction) &&
        s.LateralBudgetFraction>=0.4 &&
        s.LateralBudgetFraction<=0.85 &&
        std::isfinite(s.MinimumAcceptedSpeedCmps) &&
        s.MinimumAcceptedSpeedCmps>=100.0 &&
        s.MinimumAcceptedSpeedCmps<=600.0 &&
        s.UseOriginalSourceRoad &&
        s.NoUnrealActuation &&
        s.NoOfficialLapCredit;
}

// Combined lateral/longitudinal friction reserve is established from
// candidate bank and grade. Bank cannot become free "help": absolute
// slope contribution is subtracted conservatively from both budgets.
inline bool ComputeBankedPaceKnot(
    const WorldScaleVariantRecipe& circuit,
    WorldScaleBankedRoadProfile profile,
    BankedPaceSettings settings,
    double stationCm, double curvatureHalfWindowCm,
    BankedPaceKnot& out) noexcept
{
    out={};
    if(!ValidBankedPaceSettings(settings) ||
       !ValidWorldScaleBankedRoadProfile(circuit,profile) ||
       !std::isfinite(stationCm) ||
       !std::isfinite(curvatureHalfWindowCm) ||
       curvatureHalfWindowCm<=0.0)
        return false;
    const auto road=SampleWorldScaleBankedRoad(
        circuit,profile,stationCm,0.0);
    const double curvature=std::abs(
        WorldScaleVariantCurvatureAt(
            circuit,stationCm,curvatureHalfWindowCm));
    if(!road.Valid || !road.OriginalRoadXYQualified ||
       !road.OriginalCollisionUnaffected ||
       !std::isfinite(curvature) ||
       curvature>0.002)
        return false;
    const double g=BankedPaceGravityCmps2;
    const double normalGrip=settings.NominalGripG*
        settings.GripScale*g*
        std::cos(road.GradeRadians)*
        std::cos(road.BankRadians);
    const double bankDemand=g*std::abs(std::sin(
        road.BankRadians));
    const double lateralAvailable=normalGrip-bankDemand;
    const double longAvailable=normalGrip-bankDemand-
        g*std::max(0.0,-std::sin(road.GradeRadians));
    if(!std::isfinite(normalGrip) ||
       !std::isfinite(lateralAvailable) ||
       !std::isfinite(longAvailable) ||
       lateralAvailable<=0.0 || longAvailable<=0.0)
        return false;
    // Lower than the absolute friction limit by design: the remaining
    // circle must reserve some braking even at the local corner speed.
    const double lateralBudget=std::min(
        WorldScalePaceLateralAccelCmps2,
        settings.LateralBudgetFraction*lateralAvailable);
    const double cap=curvature<=1.0e-12 ?
        settings.MaximumSpeedCmps :
        std::min(settings.MaximumSpeedCmps,
                 std::sqrt(lateralBudget/curvature));
    const double demand=curvature*cap*cap;
    if(!std::isfinite(cap) ||
       !std::isfinite(demand) ||
       cap<settings.MinimumAcceptedSpeedCmps)
        return false;
    const double normalizedDemand=std::clamp(
        demand/std::max(1.0,normalGrip),0.0,1.0);
    const double circleReserve=normalGrip*
        std::sqrt(std::max(0.0,1.0-
                               normalizedDemand*normalizedDemand));
    const double braking=std::min(
        WorldScalePaceBrakeAccelCmps2,
        settings.BrakingEfficiency*circleReserve-bankDemand-
        g*std::max(0.0,-std::sin(road.GradeRadians)));
    const double drive=std::min(
        WorldScalePaceDriveAccelCmps2,
        settings.DriveEfficiency*circleReserve-bankDemand-
        g*std::max(0.0,std::sin(road.GradeRadians)));
    if(!std::isfinite(braking) ||
       !std::isfinite(drive) ||
       braking<=0.0 || drive<=0.0)
        return false;
    out.DistanceCm=road.WrappedStationCm;
    out.WorldHeightCm=road.WorldZcm;
    out.RoadGradeRadians=road.GradeRadians;
    out.RoadBankRadians=road.BankRadians;
    out.AbsCurvaturePerCm=curvature;
    out.EffectiveGripAccelerationCmps2=normalGrip;
    out.MaximumLateralAccelerationCmps2=lateralBudget;
    out.LateralDemandAtCapCmps2=demand;
    out.GuaranteedBrakingCmps2=braking;
    out.GuaranteedDriveCmps2=drive;
    out.LocalSpeedCapCmps=cap;
    out.TargetSpeedCmps=cap;
    out.OriginalRoadVerified=true;
    return true;
}

// A closed-lap speed-squared relaxation. Each edge is limited by the
// weaker of its endpoint acceleration budgets. Every sweep tightens
// speed only; the finish-to-start edge gets the same constraints as the
// others, without a discontinuous synthetic standing-start assumption.
inline BankedPacePlan BuildWorldScaleBankedPace(
    const WorldScaleVariantRecipe& circuit,
    WorldScaleBankedRoadProfile profile,
    BankedPaceSettings settings={}) noexcept
{
    BankedPacePlan out;
    if(!ValidWorldScaleVariantRecipe(circuit) ||
       !ValidWorldScaleBankedRoadProfile(circuit,profile) ||
       !ValidBankedPaceSettings(settings))
        return out;
    out.Circuit=circuit.Id;
    out.StepCm=circuit.LapLengthCm/
        static_cast<double>(BankedPaceCapacity);
    if(!std::isfinite(out.StepCm) ||
       out.StepCm<=0.0 || out.StepCm>=1000.0)
        return {};
    std::array<double,BankedPaceCapacity> squared{};
    out.MinimumLocalSpeedCapCmps=
        std::numeric_limits<double>::infinity();
    out.MinimumGuaranteedBrakeCmps2=
        std::numeric_limits<double>::infinity();
    for(unsigned i=0;i<BankedPaceCapacity;++i)
    {
        const double s=i*out.StepCm;
        if(!ComputeBankedPaceKnot(
            circuit,profile,settings,s,out.StepCm*0.5,
            out.Nodes[i]))
            return {};
        const auto& knot=out.Nodes[i];
        squared[i]=knot.LocalSpeedCapCmps*
            knot.LocalSpeedCapCmps;
        out.MinimumLocalSpeedCapCmps=std::min(
            out.MinimumLocalSpeedCapCmps,
            knot.LocalSpeedCapCmps);
        out.MinimumGuaranteedBrakeCmps2=std::min(
            out.MinimumGuaranteedBrakeCmps2,
            knot.GuaranteedBrakingCmps2);
        out.GreatestAbsoluteBankRadians=std::max(
            out.GreatestAbsoluteBankRadians,
            std::abs(knot.RoadBankRadians));
        out.GreatestAbsoluteGradeRadians=std::max(
            out.GreatestAbsoluteGradeRadians,
            std::abs(knot.RoadGradeRadians));
    }
    bool settled=false;
    for(unsigned pass=0;pass<BankedPaceCapacity;++pass)
    {
        bool changed=false;
        for(unsigned i=BankedPaceCapacity;i-- >0u;)
        {
            const unsigned next=(i+1u)%BankedPaceCapacity;
            const double brake=std::min(
                out.Nodes[i].GuaranteedBrakingCmps2,
                out.Nodes[next].GuaranteedBrakingCmps2);
            const double allowed=squared[next]+
                2.0*out.StepCm*brake;
            if(squared[i]>allowed)
            {
                squared[i]=allowed;
                changed=true;
            }
        }
        for(unsigned i=0;i<BankedPaceCapacity;++i)
        {
            const unsigned next=(i+1u)%BankedPaceCapacity;
            const double drive=std::min(
                out.Nodes[i].GuaranteedDriveCmps2,
                out.Nodes[next].GuaranteedDriveCmps2);
            const double allowed=squared[i]+
                2.0*out.StepCm*drive;
            if(squared[next]>allowed)
            {
                squared[next]=allowed;
                changed=true;
            }
        }
        out.RelaxationPasses=pass+1u;
        if(!changed)
        {
            settled=true;
            break;
        }
    }
    if(!settled)return {};
    out.MinimumTargetSpeedCmps=
        std::numeric_limits<double>::infinity();
    double total=0.0;
    for(unsigned i=0;i<BankedPaceCapacity;++i)
    {
        const unsigned next=(i+1u)%BankedPaceCapacity;
        const double v=std::sqrt(squared[i]);
        const double ahead=std::sqrt(squared[next]);
        const double brake=std::min(
            out.Nodes[i].GuaranteedBrakingCmps2,
            out.Nodes[next].GuaranteedBrakingCmps2);
        const double drive=std::min(
            out.Nodes[i].GuaranteedDriveCmps2,
            out.Nodes[next].GuaranteedDriveCmps2);
        if(!std::isfinite(v) || !std::isfinite(ahead) ||
           v<settings.MinimumAcceptedSpeedCmps ||
           v>out.Nodes[i].LocalSpeedCapCmps+
               BankedPaceNumericalTolerance ||
           squared[i]>squared[next]+
               2.0*out.StepCm*brake+0.01 ||
           squared[next]>squared[i]+
               2.0*out.StepCm*drive+0.01)
            return {};
        out.Nodes[i].TargetSpeedCmps=v;
        out.MinimumTargetSpeedCmps=std::min(
            out.MinimumTargetSpeedCmps,v);
        out.MaximumTargetSpeedCmps=std::max(
            out.MaximumTargetSpeedCmps,v);
        total+=2.0*out.StepCm/(v+ahead);
    }
    if(!std::isfinite(total) || total<=1.0 ||
       total>3600.0)
        return {};
    out.EstimatedGeometricLapSeconds=total;
    out.AllNodesOnOriginalRoad=true;
    out.Valid=true;
    return out;
}

struct BankedPaceSample final
{
    bool Valid=false;
    double StationCm=0.0;
    double TargetSpeedCmps=0.0;
    double WorldHeightCm=0.0;
    double RoadGradeRadians=0.0;
    double RoadBankRadians=0.0;
    bool NoOfficialResultAuthority=true;
};

inline BankedPaceSample SampleWorldScaleBankedPace(
    const WorldScaleVariantRecipe& circuit,
    const BankedPacePlan& plan,
    double unwrappedStationCm) noexcept
{
    if(!plan.Valid || plan.Circuit!=circuit.Id ||
       !ValidWorldScaleVariantRecipe(circuit) ||
       !std::isfinite(unwrappedStationCm) ||
       !std::isfinite(plan.StepCm) ||
       plan.StepCm<=0.0)
        return {};
    const double station=WorldScaleVariantWrapDistance(
        circuit,unwrappedStationCm);
    if(!std::isfinite(station))return {};
    const unsigned i=std::min(
        static_cast<unsigned>(station/plan.StepCm),
        BankedPaceCapacity-1u);
    const unsigned next=(i+1u)%BankedPaceCapacity;
    const double alpha=std::clamp(
        (station-plan.Nodes[i].DistanceCm)/plan.StepCm,
        0.0,1.0);
    const auto& a=plan.Nodes[i];
    const auto& b=plan.Nodes[next];
    const double speedSquared=
        (1.0-alpha)*a.TargetSpeedCmps*a.TargetSpeedCmps+
        alpha*b.TargetSpeedCmps*b.TargetSpeedCmps;
    const double v=std::sqrt(speedSquared);
    if(!std::isfinite(v) || v<=0.0)return {};
    // Geometry comes from the same sampled source route, not interpolated
    // XYZ across a curve (which would put the preview off the road).
    const auto pose=WorldScaleVariantCenterlineAt(
        circuit,station);
    if(!pose.Valid)return {};
    return {true,station,v,
        (1.0-alpha)*a.WorldHeightCm+alpha*b.WorldHeightCm,
        (1.0-alpha)*a.RoadGradeRadians+alpha*b.RoadGradeRadians,
        (1.0-alpha)*a.RoadBankRadians+alpha*b.RoadBankRadians,true};
}
} // namespace latebrake
