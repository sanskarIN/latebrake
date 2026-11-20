#include "Racing/PhysicalReplayCameraDirector.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool good,const char* what)
{
    if(!good)
    {
        std::cerr<<"PhysicalReplayCameraDirector FAIL: "<<what<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b,double epsilon=1.0e-6)
{
    return std::abs(a-b)<=epsilon;
}
}

int main()
{
    using namespace latebrake;
    PhysicalReplayPlaybackFrame frame;
    frame.Valid=true;
    frame.Session=200u;
    frame.SessionRevision=5u;
    frame.ActiveClockSeconds=3.0;
    frame.SourceOnly=true;
    frame.NoCollisionAuthority=true;
    frame.NoOfficialTimingAuthority=true;
    for(std::size_t i=0;i<PhysicalReplayCarCount;++i)
    {
        auto& c=frame.Cars[i];
        c.Actor=300u+i;
        c.Xcm=1000.0+1000.0*i;
        c.Ycm=500.0+300.0*i;
        c.Zcm=120.0;
        c.HeadingRadians=0.0;
        c.ObservedSpeedCmps=2500.0;
        c.ThrottleNormalized=0.5;
        c.ProvenanceMatched=true;
    }
    PhysicalReplayCameraInput input;
    input.Playback=&frame;
    input.FocusActor=300u;
    input.ActiveClockSeconds=3.0;
    input.FrameDeltaSeconds=0.016;
    input.EngineReplayAuthorized=true;
    input.UserCameraEnabled=true;
    auto pose=PlanPhysicalReplayCamera(input);
    Check(pose.Valid && pose.HasValidReplayActor &&
          pose.FocusActor==300u &&
          pose.Shot==PhysicalReplayShot::Chase &&
          pose.CameraXcm<frame.Cars[0].Xcm &&
          pose.CameraZcm>frame.Cars[0].Zcm &&
          pose.LookXcm>frame.Cars[0].Xcm &&
          !pose.OcclusionVerified &&
          !pose.NativeCameraInstalled &&
          !pose.SceneMutationIssued,
          "chase camera uses observed physical identity, never fake visibility");
    Check(pose.HorizontalFovDegrees>=50.0 &&
          pose.HorizontalFovDegrees<=95.0 &&
          pose.CameraTargetDistanceCm>400.0 &&
          pose.CameraTargetDistanceCm<10000.0 &&
          std::isfinite(pose.YawRadians) &&
          std::isfinite(pose.PitchRadians),
          "camera pitch, yaw, FOV and target distance are bounded");
    const double chaseX=pose.CameraXcm;
    input.Shot=PhysicalReplayShot::Bonnet;
    auto bonnet=PlanPhysicalReplayCamera(input);
    Check(bonnet.Valid &&
          bonnet.CameraXcm>frame.Cars[0].Xcm &&
          bonnet.CameraXcm>chaseX &&
          bonnet.CameraZcm>frame.Cars[0].Zcm,
          "bonnet camera is placed ahead and above source car");
    input.Shot=PhysicalReplayShot::HighAerial;
    auto aerial=PlanPhysicalReplayCamera(input);
    Check(aerial.Valid &&
          aerial.CameraZcm>pose.CameraZcm &&
          aerial.CameraTargetDistanceCm>pose.CameraTargetDistanceCm,
          "aerial view gains true height without mutating the track");
    input.Shot=PhysicalReplayShot::RightSide;
    auto right=PlanPhysicalReplayCamera(input);
    input.Shot=PhysicalReplayShot::LeftSide;
    auto left=PlanPhysicalReplayCamera(input);
    Check(right.Valid && left.Valid &&
          right.CameraYcm<frame.Cars[0].Ycm &&
          left.CameraYcm>frame.Cars[0].Ycm &&
          Near(left.CameraZcm,right.CameraZcm),
          "opposed sides respect physical chassis forward orientation");
    input.Shot=PhysicalReplayShot::RearFacing;
    auto rear=PlanPhysicalReplayCamera(input);
    Check(rear.Valid &&
          rear.CameraXcm>frame.Cars[0].Xcm &&
          rear.LookXcm<frame.Cars[0].Xcm,
          "rearward view points back along actual source heading");
    input.Shot=PhysicalReplayShot::Orbit;
    auto orbit=PlanPhysicalReplayCamera(input);
    Check(orbit.Valid &&
          orbit.CameraTargetDistanceCm>450.0 &&
          orbit.CameraTargetDistanceCm<3000.0,
          "orbit returns bounded cinematic camera instead of collision");
    for(PhysicalReplayShot shot:{
        PhysicalReplayShot::Chase,
        PhysicalReplayShot::Bonnet,
        PhysicalReplayShot::RightSide,
        PhysicalReplayShot::LeftSide,
        PhysicalReplayShot::HighAerial,
        PhysicalReplayShot::RearFacing,
        PhysicalReplayShot::Orbit})
    {
        input.Shot=shot;
        for(std::size_t i=0;i<PhysicalReplayCarCount;++i)
        {
            input.FocusActor=300u+i;
            const auto direct=PlanPhysicalReplayCamera(input);
            Check(direct.Valid &&
                  direct.FocusActor==input.FocusActor &&
                  direct.HasValidReplayActor &&
                  direct.NativeCameraInstalled==false,
                  "all seven visual shots work for each six-car slot");
        }
    }
    input.Shot=PhysicalReplayShot::Chase;
    input.FocusActor=300u;
    input.EngineReplayAuthorized=false;
    Check(!PlanPhysicalReplayCamera(input).Valid,
          "no camera without explicit replay authorization");
    input.EngineReplayAuthorized=true;
    input.UserCameraEnabled=false;
    Check(!PlanPhysicalReplayCamera(input).Valid,
          "hidden user camera cannot be turned on by a replay");
    input.UserCameraEnabled=true;
    input.FocusActor=9999u;
    Check(!PlanPhysicalReplayCamera(input).Valid,
          "unknown physical actor cannot obtain arbitrary replay focus");
    input.FocusActor=300u;
    frame.Cars[0].ProvenanceMatched=false;
    Check(!PlanPhysicalReplayCamera(input).Valid,
          "unverified actor provenance cannot drive a camera");
    frame.Cars[0].ProvenanceMatched=true;
    frame.NoCollisionAuthority=false;
    Check(!PlanPhysicalReplayCamera(input).Valid,
          "actor playback must explicitly preserve no-physics authority");
    frame.NoCollisionAuthority=true;
    frame.Cars[0].ObservedSpeedCmps=
        std::numeric_limits<double>::quiet_NaN();
    Check(!PlanPhysicalReplayCamera(input).Valid,
          "nonfinite chassis speed rejected");
    frame.Cars[0].ObservedSpeedCmps=2500.0;
    input.ActiveClockSeconds=4.0;
    Check(!PlanPhysicalReplayCamera(input).Valid,
          "stale or mismatched official active replay clock rejected");
    input.ActiveClockSeconds=3.0;
    input.FrameDeltaSeconds=1.0;
    Check(!PlanPhysicalReplayCamera(input).Valid,
          "frame hitch must not consume unqualified camera integration");
    input.FrameDeltaSeconds=0.016;
    input.Shot=static_cast<PhysicalReplayShot>(254u);
    Check(!PlanPhysicalReplayCamera(input).Valid,
          "unsupported shot cannot access uninitialized framing");
    input.Shot=PhysicalReplayShot::Chase;
    auto settings=PhysicalReplayCameraSettings{};
    settings.PositionSmoothingRate=std::numeric_limits<double>::infinity();
    Check(!PlanPhysicalReplayCamera(input,settings).Valid,
          "invalid smoothing parameters fail closed");
    settings=PhysicalReplayCameraSettings{};
    settings.CameraIsVisualOnly=false;
    Check(!PlanPhysicalReplayCamera(input,settings).Valid,
          "source-only camera cannot claim gameplay effect");

    PhysicalReplayCameraRig rig;
    auto first=rig.Update(input);
    Check(first.Valid && rig.HasPrevious(),
          "new cinematic view has a baseline without prior camera");
    auto identical=rig.Update(input);
    Check(identical.Valid &&
          Near(identical.CameraXcm,first.CameraXcm) &&
          Near(identical.YawRadians,first.YawRadians),
          "fixed source pose does not drift during repeated frames");
    frame.Cars[0].Xcm+=100.0;
    auto smooth=rig.Update(input);
    const auto instant=PlanPhysicalReplayCamera(input);
    Check(smooth.Valid && instant.Valid &&
          smooth.CameraXcm>first.CameraXcm &&
          smooth.CameraXcm<instant.CameraXcm,
          "same-shot movement is smoothed instead of camera teleport");
    input.Shot=PhysicalReplayShot::Bonnet;
    auto cut=rig.Update(input);
    const auto cutTarget=PlanPhysicalReplayCamera(input);
    Check(cut.Valid && cutTarget.Valid &&
          Near(cut.CameraXcm,cutTarget.CameraXcm) &&
          cut.Shot==PhysicalReplayShot::Bonnet,
          "shot transition cuts directly to new valid physical source");
    input.FocusActor=301u;
    auto newActor=rig.Update(input);
    Check(newActor.Valid &&
          newActor.FocusActor==301u &&
          Near(newActor.CameraXcm,
               PlanPhysicalReplayCamera(input).CameraXcm),
          "switching real actor avoids interpolated imaginary car collision");
    input.UserCameraEnabled=false;
    Check(!rig.Update(input).Valid && !rig.HasPrevious(),
          "camera interruption clears stale pose and visual latch");
    input.UserCameraEnabled=true;
    Check(rig.Update(input).Valid && rig.HasPrevious(),
          "new authorized camera session starts from a clean baseline");
    rig.Reset();
    Check(!rig.HasPrevious(),"explicit reset clears camera state");
    std::cout<<"PhysicalReplayCameraDirector: seven views / six cars passed\n";
    return EXIT_SUCCESS;
}
