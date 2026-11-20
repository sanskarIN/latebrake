#pragma once

#include "Track/WorldScaleBankedRoadPlan.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Future-circuit display geometry only. The active Unreal 5.8 scene keeps
// its original collision-bearing WorldStatic road and its existing racing
// clock, physical contacts, barriers, checkpoints, and AI route unchanged.
// A gravel or grass vertex here is never declared drivable collision.
inline constexpr unsigned WorldScaleRunoffSections=512u;
inline constexpr unsigned WorldScaleRunoffColumnsPerSide=3u;
inline constexpr unsigned WorldScaleRunoffSideCount=2u;
inline constexpr unsigned WorldScaleRunoffVerticesPerSection=
    WorldScaleRunoffColumnsPerSide*WorldScaleRunoffSideCount;
inline constexpr unsigned WorldScaleRunoffMeshTriangles=
    WorldScaleRunoffSections*WorldScaleRunoffSideCount*
    (WorldScaleRunoffColumnsPerSide-1u)*2u;

enum class WorldScaleRunoffSurface : std::uint8_t
{
    Unqualified,
    PermeableShoulder,
    GravelTrap,
    DrainageSwale,
    EarthBerm
};

struct WorldScaleRunoffParameters final
{
    double InnerBandStartCm=650.0;
    double GravelBandEndCm=1250.0;
    double DrainageBandEndCm=2400.0;
    double DrainSlopeFraction=0.025;
    double GrassBermRiseCm=55.0;
    double MaximumVerticalOffsetCm=220.0;
    bool RequirePhysicalRoadSeparation=true;
    bool PreserveOriginalRoadCollision=true;
    bool VisualOnly=true;
    bool NoLapOrRaceAuthority=true;
};

struct WorldScaleRunoffPoint final
{
    bool Valid=false;
    WorldScaleVariantId SourceCircuit=WorldScaleVariantId::OriginalLoop;
    WorldScaleRunoffSurface Surface=WorldScaleRunoffSurface::Unqualified;
    double StationCm=0.0;
    double SignedSideOffsetCm=0.0;
    double Xcm=0.0;
    double Ycm=0.0;
    double Zcm=0.0;
    double SlopeAwayFromRoad=0.0;
    double DrainageDepthCm=0.0;
    double ShoulderWidthCm=0.0;
    bool OutsideRoadCollision=true;
    bool ContactAuthorityIssued=false;
};

struct WorldScaleRunoffCrossSection final
{
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    double StationCm=0.0;
    std::array<WorldScaleRunoffPoint,
        WorldScaleRunoffVerticesPerSection> Points{};
    double LowestDrainagePointCm=0.0;
    double MaximumCrossSlope=0.0;
    double MinimumRoadSeparationCm=0.0;
    bool LiveCollisionUnchanged=true;
};

struct WorldScaleRunoffMeshPlan final
{
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    RoundedSmoothRoadMesh Visual{};
    double MinimumHeightCm=0.0;
    double MaximumHeightCm=0.0;
    double MinimumSourceRoadGapCm=0.0;
    unsigned QualifiedSections=0u;
    unsigned QualifiedVertices=0u;
    bool CannotBeDrivenOn=true;
    bool NoUnrealMeshInstalled=true;
    bool OriginalPhysicalRoadUnchanged=true;
};

