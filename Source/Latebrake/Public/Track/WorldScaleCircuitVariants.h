#pragma once

#include "Track/WorldScaleRoundedCircuit.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Six ORIGINAL SOURCE-ONLY 1.3-2.4km candidate circuits. These are
// geometrical plans, NOT cooked Unreal maps, collision-bearing
// actors or interchangeable with the active legacy circuit.
//
// All six are rounded rectangular closed racing routes with distinct
// extents, curve radius, sector timings and signed seam geometry.
// The exact 68 independently qualified rotated WorldStatic slabs
// will need a native UE5.8 construction and driving QA pass later.
inline constexpr unsigned WorldScaleVariantCount = 6u;
inline constexpr unsigned WorldScaleVariantCornerCount = 4u;
inline constexpr unsigned WorldScaleVariantCornerSegments = 16u;
inline constexpr unsigned WorldScaleVariantRoadPieceCount =
    4u + WorldScaleVariantCornerCount *
        WorldScaleVariantCornerSegments;
inline constexpr double WorldScaleVariantRoadHalfWidthCm = 450.0;
inline constexpr double WorldScaleVariantVisualHalfWidthCm = 425.0;
inline constexpr double WorldScaleVariantRoadTopCm = RoundedRoadTopCm;
inline constexpr double WorldScaleVariantStraightOverlapCm = 60.0;
inline constexpr double WorldScaleVariantCurveOverlapCm = 20.0;
inline constexpr unsigned WorldScaleVariantVisualMeshSections = 1024u;

enum class WorldScaleVariantId : std::uint8_t
{
    OriginalLoop = 0,
    EnduranceRing = 1,
    TechnicalCrest = 2,
    HarborClub = 3,
    CanyonRun = 4,
    TestOval = 5
};

struct WorldScaleVariantRecipe final
{
    WorldScaleVariantId Id = WorldScaleVariantId::OriginalLoop;
    const char* Name = "ORIGINAL LOOP";
    double HalfExtentXcm = 28000.0;
    double HalfExtentYcm = 18000.0;
    double CornerRadiusCm = 8000.0;
    double RoadHalfWidthCm = WorldScaleVariantRoadHalfWidthCm;
    double VisualHalfWidthCm = WorldScaleVariantVisualHalfWidthCm;
    double LapLengthCm = 0.0;
    int RoadBlockCount = 0;
    bool OriginalSourceOnly = true;
    bool NoExistingCollisionModified = true;
};

inline constexpr double WorldScaleVariantLapLength(
    double halfX,double halfY,double radius) noexcept
{
    return 4.0*(halfX-radius)+
        4.0*(halfY-radius)+2.0*RoundedPi*radius;
}

// Distinct proportions and corner severity, not scaled-up road widths
// or cosmetic recolors of a single full-size current-game level.
inline constexpr std::array<WorldScaleVariantRecipe,
    WorldScaleVariantCount> WorldScaleOriginalVariantRecipes{{
    {WorldScaleVariantId::OriginalLoop,"ORIGINAL LOOP",
        28000.0,18000.0,8000.0,450.0,425.0,
        WorldScaleVariantLapLength(28000,18000,8000),68,true,true},
    {WorldScaleVariantId::EnduranceRing,"ENDURANCE RING",
        45000.0,18000.0,8000.0,450.0,425.0,
        WorldScaleVariantLapLength(45000,18000,8000),68,true,true},
    {WorldScaleVariantId::TechnicalCrest,"TECHNICAL CREST",
        26000.0,22000.0,12000.0,450.0,425.0,
        WorldScaleVariantLapLength(26000,22000,12000),68,true,true},
    {WorldScaleVariantId::HarborClub,"HARBOR CLUB",
        23000.0,15000.0,7000.0,450.0,425.0,
        WorldScaleVariantLapLength(23000,15000,7000),68,true,true},
    {WorldScaleVariantId::CanyonRun,"CANYON RUN",
        34000.0,23000.0,9000.0,450.0,425.0,
        WorldScaleVariantLapLength(34000,23000,9000),68,true,true},
    {WorldScaleVariantId::TestOval,"TEST OVAL",
        19000.0,19000.0,14000.0,450.0,425.0,
        WorldScaleVariantLapLength(19000,19000,14000),68,true,true}
}};

