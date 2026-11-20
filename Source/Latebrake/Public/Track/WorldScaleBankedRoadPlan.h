#pragma once

#include "Track/WorldScaleCircuitVariants.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

namespace latebrake
{
// A future visual-road proposal for the six original source circuits.
// It never modifies the current Unreal flat WorldStatic collision slabs,
// drives Chaos contact, authenticates lap timing, or awards game progress.
struct WorldScaleBankedRoadProfile final
{
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    double PrimaryCrestCm=0.0;
    double SecondaryCrestCm=0.0;
    double BankAmplitudeRadians=0.0;
    double RoadTopCm=WorldScaleVariantRoadTopCm;
    bool SourceOnly=true;
    bool PreserveExistingCollision=true;
};

struct WorldScaleBankedSurfaceSample final
{
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    double WrappedStationCm=0.0;
    double LateralOffsetCm=0.0;
    double WorldXcm=0.0;
    double WorldYcm=0.0;
    double WorldZcm=0.0;
    double GradeRadians=0.0;
    double BankRadians=0.0;
    double SurfaceNormalX=0.0;
    double SurfaceNormalY=0.0;
    double SurfaceNormalZ=1.0;
    bool OriginalRoadXYQualified=false;
    bool OriginalCollisionUnaffected=true;
};

struct WorldScaleBankedWheelFootprint final
{
    bool Valid=false;
    std::array<WorldScaleBankedSurfaceSample,4> Corners{};
    double MeanCenterHeightCm=0.0;
    double FrontAxleHeightCm=0.0;
    double RearAxleHeightCm=0.0;
    double PitchRadians=0.0;
    double RollRadians=0.0;
    double MaximumWheelHeightDifferenceCm=0.0;
    bool SourceOnly=true;
};

struct WorldScaleBankedRoadAudit final
{
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    double MinimumTopCm=0.0;
    double MaximumTopCm=0.0;
    double MaximumAbsGradeRadians=0.0;
    double MaximumAbsBankRadians=0.0;
    double MaximumAdjacentTopStepCm=0.0;
    double MaximumSeamErrorCm=0.0;
    unsigned QualifiedCenterStations=0;
    unsigned QualifiedEdgeStations=0;
    bool OriginalCollisionUnchanged=true;
    bool UnrealPlaytestQualified=false;
};

inline bool ChooseWorldScaleBankedRoadProfile(
    WorldScaleVariantId id, WorldScaleBankedRoadProfile& out) noexcept
{
    out={};
    out.Circuit=id;
    switch(id)
    {
    case WorldScaleVariantId::OriginalLoop:
        out.PrimaryCrestCm=0.0;
        out.SecondaryCrestCm=0.0;
        out.BankAmplitudeRadians=0.028;
        return true;
    case WorldScaleVariantId::EnduranceRing:
        out.PrimaryCrestCm=130.0;
        out.SecondaryCrestCm=38.0;
        out.BankAmplitudeRadians=0.047;
        return true;
    case WorldScaleVariantId::TechnicalCrest:
        out.PrimaryCrestCm=345.0;
        out.SecondaryCrestCm=118.0;
        out.BankAmplitudeRadians=0.075;
        return true;
    case WorldScaleVariantId::HarborClub:
        out.PrimaryCrestCm=48.0;
        out.SecondaryCrestCm=15.0;
        out.BankAmplitudeRadians=0.033;
        return true;
    case WorldScaleVariantId::CanyonRun:
        out.PrimaryCrestCm=260.0;
        out.SecondaryCrestCm=156.0;
        out.BankAmplitudeRadians=0.095;
        return true;
    case WorldScaleVariantId::TestOval:
        out.PrimaryCrestCm=22.0;
        out.SecondaryCrestCm=5.0;
        out.BankAmplitudeRadians=0.025;
        return true;
    }
    out={};
    return false;
}

inline bool ValidWorldScaleBankedRoadProfile(
    const WorldScaleVariantRecipe& circuit,
    WorldScaleBankedRoadProfile profile) noexcept
{
    return ValidWorldScaleVariantRecipe(circuit) &&
        profile.Circuit==circuit.Id &&
        std::isfinite(profile.PrimaryCrestCm) &&
        std::isfinite(profile.SecondaryCrestCm) &&
        std::isfinite(profile.BankAmplitudeRadians) &&
        std::isfinite(profile.RoadTopCm) &&
        profile.PrimaryCrestCm>=0.0 &&
        profile.PrimaryCrestCm<=400.0 &&
        profile.SecondaryCrestCm>=0.0 &&
        profile.SecondaryCrestCm<=180.0 &&
        profile.BankAmplitudeRadians>=0.0 &&
        profile.BankAmplitudeRadians<=0.12 &&
        profile.RoadTopCm==WorldScaleVariantRoadTopCm &&
        profile.SourceOnly &&
        profile.PreserveExistingCollision;
}

// Sinusoidal harmonics are C-infinity and periodic at both start/finish
// ends. Sampling any lap (including signed distance) yields the same road.
// These proposed heights are NOT current Unreal collision heights.
inline WorldScaleBankedSurfaceSample SampleWorldScaleBankedRoad(
    const WorldScaleVariantRecipe& circuit,
    WorldScaleBankedRoadProfile profile,
    double unwrappedStationCm, double lateralOffsetCm) noexcept
{
    WorldScaleBankedSurfaceSample out;
    if(!ValidWorldScaleBankedRoadProfile(circuit,profile) ||
       !std::isfinite(unwrappedStationCm) ||
       !std::isfinite(lateralOffsetCm) ||
       std::abs(unwrappedStationCm)>1.0e9 ||
       std::abs(lateralOffsetCm)>circuit.VisualHalfWidthCm)
        return out;
    const double station=WorldScaleVariantWrapDistance(
        circuit,unwrappedStationCm);
    const auto center=WorldScaleVariantCenterlineAt(circuit,station);
    if(!std::isfinite(station) || !center.Valid)
        return out;

    const double omega=2.0*RoundedPi/circuit.LapLengthCm;
    const double phase=omega*station;
    const double banking=profile.BankAmplitudeRadians*
        std::sin(2.0*phase);
    const double bankSlope=std::tan(banking);
    const double bankDerivative=2.0*omega*
        profile.BankAmplitudeRadians*std::cos(2.0*phase);
    const double baseHeight=profile.PrimaryCrestCm*std::sin(phase)+
        profile.SecondaryCrestCm*std::sin(3.0*phase);
    const double baseDerivative=omega*(
        profile.PrimaryCrestCm*std::cos(phase)+
        3.0*profile.SecondaryCrestCm*std::cos(3.0*phase));
    const double longitudinalSlope=baseDerivative+
        lateralOffsetCm*(1.0+bankSlope*bankSlope)*bankDerivative;
    const double height=profile.RoadTopCm+baseHeight+
        lateralOffsetCm*bankSlope;

    const double heading=center.Pose.HeadingDegrees*
        RoundedPi/180.0;
    const double tx=std::cos(heading);
    const double ty=std::sin(heading);
    const double nx=-ty;
    const double ny=tx;
    const double x=center.Pose.Position.X+nx*lateralOffsetCm;
    const double y=center.Pose.Position.Y+ny*lateralOffsetCm;
    // First-order along-route derivative in the route's tangent frame.
    // Native mesh normal and wheel contact require later UE validation.
    double normalX=-longitudinalSlope*tx-bankSlope*nx;
    double normalY=-longitudinalSlope*ty-bankSlope*ny;
    const double magnitude=std::hypot(
        std::hypot(normalX,normalY),1.0);
    if(!std::isfinite(x) || !std::isfinite(y) ||
       !std::isfinite(height) || !std::isfinite(longitudinalSlope) ||
       !std::isfinite(magnitude) || magnitude<=0.0 ||
       !IsOnWorldScaleVariantRoad(circuit,{x,y}))
        return out;
    normalX/=magnitude;
    normalY/=magnitude;
    const double normalZ=1.0/magnitude;
    if(!std::isfinite(normalX) || !std::isfinite(normalY) ||
       !std::isfinite(normalZ) || normalZ<=0.0)
        return out;
    out.Valid=true;
    out.Circuit=circuit.Id;
    out.WrappedStationCm=station;
    out.LateralOffsetCm=lateralOffsetCm;
    out.WorldXcm=x;
    out.WorldYcm=y;
    out.WorldZcm=height;
    out.GradeRadians=std::atan(longitudinalSlope);
    out.BankRadians=banking;
    out.SurfaceNormalX=normalX;
    out.SurfaceNormalY=normalY;
    out.SurfaceNormalZ=normalZ;
    out.OriginalRoadXYQualified=true;
    return out;
}

// Four corners of the physical car FOOTPRINT on the proposed height field.
// No live wheel traces or physics collision guarantees are implied.
inline WorldScaleBankedWheelFootprint SampleWorldScaleBankedFootprint(
    const WorldScaleVariantRecipe& circuit,
    WorldScaleBankedRoadProfile profile,
    double centerStationCm, double laneOffsetCm,
    double wheelbaseCm=270.0, double trackWidthCm=160.0) noexcept
{
    WorldScaleBankedWheelFootprint out;
    if(!std::isfinite(centerStationCm) ||
       !std::isfinite(laneOffsetCm) ||
       !std::isfinite(wheelbaseCm) || !std::isfinite(trackWidthCm) ||
       wheelbaseCm<180.0 || wheelbaseCm>360.0 ||
       trackWidthCm<120.0 || trackWidthCm>210.0 ||
       std::abs(laneOffsetCm)+trackWidthCm*0.5>
           circuit.VisualHalfWidthCm)
        return out;
    const double front=centerStationCm+wheelbaseCm*0.5;
    const double rear=centerStationCm-wheelbaseCm*0.5;
    const double left=laneOffsetCm+trackWidthCm*0.5;
    const double right=laneOffsetCm-trackWidthCm*0.5;
    out.Corners[0]=SampleWorldScaleBankedRoad(
        circuit,profile,front,left);
    out.Corners[1]=SampleWorldScaleBankedRoad(
        circuit,profile,front,right);
    out.Corners[2]=SampleWorldScaleBankedRoad(
        circuit,profile,rear,left);
    out.Corners[3]=SampleWorldScaleBankedRoad(
        circuit,profile,rear,right);
    for(const auto& corner:out.Corners)
        if(!corner.Valid)return {};
    out.FrontAxleHeightCm=(out.Corners[0].WorldZcm+
                             out.Corners[1].WorldZcm)*0.5;
    out.RearAxleHeightCm=(out.Corners[2].WorldZcm+
                            out.Corners[3].WorldZcm)*0.5;
    out.MeanCenterHeightCm=(out.FrontAxleHeightCm+
                              out.RearAxleHeightCm)*0.5;
    out.PitchRadians=std::atan2(
        out.FrontAxleHeightCm-out.RearAxleHeightCm,wheelbaseCm);
    const double leftHeight=(out.Corners[0].WorldZcm+
                               out.Corners[2].WorldZcm)*0.5;
    const double rightHeight=(out.Corners[1].WorldZcm+
                                out.Corners[3].WorldZcm)*0.5;
    out.RollRadians=std::atan2(
        leftHeight-rightHeight,trackWidthCm);
    double bottom=out.Corners[0].WorldZcm;
    double top=bottom;
    for(const auto& corner:out.Corners)
    {
        bottom=std::min(bottom,corner.WorldZcm);
        top=std::max(top,corner.WorldZcm);
    }
    out.MaximumWheelHeightDifferenceCm=top-bottom;
    out.Valid=std::isfinite(out.PitchRadians) &&
        std::isfinite(out.RollRadians) &&
        std::isfinite(out.MaximumWheelHeightDifferenceCm);
    return out;
}

inline RoundedSmoothRoadMesh BuildWorldScaleBankedRoadVisualCandidate(
    const WorldScaleVariantRecipe& circuit,
    WorldScaleBankedRoadProfile profile)
{
    if(!ValidWorldScaleBankedRoadProfile(circuit,profile))
        return {};
    // Reuse the already independently qualified XY triangles, seam and
    // closed walls of the original six-circuit display-only ribbon.
    // No mesh object or collision component is created in the live game.
    auto mesh=BuildWorldScaleVariantVisualMesh(circuit);
    constexpr unsigned count=WorldScaleVariantVisualMeshSections;
    if(mesh.Vertices.size()!=4u*count ||
       mesh.Triangles.size()!=8u*count)
        return {};
    for(unsigned i=0;i<count;++i)
    {
        const double station=circuit.LapLengthCm*i/
            static_cast<double>(count);
        for(unsigned side=0;side<2u;++side)
        {
            const double offset=side==0u ?
                circuit.VisualHalfWidthCm :
                -circuit.VisualHalfWidthCm;
            const auto sampled=SampleWorldScaleBankedRoad(
                circuit,profile,station,offset);
            if(!sampled.Valid)return {};
            const auto top=static_cast<std::size_t>(4u*i+side);
            const auto bottom=top+2u;
            const double shift=sampled.WorldZcm-RoundedSmoothTopCm;
            mesh.Vertices[top].Z+=shift;
            mesh.Vertices[bottom].Z+=shift;
            if(!IsFiniteRoundedSmoothVertex(mesh.Vertices[top]) ||
               !IsFiniteRoundedSmoothVertex(mesh.Vertices[bottom]))
                return {};
        }
    }
    return mesh;
}

inline WorldScaleBankedRoadAudit AuditWorldScaleBankedRoad(
    const WorldScaleVariantRecipe& circuit,
    WorldScaleBankedRoadProfile profile) noexcept
{
    WorldScaleBankedRoadAudit out;
    if(!ValidWorldScaleBankedRoadProfile(circuit,profile))
        return out;
    out.Circuit=circuit.Id;
    out.MinimumTopCm=std::numeric_limits<double>::infinity();
    out.MaximumTopCm=-std::numeric_limits<double>::infinity();
    double previous=0.0;
    bool havePrevious=false;
    constexpr unsigned count=WorldScaleVariantVisualMeshSections;
    for(unsigned i=0;i<count;++i)
    {
        const double station=circuit.LapLengthCm*i/
            static_cast<double>(count);
        const auto middle=SampleWorldScaleBankedRoad(
            circuit,profile,station,0.0);
        if(!middle.Valid)return {};
        const auto left=SampleWorldScaleBankedRoad(
            circuit,profile,station,circuit.VisualHalfWidthCm);
        const auto right=SampleWorldScaleBankedRoad(
            circuit,profile,station,-circuit.VisualHalfWidthCm);
        if(!left.Valid || !right.Valid)return {};
        out.MinimumTopCm=std::min({
            out.MinimumTopCm,middle.WorldZcm,
            left.WorldZcm,right.WorldZcm});
        out.MaximumTopCm=std::max({
            out.MaximumTopCm,middle.WorldZcm,
            left.WorldZcm,right.WorldZcm});
        out.MaximumAbsGradeRadians=std::max(
            out.MaximumAbsGradeRadians,
            std::max({std::abs(middle.GradeRadians),
                      std::abs(left.GradeRadians),
                      std::abs(right.GradeRadians)}));
        out.MaximumAbsBankRadians=std::max(
            out.MaximumAbsBankRadians,std::abs(middle.BankRadians));
        if(havePrevious)
            out.MaximumAdjacentTopStepCm=std::max(
                out.MaximumAdjacentTopStepCm,
                std::abs(middle.WorldZcm-previous));
        previous=middle.WorldZcm;
        havePrevious=true;
        ++out.QualifiedCenterStations;
        out.QualifiedEdgeStations+=2u;
    }
    const auto seam=SampleWorldScaleBankedRoad(
        circuit,profile,circuit.LapLengthCm,0.0);
    const auto first=SampleWorldScaleBankedRoad(
        circuit,profile,0.0,0.0);
    if(!seam.Valid || !first.Valid)return {};
    out.MaximumAdjacentTopStepCm=std::max(
        out.MaximumAdjacentTopStepCm,
        std::abs(previous-first.WorldZcm));
    out.MaximumSeamErrorCm=std::abs(
        seam.WorldZcm-first.WorldZcm);
    out.Valid=out.QualifiedCenterStations==count &&
        out.QualifiedEdgeStations==2u*count &&
        out.MaximumAbsGradeRadians<=0.15 &&
        out.MaximumAbsBankRadians<=0.12 &&
        out.MaximumAdjacentTopStepCm<=50.0 &&
        out.MaximumSeamErrorCm<=1.0e-8 &&
        std::isfinite(out.MinimumTopCm) &&
        std::isfinite(out.MaximumTopCm);
    return out;
}
} // namespace latebrake
