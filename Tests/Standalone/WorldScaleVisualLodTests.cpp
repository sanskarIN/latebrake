#include "Track/WorldScaleVisualLodPlan.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool yes,const char* reason)
{
    if(!yes)
    {
        std::cerr<<"WorldScaleVisualLod FAIL: "<<reason<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

double OrientedArea(
    const latebrake::RoundedSmoothVertex& a,
    const latebrake::RoundedSmoothVertex& b,
    const latebrake::RoundedSmoothVertex& c) noexcept
{
    return (b.X-a.X)*(c.Y-a.Y)-
           (b.Y-a.Y)*(c.X-a.X);
}

void GeometryAndWinding()
{
    using namespace latebrake;
    const auto package=BuildWorldScaleVisualLods();
    Check(package.Valid && package.NoIndependentRoadCollision,
          "original road has a complete visual-only three-level package");
    Check(package.TotalVertices==4u*(1024u+512u+256u) &&
          package.TotalTriangles==8u*(1024u+512u+256u),
          "level budgets are exact and bounded without generated duplicates");
    for(unsigned level=0;level<WorldScaleVisualLodCount;++level)
    {
        const auto& lod=package.Levels[level];
        Check(lod.Level==static_cast<WorldScaleVisualLod>(level) &&
              lod.Valid && !lod.HasSeparateCollision &&
              lod.OriginalWorldStaticRoadUnmodified &&
              lod.Sections==WorldScaleVisualLodSections[level] &&
              lod.ValidatedOriginalRoadEndpoints==4u*lod.Sections &&
              lod.MaximumVisualWidthCm==
                  2.0*WorldScaleVisibleHalfWidthCm &&
              lod.MaximumMidpointChordErrorCm<
                  WorldScaleVisualLodMaxMidpointErrorCm[level],
              "each source level is backed by physically authored roadway");
        Check(AuditWorldScaleVisualLod(lod),
              "every mesh corner and triangle refers to original source");

        // The indexed triangle format matches the historical manifold:
        // first and second top faces both wind upward, bottom downward.
        for(unsigned station=0;station<lod.Sections;++station)
        {
            const unsigned base=8u*station;
            for(unsigned face=0;face<4u;++face)
            {
                const auto& tri=lod.Mesh.Triangles[base+face].Indices;
                const double area=OrientedArea(
                    lod.Mesh.Vertices[tri[0]],
                    lod.Mesh.Vertices[tri[1]],
                    lod.Mesh.Vertices[tri[2]]);
                Check(std::isfinite(area) &&
                      (face<2u ? area>1.0e-6 : area<-1.0e-6),
                      "real 3D visual ribbon maintains outward top/bottom normals");
            }
        }
        for(unsigned station=0;station<lod.Sections;++station)
        {
            const auto& a=lod.Mesh.Vertices[station*4u];
            const auto& b=lod.Mesh.Vertices[station*4u+1u];
            const double width=std::hypot(a.X-b.X,a.Y-b.Y);
            Check(std::abs(width-2.0*WorldScaleVisibleHalfWidthCm)<1e-4,
                  "all LOD road sections retain exact visible corridor width");
        }
    }

    const auto ref=BuildWorldScaleVisualRoadMesh();
    const auto& high=package.Levels[0].Mesh;
    Check(ref.Vertices.size()==high.Vertices.size() &&
          ref.Triangles.size()==high.Triangles.size(),
          "highest independent LOD retains the original authored topology");
    for(std::size_t i=0;i<ref.Vertices.size();++i)
    {
        const auto& a=ref.Vertices[i];
        const auto& b=high.Vertices[i];
        Check(std::hypot(a.X-b.X,a.Y-b.Y)<1e-5 &&
              std::abs(a.Z-b.Z)<1e-5,
              "world-scale original visual reference positions match high LOD");
    }

    for(double s : {0.0,WorldScaleCircuitLapCm*0.25,
                    WorldScaleCircuitLapCm*0.50,
                    WorldScaleCircuitLapCm*0.75})
    {
        for(const auto& lod:package.Levels)
        {
            const unsigned section=static_cast<unsigned>(
                s/WorldScaleCircuitLapCm*lod.Sections);
            const auto& vertex=lod.Mesh.Vertices[4u*section];
            Check(IsOnWorldScaleRoad({vertex.X,vertex.Y}),
                  "all four real corner quadrants preserve physical road support");
        }
    }
}

void SelectionWithoutOscillation()
{
    using namespace latebrake;
    WorldScaleVisualLod level=WorldScaleVisualLod::High;
    Check(ChooseWorldScaleVisualLod(0.0,level)==level,
          "near camera uses full 1024-section source mesh");
    Check(ChooseWorldScaleVisualLod(5300.0,level)==level,
          "hysteresis prevents costly near-threshold LOD flapping");
    level=ChooseWorldScaleVisualLod(6100.0,level);
    Check(level==WorldScaleVisualLod::Medium,
          "farther camera selects 512-section noncolliding overlay");
    Check(ChooseWorldScaleVisualLod(4900.0,level)==level,
          "medium LOD does not instantly switch back at 5,000cm");
    level=ChooseWorldScaleVisualLod(16100.0,level);
    Check(level==WorldScaleVisualLod::Far,
          "sufficiently distant camera selects 256-section overlay");
    Check(ChooseWorldScaleVisualLod(14500.0,level)==level,
          "far LOD stays stable near 150m boundary");
    level=ChooseWorldScaleVisualLod(13900.0,level);
    Check(level==WorldScaleVisualLod::Medium,
          "far-to-medium transition requires measured hysteresis");
    level=ChooseWorldScaleVisualLod(3900.0,level);
    Check(level==WorldScaleVisualLod::High,
          "return to close-up source geometry is deterministic");

    Check(ChooseWorldScaleVisualLod(
              std::numeric_limits<double>::quiet_NaN(),level)==level,
          "invalid camera data cannot alter engine geometry");
    Check(ChooseWorldScaleVisualLod(-1.0,level)==level,
          "negative camera distance is not a real source observation");

    WorldScaleVisualLodSettings bad{};
    bad.ForceRenderOnly=false;
    Check(!BuildWorldScaleVisualLods(bad).Valid,
          "no LOD may silently introduce an independent collision surface");
    bad={};
    bad.VisibleHalfWidthCm=475.0;
    Check(!BuildWorldScaleVisualLods(bad).Valid,
          "visual geometry cannot widen the original physical road");
    bad={};
    bad.MediumSwitchDistanceCm =
        std::numeric_limits<double>::infinity();
    Check(!BuildWorldScaleVisualLods(bad).Valid,
          "nonfinite renderer distances do not authorize a new LOD");
    bad={};
    bad.NeverChangeRaceClassification=false;
    Check(!BuildWorldScaleVisualLods(bad).Valid,
          "visual mesh selection cannot alter official race control");
    const auto invalid=BuildWorldScaleVisualLod(
        static_cast<WorldScaleVisualLod>(255));
    Check(!invalid.Valid,"invalid LOD enum does not generate a mesh");
}
} // namespace

int main()
{
    GeometryAndWinding();
    SelectionWithoutOscillation();
    std::cout<<"WorldScaleVisualLod PASS: 1024/512/256 "
                "visually backed closed meshes, original seam, "
                "top/bottom triangle winding and camera hysteresis "
                "(source only, no Unreal renderer)\n";
    return EXIT_SUCCESS;
}