inline constexpr WorldScaleVariantRecipe WorldScaleVariantAt(
    unsigned index) noexcept
{
    return index < WorldScaleVariantCount ?
        WorldScaleOriginalVariantRecipes[index] :
        WorldScaleVariantRecipe{};
}

inline bool ValidWorldScaleVariantRecipe(
    const WorldScaleVariantRecipe& p) noexcept
{
    return static_cast<unsigned>(p.Id) < WorldScaleVariantCount &&
        p.Name != nullptr &&
        std::isfinite(p.HalfExtentXcm) &&
        p.HalfExtentXcm >= 18000.0 &&
        p.HalfExtentXcm <= 50000.0 &&
        std::isfinite(p.HalfExtentYcm) &&
        p.HalfExtentYcm >= 14000.0 &&
        p.HalfExtentYcm <= 50000.0 &&
        std::isfinite(p.CornerRadiusCm) &&
        p.CornerRadiusCm >= 6000.0 &&
        p.CornerRadiusCm <= 16000.0 &&
        p.HalfExtentXcm-p.CornerRadiusCm >= 4000.0 &&
        p.HalfExtentYcm-p.CornerRadiusCm >= 4000.0 &&
        std::isfinite(p.RoadHalfWidthCm) &&
        p.RoadHalfWidthCm == WorldScaleVariantRoadHalfWidthCm &&
        std::isfinite(p.VisualHalfWidthCm) &&
        p.VisualHalfWidthCm == WorldScaleVariantVisualHalfWidthCm &&
        std::isfinite(p.LapLengthCm) &&
        p.LapLengthCm > 120000.0 &&
        p.LapLengthCm < 260000.0 &&
        std::abs(p.LapLengthCm - WorldScaleVariantLapLength(
            p.HalfExtentXcm,p.HalfExtentYcm,p.CornerRadiusCm)) < 1e-5 &&
        p.RoadBlockCount == static_cast<int>(
            WorldScaleVariantRoadPieceCount) &&
        p.OriginalSourceOnly &&
        p.NoExistingCollisionModified;
}

inline bool ValidWorldScaleVariantIndex(int index) noexcept
{
    return index >= 0 &&
        index < static_cast<int>(WorldScaleVariantCount);
}

inline WorldScaleVariantRecipe WorldScaleVariantByIndex(
    int index) noexcept
{
    return ValidWorldScaleVariantIndex(index) ?
        WorldScaleOriginalVariantRecipes[
            static_cast<std::size_t>(index)] :
        WorldScaleVariantRecipe{
            WorldScaleVariantId::OriginalLoop,"INVALID TRACK",
            0.0,0.0,0.0,0.0,0.0,0.0,0,false,false};
}

inline double WorldScaleVariantWrapDistance(
    const WorldScaleVariantRecipe& p,double distanceCm) noexcept
{
    if(!ValidWorldScaleVariantRecipe(p) ||
       !std::isfinite(distanceCm))
        return std::numeric_limits<double>::quiet_NaN();
    double wrapped=std::fmod(distanceCm,p.LapLengthCm);
    if(wrapped<0.0)wrapped+=p.LapLengthCm;
    return wrapped;
}

