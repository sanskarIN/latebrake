#pragma once

#include "Vehicles/OriginalVehicleAnimatedWheels.h"
#include "Vehicles/OriginalVehicleSurfaceShading.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace latebrake::vehiclemesh
{
// Compact only identical render attributes, within one material section
// and one animated part. The source mesh, triangle order, winding, UVs,
// normal/tangent seams and wheel-local origin remain unchanged.
struct OriginalIndexedVertex final
{
    OriginalVehiclePoint Position{};
    OriginalVehicleUv TexCoord{};
    OriginalSurfaceFrame Surface{};
};

struct OriginalIndexedSection final
{
    std::vector<OriginalIndexedVertex> Vertices{};
    std::vector<std::uint32_t> Indices{};
    std::size_t SourceTriangleCount=0;
};

struct OriginalIndexedPart final
{
    OriginalVehiclePoint Pivot{};
    std::array<OriginalIndexedSection,OriginalVehicleSectionCount> Sections{};
};

struct OriginalIndexedRender final
{
    bool Valid=false;
    int VehicleId=-1;
    int RimStyle=-1;
    std::array<OriginalIndexedPart,OriginalVehicleAnimatedPartCount> Parts{};
    std::size_t SourceCorners=0;
    std::size_t PackedVertices=0;
    std::size_t IndexCount=0;
    std::size_t NonemptySections=0;

    std::size_t ReusedCorners() const noexcept
    {
        return Valid&&SourceCorners>=PackedVertices
            ? SourceCorners-PackedVertices:0u;
    }
    double VertexReductionFraction() const noexcept
    {
        return Valid&&SourceCorners>0u
            ? static_cast<double>(ReusedCorners())/
                static_cast<double>(SourceCorners):0.0;
    }
};

inline bool ValidIndexedVertex(const OriginalIndexedVertex& v) noexcept
{
    return MeshFinitePoint(v.Position)&&
        std::isfinite(v.TexCoord.U)&&std::isfinite(v.TexCoord.V)&&
        MeshFinitePoint(v.Surface.Normal)&&MeshFinitePoint(v.Surface.Tangent)&&
        std::abs(MeshLength(v.Surface.Normal)-1.0)<=1e-8&&
        std::abs(MeshLength(v.Surface.Tangent)-1.0)<=1e-8&&
        std::abs(SurfaceDot(v.Surface.Normal,v.Surface.Tangent))<=1e-8;
}

// All finite values are compared exactly; no epsilon-based welding or
// quantization of source positions, UVs or shading attributes. Signed
// zero is equivalent under numeric equality. NaN never enters a map.
using OriginalIndexedKey=std::array<double,12>;
inline OriginalIndexedKey IndexedVertexKey(
    const OriginalIndexedVertex& v) noexcept
{
    return {v.Position.X,v.Position.Y,v.Position.Z,
        v.TexCoord.U,v.TexCoord.V,
        v.Surface.Normal.X,v.Surface.Normal.Y,v.Surface.Normal.Z,
        v.Surface.Tangent.X,v.Surface.Tangent.Y,v.Surface.Tangent.Z,
        v.Surface.FlipTangentY?1.0:0.0};
}
inline bool SameIndexedVertex(const OriginalIndexedVertex& a,
    const OriginalIndexedVertex& b) noexcept
{
    return IndexedVertexKey(a)==IndexedVertexKey(b);
}

inline OriginalIndexedVertex IndexedSourceCorner(
    const OriginalVehicleVertex& v,const OriginalSurfaceFrame& s,
    OriginalVehiclePoint pivot) noexcept
{
    return {MeshSubtract(v.Position,pivot),v.TexCoord,s};
}

inline OriginalVehiclePoint IndexedPartPivot(std::size_t part) noexcept
{
    return part==0u?OriginalVehiclePoint{}:
        OriginalWheelPivot(static_cast<int>(part)-1);
}

namespace indexed_detail
{
inline bool ValidRenderInputs(const OriginalVehicleMesh& mesh,
    const OriginalSurfaceShading& shading,
    const OriginalVehicleAnimatedParts& animated) noexcept
{
    if(!mesh.Ready||!shading.Valid||!animated.Valid||
       !OriginalVehicleSurfaceComposer{}.ValidateMesh(mesh)||
       shading.Frames.size()!=mesh.Vertices.size()||
       animated.TotalTriangles!=mesh.Triangles.size())return false;
    for(std::size_t i=0;i<mesh.Vertices.size();++i)
    {
        const auto v=IndexedSourceCorner(mesh.Vertices[i],shading.Frames[i],{});
        if(!ValidIndexedVertex(v)||
           SurfaceDot(v.Surface.Normal,mesh.Vertices[i].Normal)<0.5-1e-8)
            return false;
    }
    // Public partition objects can be malformed. Prove one-to-one
    // triangle membership, original stable order and correct part/material
    // tags before allocating or publishing an indexed render candidate.
    std::array<unsigned char,OriginalVehicleMaxTriangles> seen{};
    std::size_t total=0;
    for(std::size_t p=0;p<animated.Parts.size();++p)
    {
        const auto& part=animated.Parts[p];
        std::size_t count=0;
        if(!part.Valid)return false;
        for(std::size_t m=0;m<part.TriangleIds.size();++m)
        {
            const auto& ids=part.TriangleIds[m];
            if(ids.size()>mesh.Triangles.size())return false;
            for(std::size_t i=0;i<ids.size();++i)
            {
                const auto id=ids[i];
                if(id>=mesh.Triangles.size()||seen[id]!=0u||
                   (i>0u&&id<=ids[i-1u]))return false;
                const auto& tri=mesh.Triangles[id];
                if(static_cast<std::size_t>(tri.WheelSlot+1)!=p||
                   static_cast<std::size_t>(tri.Material)!=m||
                   (p>0u&&!IsSourceWheelTriangleNearPivot(mesh,tri)))
                    return false;
                seen[id]=1u;
                ++count;
            }
        }
        if(count==0u||count!=part.TotalTriangles)return false;
        total+=count;
    }
    if(total!=mesh.Triangles.size())return false;
    const auto body=static_cast<std::size_t>(OriginalVehicleMaterial::BodyPaint);
    const auto rubber=static_cast<std::size_t>(OriginalVehicleMaterial::Rubber);
    const auto alloy=static_cast<std::size_t>(OriginalVehicleMaterial::Alloy);
    const auto rotor=static_cast<std::size_t>(OriginalVehicleMaterial::BrakeRotor);
    if(animated.Parts[0].TriangleIds[body].empty())return false;
    for(std::size_t p=1;p<animated.Parts.size();++p)
    {
        const auto& part=animated.Parts[p];
        if(part.TriangleIds[rubber].empty()||part.TriangleIds[alloy].empty()||
           part.TriangleIds[rotor].empty()||
           part.TriangleIds[rubber].size()!=animated.Parts[1].TriangleIds[rubber].size()||
           part.TriangleIds[rotor].size()!=animated.Parts[1].TriangleIds[rotor].size())
            return false;
    }
    return true;
}

inline bool PackSection(const OriginalVehicleMesh& mesh,
    const OriginalSurfaceShading& shading,
    const std::vector<std::uint32_t>& triangles,OriginalVehiclePoint pivot,
    OriginalIndexedSection& out)
{
    if(triangles.empty())return true;
    out.SourceTriangleCount=triangles.size();
    out.Indices.reserve(triangles.size()*3u);
    out.Vertices.reserve(triangles.size()*3u);
    std::map<OriginalIndexedKey,std::uint32_t> unique;
    for(const auto id:triangles)
    {
        const auto& tri=mesh.Triangles[id];
        const std::size_t begin=out.Indices.size();
        for(const auto source:tri.Index)
        {
            const auto v=IndexedSourceCorner(mesh.Vertices[source],
                shading.Frames[source],pivot);
            if(!ValidIndexedVertex(v))return false;
            const auto key=IndexedVertexKey(v);
            const auto [it,inserted]=unique.emplace(key,
                static_cast<std::uint32_t>(out.Vertices.size()));
            if(inserted)out.Vertices.push_back(v);
            out.Indices.push_back(it->second);
        }
        // A valid authored triangle may never collapse to fewer than
        // three distinct indices during packing.
        const auto a=out.Indices[begin],b=out.Indices[begin+1u],c=out.Indices[begin+2u];
        if(a==b||b==c||a==c)return false;
    }
    return out.Indices.size()==triangles.size()*3u&&
        out.Vertices.size()<=out.Indices.size();
}
} // namespace indexed_detail

// Independent decoding audit: every output index must expand to the exact
// original corner tuple in its original triangle order. Also reject dead
// vertices, duplicate packed keys and falsified totals. This is a one-time
// source rebuild check, never part of the per-frame race/physics path.
inline bool ValidateOriginalIndexedRender(const OriginalVehicleMesh& mesh,
    const OriginalSurfaceShading& shading,
    const OriginalVehicleAnimatedParts& animated,
    const OriginalIndexedRender& packed)
{
    if(!packed.Valid||packed.VehicleId!=mesh.VehicleId||
       packed.RimStyle!=mesh.RimStyle||
       !indexed_detail::ValidRenderInputs(mesh,shading,animated))return false;
    std::size_t corners=0,vertices=0,sections=0;
    for(std::size_t p=0;p<packed.Parts.size();++p)
    {
        const auto& part=packed.Parts[p];
        const auto pivot=IndexedPartPivot(p);
        if(part.Pivot.X!=pivot.X||part.Pivot.Y!=pivot.Y||part.Pivot.Z!=pivot.Z)
            return false;
        for(std::size_t m=0;m<part.Sections.size();++m)
        {
            const auto& s=part.Sections[m];
            const auto& ids=animated.Parts[p].TriangleIds[m];
            if(s.SourceTriangleCount!=ids.size()||
               s.Indices.size()!=ids.size()*3u||
               s.Vertices.size()>s.Indices.size())return false;
            if(ids.empty())
            {
                if(!s.Vertices.empty())return false;
                continue;
            }
            if(s.Vertices.empty())return false;
            std::map<OriginalIndexedKey,std::size_t> keys;
            for(std::size_t i=0;i<s.Vertices.size();++i)
            {
                const auto& v=s.Vertices[i];
                if(!ValidIndexedVertex(v)||
                   !keys.emplace(IndexedVertexKey(v),i).second)return false;
            }
            std::vector<unsigned char> used(s.Vertices.size(),0u);
            std::size_t cursor=0;
            for(const auto id:ids)
            {
                const auto& t=mesh.Triangles[id];
                for(const auto source:t.Index)
                {
                    const auto index=s.Indices[cursor++];
                    if(index>=s.Vertices.size())return false;
                    const auto expected=IndexedSourceCorner(mesh.Vertices[source],
                        shading.Frames[source],pivot);
                    if(!SameIndexedVertex(s.Vertices[index],expected))return false;
                    used[index]=1u;
                }
            }
            for(const auto mark:used)if(mark==0u)return false;
            corners+=s.Indices.size();vertices+=s.Vertices.size();++sections;
        }
    }
    return corners==mesh.Triangles.size()*3u&&
        corners==packed.SourceCorners&&corners==packed.IndexCount&&
        vertices==packed.PackedVertices&&sections==packed.NonemptySections&&
        vertices<=corners;
}

inline OriginalIndexedRender BuildOriginalIndexedRender(
    const OriginalVehicleMesh& mesh,const OriginalSurfaceShading& shading,
    const OriginalVehicleAnimatedParts& animated)
{
    OriginalIndexedRender out{};
    if(!indexed_detail::ValidRenderInputs(mesh,shading,animated))return out;
    out.VehicleId=mesh.VehicleId;out.RimStyle=mesh.RimStyle;
    for(std::size_t p=0;p<out.Parts.size();++p)
    {
        auto& part=out.Parts[p];
        part.Pivot=IndexedPartPivot(p);
        for(std::size_t m=0;m<part.Sections.size();++m)
        {
            const auto& ids=animated.Parts[p].TriangleIds[m];
            if(ids.empty())continue;
            auto& s=part.Sections[m];
            if(!indexed_detail::PackSection(mesh,shading,ids,part.Pivot,s))
                return {};
            out.SourceCorners+=ids.size()*3u;
            out.IndexCount+=s.Indices.size();
            out.PackedVertices+=s.Vertices.size();
            ++out.NonemptySections;
        }
    }
    out.Valid=true;
    if(!ValidateOriginalIndexedRender(mesh,shading,animated,out))return {};
    return out;
}
} // namespace latebrake::vehiclemesh