inline bool ValidWorldScaleRunoffParameters(
    const WorldScaleVariantRecipe& circuit,
    WorldScaleRunoffParameters p) noexcept
{
    return ValidWorldScaleVariantRecipe(circuit) &&
        std::isfinite(p.InnerBandStartCm) &&
        p.InnerBandStartCm>circuit.RoadHalfWidthCm+100.0 &&
        p.InnerBandStartCm<=950.0 &&
        std::isfinite(p.GravelBandEndCm) &&
        p.GravelBandEndCm>=1000.0 &&
        p.GravelBandEndCm>p.InnerBandStartCm+250.0 &&
        p.GravelBandEndCm<=1800.0 &&
        std::isfinite(p.DrainageBandEndCm) &&
        p.DrainageBandEndCm>p.GravelBandEndCm+400.0 &&
        p.DrainageBandEndCm<=3000.0 &&
        std::isfinite(p.DrainSlopeFraction) &&
        p.DrainSlopeFraction>=0.005 &&
        p.DrainSlopeFraction<=0.06 &&
        std::isfinite(p.GrassBermRiseCm) &&
        p.GrassBermRiseCm>=0.0 &&
        p.GrassBermRiseCm<=150.0 &&
        std::isfinite(p.MaximumVerticalOffsetCm) &&
        p.MaximumVerticalOffsetCm>=150.0 &&
        p.MaximumVerticalOffsetCm<=400.0 &&
        p.RequirePhysicalRoadSeparation &&
        p.PreserveOriginalRoadCollision &&
        p.VisualOnly &&
        p.NoLapOrRaceAuthority;
}

inline double WorldScaleRunoffSmoothStep(double t) noexcept
{
    t=std::clamp(t,0.0,1.0);
    return t*t*(3.0-2.0*t);
}

inline WorldScaleRunoffSurface WorldScaleClassifyRunoff(
    double distanceFromRoadCm,
    WorldScaleRunoffParameters p) noexcept
{
    if(!std::isfinite(distanceFromRoadCm) ||
       distanceFromRoadCm<p.InnerBandStartCm ||
       distanceFromRoadCm>p.DrainageBandEndCm)
        return WorldScaleRunoffSurface::Unqualified;
    if(distanceFromRoadCm<p.GravelBandEndCm)
        return WorldScaleRunoffSurface::GravelTrap;
    const double span=p.DrainageBandEndCm-p.GravelBandEndCm;
    if(distanceFromRoadCm<p.GravelBandEndCm+span*0.55)
        return WorldScaleRunoffSurface::DrainageSwale;
    return WorldScaleRunoffSurface::EarthBerm;
}

// The outer shoulder follows the exact periodic Z of the original road,
// but all proposed runoff terrain starts beyond the WorldStatic collision
// road footprint. Never use an off-road surface for race-physics authority.
inline WorldScaleRunoffPoint SampleWorldScaleRunoff(
    const WorldScaleVariantRecipe& circuit,
    WorldScaleBankedRoadProfile road,
    WorldScaleRunoffParameters settings,
    double stationCm, double signedLateralCm) noexcept
{
    WorldScaleRunoffPoint out;
    if(!ValidWorldScaleRunoffParameters(circuit,settings) ||
       !ValidWorldScaleBankedRoadProfile(circuit,road) ||
       !std::isfinite(stationCm) ||
       !std::isfinite(signedLateralCm) ||
       std::abs(stationCm)>1.0e9)
        return out;
    const double distance=std::abs(signedLateralCm);
    const auto surface=WorldScaleClassifyRunoff(
        distance,settings);
    if(surface==WorldScaleRunoffSurface::Unqualified)return out;
    const double wrapped=WorldScaleVariantWrapDistance(
        circuit,stationCm);
    const auto center=WorldScaleVariantCenterlineAt(
        circuit,wrapped);
    if(!center.Valid || !std::isfinite(wrapped))return out;
    const double heading=center.Pose.HeadingDegrees*
        RoundedPi/180.0;
    const double nx=-std::sin(heading);
    const double ny=std::cos(heading);
    const CircuitPoint pos{
        center.Pose.Position.X+nx*signedLateralCm,
        center.Pose.Position.Y+ny*signedLateralCm
    };
    if(!std::isfinite(pos.X) || !std::isfinite(pos.Y) ||
       IsOnWorldScaleVariantRoad(circuit,pos))
        return out;
    const double edgeSigned=
        std::copysign(circuit.VisualHalfWidthCm,signedLateralCm);
    const auto edge=SampleWorldScaleBankedRoad(
        circuit,road,wrapped,edgeSigned);
    if(!edge.Valid)return out;

    const double beyondEdge=distance-circuit.VisualHalfWidthCm;
    const double gravelWidth=
        settings.GravelBandEndCm-circuit.VisualHalfWidthCm;
    const double downhillDepth=
        settings.DrainSlopeFraction*
        std::min(beyondEdge,gravelWidth);
    const double outerFraction=std::clamp(
        (distance-settings.GravelBandEndCm)/
        (settings.DrainageBandEndCm-settings.GravelBandEndCm),
        0.0,1.0);
    const double bernShape=WorldScaleRunoffSmoothStep(outerFraction);
    const double bermRaise=settings.GrassBermRiseCm*bernShape;
    const double height=edge.WorldZcm-downhillDepth+bermRaise;
    const double limit=settings.MaximumVerticalOffsetCm;
    if(!std::isfinite(height) ||
       std::abs(height-edge.WorldZcm)>limit)
        return out;
    const double derivative=distance<=settings.GravelBandEndCm ?
        -settings.DrainSlopeFraction :
        (settings.GrassBermRiseCm*
         6.0*outerFraction*(1.0-outerFraction))/
        (settings.DrainageBandEndCm-settings.GravelBandEndCm);
    if(!std::isfinite(derivative) ||
       std::abs(derivative)>0.3)
        return out;
    out.Valid=true;
    out.SourceCircuit=circuit.Id;
    out.Surface=surface;
    out.StationCm=wrapped;
    out.SignedSideOffsetCm=signedLateralCm;
    out.Xcm=pos.X;
    out.Ycm=pos.Y;
    out.Zcm=height;
    out.SlopeAwayFromRoad=derivative;
    out.DrainageDepthCm=downhillDepth;
    out.ShoulderWidthCm=distance-circuit.RoadHalfWidthCm;
    out.OutsideRoadCollision=true;
    out.ContactAuthorityIssued=false;
    return out;
}

