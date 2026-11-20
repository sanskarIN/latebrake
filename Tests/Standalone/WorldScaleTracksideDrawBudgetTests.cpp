#include "Track/WorldScaleTracksideDrawBudget.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool condition,const char* reason)
{
    if(!condition)
    {
        std::cerr<<"WorldScaleTracksideDrawBudget FAIL: "
                 <<reason<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double x,double y,double tolerance=1e-7)
{
    return std::abs(x-y)<=tolerance;
}
latebrake::TracksideCameraWitness CameraFor(
    const latebrake::TracksideSceneObject& anchor)
{
    latebrake::TracksideCameraWitness camera;
    camera.SourceSession=41u;
    camera.NativeCameraToken=12u;
    camera.SourceFrame=9u;
    camera.ClockSeconds=6.0;
    camera.Xcm=anchor.Xcm-1000.0;
    camera.Ycm=anchor.Ycm;
    camera.Zcm=anchor.Zcm+anchor.HeightCm*0.5;
    camera.YawRadians=0.0;
    camera.PitchRadians=0.0;
    camera.HorizontalFovDegrees=80.0;
    camera.VerticalFovDegrees=55.0;
    camera.ActualCameraTransformObserved=true;
    return camera;
}
bool ContainsId(const latebrake::TracksideDrawFrame& draw,
    std::uint64_t id)
{
    for(std::size_t i=0;i<draw.VisibleObjectCount;++i)
        if(draw.Objects[i].DeterministicId==id)return true;
    return false;
}
}

