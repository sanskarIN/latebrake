#pragma once
#include "Racecraft/SmoothRacingLine.h"
#include "Racecraft/SurfaceMaterialModel.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
inline constexpr int SpeedPlanMaximumSamples=256;

struct SpeedPlanParameters final
{
    double SampleSpacingCm=120.0;
    double MaximumSpeedCmS=5400.0;
    double MinimumCornerSpeedCmS=500.0;
    double MaximumComfortDecelerationCmS2=980.0;
    double MaximumComfortAccelerationCmS2=650.0;
    double ReactionDelaySeconds=0.22;
    double SafetyGripScale=0.84;
    double HazardClearanceCm=200.0;
    double MinimumPlanDistanceCm=500.0;
    double MaximumPlanDistanceCm=20000.0;
    double MaximumTrackCurvaturePerCm=0.01;
    double MinimumPhysicalGripG=0.10;
    double MaximumPhysicalGripG=2.5;
    double WetWeatherSpeedFraction=0.80;
    double OffPavementSpeedFraction=0.15;
    double YellowFlagSpeedFraction=0.60;
    double LowVisibilitySpeedFraction=0.70;
    double TargetSteeringSlackRadians=0.12;
    double MaximumCornerSpeedChangeCmS=1500.0;
    int MaximumBackwardPasses=4;
    bool RequireOriginalAsphalt=true;
    bool RequirePhysicalGrip=true;
};

struct SpeedPlanSource final
{
    SmoothLineGeometry Geometry{};
    double StartingDistanceCm=0.0;
    double ObservedSpeedCmS=0.0;
    double MeasuredGripG=0.0;
    double MeasuredWetness=0.0;
    double MeasuredVisibility=1.0;
    double MeasuredBrakeEfficiency=1.0;
    double MeasuredAccelerationEfficiency=1.0;
    double HorizonCm=12000.0;
    bool PhysicalRoadReady=false;
    bool PhysicalTiresReady=false;
    bool PhysicalBrakesReady=false;
    bool RaceGreen=false;
    bool CautionFlag=false;
};

struct SpeedPlanNode final
{
    double LapDistanceCm=0.0;
    double HorizonDistanceCm=0.0;
    double CurvaturePerCm=0.0;
    double CurveLimitedSpeedCmS=0.0;
    double WeatherLimitedSpeedCmS=0.0;
    double FlagLimitedSpeedCmS=0.0;
    double BrakingLimitedSpeedCmS=0.0;
    double AcceleratingLimitedSpeedCmS=0.0;
    double FinalTargetSpeedCmS=0.0;
    double RequiredBrakeDecelerationCmS2=0.0;
    double TimeToReachSeconds=0.0;
    double RoadMarginCm=0.0;
    int RoutePiece=-1;
    bool IsCorner=false;
    bool PavementProven=false;
    bool HazardAhead=false;
    bool Valid=false;
};

struct SpeedPlanResult final
{
    std::array<SpeedPlanNode,SpeedPlanMaximumSamples> Nodes{};
    int Count=0;
    double PredictedPlanSeconds=0.0;
    double FirstRequiredBrakeDistanceCm=0.0;
    double MinimumPlannedSpeedCmS=0.0;
    double MaximumPlannedSpeedCmS=0.0;
    double MeanTargetSpeedCmS=0.0;
    double MaximumRequiredDecelerationCmS2=0.0;
    double MaximumObservedCurvaturePerCm=0.0;
    double PlanHorizonCm=0.0;
    int CurvedNodes=0;
    int BrakingNodes=0;
    int FlagLimitedNodes=0;
    bool RequiresBrakingNow=false;
    bool AllOnOriginalRoad=false;
    bool AppliesVehicleCommands=false;
    bool Valid=false;
};

