#pragma once
#include "Track/CircuitCollisionPlan.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace latebrake::kerbing
{
inline constexpr int OriginalKerbStraightEdges=8;
inline constexpr int OriginalKerbMaxSectionsPerEdge=64;
inline constexpr std::size_t OriginalKerbMaxVertices=30000;
inline constexpr std::size_t OriginalKerbMaxTriangles=10000;
enum class KerbSurface : std::uint8_t
{
    RedPaint, WhitePaint, BlackAsphaltBlend,
    YellowDrainage, EdgeShadow, Count
};
struct KerbPoint final
{
    double X=0.0;
    double Y=0.0;
    double Z=0.0;
};
struct KerbTexel final
{
    double U=0.0;
    double V=0.0;
};
struct KerbVertex final
{
    KerbPoint Position{};
    KerbPoint Normal{0.0,0.0,1.0};
    KerbTexel Uv{};
};
struct KerbFace final
{
    std::array<std::uint32_t,3> Indices{};
    KerbSurface Surface=KerbSurface::WhitePaint;
    int Segment=0;
    int PhysicalSlab=0;
};
struct KerbMesh final
{
    std::vector<KerbVertex> Vertices{};
    std::vector<KerbFace> Triangles{};
    int StripePairs=0;
    int RedStripes=0;
    int WhiteStripes=0;
    int AsphaltBlendBands=0;
    int DrainageMarks=0;
    int CastShadowLines=0;
    std::array<int,OriginalKerbStraightEdges> SectionsByEdge{};
    bool PurelyVisual=true;
    bool OriginalCollisionUnmodified=true;
    bool OfficialRaceProgressUnmodified=true;
    bool Ready=false;
};
struct KerbSettings final
{
    double PanelLengthCm=160.0;
    double PaintWidthCm=58.0;
    double RoadEdgeInsetCm=45.0;
    double AsphaltBlendWidthCm=24.0;
    double ShadowLineWidthCm=2.5;
    double DrainWidthCm=2.5;
    double RoadSurfaceClearanceCm=0.28;
    double DecorativeLayerLiftCm=0.12;
    double MinimumStraightClearanceFromCornerCm=850.0;
    double MaximumWorldCoordinateCm=8000.0;
    double UvRepeatLengthCm=150.0;
    bool UseOnlyOriginalSlabFaces=true;
    bool NeverChangeTrackCollision=true;
    bool NoImpliedNewRoad=true;
};
enum class KerbFault : std::uint8_t
{
    None, InvalidSettings, InvalidPhysicalSlab,
    NonFiniteGeometry, OnUnpavedGround,
    OverBudget, IncompleteEdge
};
struct KerbResult final
{
    KerbMesh Mesh{};
    KerbFault Fault=KerbFault::InvalidSettings;
    bool Valid=false;
};
inline bool KerbFinite(double x) noexcept
{
    return std::isfinite(x);
}
inline bool KerbValidPoint(KerbPoint p) noexcept
{
    return KerbFinite(p.X) &&
        KerbFinite(p.Y) &&
        KerbFinite(p.Z);
}
inline bool KerbOnOriginalRoad(KerbPoint p,int slab) noexcept
{
    if(!KerbValidPoint(p) || slab<1 || slab>4)
        return false;
    const auto road=latebrake::PlannedCollisionBlock(slab);
    if(road.Style!=latebrake::CollisionBlockStyle::Asphalt ||
       !latebrake::IsValidCollisionBlock(road))
        return false;
    return std::abs(p.X-road.Center.X)<=50.0*road.ScaleX &&
           std::abs(p.Y-road.Center.Y)<=50.0*road.ScaleY &&
           p.Z>=road.Height+50.0*road.ScaleZ &&
           p.Z<road.Height+50.0*road.ScaleZ+1.0;
}
inline bool ValidKerbSettings(KerbSettings p) noexcept
{
    return KerbFinite(p.PanelLengthCm) &&
        p.PanelLengthCm>=80.0 &&
        p.PanelLengthCm<=300.0 &&
        KerbFinite(p.PaintWidthCm) &&
        p.PaintWidthCm>=25.0 &&
        p.PaintWidthCm<=80.0 &&
        KerbFinite(p.RoadEdgeInsetCm) &&
        p.RoadEdgeInsetCm>=30.0 &&
        p.RoadEdgeInsetCm<=100.0 &&
        KerbFinite(p.AsphaltBlendWidthCm) &&
        p.AsphaltBlendWidthCm>=10.0 &&
        p.AsphaltBlendWidthCm<=40.0 &&
        KerbFinite(p.ShadowLineWidthCm) &&
        p.ShadowLineWidthCm>=1.0 &&
        p.ShadowLineWidthCm<=6.0 &&
        KerbFinite(p.DrainWidthCm) &&
        p.DrainWidthCm>=1.0 &&
        p.DrainWidthCm<=5.0 &&
        KerbFinite(p.RoadSurfaceClearanceCm) &&
        p.RoadSurfaceClearanceCm>=0.15 &&
        p.RoadSurfaceClearanceCm<=0.65 &&
        KerbFinite(p.DecorativeLayerLiftCm) &&
        p.DecorativeLayerLiftCm>=0.04 &&
        p.DecorativeLayerLiftCm<=0.25 &&
        p.RoadSurfaceClearanceCm+p.DecorativeLayerLiftCm<1.0 &&
        KerbFinite(p.MinimumStraightClearanceFromCornerCm) &&
        p.MinimumStraightClearanceFromCornerCm>=500.0 &&
        p.MinimumStraightClearanceFromCornerCm<=1100.0 &&
        KerbFinite(p.MaximumWorldCoordinateCm) &&
        p.MaximumWorldCoordinateCm>=4500.0 &&
        p.MaximumWorldCoordinateCm<=20000.0 &&
        KerbFinite(p.UvRepeatLengthCm) &&
        p.UvRepeatLengthCm>=60.0 &&
        p.UvRepeatLengthCm<=600.0 &&
        p.UseOnlyOriginalSlabFaces &&
        p.NeverChangeTrackCollision &&
        p.NoImpliedNewRoad;
}
class OriginalCircuitKerbComposer final
{
public:
    explicit OriginalCircuitKerbComposer(
        KerbSettings settings={}) noexcept:Settings(settings){}
    KerbResult Build() const
    {
        KerbResult result;
        if(!ValidKerbSettings(Settings))
        {
            result.Fault=KerbFault::InvalidSettings;
            return result;
        }
        const auto south=latebrake::PlannedCollisionBlock(1);
        const auto north=latebrake::PlannedCollisionBlock(2);
        const auto east=latebrake::PlannedCollisionBlock(3);
        const auto west=latebrake::PlannedCollisionBlock(4);
        if(!ValidOriginalPhysicalSlabs(
               south,north,east,west))
        {
            result.Fault=KerbFault::InvalidPhysicalSlab;
            return result;
        }
        Writer writer{result.Mesh,Settings};
        // Each of the eight straight pavement edges is segmented
        // separately; no zebra curb may bridge the unsupported infield.
        const double x0=latebrake::WestX+
            Settings.MinimumStraightClearanceFromCornerCm;
        const double x1=latebrake::EastX-
            Settings.MinimumStraightClearanceFromCornerCm;
        const double y0=latebrake::SouthY+
            Settings.MinimumStraightClearanceFromCornerCm;
        const double y1=latebrake::NorthY-
            Settings.MinimumStraightClearanceFromCornerCm;
        for(int side=0;side<2;++side)
        {
            const double normal=side==0?-1.0:1.0;
            const double southLine=south.Center.Y+
                normal*(50.0*south.ScaleY-
                    Settings.RoadEdgeInsetCm);
            const double northLine=north.Center.Y+
                normal*(50.0*north.ScaleY-
                    Settings.RoadEdgeInsetCm);
            RenderEdge(writer,side,x0,x1,southLine,true,1,normal);
            RenderEdge(writer,side+2,x0,x1,northLine,true,2,normal);
            const double eastLine=east.Center.X+
                normal*(50.0*east.ScaleX-
                    Settings.RoadEdgeInsetCm);
            const double westLine=west.Center.X+
                normal*(50.0*west.ScaleX-
                    Settings.RoadEdgeInsetCm);
            RenderEdge(writer,side+4,y0,y1,eastLine,false,3,normal);
            RenderEdge(writer,side+6,y0,y1,westLine,false,4,normal);
        }
        if(writer.Fault!=KerbFault::None)
        {
            result.Fault=writer.Fault;
            result.Mesh={};
            return result;
        }
        if(!Validate(result.Mesh))
        {
            result.Fault=KerbFault::IncompleteEdge;
            result.Mesh={};
            return result;
        }
        result.Mesh.Ready=true;
        result.Fault=KerbFault::None;
        result.Valid=true;
        return result;
    }
    bool Validate(const KerbMesh& mesh) const noexcept
    {
        if(!ValidKerbSettings(Settings) ||
           !mesh.PurelyVisual ||
           !mesh.OriginalCollisionUnmodified ||
           !mesh.OfficialRaceProgressUnmodified ||
           mesh.Vertices.size()!=mesh.Triangles.size()*3 ||
           mesh.Vertices.size()>OriginalKerbMaxVertices ||
           mesh.Triangles.size()>OriginalKerbMaxTriangles ||
           mesh.Triangles.empty())
            return false;
        int counted=0;
        for(int sections:mesh.SectionsByEdge)
        {
            if(sections<5 || sections>OriginalKerbMaxSectionsPerEdge)
                return false;
            counted+=sections;
        }
        if(counted!=mesh.StripePairs ||
           mesh.RedStripes+mesh.WhiteStripes!=counted ||
           mesh.AsphaltBlendBands!=counted ||
           mesh.CastShadowLines!=counted ||
           mesh.DrainageMarks<8)
            return false;
        for(const auto& face:mesh.Triangles)
        {
            if(face.PhysicalSlab<1 || face.PhysicalSlab>4 ||
               face.Segment<0 ||
               static_cast<std::size_t>(face.Surface)>=
                   static_cast<std::size_t>(KerbSurface::Count))
                return false;
            const auto a=face.Indices[0];
            const auto b=face.Indices[1];
            const auto c=face.Indices[2];
            if(a>=mesh.Vertices.size() ||
               b>=mesh.Vertices.size() ||
               c>=mesh.Vertices.size() ||
               a==b || b==c || a==c)
                return false;
            const auto& p=mesh.Vertices[a].Position;
            const auto& q=mesh.Vertices[b].Position;
            const auto& r=mesh.Vertices[c].Position;
            if(!KerbOnOriginalRoad(p,face.PhysicalSlab) ||
               !KerbOnOriginalRoad(q,face.PhysicalSlab) ||
               !KerbOnOriginalRoad(r,face.PhysicalSlab))
                return false;
            const double area=(q.X-p.X)*(r.Y-p.Y)-
                (q.Y-p.Y)*(r.X-p.X);
            if(!KerbFinite(area) || area<=1.0e-6)
                return false;
            for(auto idx:face.Indices)
            {
                const auto& v=mesh.Vertices[idx];
                if(v.Normal.Z!=1.0 ||
                   v.Normal.X!=0.0 || v.Normal.Y!=0.0 ||
                   !KerbFinite(v.Uv.U) ||
                   !KerbFinite(v.Uv.V))
                    return false;
            }
        }
        return true;
    }
private:
    struct Writer final
    {
        KerbMesh& Mesh;
        const KerbSettings& Settings;
        KerbFault Fault=KerbFault::None;
        bool Triangle(KerbPoint a,KerbPoint b,KerbPoint c,
                      KerbSurface material,int edge,int slab)
        {
            if(Fault!=KerbFault::None)
                return false;
            if(!KerbValidPoint(a) || !KerbValidPoint(b) ||
               !KerbValidPoint(c))
            {
                Fault=KerbFault::NonFiniteGeometry;
                return false;
            }
            if(!KerbOnOriginalRoad(a,slab) ||
               !KerbOnOriginalRoad(b,slab) ||
               !KerbOnOriginalRoad(c,slab))
            {
                Fault=KerbFault::OnUnpavedGround;
                return false;
            }
            if(Mesh.Vertices.size()+3>OriginalKerbMaxVertices ||
               Mesh.Triangles.size()+1>OriginalKerbMaxTriangles)
            {
                Fault=KerbFault::OverBudget;
                return false;
            }
            const double cross=(b.X-a.X)*(c.Y-a.Y)-
                (b.Y-a.Y)*(c.X-a.X);
            if(!KerbFinite(cross) || cross<=1.0e-6)
            {
                Fault=KerbFault::NonFiniteGeometry;
                return false;
            }
            const auto index=static_cast<std::uint32_t>(
                Mesh.Vertices.size());
            const auto emit=[this](KerbPoint point)
            {
                Mesh.Vertices.push_back({
                    point,{0.0,0.0,1.0},
                    {point.X/Settings.UvRepeatLengthCm,
                     point.Y/Settings.UvRepeatLengthCm}});
            };
            emit(a);
            emit(b);
            emit(c);
            Mesh.Triangles.push_back(
                {{index,index+1,index+2},material,edge,slab});
            return true;
        }
        bool Quad(
            KerbPoint a,KerbPoint b,KerbPoint c,KerbPoint d,
            KerbSurface material,int edge,int slab)
        {
            return Triangle(a,b,c,material,edge,slab) &&
                Triangle(a,c,d,material,edge,slab);
        }
    };
    bool ValidOriginalPhysicalSlabs(
        latebrake::CircuitCollisionBlock south,
        latebrake::CircuitCollisionBlock north,
        latebrake::CircuitCollisionBlock east,
        latebrake::CircuitCollisionBlock west) const noexcept
    {
        const std::array<latebrake::CircuitCollisionBlock,4> slabs{
            south,north,east,west
        };
        for(const auto& slab:slabs)
        {
            if(!latebrake::IsValidCollisionBlock(slab) ||
               slab.Style!=latebrake::CollisionBlockStyle::Asphalt ||
               std::abs(slab.Height+50.0*slab.ScaleZ+1.0)>1e-9)
                return false;
        }
        return south.ScaleX==80.0 &&
            north.ScaleX==80.0 &&
            east.ScaleY==52.0 &&
            west.ScaleY==52.0 &&
            south.ScaleY==16.0 &&
            east.ScaleX==16.0 &&
            south.Center.Y==latebrake::SouthY &&
            north.Center.Y==latebrake::NorthY &&
            east.Center.X==latebrake::EastX &&
            west.Center.X==latebrake::WestX;
    }
    void RenderEdge(
        Writer& writer,int edge,
        double start,double end,double roadLateral,
        bool horizontal,int slab,double normal) const
    {
        if(writer.Fault!=KerbFault::None)
            return;
        const int count=static_cast<int>(std::ceil(
            (end-start)/Settings.PanelLengthCm));
        if(count<5 || count>OriginalKerbMaxSectionsPerEdge)
        {
            writer.Fault=KerbFault::InvalidSettings;
            return;
        }
        const double step=(end-start)/count;
        const double top=latebrake::PlannedCollisionBlock(slab).Height+
            50.0*latebrake::PlannedCollisionBlock(slab).ScaleZ+
            Settings.RoadSurfaceClearanceCm;
        for(int panel=0;panel<count;++panel)
        {
            const double a=start+step*panel;
            const double b=panel+1==count?
                end:start+step*(panel+1);
            const double inner=roadLateral-
                normal*Settings.PaintWidthCm;
            const double outer=roadLateral;
            const auto quad=[horizontal](double along,double offset,double z)
            {
                return horizontal?
                    KerbPoint{along,offset,z}:
                    KerbPoint{offset,along,z};
            };
            // Clockwise/anticlockwise face orientation is determined
            // by positive XY area, NOT by driving lane direction.
            const auto stripe=static_cast<KerbSurface>(
                panel%2==0?
                static_cast<int>(KerbSurface::RedPaint):
                static_cast<int>(KerbSurface::WhitePaint));
            const auto make=[&writer,&quad,edge,slab,horizontal](
                double begin,double finish,double low,
                double high,double z,KerbSurface material)
            {
                const auto a=quad(begin,low,z);
                const auto b=quad(finish,low,z);
                const auto c=quad(finish,high,z);
                const auto d=quad(begin,high,z);
                if(horizontal)
                    return writer.Quad(a,b,c,d,material,edge,slab);
                return writer.Quad(d,c,b,a,material,edge,slab);
            };
            const double y0=std::min(inner,outer);
            const double y1=std::max(inner,outer);
            if(!make(a,b,y0,y1,top,stripe))
                return;
            if(panel%2==0)
                ++writer.Mesh.RedStripes;
            else
                ++writer.Mesh.WhiteStripes;
            ++writer.Mesh.StripePairs;
            ++writer.Mesh.SectionsByEdge[
                static_cast<std::size_t>(edge)];
            const double bandA=inner-normal*Settings.AsphaltBlendWidthCm;
            if(!make(a,b,std::min(inner,bandA),
                     std::max(inner,bandA),
                     top,KerbSurface::BlackAsphaltBlend))
                return;
            ++writer.Mesh.AsphaltBlendBands;
            const double shadow=outer-
                normal*Settings.ShadowLineWidthCm;
            if(!make(a,b,std::min(shadow,outer),
                     std::max(shadow,outer),
                     top+Settings.DecorativeLayerLiftCm,
                     KerbSurface::EdgeShadow))
                return;
            ++writer.Mesh.CastShadowLines;
            if(panel%3==0)
            {
                const double mid=0.5*(a+b);
                const double half=0.5*Settings.DrainWidthCm;
                if(!make(mid-half,mid+half,
                         std::min(inner,outer),
                         std::max(inner,outer),
                         top+Settings.DecorativeLayerLiftCm,
                         KerbSurface::YellowDrainage))
                    return;
                ++writer.Mesh.DrainageMarks;
            }
        }
    }
    KerbSettings Settings{};
};
} // namespace latebrake::kerbing