inline WorldScaleRunoffCrossSection SampleWorldScaleRunoffSection(
    const WorldScaleVariantRecipe& circuit,
    WorldScaleBankedRoadProfile road,
    WorldScaleRunoffParameters settings,
    double stationCm) noexcept
{
    WorldScaleRunoffCrossSection out;
    if(!ValidWorldScaleRunoffParameters(circuit,settings) ||
       !ValidWorldScaleBankedRoadProfile(circuit,road))
        return out;
    const std::array<double,WorldScaleRunoffColumnsPerSide> offsets{{
        settings.InnerBandStartCm,
        settings.GravelBandEndCm,
        settings.DrainageBandEndCm
    }};
    out.Circuit=circuit.Id;
    for(unsigned side=0;side<WorldScaleRunoffSideCount;++side)
    {
        for(unsigned column=0;column<WorldScaleRunoffColumnsPerSide;++column)
        {
            const double sign=side==0u ? 1.0:-1.0;
            const auto point=SampleWorldScaleRunoff(
                circuit,road,settings,stationCm,
                sign*offsets[column]);
            if(!point.Valid || !point.OutsideRoadCollision ||
               point.ContactAuthorityIssued)
                return {};
            out.Points[side*WorldScaleRunoffColumnsPerSide+
                       column]=point;
            if(side==0u && column==0u)
            {
                out.MinimumRoadSeparationCm=point.ShoulderWidthCm;
                out.LowestDrainagePointCm=point.Zcm;
            }
            out.MinimumRoadSeparationCm=std::min(
                out.MinimumRoadSeparationCm,
                point.ShoulderWidthCm);
            out.LowestDrainagePointCm=std::min(
                out.LowestDrainagePointCm,point.Zcm);
            out.MaximumCrossSlope=std::max(
                out.MaximumCrossSlope,
                std::abs(point.SlopeAwayFromRoad));
        }
    }
    out.StationCm=out.Points[0].StationCm;
    out.Valid=out.MaximumCrossSlope<=0.3 &&
        out.MinimumRoadSeparationCm>=100.0;
    return out;
}

