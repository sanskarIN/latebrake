#include "Vehicles/ChaseCameraTransition.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool pass, const char* reason)
{
    if (!pass)
    {
        std::cerr << "ChaseCameraTransition: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Close(double a, double b, double eps = 1.e-8)
{
    return std::abs(a - b) <= eps;
}
void SamePose(latebrake::ChaseCameraPose a,
              latebrake::ChaseCameraPose b, const char* reason)
{
    Require(Close(a.ArmLengthCm, b.ArmLengthCm) &&
            Close(a.PitchDegrees, b.PitchDegrees) &&
            Close(a.ForwardOffsetCm, b.ForwardOffsetCm) &&
            Close(a.HeightOffsetCm, b.HeightOffsetCm), reason);
}
}

int main()
{
    using namespace latebrake;
    const CameraPreset chase = GetCameraPreset(CameraView::Chase);
    const CameraPreset close = GetCameraPreset(CameraView::CloseChase);
    const CameraPreset high = GetCameraPreset(CameraView::High);
    const ChaseCameraPose closePose = ChasePoseForPreset(close);
    const ChaseCameraPose chasePose = ChasePoseForPreset(chase);
    ChaseCameraTransition rig;

    SamePose(rig.Pose(), chasePose, "initial pose is chase");
    Require(!rig.IsActive(), "no default animation");
    Require(rig.Begin(chasePose, close), "switching chase to close animates");
    SamePose(rig.Pose(), chasePose, "starting animation never teleports");
    const ChaseCameraPose quarter = rig.Advance(0.08);
    Require(rig.IsActive() && quarter.ArmLengthCm < chasePose.ArmLengthCm &&
            quarter.ArmLengthCm > closePose.ArmLengthCm &&
            quarter.PitchDegrees > chasePose.PitchDegrees,
            "quarterway framing and pitch remain within endpoints");
    rig.Advance(0.08);
    rig.Advance(0.08);
    SamePose(rig.Advance(0.08), closePose, "destination is reached exactly");
    Require(!rig.IsActive(), "animation stops after duration");
    SamePose(rig.Advance(0.1), closePose, "finished pose stays stable");

    // Frame cadence changes must not affect the presented pose.
    rig.Snap(chase);
    rig.Begin(chasePose, high);
    const ChaseCameraPose at120Hz = [&rig]()
    {
        for (int n = 0; n < 24; ++n)
        {
            rig.Advance(1.0 / 120.0);
        }
        return rig.Pose();
    }();
    rig.Snap(chase);
    rig.Begin(chasePose, high);
    for (int n = 0; n < 12; ++n)
    {
        rig.Advance(1.0 / 60.0);
    }
    SamePose(rig.Pose(), at120Hz, "60/120Hz have matching eased pose");

    // A rapid double switch preserves the *displayed* pose continuity.
    const ChaseCameraPose mid = rig.Pose();
    Require(rig.Begin(mid, close), "re-entrant switch is accepted");
    SamePose(rig.Pose(), mid, "rapid switch never jumps backward");
    const ChaseCameraPose next = rig.Advance(1.0 / 120.0);
    Require(next.ArmLengthCm < mid.ArmLengthCm,
            "new destination moves from displayed position");
    rig.Cancel();
    SamePose(rig.Pose(), closePose, "pause and look-back cancel to target");
    Require(!rig.IsActive(), "cancel disarms the animation");

    for (const double bad : {
             0.0, -0.1, 0.251,
             std::numeric_limits<double>::infinity(),
             std::numeric_limits<double>::quiet_NaN()})
    {
        rig.Snap(chase);
        rig.Begin(chasePose, high);
        SamePose(rig.Advance(bad), ChasePoseForPreset(high),
                 "bad/stalled frame snaps without invalid transforms");
        Require(!rig.IsActive(), "bad frame leaves no pending transition");
    }
    rig.Snap(chase);
    ChaseCameraPose corrupt = chasePose;
    corrupt.PitchDegrees = std::numeric_limits<double>::quiet_NaN();
    Require(!rig.Begin(corrupt, close), "invalid source rejected");
    SamePose(rig.Pose(), chasePose, "invalid source falls back safely");
    CameraPreset badPreset = high;
    badPreset.ArmLengthCm = std::numeric_limits<double>::infinity();
    Require(!rig.Begin(chasePose, badPreset), "invalid target rejected");
    SamePose(rig.Pose(), chasePose, "invalid target cannot reach Unreal");

    for (int i = 0; i < 10000; ++i)
    {
        const CameraPreset target = i % 2 ? close : high;
        const ChaseCameraPose displayed = rig.Pose();
        Require(rig.Begin(displayed, target), "stress switch admitted");
        const ChaseCameraPose p = rig.Advance(1.0 / 120.0);
        Require(IsSafeChaseCameraPose(p), "10k rapid view changes stay finite");
    }
    rig.Snap(chase);
    SamePose(rig.Pose(), chasePose, "race restart resets rig to chase");

    std::cout << "Chase camera transition contracts passed.\n";
    return EXIT_SUCCESS;
}
