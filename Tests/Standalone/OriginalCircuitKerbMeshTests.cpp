#include "Track/OriginalCircuitKerbMesh.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace latebrake::kerbing;
static void Check(bool condition,const char* message)
{
    if(!condition)
    {
        std::cerr<<"Original circuit curbs FAIL: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
static void EightOriginalPavedEdges()
{
    OriginalCircuitKerbComposer builder;
    const auto built=builder.Build();
    const auto& mesh=built.Mesh;
    Check(built.Valid && mesh.Ready &&
          built.Fault==KerbFault::None &&
          builder.Validate(mesh) &&
          mesh.PurelyVisual &&
          mesh.OriginalCollisionUnmodified &&
          mesh.OfficialRaceProgressUnmodified,
          "alternating painted curb geometry remains original and visual-only");
    Check(mesh.StripePairs>80 &&
          mesh.RedStripes>20 &&
          mesh.WhiteStripes>20 &&
          mesh.AsphaltBlendBands==mesh.StripePairs &&
          mesh.CastShadowLines==mesh.StripePairs &&
          mesh.DrainageMarks>20 &&
          mesh.Triangles.size()>400,
          "all sections have clear contrasting pavement details");
    int pieces=0;
    for(int length:mesh.SectionsByEdge)
    {
        Check(length>=5 && length<=OriginalKerbMaxSectionsPerEdge,
              "each of eight original paved edges is separately supported");
        pieces+=length;
    }
    Check(pieces==mesh.StripePairs &&
          mesh.Vertices.size()==mesh.Triangles.size()*3,
          "material and geometry counts reconcile");
    for(const auto& face:mesh.Triangles)
    {
        Check(face.PhysicalSlab>=1 && face.PhysicalSlab<=4,
              "every zebra curb is attached to real collision-bearing asphalt");
        for(auto id:face.Indices)
        {
            Check(id<mesh.Vertices.size(),"bounded index");
            const auto& vertex=mesh.Vertices[id];
            Check(KerbOnOriginalRoad(vertex.Position,face.PhysicalSlab),
                  "no kerb may draw a fake new driving surface on grass");
            Check(vertex.Position.Z>-1.0 &&
                  vertex.Position.Z<0.0,
                  "paint floats only tenths of a centimetre above asphalt");
        }
    }
}
static void DeterministicTrackColorAndPosition()
{
    OriginalCircuitKerbComposer builder;
    const auto first=builder.Build();
    const auto second=builder.Build();
    Check(first.Valid && second.Valid &&
          first.Mesh.Triangles.size()==second.Mesh.Triangles.size() &&
          first.Mesh.StripePairs==second.Mesh.StripePairs,
          "same physical original circuit yields same curb layout");
    for(std::size_t i=0;i<first.Mesh.Vertices.size();++i)
    {
        const auto& a=first.Mesh.Vertices[i];
        const auto& b=second.Mesh.Vertices[i];
        Check(a.Position.X==b.Position.X &&
              a.Position.Y==b.Position.Y &&
              a.Position.Z==b.Position.Z &&
              a.Uv.U==b.Uv.U &&
              a.Uv.V==b.Uv.V,
              "full generated vertex sequence is repeatable");
    }
    for(std::size_t i=0;i<first.Mesh.Triangles.size();++i)
    {
        const auto& a=first.Mesh.Triangles[i];
        const auto& b=second.Mesh.Triangles[i];
        Check(a.Indices==b.Indices &&
              a.Surface==b.Surface &&
              a.Segment==b.Segment &&
              a.PhysicalSlab==b.PhysicalSlab,
              "stripe color and real asphalt ownership never drift");
    }
}
static void MalformedPhysicalAndVisualInputs()
{
    KerbSettings p;
    Check(ValidKerbSettings(p),"finite safe defaults");
    p.NeverChangeTrackCollision=false;
    Check(!OriginalCircuitKerbComposer(p).Build().Valid,
          "visual kerb cannot become new rigid collision");
    p={};
    p.NoImpliedNewRoad=false;
    Check(!OriginalCircuitKerbComposer(p).Build().Valid,
          "paint is restricted to existing physical pavement");
    p={};
    p.PaintWidthCm=1000.0;
    Check(!OriginalCircuitKerbComposer(p).Build().Valid,
          "implausible curb width refused");
    p={};
    p.PanelLengthCm=std::numeric_limits<double>::quiet_NaN();
    Check(!OriginalCircuitKerbComposer(p).Build().Valid,
          "nonfinite source never creates geometry");
    p={};
    p.DecorativeLayerLiftCm=3.0;
    Check(!OriginalCircuitKerbComposer(p).Build().Valid,
          "no fake raised driveable curb beyond actual road");
    p={};
    p.MinimumStraightClearanceFromCornerCm=400.0;
    Check(!OriginalCircuitKerbComposer(p).Build().Valid,
          "avoid unsupported junction decoration");
    OriginalCircuitKerbComposer original;
    auto mesh=original.Build().Mesh;
    mesh.OriginalCollisionUnmodified=false;
    Check(!original.Validate(mesh),
          "a render mesh cannot claim a collision override");
    mesh=original.Build().Mesh;
    mesh.Triangles[0].Indices[0]=
        static_cast<std::uint32_t>(mesh.Vertices.size()+5);
    Check(!original.Validate(mesh),
          "broken index cannot reach the renderer");
    mesh=original.Build().Mesh;
    mesh.Vertices[0].Position={0.0,0.0,0.0};
    Check(!original.Validate(mesh),
          "grass/infield geometry must fail the physical road proof");
}
static void AlternativeDensityAndPaint()
{
    KerbSettings settings;
    settings.PanelLengthCm=110.0;
    settings.PaintWidthCm=40.0;
    settings.RoadEdgeInsetCm=55.0;
    settings.MinimumStraightClearanceFromCornerCm=900.0;
    const OriginalCircuitKerbComposer builder(settings);
    const auto result=builder.Build();
    Check(result.Valid &&
          result.Mesh.StripePairs>100 &&
          builder.Validate(result.Mesh),
          "higher density original circuit geometry remains bounded");
    Check(!KerbOnOriginalRoad({0.0,0.0,-0.6},1),
          "interior grass never accepts a red/white curb");
    Check(!KerbOnOriginalRoad({
          std::numeric_limits<double>::quiet_NaN(),0.0,0.0},1),
          "nonfinite coordinates are unpaintable");
}
int main()
{
    EightOriginalPavedEdges();
    DeterministicTrackColorAndPosition();
    MalformedPhysicalAndVisualInputs();
    AlternativeDensityAndPaint();
    std::cout<<"Original circuit 8-edge physical asphalt kerb layout validated\n";
}
