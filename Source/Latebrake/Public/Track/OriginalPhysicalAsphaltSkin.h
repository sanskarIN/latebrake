#pragma once

#include "Track/CircuitCollisionPlan.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace latebrake::trackskin
{
// All coordinates refer to the nine original collision-bearing Unreal cubes.
// The shell is purely visual and cannot introduce a new racing surface.
inline constexpr std::size_t MaximumSkinVertices=32000;
inline constexpr std::size_t MaximumSkinTriangles=18000;
inline constexpr double OriginalSkinCubeSizeCm=100.0;
enum class AsphaltSkinLayer : std::uint8_t
{
    FreshAsphalt, MidtoneAsphalt, WeatheredAsphalt,
    RubberGrain, JointSeam, RepairedAsphalt, Count
};
struct AsphaltSkinPoint final
{
    double X=0.0;
    double Y=0.0;
    double Z=0.0;
};
struct AsphaltSkinUv final
{
    double U=0.0;
    double V=0.0;
};
struct AsphaltSkinVertex final
{
    AsphaltSkinPoint Position{};
    AsphaltSkinPoint Normal{0.0,0.0,1.0};
    AsphaltSkinUv Uv{};
};
struct AsphaltSkinTriangle final
{
    std::array<std::uint32_t,3> Indices{};
    AsphaltSkinLayer Layer=AsphaltSkinLayer::FreshAsphalt;
    int SourceSlab=0;
};
struct AsphaltSkinBounds final
{
    AsphaltSkinPoint Minimum{
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity()};
    AsphaltSkinPoint Maximum{
        -std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()};
};
struct AsphaltSkinMesh final
{
    std::vector<AsphaltSkinVertex> Vertices{};
    std::vector<AsphaltSkinTriangle> Triangles{};
    AsphaltSkinBounds Bounds{};
    std::array<int,4> BaseQuadsBySlab{};
    std::array<int,static_cast<std::size_t>(AsphaltSkinLayer::Count)> LayerQuads{};
    int BaseQuads=0;
    int SurfaceRepairs=0;
    int RubberPatches=0;
    int JointSeams=0;
    int IgnoredOverlappingSections=0;
    bool HasIndependentCollision=false;
    bool ChangesRaceTiming=false;
    bool VisualOnly=true;
    bool Ready=false;
};
struct AsphaltSkinSettings final
{
    double TargetCellCm=260.0;
    double MaximumCellCm=320.0;
    double MinimumCellCm=95.0;
    double RoadTopClearanceCm=0.25;
    double OverlayClearanceCm=0.08;
    double SurfaceUvRepeatCm=235.0;
    double RepairInsetCm=27.0;
    double SeamHalfWidthCm=1.5;
    double RubberStripeHalfWidthCm=10.0;
    double MaximumWorldCoordinateCm=20000.0;
    int RepairEveryTiles=13;
    int RubberEveryTiles=7;
    int SeamEveryTiles=9;
    bool RequireOriginalAsphalt=true;
    bool VisualOnly=true;
    bool DoNotModifyCollision=true;
    bool DoNotAwardRaceCredits=true;
};
enum class AsphaltSkinFault : std::uint8_t
{
    None, InvalidSettings, MissingPhysicalAsphalt,
    NonFiniteGeometry, OutsidePhysicalAsphalt,
    CapacityExceeded, TriangleDegenerate, IncompleteSurface
};
struct AsphaltSkinResult final
{
    AsphaltSkinMesh Mesh{};
    AsphaltSkinFault Fault=AsphaltSkinFault::InvalidSettings;
    bool Valid=false;
};
inline bool SkinFinite(double value) noexcept
{
    return std::isfinite(value);
}
inline bool SkinFinitePoint(AsphaltSkinPoint point) noexcept
{
    return SkinFinite(point.X) &&
        SkinFinite(point.Y) &&
        SkinFinite(point.Z);
}
inline double SkinSlabTop(const latebrake::CircuitCollisionBlock& slab) noexcept
{
    return slab.Height+0.5*OriginalSkinCubeSizeCm*slab.ScaleZ;
}
inline double SkinMinX(const latebrake::CircuitCollisionBlock& slab) noexcept
{
    return slab.Center.X-0.5*OriginalSkinCubeSizeCm*slab.ScaleX;
}
inline double SkinMaxX(const latebrake::CircuitCollisionBlock& slab) noexcept
{
    return slab.Center.X+0.5*OriginalSkinCubeSizeCm*slab.ScaleX;
}
inline double SkinMinY(const latebrake::CircuitCollisionBlock& slab) noexcept
{
    return slab.Center.Y-0.5*OriginalSkinCubeSizeCm*slab.ScaleY;
}
inline double SkinMaxY(const latebrake::CircuitCollisionBlock& slab) noexcept
{
    return slab.Center.Y+0.5*OriginalSkinCubeSizeCm*slab.ScaleY;
}
inline bool OriginalSkinPhysicalAsphalt(double x,double y) noexcept
{
    if(!SkinFinite(x) || !SkinFinite(y))
        return false;
    for(int i=1;i<=4;++i)
    {
        const auto slab=latebrake::PlannedCollisionBlock(i);
        if(slab.Style==latebrake::CollisionBlockStyle::Asphalt &&
           x>=SkinMinX(slab) && x<=SkinMaxX(slab) &&
           y>=SkinMinY(slab) && y<=SkinMaxY(slab))
            return true;
    }
    return false;
}
inline bool ValidAsphaltSkinSettings(const AsphaltSkinSettings& p) noexcept
{
    return SkinFinite(p.TargetCellCm) &&
        p.TargetCellCm>=100.0 && p.TargetCellCm<=400.0 &&
        SkinFinite(p.MaximumCellCm) &&
        p.MaximumCellCm>=p.TargetCellCm &&
        p.MaximumCellCm<=500.0 &&
        SkinFinite(p.MinimumCellCm) &&
        p.MinimumCellCm>=60.0 &&
        p.MinimumCellCm<p.TargetCellCm &&
        SkinFinite(p.RoadTopClearanceCm) &&
        p.RoadTopClearanceCm>=0.15 &&
        p.RoadTopClearanceCm<=0.8 &&
        SkinFinite(p.OverlayClearanceCm) &&
        p.OverlayClearanceCm>=0.03 &&
        p.OverlayClearanceCm<=0.2 &&
        SkinFinite(p.SurfaceUvRepeatCm) &&
        p.SurfaceUvRepeatCm>=100.0 &&
        p.SurfaceUvRepeatCm<=700.0 &&
        SkinFinite(p.RepairInsetCm) &&
        p.RepairInsetCm>=8.0 &&
        p.RepairInsetCm<=65.0 &&
        SkinFinite(p.SeamHalfWidthCm) &&
        p.SeamHalfWidthCm>=0.5 &&
        p.SeamHalfWidthCm<=6.0 &&
        SkinFinite(p.RubberStripeHalfWidthCm) &&
        p.RubberStripeHalfWidthCm>=4.0 &&
        p.RubberStripeHalfWidthCm<=20.0 &&
        SkinFinite(p.MaximumWorldCoordinateCm) &&
        p.MaximumWorldCoordinateCm>=11000.0 &&
        p.MaximumWorldCoordinateCm<=100000.0 &&
        p.RepairEveryTiles>=3 &&
        p.RepairEveryTiles<=40 &&
        p.RubberEveryTiles>=3 &&
        p.RubberEveryTiles<=40 &&
        p.SeamEveryTiles>=3 &&
        p.SeamEveryTiles<=40 &&
        p.RequireOriginalAsphalt &&
        p.VisualOnly &&
        p.DoNotModifyCollision &&
        p.DoNotAwardRaceCredits;
}
inline std::uint32_t SkinHash(std::uint32_t x) noexcept
{
    x^=x>>16;
    x*=0x7feb352dU;
    x^=x>>15;
    x*=0x846ca68bU;
    x^=x>>16;
    return x;
}
inline std::uint32_t SkinTileKey(int slab,int x,int y) noexcept
{
    return SkinHash(static_cast<std::uint32_t>(slab)*0x9e3779b9U ^
                    static_cast<std::uint32_t>(x)*0x85ebca6bU ^
                    static_cast<std::uint32_t>(y)*0xc2b2ae35U);
}
class OriginalPhysicalAsphaltSkin final
{
public:
    explicit OriginalPhysicalAsphaltSkin(
        AsphaltSkinSettings settings={}) noexcept
        : Settings(settings)
    {
    }
    AsphaltSkinResult Build() const
    {
        AsphaltSkinResult result;
        if(!ValidAsphaltSkinSettings(Settings))
        {
            result.Fault=AsphaltSkinFault::InvalidSettings;
            return result;
        }
        if(!ValidateOriginalAsphalt())
        {
            result.Fault=AsphaltSkinFault::MissingPhysicalAsphalt;
            return result;
        }
        auto& mesh=result.Mesh;
        mesh.Vertices.reserve(10000);
        mesh.Triangles.reserve(6000);
        Writer writer{mesh,Settings};
        // Draw the entire south and north collision slabs. They own
        // intersections, so vertical slabs do not draw a second skin.
        for(int i=1;i<=2;++i)
        {
            const auto slab=latebrake::PlannedCollisionBlock(i);
            RasterizeRect(
                writer,slab,i,SkinMinX(slab),SkinMaxX(slab),
                SkinMinY(slab),SkinMaxY(slab));
        }
        // The vertical slabs are split at the full physical south/north
        // asphalt Y extents. No duplicated coplanar skin or z-fighting at
        // the four intersections, even on rotated camera views.
        const auto south=latebrake::PlannedCollisionBlock(1);
        const auto north=latebrake::PlannedCollisionBlock(2);
        for(int i=3;i<=4;++i)
        {
            const auto slab=latebrake::PlannedCollisionBlock(i);
            const double min=SkinMinY(slab);
            const double max=SkinMaxY(slab);
            const std::array<std::array<double,2>,3> intervals{{
                {{min,std::min(max,SkinMinY(south))}},
                {{std::max(min,SkinMaxY(south)),
                  std::min(max,SkinMinY(north))}},
                {{std::max(min,SkinMaxY(north)),max}}
            }};
            for(const auto& interval:intervals)
            {
                if(interval[1]>interval[0]+Settings.MinimumCellCm)
                {
                    RasterizeRect(
                        writer,slab,i,
                        SkinMinX(slab),SkinMaxX(slab),
                        interval[0],interval[1]);
                }
                else
                {
                    ++mesh.IgnoredOverlappingSections;
                }
            }
        }
        if(writer.Fault!=AsphaltSkinFault::None)
        {
            result.Fault=writer.Fault;
            result.Mesh={};
            return result;
        }
        if(!ValidateMesh(mesh))
        {
            result.Fault=AsphaltSkinFault::IncompleteSurface;
            result.Mesh={};
            return result;
        }
        mesh.Ready=true;
        result.Fault=AsphaltSkinFault::None;
        result.Valid=true;
        return result;
    }
    bool ValidateMesh(const AsphaltSkinMesh& mesh) const noexcept
    {
        if(!ValidAsphaltSkinSettings(Settings) ||
           mesh.HasIndependentCollision ||
           mesh.ChangesRaceTiming ||
           !mesh.VisualOnly ||
           mesh.Vertices.size()>MaximumSkinVertices ||
           mesh.Triangles.size()>MaximumSkinTriangles ||
           mesh.Triangles.empty() ||
           mesh.Vertices.size()!=mesh.Triangles.size()*3)
            return false;
        if(!SkinFinitePoint(mesh.Bounds.Minimum) ||
           !SkinFinitePoint(mesh.Bounds.Maximum) ||
           mesh.Bounds.Minimum.X>=mesh.Bounds.Maximum.X ||
           mesh.Bounds.Minimum.Y>=mesh.Bounds.Maximum.Y)
            return false;
        int count=0;
        for(int c:mesh.BaseQuadsBySlab)
        {
            if(c<=0)
                return false;
            count+=c;
        }
        if(count!=mesh.BaseQuads ||
           mesh.BaseQuads<400 ||
           mesh.LayerQuads[0]+mesh.LayerQuads[1]+
             mesh.LayerQuads[2]!=mesh.BaseQuads ||
           mesh.LayerQuads[3]!=mesh.RubberPatches ||
           mesh.LayerQuads[4]!=mesh.JointSeams ||
           mesh.LayerQuads[5]!=mesh.SurfaceRepairs)
            return false;
        for(const auto& vertex:mesh.Vertices)
        {
            if(!SkinFinitePoint(vertex.Position) ||
               !SkinFinitePoint(vertex.Normal) ||
               !SkinFinite(vertex.Uv.U) ||
               !SkinFinite(vertex.Uv.V) ||
               std::abs(vertex.Normal.X)>1e-9 ||
               std::abs(vertex.Normal.Y)>1e-9 ||
               std::abs(vertex.Normal.Z-1.0)>1e-9 ||
               std::abs(vertex.Position.X)>
                   Settings.MaximumWorldCoordinateCm ||
               std::abs(vertex.Position.Y)>
                   Settings.MaximumWorldCoordinateCm ||
               !OriginalSkinPhysicalAsphalt(
                   vertex.Position.X,vertex.Position.Y))
                return false;
            const double baseline=SkinSlabTop(
                latebrake::PlannedCollisionBlock(1));
            if(vertex.Position.Z<
                   baseline+Settings.RoadTopClearanceCm-1e-5 ||
               vertex.Position.Z>
                   baseline+Settings.RoadTopClearanceCm+
                   Settings.OverlayClearanceCm+0.00001)
                return false;
        }
        for(const auto& triangle:mesh.Triangles)
        {
            if(triangle.SourceSlab<1 || triangle.SourceSlab>4 ||
               static_cast<std::size_t>(triangle.Layer)>=
                   mesh.LayerQuads.size())
                return false;
            const auto a=triangle.Indices[0];
            const auto b=triangle.Indices[1];
            const auto c=triangle.Indices[2];
            if(a>=mesh.Vertices.size() ||
               b>=mesh.Vertices.size() ||
               c>=mesh.Vertices.size() ||
               a==b || b==c || a==c)
                return false;
            const auto& p=mesh.Vertices[a].Position;
            const auto& q=mesh.Vertices[b].Position;
            const auto& r=mesh.Vertices[c].Position;
            const double area=
                (q.X-p.X)*(r.Y-p.Y)-
                (q.Y-p.Y)*(r.X-p.X);
            if(!SkinFinite(area) || area<=1e-6)
                return false;
        }
        return true;
    }
private:
    struct Writer final
    {
        AsphaltSkinMesh& Mesh;
        const AsphaltSkinSettings& Settings;
        AsphaltSkinFault Fault=AsphaltSkinFault::None;

        void Bound(AsphaltSkinPoint point) noexcept
        {
            Mesh.Bounds.Minimum.X=
                std::min(Mesh.Bounds.Minimum.X,point.X);
            Mesh.Bounds.Minimum.Y=
                std::min(Mesh.Bounds.Minimum.Y,point.Y);
            Mesh.Bounds.Minimum.Z=
                std::min(Mesh.Bounds.Minimum.Z,point.Z);
            Mesh.Bounds.Maximum.X=
                std::max(Mesh.Bounds.Maximum.X,point.X);
            Mesh.Bounds.Maximum.Y=
                std::max(Mesh.Bounds.Maximum.Y,point.Y);
            Mesh.Bounds.Maximum.Z=
                std::max(Mesh.Bounds.Maximum.Z,point.Z);
        }
        bool Triangle(AsphaltSkinPoint a,
                      AsphaltSkinPoint b,
                      AsphaltSkinPoint c,
                      AsphaltSkinLayer layer,
                      int slab)
        {
            if(Fault!=AsphaltSkinFault::None)
                return false;
            if(!SkinFinitePoint(a) || !SkinFinitePoint(b) ||
               !SkinFinitePoint(c))
            {
                Fault=AsphaltSkinFault::NonFiniteGeometry;
                return false;
            }
            const double twiceArea=
                (b.X-a.X)*(c.Y-a.Y)-
                (b.Y-a.Y)*(c.X-a.X);
            if(twiceArea<=1e-6 || !SkinFinite(twiceArea))
            {
                Fault=AsphaltSkinFault::TriangleDegenerate;
                return false;
            }
            if(!OriginalSkinPhysicalAsphalt(a.X,a.Y) ||
               !OriginalSkinPhysicalAsphalt(b.X,b.Y) ||
               !OriginalSkinPhysicalAsphalt(c.X,c.Y))
            {
                Fault=AsphaltSkinFault::OutsidePhysicalAsphalt;
                return false;
            }
            if(Mesh.Vertices.size()+3>MaximumSkinVertices ||
               Mesh.Triangles.size()+1>MaximumSkinTriangles)
            {
                Fault=AsphaltSkinFault::CapacityExceeded;
                return false;
            }
            const auto first=static_cast<std::uint32_t>(
                Mesh.Vertices.size());
            const auto store=[this](AsphaltSkinPoint p)
            {
                Mesh.Vertices.push_back({p,{0.0,0.0,1.0},{
                    p.X/Settings.SurfaceUvRepeatCm,
                    p.Y/Settings.SurfaceUvRepeatCm}});
                Bound(p);
            };
            store(a);
            store(b);
            store(c);
            Mesh.Triangles.push_back(
                {{first,first+1,first+2},layer,slab});
            return true;
        }
        bool Quad(double left,double right,
                  double bottom,double top,double z,
                  AsphaltSkinLayer layer,int slab)
        {
            if(right<=left || top<=bottom)
            {
                Fault=AsphaltSkinFault::TriangleDegenerate;
                return false;
            }
            if(!Triangle({left,bottom,z},{right,bottom,z},
                         {right,top,z},layer,slab))
                return false;
            if(!Triangle({left,bottom,z},{right,top,z},
                         {left,top,z},layer,slab))
                return false;
            ++Mesh.LayerQuads[static_cast<std::size_t>(layer)];
            return true;
        }
    };
    bool ValidateOriginalAsphalt() const noexcept
    {
        const auto south=latebrake::PlannedCollisionBlock(1);
        const auto north=latebrake::PlannedCollisionBlock(2);
        if(SkinMaxY(south)>=SkinMinY(north))
            return false;
        const double top=SkinSlabTop(south);
        if(std::abs(top+1.0)>1e-4)
            return false;
        for(int i=1;i<=4;++i)
        {
            const auto slab=latebrake::PlannedCollisionBlock(i);
            if(!latebrake::IsValidCollisionBlock(slab) ||
               slab.Style!=latebrake::CollisionBlockStyle::Asphalt ||
               std::abs(SkinSlabTop(slab)-top)>1e-7 ||
               SkinMinX(slab)<-Settings.MaximumWorldCoordinateCm ||
               SkinMaxX(slab)>Settings.MaximumWorldCoordinateCm ||
               SkinMinY(slab)<-Settings.MaximumWorldCoordinateCm ||
               SkinMaxY(slab)>Settings.MaximumWorldCoordinateCm)
                return false;
        }
        // All intersections must be completely owned by the horizontal
        // slabs so the vertical subtraction has no ambiguous uncovered
        // strip or second coplanar visual shell.
        for(int i=3;i<=4;++i)
        {
            const auto slab=latebrake::PlannedCollisionBlock(i);
            if(SkinMinX(slab)<SkinMinX(south) ||
               SkinMaxX(slab)>SkinMaxX(south) ||
               SkinMinX(slab)<SkinMinX(north) ||
               SkinMaxX(slab)>SkinMaxX(north))
                return false;
        }
        return true;
    }
    void RasterizeRect(
        Writer& writer,
        const latebrake::CircuitCollisionBlock& slab,
        int slabIndex,double left,double right,
        double bottom,double top) const
    {
        if(writer.Fault!=AsphaltSkinFault::None)
            return;
        const double width=right-left;
        const double height=top-bottom;
        if(width<Settings.MinimumCellCm ||
           height<Settings.MinimumCellCm)
        {
            writer.Fault=AsphaltSkinFault::IncompleteSurface;
            return;
        }
        const int columns=std::max(
            1,static_cast<int>(std::ceil(
                width/Settings.TargetCellCm)));
        const int rows=std::max(
            1,static_cast<int>(std::ceil(
                height/Settings.TargetCellCm)));
        const double dx=width/columns;
        const double dy=height/rows;
        if(dx<Settings.MinimumCellCm ||
           dy<Settings.MinimumCellCm ||
           dx>Settings.MaximumCellCm ||
           dy>Settings.MaximumCellCm)
        {
            writer.Fault=AsphaltSkinFault::InvalidSettings;
            return;
        }
        const double surfaceZ=
            SkinSlabTop(slab)+Settings.RoadTopClearanceCm;
        const double overlayZ=
            surfaceZ+Settings.OverlayClearanceCm;
        for(int row=0;row<rows;++row)
        {
            const double y0=bottom+dy*row;
            const double y1=row+1==rows?
                top:bottom+dy*(row+1);
            for(int col=0;col<columns;++col)
            {
                const double x0=left+dx*col;
                const double x1=col+1==columns?
                    right:left+dx*(col+1);
                const auto key=SkinTileKey(slabIndex,col,row);
                AsphaltSkinLayer layer=AsphaltSkinLayer::FreshAsphalt;
                switch(key%3U)
                {
                case 1:layer=AsphaltSkinLayer::MidtoneAsphalt;break;
                case 2:layer=AsphaltSkinLayer::WeatheredAsphalt;break;
                default:break;
                }
                if(!writer.Quad(x0,x1,y0,y1,surfaceZ,layer,slabIndex))
                    return;
                ++writer.Mesh.BaseQuads;
                ++writer.Mesh.BaseQuadsBySlab[
                    static_cast<std::size_t>(slabIndex-1)];
                // Small, fully internal repair rectangles are darkened
                // patches, not pothole collision or altered road friction.
                const double repairInset=Settings.RepairInsetCm;
                if(key%static_cast<std::uint32_t>(
                       Settings.RepairEveryTiles)==0U)
                {
                    const double quarter=(x1-x0)*0.20;
                    if(!writer.Quad(
                           x0+repairInset,
                           x1-repairInset,
                           y0+repairInset+0.12*quarter,
                           y1-repairInset,
                           overlayZ,AsphaltSkinLayer::RepairedAsphalt,
                           slabIndex))
                        return;
                    ++writer.Mesh.SurfaceRepairs;
                }
                // The dark marks are intentionally tied to existing
                // asphalt tiles. They are only seen at selected cells,
                // never interpreted as actual current vehicle contact.
                if(key%static_cast<std::uint32_t>(
                       Settings.RubberEveryTiles)==1U)
                {
                    const double midY=0.5*(y0+y1);
                    const double half=Settings.RubberStripeHalfWidthCm;
                    if(!writer.Quad(
                           x0+repairInset,x1-repairInset,
                           midY-half,midY+half,overlayZ,
                           AsphaltSkinLayer::RubberGrain,slabIndex))
                        return;
                    ++writer.Mesh.RubberPatches;
                }
                if(col>0 && col%Settings.SeamEveryTiles==0)
                {
                    const double mid=0.5*(x0+x1);
                    if(!writer.Quad(
                           mid-Settings.SeamHalfWidthCm,
                           mid+Settings.SeamHalfWidthCm,
                           y0+repairInset,y1-repairInset,overlayZ,
                           AsphaltSkinLayer::JointSeam,slabIndex))
                        return;
                    ++writer.Mesh.JointSeams;
                }
            }
        }
    }
    AsphaltSkinSettings Settings{};
};
} // namespace latebrake::trackskin