inline bool ValidSpeedPlanParameters(
    SpeedPlanParameters p) noexcept
{
    return Finite(p.SampleSpacingCm) &&
        p.SampleSpacingCm>=10.0 &&
        p.SampleSpacingCm<=400.0 &&
        Finite(p.MaximumSpeedCmS) &&
        p.MaximumSpeedCmS>0.0 &&
        p.MaximumSpeedCmS<=MaximumPhysicalSpeedCmS &&
        Finite(p.MinimumCornerSpeedCmS) &&
        p.MinimumCornerSpeedCmS>0.0 &&
        p.MinimumCornerSpeedCmS<p.MaximumSpeedCmS &&
        Finite(p.MaximumComfortDecelerationCmS2) &&
        p.MaximumComfortDecelerationCmS2>0.0 &&
        Finite(p.MaximumComfortAccelerationCmS2) &&
        p.MaximumComfortAccelerationCmS2>0.0 &&
        Finite(p.ReactionDelaySeconds) &&
        p.ReactionDelaySeconds>=0.0 &&
        Finite(p.SafetyGripScale) &&
        p.SafetyGripScale>0.0 &&
        p.SafetyGripScale<=1.0 &&
        Finite(p.HazardClearanceCm) &&
        p.HazardClearanceCm>=0.0 &&
        Finite(p.MinimumPlanDistanceCm) &&
        p.MinimumPlanDistanceCm>0.0 &&
        Finite(p.MaximumPlanDistanceCm) &&
        p.MaximumPlanDistanceCm>=p.MinimumPlanDistanceCm &&
        Finite(p.MinimumPhysicalGripG) &&
        p.MinimumPhysicalGripG>0.0 &&
        Finite(p.MaximumPhysicalGripG) &&
        p.MaximumPhysicalGripG>=p.MinimumPhysicalGripG &&
        Finite(p.WetWeatherSpeedFraction) &&
        p.WetWeatherSpeedFraction>0.0 &&
        p.WetWeatherSpeedFraction<=1.0 &&
        Finite(p.YellowFlagSpeedFraction) &&
        p.YellowFlagSpeedFraction>0.0 &&
        p.YellowFlagSpeedFraction<=1.0 &&
        p.MaximumBackwardPasses>=1 &&
        p.MaximumBackwardPasses<=16;
}

inline bool ValidSpeedPlanSource(
    const SpeedPlanSource& s,
    SpeedPlanParameters p) noexcept
{
    return s.Geometry.Valid &&
        s.PhysicalRoadReady &&
        (!p.RequirePhysicalGrip ||
         (s.PhysicalTiresReady && s.PhysicalBrakesReady)) &&
        (s.RaceGreen || s.CautionFlag) &&
        Finite(s.StartingDistanceCm) &&
        Finite(s.ObservedSpeedCmS) &&
        Finite(s.MeasuredGripG) &&
        Finite(s.MeasuredWetness) &&
        Finite(s.MeasuredVisibility) &&
        Finite(s.MeasuredBrakeEfficiency) &&
        Finite(s.MeasuredAccelerationEfficiency) &&
        Finite(s.HorizonCm) &&
        s.ObservedSpeedCmS>=0.0 &&
        s.ObservedSpeedCmS<=MaximumPhysicalSpeedCmS &&
        s.MeasuredGripG>=p.MinimumPhysicalGripG &&
        s.MeasuredGripG<=p.MaximumPhysicalGripG &&
        s.MeasuredWetness>=0.0 &&
        s.MeasuredWetness<=1.0 &&
        s.MeasuredVisibility>=0.0 &&
        s.MeasuredVisibility<=1.0 &&
        s.MeasuredBrakeEfficiency>0.0 &&
        s.MeasuredBrakeEfficiency<=1.5 &&
        s.MeasuredAccelerationEfficiency>0.0 &&
        s.MeasuredAccelerationEfficiency<=1.5 &&
        s.HorizonCm>=p.MinimumPlanDistanceCm &&
        s.HorizonCm<=p.MaximumPlanDistanceCm;
}

