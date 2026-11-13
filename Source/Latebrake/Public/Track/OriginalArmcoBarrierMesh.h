#pragma once
#include "Track/CircuitCollisionPlan.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace latebrake::armco
{
inline constexpr int OriginalBarrierFirstBlock=5;
inline constexpr int OriginalBarrierLastBlock=8;
inline constexpr int OriginalBarrierCount=4;
inline constexpr int OriginalArmcoRailLayers=3;
inline constexpr std::size_t OriginalArmcoMaxVertices=90000;
inline constexpr std::size_t OriginalArmcoMaxTriangles=35000;
enum class ArmcoLayer : std::uint8_t
{
    CorrugatedSteel, PostSteel, BoltFace, RedSafetyMarker, Count
};
struct ArmcoPoint final
{
    double X=0.0;
    double Y=0.0;
    double Z=0.0;
};
struct ArmcoUv final
{
    double U=0.0;
    double V=0.0;
};
struct ArmcoVertex final
{
    ArmcoPoint Position{};
    ArmcoPoint Normal{};
    ArmcoUv Uv{};
};
struct ArmcoTriangle final
{
    std::array<std::uint32_t,3> Indices{};
    ArmcoLayer Layer=ArmcoLayer::CorrugatedSteel;
    int PhysicalBarrier=-1;
};
struct ArmcoBounds final
{
    ArmcoPoint Min{
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity()};
    ArmcoPoint Max{
        -std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()};
};
struct ArmcoMesh final
{
    std::vector<ArmcoVertex> Vertices{};
    std::vector<ArmcoTriangle> Triangles{};
    ArmcoBounds Bounds{};
    std::array<int,OriginalBarrierCount> RailSections{};
    std::array<int,OriginalBarrierCount> SteelPosts{};
    std::array<int,OriginalBarrierCount> VerifiedBarriers{};
    int CorrugationFaces=0;
    int EndCaps=0;
    int SafetyBands=0;
    int BoltHeads=0;
    bool VisualOnly=true;
    bool OriginalRoadCollisionUntouched=true;
    bool NoSeparateCollision=true;
    bool NoOfficialLapAuthority=true;
    bool Ready=false;
};
struct ArmcoSettings final
{
    double RailSamplingLengthCm=175.0;
    double MaximumSectionLengthCm=220.0;
    double PostSpacingCm=340.0;
    double PostSectionCm=18.0;
    double PostHeightCm=164.0;
    double RailHeightCm=26.0;
    double RailBaseCm=13.0;
    double RailVerticalSpacingCm=40.0;
    double InnerFaceDepthCm=43.0;
    double WaveDepthCm=4.0;
    double MetalThicknessCm=2.5;
    double BoltRadiusCm=4.0;
    double SafetyBandHeightCm=5.0;
    double UvRepeatLengthCm=250.0;
    bool OnlyExistingFourBarrierBlocks=true;
    bool AlwaysRenderInsideCollisionVolume=true;
    bool NeverCreateIndependentCollision=true;
    bool NeverInfluenceRaceClassification=true;
};
enum class ArmcoFault : std::uint8_t
{
    None, InvalidSettings, AlteredPhysicalBarrierPlan,
    InvalidWorldPosition, DegenerateFace, CapacityExceeded,
    UnsupportedBarrier, IncompleteMesh
};
struct ArmcoResult final
{
    ArmcoMesh Mesh{};
    ArmcoFault Fault=ArmcoFault::InvalidSettings;
    bool Valid=false;
};
inline bool ArmcoFinite(double x) noexcept
{
    return std::isfinite(x);
}
inline bool ArmcoValidPoint(ArmcoPoint p) noexcept
{
    return ArmcoFinite(p.X) && ArmcoFinite(p.Y) && ArmcoFinite(p.Z);
}
inline ArmcoPoint ArmcoSubtract(ArmcoPoint a,ArmcoPoint b) noexcept
{
    return {a.X-b.X,a.Y-b.Y,a.Z-b.Z};
}
inline ArmcoPoint ArmcoCross(ArmcoPoint a,ArmcoPoint b) noexcept
{
    return {
        a.Y*b.Z-a.Z*b.Y,
        a.Z*b.X-a.X*b.Z,
        a.X*b.Y-a.Y*b.X
    };
}
inline double ArmcoLength(ArmcoPoint p) noexcept
{
    return std::hypot(p.X,p.Y,p.Z);
}
inline bool ArmcoInsidePhysicalBarrier(
    ArmcoPoint point,int barrier,
    double tolerance=1.0e-4) noexcept
{
    if(!ArmcoValidPoint(point) ||
       barrier<OriginalBarrierFirstBlock ||
       barrier>OriginalBarrierLastBlock)
        return false;
    const auto block=latebrake::PlannedCollisionBlock(barrier);
    if(block.Style!=latebrake::CollisionBlockStyle::Barrier ||
       !latebrake::IsValidCollisionBlock(block))
        return false;
    return std::abs(point.X-block.Center.X)<=
               50.0*block.ScaleX+tolerance &&
        std::abs(point.Y-block.Center.Y)<=
               50.0*block.ScaleY+tolerance &&
        std::abs(point.Z-block.Height)<=
               50.0*block.ScaleZ+tolerance;
}
inline bool ArmcoSettingsValid(const ArmcoSettings& p) noexcept
{
    return ArmcoFinite(p.RailSamplingLengthCm) &&
        p.RailSamplingLengthCm>=65.0 &&
        p.RailSamplingLengthCm<=275.0 &&
        ArmcoFinite(p.MaximumSectionLengthCm) &&
        p.MaximumSectionLengthCm>=p.RailSamplingLengthCm &&
        p.MaximumSectionLengthCm<=320.0 &&
        ArmcoFinite(p.PostSpacingCm) &&
        p.PostSpacingCm>=200.0 && p.PostSpacingCm<=550.0 &&
        ArmcoFinite(p.PostSectionCm) &&
        p.PostSectionCm>=10.0 && p.PostSectionCm<=28.0 &&
        ArmcoFinite(p.PostHeightCm) &&
        p.PostHeightCm>=140.0 && p.PostHeightCm<=180.0 &&
        ArmcoFinite(p.RailHeightCm) &&
        p.RailHeightCm>=20.0 && p.RailHeightCm<=32.0 &&
        ArmcoFinite(p.RailBaseCm) &&
        p.RailBaseCm>=-15.0 && p.RailBaseCm<=30.0 &&
        ArmcoFinite(p.RailVerticalSpacingCm) &&
        p.RailVerticalSpacingCm>=35.0 &&
        p.RailVerticalSpacingCm<=45.0 &&
        ArmcoFinite(p.InnerFaceDepthCm) &&
        p.InnerFaceDepthCm>=36.0 &&
        p.InnerFaceDepthCm<=45.0 &&
        ArmcoFinite(p.WaveDepthCm) &&
        p.WaveDepthCm>=1.0 && p.WaveDepthCm<=4.5 &&
        ArmcoFinite(p.MetalThicknessCm) &&
        p.MetalThicknessCm>=1.0 &&
        p.MetalThicknessCm<=4.0 &&
        ArmcoFinite(p.BoltRadiusCm) &&
        p.BoltRadiusCm>=2.0 &&
        p.BoltRadiusCm<=5.0 &&
        ArmcoFinite(p.SafetyBandHeightCm) &&
        p.SafetyBandHeightCm>=2.0 &&
        p.SafetyBandHeightCm<=8.0 &&
        ArmcoFinite(p.UvRepeatLengthCm) &&
        p.UvRepeatLengthCm>=100.0 &&
        p.UvRepeatLengthCm<=500.0 &&
        p.OnlyExistingFourBarrierBlocks &&
        p.AlwaysRenderInsideCollisionVolume &&
        p.NeverCreateIndependentCollision &&
        p.NeverInfluenceRaceClassification;
}
class OriginalArmcoBarrierMeshBuilder final
{
public:
    explicit OriginalArmcoBarrierMeshBuilder(
        ArmcoSettings settings={}) noexcept:Settings(settings){}
    ArmcoResult Build() const
    {
        ArmcoResult result;
        if(!ArmcoSettingsValid(Settings))
        {
            result.Fault=ArmcoFault::InvalidSettings;
            return result;
        }
        if(!OriginalCollisionPlanMatches())
        {
            result.Fault=ArmcoFault::AlteredPhysicalBarrierPlan;
            return result;
        }
        auto& mesh=result.Mesh;
        mesh.Vertices.reserve(28000);
        mesh.Triangles.reserve(12000);
        Writer writer{mesh,Settings};
        for(int block=OriginalBarrierFirstBlock;
            block<=OriginalBarrierLastBlock;++block)
        {
            WriteOneBarrier(writer,block);
            if(writer.Fault!=ArmcoFault::None)
                break;
        }
        if(writer.Fault!=ArmcoFault::None)
        {
            result.Fault=writer.Fault;
            result.Mesh={};
            return result;
        }
        if(!ValidateMesh(mesh))
        {
            result.Fault=ArmcoFault::IncompleteMesh;
            result.Mesh={};
            return result;
        }
        mesh.Ready=true;
        result.Fault=ArmcoFault::None;
        result.Valid=true;
        return result;
    }
    bool ValidateMesh(const ArmcoMesh& mesh) const noexcept
    {
        if(!ArmcoSettingsValid(Settings) ||
           !OriginalCollisionPlanMatches() ||
           !mesh.VisualOnly ||
           !mesh.NoSeparateCollision ||
           !mesh.OriginalRoadCollisionUntouched ||
           !mesh.NoOfficialLapAuthority ||
           mesh.Triangles.empty() ||
           mesh.Vertices.size()!=mesh.Triangles.size()*3 ||
           mesh.Vertices.size()>OriginalArmcoMaxVertices ||
           mesh.Triangles.size()>OriginalArmcoMaxTriangles)
            return false;
        int totalRails=0;
        int totalPosts=0;
        for(int i=0;i<OriginalBarrierCount;++i)
        {
            if(mesh.RailSections[static_cast<std::size_t>(i)]<5 ||
               mesh.SteelPosts[static_cast<std::size_t>(i)]<5 ||
               mesh.VerifiedBarriers[static_cast<std::size_t>(i)]!=1)
                return false;
            totalRails+=mesh.RailSections[static_cast<std::size_t>(i)];
            totalPosts+=mesh.SteelPosts[static_cast<std::size_t>(i)];
        }
        if(totalRails<OriginalArmcoRailLayers*30 ||
           totalPosts<30 ||
           mesh.CorrugationFaces<200 ||
           mesh.EndCaps<OriginalArmcoRailLayers*OriginalBarrierCount ||
           mesh.SafetyBands<OriginalBarrierCount ||
           mesh.BoltHeads<OriginalBarrierCount*8)
            return false;
        if(!ArmcoValidPoint(mesh.Bounds.Min) ||
           !ArmcoValidPoint(mesh.Bounds.Max) ||
           mesh.Bounds.Min.X>=mesh.Bounds.Max.X ||
           mesh.Bounds.Min.Y>=mesh.Bounds.Max.Y ||
           mesh.Bounds.Min.Z>=mesh.Bounds.Max.Z)
            return false;
        for(const auto& triangle:mesh.Triangles)
        {
            if(triangle.PhysicalBarrier<OriginalBarrierFirstBlock ||
               triangle.PhysicalBarrier>OriginalBarrierLastBlock ||
               static_cast<std::size_t>(triangle.Layer)>=
                   static_cast<std::size_t>(ArmcoLayer::Count))
                return false;
            const auto a=triangle.Indices[0];
            const auto b=triangle.Indices[1];
            const auto c=triangle.Indices[2];
            if(a==b || b==c || a==c ||
               a>=mesh.Vertices.size() ||
               b>=mesh.Vertices.size() ||
               c>=mesh.Vertices.size())
                return false;
            const auto& p=mesh.Vertices[a].Position;
            const auto& q=mesh.Vertices[b].Position;
            const auto& r=mesh.Vertices[c].Position;
            if(!ArmcoInsidePhysicalBarrier(
                   p,triangle.PhysicalBarrier) ||
               !ArmcoInsidePhysicalBarrier(
                   q,triangle.PhysicalBarrier) ||
               !ArmcoInsidePhysicalBarrier(
                   r,triangle.PhysicalBarrier))
                return false;
            const auto area=ArmcoLength(ArmcoCross(
                ArmcoSubtract(q,p),ArmcoSubtract(r,p)));
            if(!ArmcoFinite(area) || area<=1e-8)
                return false;
            for(std::uint32_t idx:triangle.Indices)
            {
                const auto& v=mesh.Vertices[idx];
                if(!ArmcoValidPoint(v.Normal) ||
                   std::abs(ArmcoLength(v.Normal)-1.0)>1e-7 ||
                   !ArmcoFinite(v.Uv.U) ||
                   !ArmcoFinite(v.Uv.V))
                    return false;
            }
        }
        return true;
    }
private:
    struct PhysicalBarrierAxes final
    {
        int Block=0;
        double Start=0.0;
        double End=0.0;
        double Center=0.0;
        double InwardSign=0.0;
        bool Horizontal=false;
    };
    static PhysicalBarrierAxes AxesFor(int block) noexcept
    {
        const auto slab=latebrake::PlannedCollisionBlock(block);
        if(block==5 || block==6)
        {
            return {block,
                slab.Center.X-50.0*slab.ScaleX,
                slab.Center.X+50.0*slab.ScaleX,
                slab.Center.Y,block==5?-1.0:1.0,true};
        }
        return {block,
            slab.Center.Y-50.0*slab.ScaleY,
            slab.Center.Y+50.0*slab.ScaleY,
            slab.Center.X,block==7?-1.0:1.0,false};
    }
    static ArmcoPoint Point(
        PhysicalBarrierAxes axes,double s,double depth,
        double height) noexcept
    {
        return axes.Horizontal?
            ArmcoPoint{s,axes.Center+axes.InwardSign*depth,height}:
            ArmcoPoint{axes.Center+axes.InwardSign*depth,s,height};
    }
    struct Writer final
    {
        ArmcoMesh& Mesh;
        const ArmcoSettings& Settings;
        ArmcoFault Fault=ArmcoFault::None;
        void Bound(ArmcoPoint p) noexcept
        {
            Mesh.Bounds.Min.X=std::min(Mesh.Bounds.Min.X,p.X);
            Mesh.Bounds.Min.Y=std::min(Mesh.Bounds.Min.Y,p.Y);
            Mesh.Bounds.Min.Z=std::min(Mesh.Bounds.Min.Z,p.Z);
            Mesh.Bounds.Max.X=std::max(Mesh.Bounds.Max.X,p.X);
            Mesh.Bounds.Max.Y=std::max(Mesh.Bounds.Max.Y,p.Y);
            Mesh.Bounds.Max.Z=std::max(Mesh.Bounds.Max.Z,p.Z);
        }
        bool Triangle(ArmcoPoint a,ArmcoPoint b,ArmcoPoint c,
                      int physical,ArmcoLayer layer)
        {
            if(Fault!=ArmcoFault::None)
                return false;
            if(!ArmcoValidPoint(a) || !ArmcoValidPoint(b) ||
               !ArmcoValidPoint(c) ||
               !ArmcoInsidePhysicalBarrier(a,physical) ||
               !ArmcoInsidePhysicalBarrier(b,physical) ||
               !ArmcoInsidePhysicalBarrier(c,physical))
            {
                Fault=ArmcoFault::InvalidWorldPosition;
                return false;
            }
            auto normal=ArmcoCross(
                ArmcoSubtract(b,a),ArmcoSubtract(c,a));
            const double length=ArmcoLength(normal);
            if(!ArmcoFinite(length) || length<=1e-8)
            {
                Fault=ArmcoFault::DegenerateFace;
                return false;
            }
            normal.X/=length;
            normal.Y/=length;
            normal.Z/=length;
            if(Mesh.Vertices.size()+3>OriginalArmcoMaxVertices ||
               Mesh.Triangles.size()+1>OriginalArmcoMaxTriangles)
            {
                Fault=ArmcoFault::CapacityExceeded;
                return false;
            }
            const auto start=static_cast<std::uint32_t>(
                Mesh.Vertices.size());
            const auto add=[this,normal](ArmcoPoint p)
            {
                Mesh.Vertices.push_back({
                    p,normal,{p.X/Settings.UvRepeatLengthCm,
                              p.Y/Settings.UvRepeatLengthCm+
                              p.Z/Settings.UvRepeatLengthCm}});
                Bound(p);
            };
            add(a);
            add(b);
            add(c);
            Mesh.Triangles.push_back(
                {{start,start+1,start+2},layer,physical});
            return true;
        }
        bool Quad(
            ArmcoPoint a,ArmcoPoint b,ArmcoPoint c,ArmcoPoint d,
            int physical,ArmcoLayer layer)
        {
            return Triangle(a,b,c,physical,layer) &&
                Triangle(a,c,d,physical,layer);
        }
        void Box(
            PhysicalBarrierAxes axes,
            double a,double b,double inner,double outer,
            double bottom,double top,ArmcoLayer layer)
        {
            if(b<=a || outer<=inner || top<=bottom)
            {
                Fault=ArmcoFault::DegenerateFace;
                return;
            }
            const auto p=[axes](double s,double d,double z)
            {
                return Point(axes,s,d,z);
            };
            const auto v0=p(a,inner,bottom);
            const auto v1=p(b,inner,bottom);
            const auto v2=p(b,outer,bottom);
            const auto v3=p(a,outer,bottom);
            const auto v4=p(a,inner,top);
            const auto v5=p(b,inner,top);
            const auto v6=p(b,outer,top);
            const auto v7=p(a,outer,top);
            Quad(v0,v3,v2,v1,axes.Block,layer);
            Quad(v4,v5,v6,v7,axes.Block,layer);
            Quad(v0,v1,v5,v4,axes.Block,layer);
            Quad(v1,v2,v6,v5,axes.Block,layer);
            Quad(v2,v3,v7,v6,axes.Block,layer);
            Quad(v3,v0,v4,v7,axes.Block,layer);
        }
    };
    bool OriginalCollisionPlanMatches() const noexcept
    {
        for(int i=OriginalBarrierFirstBlock;
            i<=OriginalBarrierLastBlock;++i)
        {
            const auto block=latebrake::PlannedCollisionBlock(i);
            if(block.Style!=latebrake::CollisionBlockStyle::Barrier ||
               !latebrake::IsValidCollisionBlock(block) ||
               block.ScaleZ!=2.0 ||
               block.Height!=65.0)
                return false;
            if(i<=6)
            {
                if(block.ScaleX<70.0 ||
                   block.ScaleY!=1.0)
                    return false;
            }
            else if(block.ScaleX!=1.0 ||
                    block.ScaleY<50.0)
                return false;
        }
        return true;
    }
    void WriteRail(
        Writer& writer,PhysicalBarrierAxes axes,
        double railBottom) const
    {
        const int sections=std::max(1,static_cast<int>(
            std::ceil((axes.End-axes.Start)/
                      Settings.RailSamplingLengthCm)));
        const double ds=(axes.End-axes.Start)/sections;
        if(ds>Settings.MaximumSectionLengthCm)
        {
            writer.Fault=ArmcoFault::InvalidSettings;
            return;
        }
        const double h=Settings.RailHeightCm;
        const std::array<double,5> depths{{
            Settings.InnerFaceDepthCm,
            Settings.InnerFaceDepthCm+Settings.WaveDepthCm,
            Settings.InnerFaceDepthCm+0.5*Settings.WaveDepthCm,
            Settings.InnerFaceDepthCm+Settings.WaveDepthCm,
            Settings.InnerFaceDepthCm
        }};
        const auto vertex=[axes,railBottom,h,&depths](
            double s,int stripe,double shift)
        {
            return Point(
                axes,s,depths[static_cast<std::size_t>(stripe)]+shift,
                railBottom+h*static_cast<double>(stripe)/4.0);
        };
        for(int i=0;i<sections;++i)
        {
            const double s0=axes.Start+ds*i;
            const double s1=i+1==sections?
                axes.End:axes.Start+ds*(i+1);
            for(int strip=0;strip<4;++strip)
            {
                const auto a=vertex(s0,strip,0.0);
                const auto b=vertex(s1,strip,0.0);
                const auto c=vertex(s1,strip+1,0.0);
                const auto d=vertex(s0,strip+1,0.0);
                if(!writer.Quad(a,b,c,d,axes.Block,
                                ArmcoLayer::CorrugatedSteel))
                    return;
                const auto backA=vertex(
                    s0,strip,-Settings.MetalThicknessCm);
                const auto backB=vertex(
                    s1,strip,-Settings.MetalThicknessCm);
                const auto backC=vertex(
                    s1,strip+1,-Settings.MetalThicknessCm);
                const auto backD=vertex(
                    s0,strip+1,-Settings.MetalThicknessCm);
                if(!writer.Quad(
                       backD,backC,backB,backA,
                       axes.Block,ArmcoLayer::CorrugatedSteel))
                    return;
                writer.Mesh.CorrugationFaces+=2;
            }
            ++writer.Mesh.RailSections[
                static_cast<std::size_t>(axes.Block-5)];
        }
        // Both end caps are real separate faces of the metal shell.
        for(int stripe=0;stripe<4;++stripe)
        {
            const auto a=vertex(axes.Start,stripe,0.0);
            const auto b=vertex(axes.Start,stripe+1,0.0);
            const auto c=vertex(
                axes.Start,stripe+1,-Settings.MetalThicknessCm);
            const auto d=vertex(
                axes.Start,stripe,-Settings.MetalThicknessCm);
            writer.Quad(a,b,c,d,axes.Block,ArmcoLayer::CorrugatedSteel);
            const auto e=vertex(axes.End,stripe,0.0);
            const auto f=vertex(axes.End,stripe+1,0.0);
            const auto g=vertex(
                axes.End,stripe+1,-Settings.MetalThicknessCm);
            const auto hEnd=vertex(
                axes.End,stripe,-Settings.MetalThicknessCm);
            writer.Quad(hEnd,g,f,e,axes.Block,
                        ArmcoLayer::CorrugatedSteel);
            writer.Mesh.EndCaps+=2;
        }
    }
    void WritePosts(Writer& writer,PhysicalBarrierAxes axes) const
    {
        const int count=std::max(
            2,static_cast<int>(
                std::floor((axes.End-axes.Start)/
                           Settings.PostSpacingCm)));
        const double step=(axes.End-axes.Start)/
            static_cast<double>(count);
        for(int i=1;i<count;++i)
        {
            const double location=axes.Start+step*i;
            const double half=0.5*Settings.PostSectionCm;
            writer.Box(
                axes,location-half,location+half,
                12.0,12.0+Settings.PostSectionCm,
                -25.0,-25.0+Settings.PostHeightCm,
                ArmcoLayer::PostSteel);
            if(writer.Fault!=ArmcoFault::None)
                return;
            ++writer.Mesh.SteelPosts[
                static_cast<std::size_t>(axes.Block-5)];
            for(int rail=0;rail<OriginalArmcoRailLayers;++rail)
            {
                const double z=Settings.RailBaseCm+
                    Settings.RailVerticalSpacingCm*rail+
                    0.5*Settings.RailHeightCm;
                writer.Box(
                    axes,location-Settings.BoltRadiusCm,
                    location+Settings.BoltRadiusCm,
                    Settings.InnerFaceDepthCm+
                        Settings.WaveDepthCm+0.2,
                    Settings.InnerFaceDepthCm+
                        Settings.WaveDepthCm+1.4,
                    z-Settings.BoltRadiusCm,
                    z+Settings.BoltRadiusCm,
                    ArmcoLayer::BoltFace);
                if(writer.Fault!=ArmcoFault::None)
                    return;
                ++writer.Mesh.BoltHeads;
            }
        }
    }
    void WriteSafetyBands(Writer& writer,PhysicalBarrierAxes axes) const
    {
        const int markers=5;
        const double distance=(axes.End-axes.Start)/
            static_cast<double>(markers+1);
        for(int i=1;i<=markers;++i)
        {
            const double center=axes.Start+distance*i;
            const double width=18.0;
            const double z=Settings.RailBaseCm+
                Settings.RailVerticalSpacingCm*2.0+
                0.5*Settings.RailHeightCm;
            writer.Box(
                axes,center-width,center+width,
                Settings.InnerFaceDepthCm+Settings.WaveDepthCm+1.4,
                Settings.InnerFaceDepthCm+Settings.WaveDepthCm+2.5,
                z-Settings.SafetyBandHeightCm,
                z+Settings.SafetyBandHeightCm,
                ArmcoLayer::RedSafetyMarker);
            if(writer.Fault!=ArmcoFault::None)
                return;
            ++writer.Mesh.SafetyBands;
        }
    }
    void WriteOneBarrier(Writer& writer,int index) const
    {
        const auto axes=AxesFor(index);
        for(int layer=0;layer<OriginalArmcoRailLayers;++layer)
        {
            WriteRail(writer,axes,
                Settings.RailBaseCm+
                Settings.RailVerticalSpacingCm*layer);
            if(writer.Fault!=ArmcoFault::None)
                return;
        }
        WritePosts(writer,axes);
        if(writer.Fault!=ArmcoFault::None)
            return;
        WriteSafetyBands(writer,axes);
        if(writer.Fault!=ArmcoFault::None)
            return;
        ++writer.Mesh.VerifiedBarriers[
            static_cast<std::size_t>(index-OriginalBarrierFirstBlock)];
    }
    ArmcoSettings Settings{};
};
} // namespace latebrake::armco
