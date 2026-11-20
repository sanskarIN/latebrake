#pragma once

#include "Track/WorldScaleRunoffTerrainPlan.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace latebrake
{
// ORIGINAL candidate trackside scene, intentionally independent of the
// existing drivable Chaos road/collisions and any paid race authority.
// These are deterministic proposal positions and low-LOD proxy meshes,
// not claims of realistic finished art, imported assets or cooked maps.
inline constexpr unsigned TracksideSceneStationsPerSide=64u;
inline constexpr unsigned TracksideSceneObjectCount=
    TracksideSceneStationsPerSide*2u;
inline constexpr double TracksideSceneMaximumFootprintHalfCm=900.0;

enum class TracksideSceneKind : std::uint8_t
{
    Floodlight,
    MarshalPost,
    CameraTower,
    RescueAccess,
    WayfindingSign,
    Grandstand,
    WoodlandCanopy
};

struct TracksideSceneSettings final
{
    double WorldRoadClearanceCm=175.0;
    double MinimumAdjacentStationGapCm=1700.0;
    double MaximumStructureHeightCm=1600.0;
    double MaximumProxyDrawDistanceCm=65000.0;
    double BaseTerrainEmbedCm=5.0;
    double MinimumShaftHeightCm=120.0;
    bool PreserveOriginalRoadBody=true;
    bool NoRuntimeActorSpawn=true;
    bool VisualMeshesHaveNoCollision=true;
    bool PreserveOriginalRaceClock=true;
};

struct TracksideSceneKindRecipe final
{
    TracksideSceneKind Kind=TracksideSceneKind::Floodlight;
    double HalfWidthCm=80.0;
    double HalfLengthCm=80.0;
    double HeightCm=750.0;
    double MaxDrawDistanceCm=28000.0;
    unsigned ProxyMaterialIndex=0u;
    bool SourceOnly=true;
};

struct TracksideSceneObject final
{
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    TracksideSceneKind Kind=TracksideSceneKind::Floodlight;
    double StationCm=0.0;
    double SignedOffsetCm=0.0;
    double Xcm=0.0;
    double Ycm=0.0;
    double Zcm=0.0;
    double YawRadians=0.0;
    double HalfWidthCm=0.0;
    double HalfLengthCm=0.0;
    double HeightCm=0.0;
    double MaximumDrawDistanceCm=0.0;
    unsigned MaterialIndex=0u;
    std::uint64_t DeterministicId=0;
    bool EveryBaseCornerOutsideRoad=false;
    bool UnrealActorSpawned=false;
    bool DrivableCollisionAdded=false;
};

struct TracksideScenePlan final
{
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::array<TracksideSceneObject,TracksideSceneObjectCount>
        Objects{};
    unsigned QualifiedObjects=0u;
    double MinimumObjectRoadClearanceCm=0.0;
    double SmallestForwardObjectSpacingCm=0.0;
    double TallestObjectCm=0.0;
    double FurthestDrawDistanceCm=0.0;
    bool OriginalCollisionUnchanged=true;
    bool SourceOnly=true;
};

struct TracksideProxyMesh final
{
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::vector<RoundedSmoothVertex> Vertices;
    std::vector<RoundedSmoothTriangle> Triangles;
    std::vector<unsigned> TriangleMaterial;
    unsigned QualifiedObjects=0u;
    bool HasCollisionBodies=false;
    bool InstalledInUnreal=false;
};

inline bool ValidTracksideSceneSettings(
    TracksideSceneSettings p) noexcept
{
    const auto finite=[](double v) noexcept {
        return std::isfinite(v);
    };
    return finite(p.WorldRoadClearanceCm) &&
        p.WorldRoadClearanceCm>=120.0 &&
        p.WorldRoadClearanceCm<=500.0 &&
        finite(p.MinimumAdjacentStationGapCm) &&
        p.MinimumAdjacentStationGapCm>=1200.0 &&
        p.MinimumAdjacentStationGapCm<=4000.0 &&
        finite(p.MaximumStructureHeightCm) &&
        p.MaximumStructureHeightCm>=500.0 &&
        p.MaximumStructureHeightCm<=2500.0 &&
        finite(p.MaximumProxyDrawDistanceCm) &&
        p.MaximumProxyDrawDistanceCm>=5000.0 &&
        p.MaximumProxyDrawDistanceCm<=100000.0 &&
        finite(p.BaseTerrainEmbedCm) &&
        p.BaseTerrainEmbedCm>=0.0 &&
        p.BaseTerrainEmbedCm<=25.0 &&
        finite(p.MinimumShaftHeightCm) &&
        p.MinimumShaftHeightCm>=100.0 &&
        p.MinimumShaftHeightCm<=300.0 &&
        p.PreserveOriginalRoadBody &&
        p.NoRuntimeActorSpawn &&
        p.VisualMeshesHaveNoCollision &&
        p.PreserveOriginalRaceClock;
}

inline TracksideSceneKindRecipe TracksideKindRecipe(
    TracksideSceneKind kind) noexcept
{
    switch(kind)
    {
    case TracksideSceneKind::Floodlight:
        return {kind,38.0,38.0,900.0,50000.0,1u,true};
    case TracksideSceneKind::MarshalPost:
        return {kind,165.0,150.0,300.0,17000.0,2u,true};
    case TracksideSceneKind::CameraTower:
        return {kind,95.0,95.0,1100.0,40000.0,3u,true};
    case TracksideSceneKind::RescueAccess:
        return {kind,400.0,650.0,350.0,15000.0,4u,true};
    case TracksideSceneKind::WayfindingSign:
        return {kind,200.0,70.0,450.0,24000.0,5u,true};
    case TracksideSceneKind::Grandstand:
        return {kind,850.0,750.0,1350.0,60000.0,6u,true};
    case TracksideSceneKind::WoodlandCanopy:
        return {kind,250.0,260.0,1200.0,35000.0,7u,true};
    }
    return {};
}

inline bool ValidTracksideRecipe(
    TracksideSceneKindRecipe item,
    TracksideSceneSettings settings={}) noexcept
{
    return ValidTracksideSceneSettings(settings) &&
        std::isfinite(item.HalfWidthCm) &&
        std::isfinite(item.HalfLengthCm) &&
        std::isfinite(item.HeightCm) &&
        std::isfinite(item.MaxDrawDistanceCm) &&
        item.HalfWidthCm>25.0 &&
        item.HalfLengthCm>25.0 &&
        item.HalfWidthCm<=TracksideSceneMaximumFootprintHalfCm &&
        item.HalfLengthCm<=TracksideSceneMaximumFootprintHalfCm &&
        item.HeightCm>=settings.MinimumShaftHeightCm &&
        item.HeightCm<=settings.MaximumStructureHeightCm &&
        item.MaxDrawDistanceCm>0.0 &&
        item.MaxDrawDistanceCm<=
            settings.MaximumProxyDrawDistanceCm &&
        item.ProxyMaterialIndex>0u &&
        item.ProxyMaterialIndex<=7u &&
        item.SourceOnly;
}

inline bool TracksideFootprintEntirelyOffRoad(
    const WorldScaleVariantRecipe& circuit,
    CircuitPoint center,
    double yawRadians,
    double halfWidthCm,
    double halfLengthCm) noexcept
{
    if(!ValidWorldScaleVariantRecipe(circuit) ||
       !std::isfinite(center.X) ||
       !std::isfinite(center.Y) ||
       !std::isfinite(yawRadians) ||
       !std::isfinite(halfWidthCm) ||
       !std::isfinite(halfLengthCm) ||
       halfWidthCm<0.0 ||
       halfLengthCm<0.0)
        return false;
    const double tx=std::cos(yawRadians);
    const double ty=std::sin(yawRadians);
    const double nx=-ty;
    const double ny=tx;
    for(double along:{-halfLengthCm,0.0,halfLengthCm})
    {
        for(double across:{-halfWidthCm,0.0,halfWidthCm})
        {
            const CircuitPoint candidate{
                center.X+tx*along+nx*across,
                center.Y+ty*along+ny*across
            };
            if(IsOnWorldScaleVariantRoad(circuit,candidate))
                return false;
        }
    }
    return true;
}

inline TracksideSceneObject PlaceTracksideObject(
    const WorldScaleVariantRecipe& circuit,
    WorldScaleBankedRoadProfile road,
    WorldScaleRunoffParameters runoff,
    TracksideSceneSettings settings,
    unsigned stationIndex,
    unsigned side) noexcept
{
    TracksideSceneObject out;
    if(!ValidWorldScaleVariantRecipe(circuit) ||
       !ValidWorldScaleBankedRoadProfile(circuit,road) ||
       !ValidWorldScaleRunoffParameters(circuit,runoff) ||
       !ValidTracksideSceneSettings(settings) ||
       stationIndex>=TracksideSceneStationsPerSide ||
       side>=2u)
        return out;
    const double station=circuit.LapLengthCm*
        static_cast<double>(stationIndex)/
        TracksideSceneStationsPerSide;
    const unsigned choice=(stationIndex+
        static_cast<unsigned>(circuit.Id)*3u+side*2u)%7u;
    const auto recipe=TracksideKindRecipe(
        static_cast<TracksideSceneKind>(choice));
    if(!ValidTracksideRecipe(recipe,settings))
        return out;
    const double sign=side==0u ? 1.0:-1.0;
    const double offset=sign*runoff.DrainageBandEndCm;
    const auto ground=SampleWorldScaleRunoff(
        circuit,road,runoff,station,offset);
    const auto center=WorldScaleVariantCenterlineAt(
        circuit,station);
    if(!ground.Valid || !center.Valid ||
       !ground.OutsideRoadCollision)
        return out;
    const double yaw=center.Pose.HeadingDegrees*
        RoundedPi/180.0;
    const CircuitPoint world{ground.Xcm,ground.Ycm};
    if(!TracksideFootprintEntirelyOffRoad(
        circuit,world,yaw,recipe.HalfWidthCm,
        recipe.HalfLengthCm))
        return out;
    // The clearance estimate is a conservative layout minimum, not a
    // native swept-car collision proof. Corners independently pass the
    // actual original union-of-68-road-pieces test above.
    const double margin=std::abs(offset)-
        circuit.RoadHalfWidthCm-
        std::max(recipe.HalfWidthCm,recipe.HalfLengthCm);
    if(margin<settings.WorldRoadClearanceCm)
        return out;
    out.Valid=true;
    out.Circuit=circuit.Id;
    out.Kind=recipe.Kind;
    out.StationCm=station;
    out.SignedOffsetCm=offset;
    out.Xcm=world.X;
    out.Ycm=world.Y;
    out.Zcm=ground.Zcm-settings.BaseTerrainEmbedCm;
    out.YawRadians=yaw;
    out.HalfWidthCm=recipe.HalfWidthCm;
    out.HalfLengthCm=recipe.HalfLengthCm;
    out.HeightCm=recipe.HeightCm;
    out.MaximumDrawDistanceCm=recipe.MaxDrawDistanceCm;
    out.MaterialIndex=recipe.ProxyMaterialIndex;
    out.DeterministicId=
        static_cast<std::uint64_t>(circuit.Id)*1000000u+
        static_cast<std::uint64_t>(side)*10000u+
        static_cast<std::uint64_t>(stationIndex)+1u;
    out.EveryBaseCornerOutsideRoad=true;
    return out;
}

inline TracksideScenePlan BuildTracksideScenePlan(
    const WorldScaleVariantRecipe& circuit,
    WorldScaleBankedRoadProfile road,
    WorldScaleRunoffParameters runoff={},
    TracksideSceneSettings settings={}) noexcept
{
    TracksideScenePlan out;
    if(!ValidWorldScaleVariantRecipe(circuit) ||
       !ValidWorldScaleBankedRoadProfile(circuit,road) ||
       !ValidWorldScaleRunoffParameters(circuit,runoff) ||
       !ValidTracksideSceneSettings(settings))
        return out;
    out.Circuit=circuit.Id;
    out.MinimumObjectRoadClearanceCm=
        std::numeric_limits<double>::infinity();
    out.SmallestForwardObjectSpacingCm=
        circuit.LapLengthCm/TracksideSceneStationsPerSide;
    if(out.SmallestForwardObjectSpacingCm<
           settings.MinimumAdjacentStationGapCm)
        return {};
    for(unsigned side=0u;side<2u;++side)
    {
        for(unsigned i=0u;i<TracksideSceneStationsPerSide;++i)
        {
            const auto item=PlaceTracksideObject(
                circuit,road,runoff,settings,i,side);
            if(!item.Valid)
                return {};
            const unsigned index=side*
                TracksideSceneStationsPerSide+i;
            out.Objects[index]=item;
            ++out.QualifiedObjects;
            const double clearance=std::abs(item.SignedOffsetCm)-
                circuit.RoadHalfWidthCm-
                std::max(item.HalfWidthCm,item.HalfLengthCm);
            out.MinimumObjectRoadClearanceCm=std::min(
                out.MinimumObjectRoadClearanceCm,clearance);
            out.TallestObjectCm=std::max(
                out.TallestObjectCm,item.HeightCm);
            out.FurthestDrawDistanceCm=std::max(
                out.FurthestDrawDistanceCm,
                item.MaximumDrawDistanceCm);
        }
    }
    out.Valid=out.QualifiedObjects==TracksideSceneObjectCount &&
        out.MinimumObjectRoadClearanceCm>=
            settings.WorldRoadClearanceCm &&
        out.TallestObjectCm<=
            settings.MaximumStructureHeightCm;
    return out;
}

inline bool TracksideAppendProxyBox(
    TracksideProxyMesh& mesh,
    const TracksideSceneObject& anchor,
    double localXcm,double localYcm,double localZcm,
    double halfXcm,double halfYcm,double halfZcm,
    unsigned material) 
{
    if(!anchor.Valid ||
       !std::isfinite(localXcm) ||
       !std::isfinite(localYcm) ||
       !std::isfinite(localZcm) ||
       !std::isfinite(halfXcm) ||
       !std::isfinite(halfYcm) ||
       !std::isfinite(halfZcm) ||
       halfXcm<=0.0 || halfYcm<=0.0 || halfZcm<=0.0 ||
       mesh.Vertices.size()+8u>65536u ||
       material==0u || material>7u)
        return false;
    const double tx=std::cos(anchor.YawRadians);
    const double ty=std::sin(anchor.YawRadians);
    const double nx=-ty;
    const double ny=tx;
    const auto start=static_cast<std::uint32_t>(
        mesh.Vertices.size());
    for(double z : {-halfZcm,halfZcm})
    {
        for(double x : {-halfXcm,halfXcm})
        {
            for(double y : {-halfYcm,halfYcm})
            {
                const double localX=localXcm+x;
                const double localY=localYcm+y;
                const RoundedSmoothVertex point{
                    anchor.Xcm+tx*localX+nx*localY,
                    anchor.Ycm+ty*localX+ny*localY,
                    anchor.Zcm+localZcm+z
                };
                if(!IsFiniteRoundedSmoothVertex(point))
                    return false;
                mesh.Vertices.push_back(point);
            }
        }
    }
    const std::array<std::array<std::uint32_t,3>,12> tris{{
        {{0,1,3}},{{0,3,2}},
        {{4,6,7}},{{4,7,5}},
        {{0,4,5}},{{0,5,1}},
        {{2,3,7}},{{2,7,6}},
        {{0,2,6}},{{0,6,4}},
        {{1,5,7}},{{1,7,3}}
    }};
    for(const auto& tri:tris)
    {
        mesh.Triangles.push_back(
            {{{start+tri[0],start+tri[1],start+tri[2]}}});
        mesh.TriangleMaterial.push_back(material);
    }
    return true;
}

inline bool EmitTracksideObjectProxy(
    TracksideProxyMesh& mesh,
    const TracksideSceneObject& object)
{
    if(!object.Valid || !object.EveryBaseCornerOutsideRoad ||
       object.UnrealActorSpawned || object.DrivableCollisionAdded)
        return false;
    // The simple, low-LOD authored silhouettes deliberately do NOT
    // pretend to be finished photoreal meshes or imported real brands.
    const auto pole=[&](double size,double h,unsigned material) {
        return TracksideAppendProxyBox(mesh,object,
            0.0,0.0,h*0.5,size,size,h*0.5,material);
    };
    switch(object.Kind)
    {
    case TracksideSceneKind::Floodlight:
        return pole(22.0,object.HeightCm-120.0,1u) &&
            TracksideAppendProxyBox(mesh,object,0.0,0.0,
                object.HeightCm-80.0,200.0,35.0,65.0,1u);
    case TracksideSceneKind::MarshalPost:
        return TracksideAppendProxyBox(mesh,object,0.0,0.0,
            130.0,140.0,135.0,130.0,2u) &&
            TracksideAppendProxyBox(mesh,object,0.0,0.0,
            280.0,170.0,150.0,20.0,2u);
    case TracksideSceneKind::CameraTower:
        return pole(40.0,object.HeightCm-110.0,3u) &&
            TracksideAppendProxyBox(mesh,object,0.0,0.0,
                object.HeightCm-50.0,105.0,50.0,45.0,3u);
    case TracksideSceneKind::RescueAccess:
        return TracksideAppendProxyBox(mesh,object,0.0,0.0,
            150.0,object.HalfLengthCm,
            object.HalfWidthCm,150.0,4u) &&
            TracksideAppendProxyBox(mesh,object,0.0,0.0,
            320.0,object.HalfLengthCm-30.0,
            object.HalfWidthCm-30.0,20.0,4u);
    case TracksideSceneKind::WayfindingSign:
        return pole(20.0,object.HeightCm-120.0,5u) &&
            TracksideAppendProxyBox(mesh,object,0.0,0.0,
                object.HeightCm-95.0,220.0,50.0,70.0,5u);
    case TracksideSceneKind::Grandstand:
        return TracksideAppendProxyBox(mesh,object,0.0,0.0,
            300.0,object.HalfLengthCm,
            object.HalfWidthCm,300.0,6u) &&
            TracksideAppendProxyBox(mesh,object,0.0,0.0,
            750.0,object.HalfLengthCm-80.0,
            object.HalfWidthCm-80.0,150.0,6u) &&
            TracksideAppendProxyBox(mesh,object,0.0,0.0,
            1200.0,object.HalfLengthCm,
            object.HalfWidthCm-60.0,70.0,6u);
    case TracksideSceneKind::WoodlandCanopy:
        return pole(35.0,object.HeightCm-250.0,7u) &&
            TracksideAppendProxyBox(mesh,object,0.0,0.0,
                object.HeightCm-220.0,240.0,235.0,220.0,7u);
    }
    return false;
}

inline TracksideProxyMesh BuildTracksideProxyMesh(
    const TracksideScenePlan& plan)
{
    TracksideProxyMesh out;
    if(!plan.Valid ||
       plan.QualifiedObjects!=TracksideSceneObjectCount)
        return out;
    out.Circuit=plan.Circuit;
    out.Vertices.reserve(TracksideSceneObjectCount*24u);
    out.Triangles.reserve(TracksideSceneObjectCount*36u);
    out.TriangleMaterial.reserve(TracksideSceneObjectCount*36u);
    for(const auto& object:plan.Objects)
    {
        if(!object.Valid || object.Circuit!=plan.Circuit ||
           !EmitTracksideObjectProxy(out,object))
            return {};
        ++out.QualifiedObjects;
    }
    out.Valid=out.QualifiedObjects==TracksideSceneObjectCount &&
        out.Vertices.size()>=TracksideSceneObjectCount*8u &&
        out.Vertices.size()<=TracksideSceneObjectCount*24u &&
        out.Triangles.size()==out.TriangleMaterial.size();
    return out;
}
} // namespace latebrake