inline double SpeedPlanCornerLimit(
    double curvaturePerCm,double measuredGripG,
    SpeedPlanParameters p) noexcept
{
    if (!Finite(curvaturePerCm) ||
        !Finite(measuredGripG) ||
        measuredGripG<=0.0) return 0.0;
    if (std::abs(curvaturePerCm)<=PhysicsEpsilon)
        return p.MaximumSpeedCmS;
    return std::clamp(
        MaximumCurveSpeedCmS(
            curvaturePerCm,
            measuredGripG*p.SafetyGripScale),
        p.MinimumCornerSpeedCmS,p.MaximumSpeedCmS);
}

inline SpeedPlanNode BuildSpeedNode(
    SmoothLineGeometry route,double distanceCm,
    double horizonCm,SpeedPlanSource source,
    SpeedPlanParameters settings) noexcept
{
    SpeedPlanNode node;
    const auto sample=
        SampleSmoothRacingLine(route,distanceCm);
    if (!sample.Valid ||
        (settings.RequireOriginalAsphalt &&
         !sample.InsideAuthoredRoad))
        return node;
    node.LapDistanceCm=sample.DistanceCm;
    node.HorizonDistanceCm=horizonCm;
    node.CurvaturePerCm=sample.CurvaturePerCm;
    node.RoadMarginCm=sample.SourceRoadMarginCm;
    node.RoutePiece=sample.PieceIndex;
    node.IsCorner=sample.Curved;
    node.PavementProven=sample.InsideAuthoredRoad;
    node.CurveLimitedSpeedCmS=
        SpeedPlanCornerLimit(sample.CurvaturePerCm,
                             source.MeasuredGripG,
                             settings);
    const double wetReduction=1.0-
        (1.0-settings.WetWeatherSpeedFraction) *
        source.MeasuredWetness;
    const double visibilityReduction=Lerp(
        settings.LowVisibilitySpeedFraction,
        1.0,source.MeasuredVisibility);
    node.WeatherLimitedSpeedCmS=
        settings.MaximumSpeedCmS *
        wetReduction * visibilityReduction;
    node.FlagLimitedSpeedCmS=
        source.CautionFlag ?
        settings.MaximumSpeedCmS *
            settings.YellowFlagSpeedFraction :
        settings.MaximumSpeedCmS;
    node.FinalTargetSpeedCmS=std::min({
        node.CurveLimitedSpeedCmS,
        node.WeatherLimitedSpeedCmS,
        node.FlagLimitedSpeedCmS
    });
    node.BrakingLimitedSpeedCmS=node.FinalTargetSpeedCmS;
    node.AcceleratingLimitedSpeedCmS=node.FinalTargetSpeedCmS;
    node.Valid=Finite(node.FinalTargetSpeedCmS) &&
        node.FinalTargetSpeedCmS>0.0;
    return node;
}

