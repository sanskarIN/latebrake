#include "Vehicles/OriginalVehicleIndexedRender.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake::vehiclemesh;
void Check(bool ok,const char* why)
{
    if(!ok){std::cerr<<"IndexedRender: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
void AttributeIdentity()
{
    OriginalIndexedVertex a{{1,2,3},{0.25,0.75},{{0,0,1},{1,0,0},false}};
    Check(ValidIndexedVertex(a),"unit normal and tangent accepted");
    auto b=a;b.Position.X+=1e-9;
    Check(!SameIndexedVertex(a,b),"no epsilon position welding");
    b=a;b.TexCoord.U+=1e-9;
    Check(!SameIndexedVertex(a,b),"UV seam remains distinct");
    b=a;b.Surface.Normal={0,1,0};
    Check(ValidIndexedVertex(b)&&!SameIndexedVertex(a,b),"hard normal seam distinct");
    b=a;b.Surface.Tangent={0,1,0};
    Check(ValidIndexedVertex(b)&&!SameIndexedVertex(a,b),"tangent seam distinct");
    b=a;b.Surface.FlipTangentY=true;
    Check(!SameIndexedVertex(a,b),"mirrored tangent seam distinct");
    b=a;b.Surface.Tangent={0,0,1};
    Check(!ValidIndexedVertex(b),"parallel tangent rejected");
    b=a;b.Position.X=std::numeric_limits<double>::quiet_NaN();
    Check(!ValidIndexedVertex(b),"NaN key prohibited");
}
void AllOriginalMeshes()
{
    std::size_t allCorners=0,allPacked=0;
    for(int car=0;car<6;++car)
    for(int rim=0;rim<OriginalVehicleRimStyleCount;++rim)
    {
        const auto built=OriginalVehicleSurfaceComposer{}.Build(car,rim);
        Check(built.Valid,"actual original geometry");
        const auto& mesh=built.Mesh;
        const auto shading=BuildOriginalSurfaceShading(mesh);
        const auto animated=MakeOriginalVehicleAnimatedParts(mesh);
        const auto packed=BuildOriginalIndexedRender(mesh,shading,animated);
        Check(packed.Valid&&packed.VehicleId==car&&packed.RimStyle==rim&&
            packed.SourceCorners==mesh.Triangles.size()*3u&&
            packed.IndexCount==packed.SourceCorners&&
            packed.PackedVertices<packed.SourceCorners&&
            packed.ReusedCorners()>0u&&packed.VertexReductionFraction()>0.0&&
            packed.VertexReductionFraction()<1.0&&
            ValidateOriginalIndexedRender(mesh,shading,animated,packed),
            "all 18 source configurations pack and decode without loss");
        const auto repeat=BuildOriginalIndexedRender(mesh,shading,animated);
        Check(repeat.Valid&&repeat.PackedVertices==packed.PackedVertices,
            "deterministic total count");
        for(std::size_t p=0;p<packed.Parts.size();++p)
        for(std::size_t m=0;m<packed.Parts[p].Sections.size();++m)
        {
            const auto& part=packed.Parts[p];
            const auto& section=part.Sections[m];
            const auto& again=repeat.Parts[p].Sections[m];
            const auto& source=animated.Parts[p].TriangleIds[m];
            Check(section.Indices==again.Indices&&
                section.Vertices.size()==again.Vertices.size()&&
                section.SourceTriangleCount==source.size(),
                "deterministic per-section index stream");
            for(std::size_t i=0;i<section.Vertices.size();++i)
                Check(SameIndexedVertex(section.Vertices[i],again.Vertices[i]),
                    "deterministic original attributes");
            std::size_t corner=0;
            for(const auto tri:source)
            for(const auto index:mesh.Triangles[tri].Index)
            {
                const auto& original=mesh.Vertices[index];
                const auto& v=section.Vertices[section.Indices[corner++]];
                Check(v.Position.X==original.Position.X-part.Pivot.X&&
                    v.Position.Y==original.Position.Y-part.Pivot.Y&&
                    v.Position.Z==original.Position.Z-part.Pivot.Z&&
                    v.TexCoord.U==original.TexCoord.U&&v.TexCoord.V==original.TexCoord.V&&
                    v.Surface.FlipTangentY==shading.Frames[index].FlipTangentY&&
                    v.Surface.Normal.X==shading.Frames[index].Normal.X&&
                    v.Surface.Normal.Y==shading.Frames[index].Normal.Y&&
                    v.Surface.Normal.Z==shading.Frames[index].Normal.Z&&
                    v.Surface.Tangent.X==shading.Frames[index].Tangent.X&&
                    v.Surface.Tangent.Y==shading.Frames[index].Tangent.Y&&
                    v.Surface.Tangent.Z==shading.Frames[index].Tangent.Z,
                    "independent exact per-corner oracle retains authored topology and seams");
            }
        }
        allCorners+=packed.SourceCorners;allPacked+=packed.PackedVertices;
    }
    std::cout<<"18 configurations: "<<allCorners<<" source corners -> "
        <<allPacked<<" packed vertices (indices unchanged)\n";
}
void MalformedInputs()
{
    const auto mesh=OriginalVehicleSurfaceComposer{}.Build(2,1).Mesh;
    const auto shading=BuildOriginalSurfaceShading(mesh);
    const auto parts=MakeOriginalVehicleAnimatedParts(mesh);
    const auto good=BuildOriginalIndexedRender(mesh,shading,parts);
    Check(good.Valid,"baseline packed geometry");
    auto badShade=shading;badShade.Frames.pop_back();
    Check(!BuildOriginalIndexedRender(mesh,badShade,parts).Valid,"short frame stream");
    badShade=shading;badShade.Frames[0].Normal={0,0,0};
    Check(!BuildOriginalIndexedRender(mesh,badShade,parts).Valid,"invalid frame");
    auto badParts=parts;
    auto& body=badParts.Parts[0].TriangleIds[0];
    Check(body.size()>2u,"body fixture has multiple triangles");
    body[1]=body[0];
    Check(!BuildOriginalIndexedRender(mesh,shading,badParts).Valid,"duplicate triangle");
    badParts=parts;badParts.Parts[0].TriangleIds[0][0]=
        static_cast<std::uint32_t>(mesh.Triangles.size());
    Check(!BuildOriginalIndexedRender(mesh,shading,badParts).Valid,"out of bounds triangle");
    badParts=parts;
    std::swap(badParts.Parts[0].TriangleIds[0][0],badParts.Parts[0].TriangleIds[0][1]);
    Check(!BuildOriginalIndexedRender(mesh,shading,badParts).Valid,"reordered source triangles");
    auto bad=good;
    bad.Parts[0].Sections[0].Indices[0]=
        static_cast<std::uint32_t>(bad.Parts[0].Sections[0].Vertices.size());
    Check(!ValidateOriginalIndexedRender(mesh,shading,parts,bad),"invalid packed index");
    bad=good;bad.Parts[0].Sections[0].Vertices[0].TexCoord.U+=0.125;
    Check(!ValidateOriginalIndexedRender(mesh,shading,parts,bad),"altered UV");
    bad=good;bad.Parts[1].Pivot.X+=1.0;
    Check(!ValidateOriginalIndexedRender(mesh,shading,parts,bad),"incorrect wheel pivot");
    bad=good;bad.PackedVertices++;
    Check(!ValidateOriginalIndexedRender(mesh,shading,parts,bad),"forged count");
    bad=good;bad.Parts[0].Sections[0].Vertices.push_back(bad.Parts[0].Sections[0].Vertices[0]);
    Check(!ValidateOriginalIndexedRender(mesh,shading,parts,bad),"duplicate or unused vertex");
}
}
int main()
{
    AttributeIdentity();AllOriginalMeshes();MalformedInputs();
    return EXIT_SUCCESS;
}