int main()
{
    using namespace latebrake;
    auto settings=TracksideDrawSettings{};
    Check(ValidTracksideDrawSettings(settings),
          "default original rendering budget is finite");
    settings.MaximumVisibleObjects=0u;
    Check(!ValidTracksideDrawSettings(settings),
          "zero draw budget rejected");
    settings={};
    settings.MaximumEstimatedTriangles=0u;
    Check(!ValidTracksideDrawSettings(settings),
          "empty triangle budget rejected");
    settings={};
    settings.MaximumFullObjects=129u;
    Check(!ValidTracksideDrawSettings(settings),
          "detail budget cannot exceed object cap");
    settings={};
    settings.NeverSpawnUnrealMesh=false;
    Check(!ValidTracksideDrawSettings(settings),
          "no portable decision can spawn native Unreal mesh");
    settings={};
    settings.AbsoluteMaximumDistanceCm=
        std::numeric_limits<double>::infinity();
    Check(!ValidTracksideDrawSettings(settings),
          "nonfinite draw distance rejected");
    settings={};
    Check(TracksideEstimatedTriangles(
              TracksideSceneKind::Grandstand,
              TracksideDetailLevel::Full)==36u &&
          TracksideEstimatedTriangles(
              TracksideSceneKind::Floodlight,
              TracksideDetailLevel::Full)==24u &&
          TracksideEstimatedTriangles(
              TracksideSceneKind::Floodlight,
              TracksideDetailLevel::Reduced)==12u &&
          TracksideEstimatedTriangles(
              TracksideSceneKind::Floodlight,
              TracksideDetailLevel::Landmark)==2u &&
          TracksideEstimatedTriangles(
              TracksideSceneKind::Floodlight,
              TracksideDetailLevel::Omitted)==0u,
          "LOD triangle costs correspond to real low-poly proxy budgets");
    Check(TracksideDrawPriority(TracksideSceneKind::RescueAccess)>
          TracksideDrawPriority(TracksideSceneKind::WoodlandCanopy),
          "marshal/rescue direction cues take priority over foliage");
    Check(ChooseTracksideDetail(7999.0,{})==
              TracksideDetailLevel::Full &&
          ChooseTracksideDetail(12000.0,{})==
              TracksideDetailLevel::Reduced &&
          ChooseTracksideDetail(50000.0,{})==
              TracksideDetailLevel::Landmark &&
          ChooseTracksideDetail(80000.0,{})==
              TracksideDetailLevel::Omitted,
          "near/medium/far visibility transitions are explicit");

    unsigned verified=0u;
    for(unsigned i=0u;i<WorldScaleVariantCount;++i)
    {
        const auto track=WorldScaleOriginalVariantRecipes[i];
        WorldScaleBankedRoadProfile road;
        Check(ChooseWorldScaleBankedRoadProfile(track.Id,road),
              "all original six tracks provide banked-source road");
        const auto scene=BuildTracksideScenePlan(track,road);
        Check(scene.Valid &&
              scene.QualifiedObjects==TracksideSceneObjectCount,
              "original authored 128-object scene exists for each track");
        const auto anchor=scene.Objects[0];
        const auto camera=CameraFor(anchor);
        Check(ValidTracksideCameraWitness(camera,{}),
              "genuine-source camera contract has finite bounded pose");
        auto plan=BuildTracksideDrawFrame(scene,camera);
        Check(plan.Valid &&
              plan.Circuit==track.Id &&
              plan.SourceSession==camera.SourceSession &&
              plan.SourceFrame==camera.SourceFrame &&
              plan.TotalCandidateObjects==TracksideSceneObjectCount &&
              plan.VisibleObjectCount>0u &&
              plan.VisibleObjectCount<=40u &&
              plan.EstimatedTriangles<=440u &&
              plan.NoEngineRendererMutated &&
              plan.RealOcclusionUnverified &&
              plan.OriginalRaceUntouched,
              "bounded scene performs camera-facing frustum/LOD culling");
        Check(ContainsId(plan,anchor.DeterministicId),
              "nearest observed prop remains visible from front");
        Check(plan.TotalCandidateObjects==
                  plan.RejectedByFrustum+plan.FacingViewObjects &&
              plan.FacingViewObjects>=plan.VisibleObjectCount &&
              plan.VisibleObjectCount==
                  plan.FullDetailCount+
                  plan.ReducedDetailCount+
                  plan.LandmarkCount,
              "every source object accounted for exactly once");
        std::size_t summedTriangles=0u;
        double lastDistance=-1.0;
        for(std::size_t j=0;j<plan.VisibleObjectCount;++j)
        {
            const auto& object=plan.Objects[j];
            Check(object.DeterministicId!=0u &&
                  object.SourceFootprintVerified &&
                  object.NoRealOcclusionTest &&
                  !object.RenderSubmissionOccurred &&
                  object.ClockSeconds==camera.ClockSeconds &&
                  object.Detail!=TracksideDetailLevel::Omitted &&
                  object.MaterialIndex>=1u &&
                  object.MaterialIndex<=7u,
                  "all emitted visibility entries retain no-actor authority");
            Check(object.DistanceToBoundsCm>=lastDistance &&
                  object.DistanceToBoundsCm>=0.0 &&
                  std::isfinite(object.AngularOffsetRadians),
                  "visibility list sorts by genuine bounded camera distance");
            Check(object.EstimatedTriangles==
                  TracksideEstimatedTriangles(object.Kind,object.Detail),
                  "draw cost follows actual source-proxy LOD classification");
            summedTriangles+=object.EstimatedTriangles;
            lastDistance=object.DistanceToBoundsCm;
            for(std::size_t earlier=0;earlier<j;++earlier)
                Check(object.DeterministicId!=
                      plan.Objects[earlier].DeterministicId,
                      "same source prop never rendered or counted twice");
        }
        Check(summedTriangles==plan.EstimatedTriangles &&
              summedTriangles<=settings.MaximumEstimatedTriangles,
              "strict draw budget equals sum of accepted triangles");
        const auto repeat=BuildTracksideDrawFrame(scene,camera);
        Check(repeat.Valid &&
              repeat.VisibleObjectCount==plan.VisibleObjectCount &&
              repeat.EstimatedTriangles==plan.EstimatedTriangles,
              "identical authored scene and camera produce reproducible budget");
        for(std::size_t j=0;j<plan.VisibleObjectCount;++j)
            Check(plan.Objects[j].DeterministicId==
                  repeat.Objects[j].DeterministicId &&
                  plan.Objects[j].Detail==repeat.Objects[j].Detail &&
                  Near(plan.Objects[j].DistanceToBoundsCm,
                       repeat.Objects[j].DistanceToBoundsCm),
                  "selection order remains deterministic without timers");

        auto narrow=TracksideDrawSettings{};
        narrow.MaximumVisibleObjects=1u;
        narrow.MaximumFullObjects=0u;
        narrow.MaximumReducedObjects=0u;
        narrow.MaximumEstimatedTriangles=12u;
        const auto minimal=BuildTracksideDrawFrame(
            scene,camera,narrow);
        Check(minimal.Valid &&
              minimal.VisibleObjectCount==1u &&
              minimal.LandmarkCount==1u &&
              minimal.EstimatedTriangles==2u &&
              minimal.RejectedByBudget>0u,
              "tight budgets downgrade to billboard before dropping geometry");
        auto cameraBack=camera;
        // Some source objects are wide grandstands: their bounding sphere
        // may still enter a reversed frustum at only 1000 cm. Move far
        // enough behind the observed prop to prove complete rejection.
        cameraBack.Xcm=anchor.Xcm-5000.0;
        cameraBack.YawRadians=TracksideDrawPi;
        const auto reverse=BuildTracksideDrawFrame(
            scene,cameraBack);
        Check(reverse.Valid &&
              !ContainsId(reverse,anchor.DeterministicId),
              "face-away frustum rejects nearby prop behind the camera");

        auto untrusted=camera;
        untrusted.ActualCameraTransformObserved=false;
        Check(!BuildTracksideDrawFrame(
                scene,untrusted).Valid,
              "invented camera pose cannot claim a native visual source");
        untrusted=camera;
        untrusted.NativeCameraToken=0u;
        Check(!BuildTracksideDrawFrame(
                scene,untrusted).Valid,
              "unidentified camera actor cannot authorize visibility");
        untrusted=camera;
        untrusted.YawRadians=
            std::numeric_limits<double>::quiet_NaN();
        Check(!BuildTracksideDrawFrame(
                scene,untrusted).Valid,
              "nonfinite camera orientation rejected");
        untrusted=camera;
        untrusted.ClockSeconds=-1.0;
        Check(!BuildTracksideDrawFrame(
                scene,untrusted).Valid,
              "nonmonotonic source timestamp rejected");

        auto corrupt=scene;
        corrupt.Objects[1].DeterministicId=
            corrupt.Objects[0].DeterministicId;
        Check(!BuildTracksideDrawFrame(
                corrupt,camera).Valid,
              "duplicate source object ID fails the entire draw frame");
        corrupt=scene;
        corrupt.Objects[2].EveryBaseCornerOutsideRoad=false;
        Check(!BuildTracksideDrawFrame(
                corrupt,camera).Valid,
              "unproven road clearance cannot be hidden by draw culling");
        corrupt=scene;
        corrupt.OriginalCollisionUnchanged=false;
        Check(!BuildTracksideDrawFrame(
                corrupt,camera).Valid,
              "a visual proposal cannot change genuine road collision");
        ++verified;
    }
    Check(verified==WorldScaleVariantCount,
          "all six original trackside draw budgets independently audited");
    std::cout<<"WorldScaleTracksideDrawBudget: six circuits passed\n";
    return EXIT_SUCCESS;
}