inline SpeedPlanResult BuildCornerSpeedPlan(
    SpeedPlanSource source,
    SpeedPlanParameters params={}) noexcept
{
    SpeedPlanResult plan;
    if (!ValidSpeedPlanParameters(params) ||
        !ValidSpeedPlanSource(source,params))
        return plan;
    plan.PlanHorizonCm=source.HorizonCm;
    plan.Count=std::clamp(
        static_cast<int>(std::ceil(source.HorizonCm/
                                   params.SampleSpacingCm))+1,
        2,SpeedPlanMaximumSamples);
    const double step=source.HorizonCm/
        static_cast<double>(plan.Count-1);
    plan.MinimumPlannedSpeedCmS=
        std::numeric_limits<double>::infinity();
    plan.AllOnOriginalRoad=true;
    for (int i=0;i<plan.Count;++i)
    {
        auto& node=plan.Nodes[static_cast<std::size_t>(i)];
        const double distance=step*i;
        node=BuildSpeedNode(
            source.Geometry,
            source.StartingDistanceCm+distance,
            distance,source,params);
        if (!node.Valid)
        {
            plan.AllOnOriginalRoad=false;
            return {};
        }
        plan.CurvedNodes+=node.IsCorner?1:0;
        plan.MaximumObservedCurvaturePerCm=std::max(
            plan.MaximumObservedCurvaturePerCm,
            std::abs(node.CurvaturePerCm));
        plan.FlagLimitedNodes+=source.CautionFlag?1:0;
    }
    const double brake=
        params.MaximumComfortDecelerationCmS2 *
        source.MeasuredBrakeEfficiency *
        std::clamp(source.MeasuredGripG,0.25,2.0);
    for (int pass=0;pass<params.MaximumBackwardPasses;++pass)
    {
        for (int i=plan.Count-2;i>=0;--i)
        {
            auto& current=plan.Nodes[static_cast<std::size_t>(i)];
            const auto& ahead=
                plan.Nodes[static_cast<std::size_t>(i+1)];
            const double available=std::sqrt(
                Square(ahead.FinalTargetSpeedCmS)+
                2.0*brake*step);
            current.BrakingLimitedSpeedCmS=std::min(
                current.BrakingLimitedSpeedCmS,available);
            current.FinalTargetSpeedCmS=std::min(
                current.FinalTargetSpeedCmS,available);
        }
    }
    const double acceleration=
        params.MaximumComfortAccelerationCmS2 *
        source.MeasuredAccelerationEfficiency;
    double previousSpeed=source.ObservedSpeedCmS;
    for (int i=0;i<plan.Count;++i)
    {
        auto& node=plan.Nodes[static_cast<std::size_t>(i)];
        const double available=std::sqrt(
            Square(previousSpeed)+
            2.0*acceleration*step);
        node.AcceleratingLimitedSpeedCmS=available;
        node.FinalTargetSpeedCmS=std::min(
            node.FinalTargetSpeedCmS,available);
        node.RequiredBrakeDecelerationCmS2=std::max(
            0.0,SafeDivision(
                Square(previousSpeed)-
                Square(node.FinalTargetSpeedCmS),
                2.0*step));
        if (node.RequiredBrakeDecelerationCmS2>1.0)
        {
            ++plan.BrakingNodes;
            if (plan.FirstRequiredBrakeDistanceCm==0.0)
                plan.FirstRequiredBrakeDistanceCm=
                    node.HorizonDistanceCm;
        }
        plan.MaximumRequiredDecelerationCmS2=std::max(
            plan.MaximumRequiredDecelerationCmS2,
            node.RequiredBrakeDecelerationCmS2);
        plan.MinimumPlannedSpeedCmS=std::min(
            plan.MinimumPlannedSpeedCmS,
            node.FinalTargetSpeedCmS);
        plan.MaximumPlannedSpeedCmS=std::max(
            plan.MaximumPlannedSpeedCmS,
            node.FinalTargetSpeedCmS);
        plan.MeanTargetSpeedCmS+=node.FinalTargetSpeedCmS;
        if (i>0)
        {
            const double intervalSpeed=std::max(
                40.0,0.5*(previousSpeed+
                           node.FinalTargetSpeedCmS));
            const double seconds=step/intervalSpeed;
            node.TimeToReachSeconds=
                plan.PredictedPlanSeconds+seconds;
            plan.PredictedPlanSeconds+=seconds;
        }
        previousSpeed=node.FinalTargetSpeedCmS;
    }
    plan.MeanTargetSpeedCmS/=plan.Count;
    plan.RequiresBrakingNow=
        source.ObservedSpeedCmS>
        plan.Nodes[0].FinalTargetSpeedCmS+100.0;
    plan.AppliesVehicleCommands=false;
    plan.Valid=plan.AllOnOriginalRoad &&
        Finite(plan.PredictedPlanSeconds);
    return plan;
}

struct SpeedTargetAdvice final
{
    double TargetSpeedCmS=0.0;
    double MinimumFutureSpeedCmS=0.0;
    double DistanceToBrakingCm=0.0;
    double RequiredDecelerationCmS2=0.0;
    double SuggestedThrottleCeiling=0.0;
    double SuggestedBrakeFraction=0.0;
    double Confidence=0.0;
    bool WouldBrake=false;
    bool CurveAhead=false;
    bool RestrictionOnly=true;
    bool ActualVehicleControlled=false;
    bool Valid=false;
};