inline RoundedCenterlinePose WorldScaleVariantCenterlineAt(
    const WorldScaleVariantRecipe& p,double distanceCm) noexcept
{
    if(!ValidWorldScaleVariantRecipe(p) ||
       !std::isfinite(distanceCm))return {};
    double s=WorldScaleVariantWrapDistance(p,distanceCm);
    if(!std::isfinite(s))return {};
    const double tx=p.HalfExtentXcm-p.CornerRadiusCm;
    const double ty=p.HalfExtentYcm-p.CornerRadiusCm;
    const double cornerLength=RoundedHalfPi*p.CornerRadiusCm;
    const std::array<CircuitPoint,4> centers{{
        {tx,-ty},{tx,ty},{-tx,ty},{-tx,-ty}
    }};
    for(unsigned section=0;section<8u;++section)
    {
        if(section%2u==0u)
        {
            const unsigned straight=section/2u;
            const double length=
                straight%2u==0u ? 2.0*tx : 2.0*ty;
            if(s<length)
            {
                switch(straight)
                {
                case 0u: return {{{-tx+s,-p.HalfExtentYcm},0.0},true};
                case 1u: return {{{p.HalfExtentXcm,-ty+s},90.0},true};
                case 2u: return {{{tx-s,p.HalfExtentYcm},180.0},true};
                default: return {{{-p.HalfExtentXcm,ty-s},270.0},true};
                }
            }
            s-=length;
        }
        else
        {
            const unsigned corner=section/2u;
            if(s<cornerLength || section==7u)
            {
                const double angle=-RoundedHalfPi+
                    corner*RoundedHalfPi+s/p.CornerRadiusCm;
                const auto& c=centers[corner];
                return {{{
                    c.X+p.CornerRadiusCm*std::cos(angle),
                    c.Y+p.CornerRadiusCm*std::sin(angle)
                },(angle+RoundedHalfPi)*180.0/RoundedPi},true};
            }
            s-=cornerLength;
        }
    }
    return {};
}

inline RoundedPhysicalRoadPiece WorldScaleVariantPhysicalRoadPiece(
    const WorldScaleVariantRecipe& p,int index) noexcept
{
    if(!ValidWorldScaleVariantRecipe(p) ||
       index<0 || index>=p.RoadBlockCount)return {};
    const double tx=p.HalfExtentXcm-p.CornerRadiusCm;
    const double ty=p.HalfExtentYcm-p.CornerRadiusCm;
    constexpr double baseHeight=RoundedRoadCenterHeightCm;
    constexpr double depth=RoundedRoadDepthScale;
    const double width=2.0*p.RoadHalfWidthCm/100.0;
    if(index<4)
    {
        const double horizontal=(2.0*tx+
            2.0*WorldScaleVariantStraightOverlapCm)/100.0;
        const double vertical=(2.0*ty+
            2.0*WorldScaleVariantStraightOverlapCm)/100.0;
        switch(index)
        {
        case 0: return {{0.0,-p.HalfExtentYcm},baseHeight,
                        horizontal,width,depth,0.0,true};
        case 1: return {{p.HalfExtentXcm,0.0},baseHeight,
                        vertical,width,depth,90.0,true};
        case 2: return {{0.0,p.HalfExtentYcm},baseHeight,
                        horizontal,width,depth,180.0,true};
        default:return {{-p.HalfExtentXcm,0.0},baseHeight,
                        vertical,width,depth,270.0,true};
        }
    }
    const std::array<CircuitPoint,4> cornerCenters{{
        {tx,-ty},{tx,ty},{-tx,ty},{-tx,-ty}
    }};
    const int corner=(index-4)/
        static_cast<int>(WorldScaleVariantCornerSegments);
    const int segment=(index-4)%
        static_cast<int>(WorldScaleVariantCornerSegments);
    const double angleStep=RoundedHalfPi/
        WorldScaleVariantCornerSegments;
    const double angle=-RoundedHalfPi+
        corner*RoundedHalfPi+
        (segment+0.5)*angleStep;
    const double halfChord=p.CornerRadiusCm*
        std::sin(angleStep*0.5);
    const double halfWedge=p.RoadHalfWidthCm*
        std::sin(angleStep*0.5);
    const double length=2.0*(halfChord+halfWedge+
        WorldScaleVariantCurveOverlapCm);
    const auto& center=cornerCenters[
        static_cast<unsigned>(corner)];
    return {{center.X+p.CornerRadiusCm*std::cos(angle),
             center.Y+p.CornerRadiusCm*std::sin(angle)},
            baseHeight,length/100.0,width,depth,
            (angle+RoundedHalfPi)*180.0/RoundedPi,true};
}