inline WorldScaleRunoffMeshPlan BuildWorldScaleRunoffMesh(
    const WorldScaleVariantRecipe& circuit,
    WorldScaleBankedRoadProfile road,
    WorldScaleRunoffParameters settings={})
{
    WorldScaleRunoffMeshPlan out;
    if(!ValidWorldScaleRunoffParameters(circuit,settings) ||
       !ValidWorldScaleBankedRoadProfile(circuit,road))
        return out;
    out.Circuit=circuit.Id;
    out.MinimumHeightCm=std::numeric_limits<double>::infinity();
    out.MaximumHeightCm=-std::numeric_limits<double>::infinity();
    out.MinimumSourceRoadGapCm=
        std::numeric_limits<double>::infinity();
    out.Visual.Vertices.reserve(
        WorldScaleRunoffSections*WorldScaleRunoffVerticesPerSection);
    out.Visual.Triangles.reserve(WorldScaleRunoffMeshTriangles);
    for(unsigned station=0;station<WorldScaleRunoffSections;++station)
    {
        const double s=circuit.LapLengthCm*
            static_cast<double>(station)/
            WorldScaleRunoffSections;
        const auto section=SampleWorldScaleRunoffSection(
            circuit,road,settings,s);
        if(!section.Valid)return {};
        for(const auto& point:section.Points)
        {
            if(!point.Valid || !point.OutsideRoadCollision)
                return {};
            RoundedSmoothVertex vertex{
                point.Xcm,point.Ycm,point.Zcm};
            if(!IsFiniteRoundedSmoothVertex(vertex))
                return {};
            out.Visual.Vertices.push_back(vertex);
            out.MinimumHeightCm=std::min(
                out.MinimumHeightCm,point.Zcm);
            out.MaximumHeightCm=std::max(
                out.MaximumHeightCm,point.Zcm);
            out.MinimumSourceRoadGapCm=std::min(
                out.MinimumSourceRoadGapCm,
                point.ShoulderWidthCm);
            ++out.QualifiedVertices;
        }
        ++out.QualifiedSections;
    }
    // Each shoulder is an independent, open-width, closed-lap ribbon.
    // Its top strips are deliberately NONCOLLIDING. No bottom collision
    // faces or fake drivable surface are produced by this visual plan.
    for(std::uint32_t i=0;i<WorldScaleRunoffSections;++i)
    {
        const std::uint32_t next=
            (i+1u)%WorldScaleRunoffSections;
        const std::uint32_t a=i*
            WorldScaleRunoffVerticesPerSection;
        const std::uint32_t b=next*
            WorldScaleRunoffVerticesPerSection;
        for(std::uint32_t side=0;side<WorldScaleRunoffSideCount;++side)
        {
            for(std::uint32_t c=0;
                c+1u<WorldScaleRunoffColumnsPerSide;++c)
            {
                const std::uint32_t offset=
                    side*WorldScaleRunoffColumnsPerSide+c;
                const std::uint32_t aa=a+offset;
                const std::uint32_t ab=a+offset+1u;
                const std::uint32_t ba=b+offset;
                const std::uint32_t bb=b+offset+1u;
                if(side==0u)
                {
                    out.Visual.Triangles.push_back({
                        {aa,bb,ba}});
                    out.Visual.Triangles.push_back({
                        {aa,ab,bb}});
                }
                else
                {
                    out.Visual.Triangles.push_back({
                        {aa,ba,bb}});
                    out.Visual.Triangles.push_back({
                        {aa,bb,ab}});
                }
            }
        }
    }
    out.Valid=out.QualifiedSections==WorldScaleRunoffSections &&
        out.QualifiedVertices==WorldScaleRunoffSections*
            WorldScaleRunoffVerticesPerSection &&
        out.Visual.Triangles.size()==WorldScaleRunoffMeshTriangles &&
        out.MinimumSourceRoadGapCm>=100.0 &&
        out.MinimumHeightCm<=out.MaximumHeightCm;
    return out;
}
} // namespace latebrake