inline SpeedTargetAdvice ReviewUpcomingSpeed(
    const SpeedPlanResult& plan,
    double currentSpeedCmS,double previewSeconds,
    double maximumDecelerationCmS2=1100.0) noexcept
{
    SpeedTargetAdvice out;
    if (!plan.Valid ||plan.Count<1 ||
        !Finite(currentSpeedCmS) ||
        !Finite(previewSeconds) ||
        !Finite(maximumDecelerationCmS2) ||
        currentSpeedCmS<0.0 ||
        previewSeconds<0.0 ||
        maximumDecelerationCmS2<=0.0) return out;
    const double previewDistance=
        std::clamp(currentSpeedCmS*previewSeconds,
                   0.0,plan.PlanHorizonCm);
    const SpeedPlanNode* selected=&plan.Nodes[0];
    out.MinimumFutureSpeedCmS=
        std::numeric_limits<double>::infinity();
    for (int i=0;i<plan.Count;++i)
    {
        const auto& node=plan.Nodes[static_cast<std::size_t>(i)];
        if (node.HorizonDistanceCm<=previewDistance)
        {
            selected=&node;
            out.MinimumFutureSpeedCmS=std::min(
                out.MinimumFutureSpeedCmS,
                node.FinalTargetSpeedCmS);
            out.CurveAhead=out.CurveAhead||node.IsCorner;
        }
    }
    out.TargetSpeedCmS=selected->FinalTargetSpeedCmS;
    out.DistanceToBrakingCm=
        plan.FirstRequiredBrakeDistanceCm;
    out.RequiredDecelerationCmS2=std::max(
        0.0,SafeDivision(
            Square(currentSpeedCmS)-
            Square(out.TargetSpeedCmS),
            2.0*std::max(100.0,previewDistance)));
    out.WouldBrake=
        currentSpeedCmS>out.TargetSpeedCmS+80.0;
    out.SuggestedBrakeFraction=Clamp01(
        out.RequiredDecelerationCmS2/
        maximumDecelerationCmS2);
    out.SuggestedThrottleCeiling=out.WouldBrake ?
        0.0:Clamp01(0.5+SafeDivision(
            out.TargetSpeedCmS-currentSpeedCmS,800.0));
    out.Confidence=
        plan.AllOnOriginalRoad ? 1.0:0.0;
    out.ActualVehicleControlled=false;
    out.Valid=true;
    return out;
}

class CornerSpeedPlanCache final
{
public:
    bool Configure(SpeedPlanParameters params) noexcept
    {
        if (!ValidSpeedPlanParameters(params)) return false;
        Params=params;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Replans=0;
        RejectedPlans=0;
        GripChanges=0;
        LastGrip=0.0;
        LastWet=0.0;
    }
    SpeedPlanResult Rebuild(SpeedPlanSource source) noexcept
    {
        const auto next=BuildCornerSpeedPlan(source,Params);
        if (!next.Valid)
        {
            ++RejectedPlans;
            return {};
        }
        if (Replans>0 &&
            std::abs(source.MeasuredGripG-LastGrip)>0.1)
            ++GripChanges;
        LastGrip=source.MeasuredGripG;
        LastWet=source.MeasuredWetness;
        Last=next;
        ++Replans;
        return Last;
    }
    SpeedPlanResult Snapshot() const noexcept
    {
        return Last;
    }
    std::uint64_t QualifiedReplans() const noexcept
    {
        return Replans;
    }
    std::uint64_t RejectedReplans() const noexcept
    {
        return RejectedPlans;
    }
private:
    SpeedPlanParameters Params{};
    SpeedPlanResult Last{};
    std::uint64_t Replans=0;
    std::uint64_t RejectedPlans=0;
    std::uint64_t GripChanges=0;
    double LastGrip=0.0;
    double LastWet=0.0;
};
} // namespace latebrake::racecraft