inline bool IsOnWorldScaleVariantRoad(
    const WorldScaleVariantRecipe& p,CircuitPoint point) noexcept
{
    if(!ValidWorldScaleVariantRecipe(p) ||
       !std::isfinite(point.X) ||
       !std::isfinite(point.Y) ||
       std::abs(point.X)>1000000.0 ||
       std::abs(point.Y)>1000000.0)return false;
    for(int i=0;i<p.RoadBlockCount;++i)
    {
        if(ContainsRoundedPhysicalRoadPoint(
            WorldScaleVariantPhysicalRoadPiece(p,i),point))
            return true;
    }
    return false;
}

inline bool WorldScaleVariantContinuousRoadProof(
    const WorldScaleVariantRecipe& p,
    unsigned samplesPerLap=1024u) noexcept
{
    if(!ValidWorldScaleVariantRecipe(p) ||
       samplesPerLap<256u || samplesPerLap>4096u)
        return false;
    for(unsigned i=0;i<samplesPerLap;++i)
    {
        const double s=p.LapLengthCm*i/
            static_cast<double>(samplesPerLap);
        const auto pose=WorldScaleVariantCenterlineAt(p,s);
        if(!pose.Valid ||
           !IsOnWorldScaleVariantRoad(p,pose.Pose.Position))
            return false;
        const double angle=pose.Pose.HeadingDegrees*
            RoundedPi/180.0;
        const double nx=-std::sin(angle);
        const double ny=std::cos(angle);
        for(double lateral : {-425.0,-250.0,0.0,250.0,425.0})
        {
            const CircuitPoint pt{
                pose.Pose.Position.X+nx*lateral,
                pose.Pose.Position.Y+ny*lateral
            };
            if(!IsOnWorldScaleVariantRoad(p,pt))
                return false;
        }
    }
    return true;
}

inline RoundedSmoothRoadMesh BuildWorldScaleVariantVisualMesh(
    const WorldScaleVariantRecipe& p)
{
    if(!ValidWorldScaleVariantRecipe(p))
        return {};
    RoundedSmoothRoadMesh mesh;
    const unsigned sections=WorldScaleVariantVisualMeshSections;
    mesh.Vertices.reserve(4u*sections);
    mesh.Triangles.reserve(8u*sections);
    for(unsigned i=0;i<sections;++i)
    {
        const double s=p.LapLengthCm*i/
            static_cast<double>(sections);
        const auto center=WorldScaleVariantCenterlineAt(p,s);
        if(!center.Valid)return {};
        const double heading=center.Pose.HeadingDegrees*
            RoundedPi/180.0;
        const double nx=-std::sin(heading);
        const double ny=std::cos(heading);
        for(double z : {RoundedSmoothTopCm,
                        RoundedSmoothBottomCm})
        {
            for(double side : {1.0,-1.0})
            {
                RoundedSmoothVertex v{
                    center.Pose.Position.X+nx*side*p.VisualHalfWidthCm,
                    center.Pose.Position.Y+ny*side*p.VisualHalfWidthCm,z
                };
                if(!IsFiniteRoundedSmoothVertex(v) ||
                   !IsOnWorldScaleVariantRoad(p,{v.X,v.Y}))
                    return {};
                mesh.Vertices.push_back(v);
            }
        }
    }
    for(std::uint32_t i=0;i<sections;++i)
    {
        const std::uint32_t j=(i+1u)%sections;
        const std::uint32_t a=4u*i;
        const std::uint32_t b=4u*j;
        for(const std::array<std::uint32_t,3>& tri:{
            std::array<std::uint32_t,3>{a,a+1u,b+1u},
            std::array<std::uint32_t,3>{a,b+1u,b},
            std::array<std::uint32_t,3>{a+2u,b+3u,a+3u},
            std::array<std::uint32_t,3>{a+2u,b+2u,b+3u},
            std::array<std::uint32_t,3>{a,b,b+2u},
            std::array<std::uint32_t,3>{a,b+2u,a+2u},
            std::array<std::uint32_t,3>{a+1u,a+3u,b+3u},
            std::array<std::uint32_t,3>{a+1u,b+3u,b+1u}
        })
        {
            mesh.Triangles.push_back({tri});
        }
    }
    if(mesh.Vertices.size()!=4u*sections ||
       mesh.Triangles.size()!=8u*sections)
        return {};
    return mesh;
}
} // namespace latebrake
