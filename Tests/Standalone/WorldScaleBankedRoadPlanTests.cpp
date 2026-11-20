#include "Track/WorldScaleBankedRoadPlan.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool good, const char* text)
{
    if(!good)
    {
        std::cerr<<"WorldScaleBankedRoadPlan FAIL: "<<text<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b,double tolerance=1.0e-8)
{
    return std::abs(a-b)<=tolerance;
}
}

int main()
{
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    const double inf=std::numeric_limits<double>::infinity();
    WorldScaleBankedRoadProfile invalid{};
    Check(!ChooseWorldScaleBankedRoadProfile(
        static_cast<WorldScaleVariantId>(6),invalid),
        "invalid circuit cannot select a profile");
    Check(!SampleWorldScaleBankedRoad(
        WorldScaleOriginalVariantRecipes[0],invalid,nan,0.0).Valid,
        "nonfinite circuit station fails closed");

    unsigned audited=0;
    for(unsigned id=0;id<WorldScaleVariantCount;++id)
    {
        const auto circuit=WorldScaleOriginalVariantRecipes[id];
        WorldScaleBankedRoadProfile profile;
        Check(ChooseWorldScaleBankedRoadProfile(circuit.Id,profile),
              "select every authentic circuit profile");
        Check(ValidWorldScaleBankedRoadProfile(circuit,profile),
              "profile must match source route and no-collision guarantee");
        const double lap=circuit.LapLengthCm;
        const auto start=SampleWorldScaleBankedRoad(
            circuit,profile,0.0,0.0);
        const auto finish=SampleWorldScaleBankedRoad(
            circuit,profile,lap,0.0);
        const auto minusLap=SampleWorldScaleBankedRoad(
            circuit,profile,-lap,0.0);
        Check(start.Valid && finish.Valid && minusLap.Valid,
              "signed lap loop remains fully sampled");
        Check(Near(start.WorldZcm,profile.RoadTopCm) &&
              Near(start.WorldZcm,finish.WorldZcm) &&
              Near(start.WorldZcm,minusLap.WorldZcm),
              "periodic elevation is exactly continuous at start and finish");
        Check(Near(start.GradeRadians,finish.GradeRadians) &&
              Near(start.BankRadians,finish.BankRadians),
              "periodic slope and banking do not jump at lap seam");
        Check(Near(start.SurfaceNormalX,finish.SurfaceNormalX) &&
              Near(start.SurfaceNormalY,finish.SurfaceNormalY) &&
              Near(start.SurfaceNormalZ,finish.SurfaceNormalZ),
              "normal continuity preserved through seam");

        const std::array<double,6> stations{{
            0.0,0.125*lap,0.25*lap,0.5*lap,
            0.875*lap,lap-1.0
        }};
        for(double station:stations)
        {
            const auto middle=SampleWorldScaleBankedRoad(
                circuit,profile,station,0.0);
            const auto left=SampleWorldScaleBankedRoad(
                circuit,profile,station,circuit.VisualHalfWidthCm);
            const auto right=SampleWorldScaleBankedRoad(
                circuit,profile,station,-circuit.VisualHalfWidthCm);
            const auto over=SampleWorldScaleBankedRoad(
                circuit,profile,station+lap,0.0);
            Check(middle.Valid && left.Valid && right.Valid &&
                  over.Valid,"center and both wheel-side XY road proof");
            Check(Near(middle.WorldZcm,over.WorldZcm,1e-7),
                  "whole-lap station shift repeats physical geometry");
            Check(middle.OriginalRoadXYQualified &&
                  middle.OriginalCollisionUnaffected &&
                  left.OriginalRoadXYQualified &&
                  right.OriginalRoadXYQualified,
                  "road proof must never imply native Chaos authority");
            const double normalLength=std::hypot(
                std::hypot(middle.SurfaceNormalX,
                           middle.SurfaceNormalY),
                middle.SurfaceNormalZ);
            Check(Near(normalLength,1.0,1e-12) &&
                  middle.SurfaceNormalZ>0.95,
                  "normal stays unit-length and points upward");
            Check(std::abs(middle.GradeRadians)<0.15 &&
                  std::abs(middle.BankRadians)<0.12,
                  "sampled surface remains within conservative pitch/bank");
            Check(Near(middle.WorldZcm,
                      (left.WorldZcm+right.WorldZcm)*0.5,1e-8),
                  "lateral banking is symmetric around centerline");
            Check(!SampleWorldScaleBankedRoad(
                      circuit,profile,station,
                      circuit.VisualHalfWidthCm+0.001).Valid,
                  "cannot sample beyond proven original visual road");
        }

        const double quarter=lap/8.0;
        const auto leftBank=SampleWorldScaleBankedRoad(
            circuit,profile,quarter,300.0);
        const auto rightBank=SampleWorldScaleBankedRoad(
            circuit,profile,quarter,-300.0);
        Check(leftBank.Valid && rightBank.Valid &&
              leftBank.WorldZcm>rightBank.WorldZcm,
              "positive banking tilts left-side road above right-side road");
        Check(Near(leftBank.WorldZcm-rightBank.WorldZcm,
                   600.0*std::tan(profile.BankAmplitudeRadians),1e-6),
              "bank angle is measured from lateral elevation");
        const auto flatAtBank=SampleWorldScaleBankedRoad(
            circuit,profile,quarter,0.0);
        Check(Near(flatAtBank.BankRadians,
                   profile.BankAmplitudeRadians,1e-9),
              "peak bank parameter survives exact evaluation");

        const auto footprint=SampleWorldScaleBankedFootprint(
            circuit,profile,0.125*lap,0.0);
        Check(footprint.Valid && footprint.SourceOnly &&
              footprint.MaximumWheelHeightDifferenceCm>=0.0 &&
              std::abs(footprint.PitchRadians)<0.15 &&
              std::abs(footprint.RollRadians)<0.12,
              "finite four-wheel contact height forecast stays source-only");
        for(const auto& corner:footprint.Corners)
            Check(corner.Valid && corner.OriginalRoadXYQualified,
                  "every wheel center remains on authored original road");
        Check(!SampleWorldScaleBankedFootprint(
            circuit,profile,0.125*lap,400.0).Valid,
            "wheel footprint outside physical road is rejected");
        Check(!SampleWorldScaleBankedFootprint(
            circuit,profile,0.125*lap,0.0,-1.0,160.0).Valid,
            "negative wheelbase is rejected");
        Check(!SampleWorldScaleBankedFootprint(
            circuit,profile,0.125*lap,0.0,270.0,nan).Valid,
            "nonfinite wheel track is rejected");

        const auto audit=AuditWorldScaleBankedRoad(circuit,profile);
        Check(audit.Valid &&
              audit.QualifiedCenterStations==
                  WorldScaleVariantVisualMeshSections &&
              audit.QualifiedEdgeStations==
                  2u*WorldScaleVariantVisualMeshSections &&
              audit.MaximumSeamErrorCm<1e-8 &&
              audit.MaximumAdjacentTopStepCm<50.0 &&
              audit.OriginalCollisionUnchanged &&
              !audit.UnrealPlaytestQualified,
              "full 1024-station six-circuit source-only QA audit");
        if(circuit.Id==WorldScaleVariantId::TechnicalCrest)
            Check(audit.MaximumTopCm-audit.MinimumTopCm>650.0,
                  "technical crest has meaningful original terrain relief");
        if(circuit.Id==WorldScaleVariantId::OriginalLoop)
        {
            // The original loop has ZERO longitudinal crest, but its
            // proposed visual road still has a small, nonzero cross-bank.
            // Audit extrema include left/right edges, not just centerline.
            const auto centerQuarter=SampleWorldScaleBankedRoad(
                circuit,profile,0.125*lap,0.0);
            Check(centerQuarter.Valid &&
                  Near(centerQuarter.WorldZcm,-1.0) &&
                  audit.MinimumTopCm<-1.0 &&
                  audit.MaximumTopCm>-1.0 &&
                  audit.MaximumTopCm-audit.MinimumTopCm<30.0,
                  "flat centerline retains its bounded edge banking");
        }
        const auto mesh=BuildWorldScaleBankedRoadVisualCandidate(
            circuit,profile);
        Check(mesh.Vertices.size()==
                  4u*WorldScaleVariantVisualMeshSections &&
              mesh.Triangles.size()==
                  8u*WorldScaleVariantVisualMeshSections,
              "new vertical geometry reuses qualified closed-loop faces");
        Check(Near(mesh.Vertices[0].Z-mesh.Vertices[2].Z,
                   RoundedSmoothTopCm-RoundedSmoothBottomCm,1e-8),
              "banked road retains the original continuous road thickness");
        const auto startLeft=SampleWorldScaleBankedRoad(
            circuit,profile,0.0,circuit.VisualHalfWidthCm);
        const auto startRight=SampleWorldScaleBankedRoad(
            circuit,profile,0.0,-circuit.VisualHalfWidthCm);
        Check(Near(mesh.Vertices[0].Z,startLeft.WorldZcm,1e-8) &&
              Near(mesh.Vertices[1].Z,startRight.WorldZcm,1e-8),
              "3D candidate top vertices follow their proposed road");
        ++audited;
    }
    Check(audited==6,"all six independent original tracks audited");

    const auto circuit=WorldScaleOriginalVariantRecipes[3];
    WorldScaleBankedRoadProfile p;
    Check(ChooseWorldScaleBankedRoadProfile(circuit.Id,p),
          "choose harbor profile");
    p.Circuit=WorldScaleVariantId::CanyonRun;
    Check(!ValidWorldScaleBankedRoadProfile(circuit,p),
          "wrong track provenance is rejected");
    p.Circuit=circuit.Id;
    p.BankAmplitudeRadians=0.30;
    Check(!ValidWorldScaleBankedRoadProfile(circuit,p),
          "unphysical overbank rejected");
    p.BankAmplitudeRadians=0.033;
    p.PrimaryCrestCm=nan;
    Check(!ValidWorldScaleBankedRoadProfile(circuit,p),
          "nonfinite height rejected");
    p.PrimaryCrestCm=48.0;
    p.SecondaryCrestCm=inf;
    Check(!ValidWorldScaleBankedRoadProfile(circuit,p),
          "infinite waveform rejected");
    p.SecondaryCrestCm=15.0;
    p.PreserveExistingCollision=false;
    Check(!BuildWorldScaleBankedRoadVisualCandidate(circuit,p)
               .Vertices.size(),
          "no proposal can authorize replacement of existing collision");
    std::cout<<"WorldScaleBankedRoadPlan: six circuits audited\n";
    return EXIT_SUCCESS;
}
