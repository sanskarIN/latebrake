#pragma once

#include "Vehicles/OriginalVehicleSurfaceMesh.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <numeric>
#include <tuple>
#include <vector>

namespace latebrake::vehiclemesh
{
// Render-only normals/tangents for the original triangle-soup authoring
// format. Position, UV, topology, wheel ownership and collision remain
// identical. Smoothing connects only exactly two compatible faces
// sharing an edge; coincident vertices alone cannot join surfaces.
struct OriginalSurfaceShadingSettings final
{
    double CreaseDegrees=48.0;
    double WeldToleranceCm=0.00001;
    bool SmoothBody=true;
    bool SmoothGlass=true;
    bool SmoothRubber=true;
};

struct OriginalSurfaceFrame final
{
    OriginalVehiclePoint Normal{};
    OriginalVehiclePoint Tangent{};
    bool FlipTangentY=false;
};

struct OriginalSurfaceShading final
{
    bool Valid=false;
    std::vector<OriginalSurfaceFrame> Frames{};
    std::size_t SmoothEdges=0;
    std::size_t HardEdges=0;
    std::size_t NonManifoldEdges=0;
    std::size_t BoundaryEdges=0;
    std::size_t ChangedNormals=0;
    std::size_t FallbackTangents=0;
};

inline double SurfaceDot(OriginalVehiclePoint a,OriginalVehiclePoint b) noexcept
{
    return a.X*b.X+a.Y*b.Y+a.Z*b.Z;
}
inline OriginalVehiclePoint SurfaceAdd(
    OriginalVehiclePoint a,OriginalVehiclePoint b) noexcept
{
    return {a.X+b.X,a.Y+b.Y,a.Z+b.Z};
}
inline OriginalVehiclePoint SurfaceScale(OriginalVehiclePoint a,double s) noexcept
{
    return {a.X*s,a.Y*s,a.Z*s};
}
inline bool SurfaceNormalize(OriginalVehiclePoint input,
    OriginalVehiclePoint& output) noexcept
{
    const double length=MeshLength(input);
    if(!MeshFinitePoint(input)||!std::isfinite(length)||length<1e-12)
        return false;
    output=SurfaceScale(input,1.0/length);
    return MeshFinitePoint(output);
}
inline bool ValidOriginalSurfaceShadingSettings(
    OriginalSurfaceShadingSettings s) noexcept
{
    return std::isfinite(s.CreaseDegrees)&&s.CreaseDegrees>=0.0&&
        s.CreaseDegrees<=75.0&&std::isfinite(s.WeldToleranceCm)&&
        s.WeldToleranceCm>=1e-7&&s.WeldToleranceCm<=1e-3;
}
inline bool OriginalSurfaceMaterialSmooth(
    OriginalVehicleMaterial m,OriginalSurfaceShadingSettings s) noexcept
{
    switch(m)
    {
    case OriginalVehicleMaterial::BodyPaint:return s.SmoothBody;
    case OriginalVehicleMaterial::ClearGlass:return s.SmoothGlass;
    case OriginalVehicleMaterial::Rubber:return s.SmoothRubber;
    default:return false;
    }
}

namespace shading_detail
{
using WeldKey=std::array<std::int64_t,3>;
inline WeldKey Quantize(OriginalVehiclePoint p,double step) noexcept
{
    // Composer validates bounded centimetre coordinates before this call.
    // Small grid keys close sin(2pi) wheel seams without altering positions.
    return {static_cast<std::int64_t>(std::llround(p.X/step)),
            static_cast<std::int64_t>(std::llround(p.Y/step)),
            static_cast<std::int64_t>(std::llround(p.Z/step))};
}

class CornerSets final
{
public:
    explicit CornerSets(std::size_t count):parent_(count),rank_(count,0u)
    {
        std::iota(parent_.begin(),parent_.end(),std::size_t{0});
    }
    std::size_t Find(std::size_t i) noexcept
    {
        while(parent_[i]!=i)
        {
            parent_[i]=parent_[parent_[i]];
            i=parent_[i];
        }
        return i;
    }
    void Join(std::size_t a,std::size_t b) noexcept
    {
        a=Find(a);b=Find(b);
        if(a==b)return;
        if(rank_[a]<rank_[b])std::swap(a,b);
        parent_[b]=a;
        if(rank_[a]==rank_[b])++rank_[a];
    }
private:
    std::vector<std::size_t> parent_;
    std::vector<unsigned> rank_;
};

struct FaceFrame final
{
    OriginalVehiclePoint Normal{};
    OriginalVehiclePoint Tangent{};
    OriginalVehiclePoint Bitangent{};
    std::array<double,3> CornerAngle{};
    bool UvUsable=false;
};

inline bool BuildFaceFrame(const OriginalVehicleMesh& mesh,
    const OriginalVehicleTriangle& tri,FaceFrame& frame) noexcept
{
    const auto& a=mesh.Vertices[tri.Index[0]];
    const auto& b=mesh.Vertices[tri.Index[1]];
    const auto& c=mesh.Vertices[tri.Index[2]];
    const auto e1=MeshSubtract(b.Position,a.Position);
    const auto e2=MeshSubtract(c.Position,a.Position);
    if(!SurfaceNormalize(MeshCross(e1,e2),frame.Normal))return false;
    for(unsigned corner=0;corner<3u;++corner)
    {
        const auto& v=mesh.Vertices[tri.Index[corner]];
        // The composer produces one authored flat normal per corner.
        // Reject other topology/normal contracts instead of silently
        // interpreting arbitrary imported meshes as source triangles.
        if(SurfaceDot(v.Normal,frame.Normal)<1.0-1e-6)return false;
        const auto p=v.Position;
        OriginalVehiclePoint d0{},d1{};
        if(!SurfaceNormalize(MeshSubtract(
            mesh.Vertices[tri.Index[(corner+1u)%3u]].Position,p),d0)||
           !SurfaceNormalize(MeshSubtract(
            mesh.Vertices[tri.Index[(corner+2u)%3u]].Position,p),d1))
            return false;
        const double angle=std::acos(std::clamp(SurfaceDot(d0,d1),-1.0,1.0));
        if(!std::isfinite(angle)||angle<=0.0)return false;
        frame.CornerAngle[corner]=angle;
    }
    const double du1=b.TexCoord.U-a.TexCoord.U,dv1=b.TexCoord.V-a.TexCoord.V;
    const double du2=c.TexCoord.U-a.TexCoord.U,dv2=c.TexCoord.V-a.TexCoord.V;
    const double determinant=du1*dv2-du2*dv1;
    if(std::isfinite(determinant)&&std::abs(determinant)>1e-12)
    {
        const auto t=SurfaceScale(MeshSubtract(
            SurfaceScale(e1,dv2),SurfaceScale(e2,dv1)),1.0/determinant);
        const auto bt=SurfaceScale(MeshSubtract(
            SurfaceScale(e2,du1),SurfaceScale(e1,du2)),1.0/determinant);
        frame.UvUsable=SurfaceNormalize(t,frame.Tangent)&&
            SurfaceNormalize(bt,frame.Bitangent);
    }
    return true;
}

struct EdgeUse final
{
    std::size_t KeyA=0,KeyB=0;
    std::size_t CornerA=0,CornerB=0;
    std::size_t Face=0;
    int Wheel=-1;
    OriginalVehicleMaterial Material=OriginalVehicleMaterial::BodyPaint;
    bool Forward=false;
};
inline auto EdgeKey(const EdgeUse& e) noexcept
{
    // Keep separate materials/wheel slots in the same geometric bucket:
    // they are classified as hard edges, never joined across a boundary.
    return std::tie(e.KeyA,e.KeyB);
}
inline bool SameEdge(const EdgeUse& a,const EdgeUse& b) noexcept
{
    return EdgeKey(a)==EdgeKey(b);
}
inline bool CompatibleEdge(const EdgeUse& a,const EdgeUse& b,
    const std::vector<FaceFrame>& faces,OriginalSurfaceShadingSettings settings,
    double cosine) noexcept
{
    return a.Face!=b.Face&&a.Forward!=b.Forward&&
        a.Wheel==b.Wheel&&a.Material==b.Material&&
        OriginalSurfaceMaterialSmooth(a.Material,settings)&&
        SurfaceDot(faces[a.Face].Normal,faces[b.Face].Normal)>=cosine;
}
inline bool StableTangent(OriginalVehiclePoint n,OriginalVehiclePoint& t) noexcept
{
    // Select the least aligned axis, then project into the tangent plane.
    const double x=std::abs(n.X),y=std::abs(n.Y),z=std::abs(n.Z);
    const OriginalVehiclePoint axis=x<=y&&x<=z
        ? OriginalVehiclePoint{1.0,0.0,0.0}
        : y<=z?OriginalVehiclePoint{0.0,1.0,0.0}
              :OriginalVehiclePoint{0.0,0.0,1.0};
    return SurfaceNormalize(MeshSubtract(axis,SurfaceScale(n,SurfaceDot(n,axis))),t);
}
} // namespace shading_detail

inline OriginalSurfaceShading BuildOriginalSurfaceShading(
    const OriginalVehicleMesh& mesh,OriginalSurfaceShadingSettings settings={})
{
    using namespace shading_detail;
    if(!mesh.Ready||!ValidOriginalSurfaceShadingSettings(settings)||
       !OriginalVehicleSurfaceComposer{}.ValidateMesh(mesh)||
       mesh.Vertices.size()!=mesh.Triangles.size()*3u)return {};
    OriginalSurfaceShading result{};
    const std::size_t corners=mesh.Vertices.size();
    const std::size_t faces=mesh.Triangles.size();
    std::map<WeldKey,std::size_t> weld;
    std::vector<std::size_t> weldIds(corners);
    for(std::size_t i=0;i<corners;++i)
    {
        const auto key=Quantize(mesh.Vertices[i].Position,settings.WeldToleranceCm);
        const auto inserted=weld.emplace(key,weld.size());
        weldIds[i]=inserted.first->second;
    }

    std::vector<FaceFrame> frames(faces);
    std::vector<unsigned char> seen(corners,0u);
    std::vector<EdgeUse> edges;
    edges.reserve(faces*3u);
    for(std::size_t face=0;face<faces;++face)
    {
        const auto& tri=mesh.Triangles[face];
        if(!BuildFaceFrame(mesh,tri,frames[face]))return {};
        for(const auto index:tri.Index)
        {
            if(seen[index]!=0u)return {}; // original triangle soup only
            seen[index]=1u;
        }
        for(unsigned i=0;i<3u;++i)
        {
            const std::size_t a=tri.Index[i],b=tri.Index[(i+1u)%3u];
            const auto wa=weldIds[a],wb=weldIds[b];
            if(wa==wb)return {}; // grid would collapse an actual edge
            const bool forward=wa<wb;
            edges.push_back({std::min(wa,wb),std::max(wa,wb),
                forward?a:b,forward?b:a,face,static_cast<int>(tri.WheelSlot),
                tri.Material,forward});
        }
    }
    if(std::find(seen.begin(),seen.end(),static_cast<unsigned char>(0))!=seen.end())
        return {};

    // O(n log n), deterministic sorted edge groups. A manifold pair may
    // smooth only if its own angle/material/part allow it. Non-manifold
    // groups fail closed locally, preserving their original hard seams.
    std::sort(edges.begin(),edges.end(),[](const EdgeUse& a,const EdgeUse& b)
    {
        if(EdgeKey(a)!=EdgeKey(b))return EdgeKey(a)<EdgeKey(b);
        return std::tie(a.Face,a.CornerA,a.CornerB)<
            std::tie(b.Face,b.CornerA,b.CornerB);
    });
    CornerSets groups(corners);
    const double cosine=std::cos(settings.CreaseDegrees*OriginalVehiclePi/180.0);
    for(std::size_t begin=0;begin<edges.size();)
    {
        std::size_t end=begin+1u;
        while(end<edges.size()&&SameEdge(edges[begin],edges[end]))++end;
        const auto count=end-begin;
        if(count==1u)++result.BoundaryEdges;
        else if(count!=2u)++result.NonManifoldEdges;
        else if(CompatibleEdge(edges[begin],edges[begin+1u],frames,settings,cosine))
        {
            const auto& a=edges[begin];
            const auto& b=edges[begin+1u];
            groups.Join(a.CornerA,b.CornerA);
            groups.Join(a.CornerB,b.CornerB);
            ++result.SmoothEdges;
        }
        else ++result.HardEdges;
        begin=end;
    }

    // Angle weighting removes triangulation-dependent normal bias from
    // long thin body panels. UVs intentionally remain duplicated so
    // texture seams, authored panel divisions and wheel pivots survive.
    std::vector<OriginalVehiclePoint> sums(corners);
    for(std::size_t face=0;face<faces;++face)
    {
        const auto& tri=mesh.Triangles[face];
        const auto& frame=frames[face];
        for(unsigned c=0;c<3u;++c)
        {
            const auto root=groups.Find(tri.Index[c]);
            sums[root]=SurfaceAdd(sums[root],
                SurfaceScale(frame.Normal,frame.CornerAngle[c]));
        }
    }

    result.Frames.resize(corners);
    for(std::size_t face=0;face<faces;++face)
    {
        const auto& tri=mesh.Triangles[face];
        const auto& frame=frames[face];
        for(unsigned c=0;c<3u;++c)
        {
            const auto index=tri.Index[c];
            auto& out=result.Frames[index];
            if(!SurfaceNormalize(sums[groups.Find(index)],out.Normal)||
               SurfaceDot(out.Normal,frame.Normal)<0.5)
                out.Normal=frame.Normal;
            if(SurfaceDot(out.Normal,mesh.Vertices[index].Normal)<1.0-1e-8)
                ++result.ChangedNormals;
            const auto projected=MeshSubtract(frame.Tangent,
                SurfaceScale(out.Normal,SurfaceDot(out.Normal,frame.Tangent)));
            if(!frame.UvUsable||!SurfaceNormalize(projected,out.Tangent))
            {
                if(!StableTangent(out.Normal,out.Tangent))return {};
                ++result.FallbackTangents;
            }
            else
            {
                // Mirrored UVs need the opposite tangent-Y orientation.
                out.FlipTangentY=SurfaceDot(
                    MeshCross(out.Normal,out.Tangent),frame.Bitangent)<0.0;
            }
            if(!MeshFinitePoint(out.Normal)||!MeshFinitePoint(out.Tangent)||
               std::abs(MeshLength(out.Normal)-1.0)>1e-8||
               std::abs(MeshLength(out.Tangent)-1.0)>1e-8||
               std::abs(SurfaceDot(out.Normal,out.Tangent))>1e-8)return {};
        }
    }
    result.Valid=result.Frames.size()==corners;
    return result;
}
} // namespace latebrake::vehiclemesh
