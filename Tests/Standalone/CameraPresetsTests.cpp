#include "Vehicles/CameraPresets.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace
{
void Require(bool yes, const char* why)
{
    if (!yes)
    {
        std::cerr << "CameraPresets FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b)
{
    return std::abs(a - b) < 1e-9;
}
}

int main()
{
    using namespace latebrake;
    CameraViewCycle cycle;
    Require(cycle.Current() == CameraView::Chase,
            "default view is safe wide chase");
    for (auto next : {CameraView::CloseChase, CameraView::Hood,
                      CameraView::High, CameraView::Chase})
    {
        Require(cycle.Next() == next, "C cycles four distinct views in order");
        const CameraPreset p = GetCameraPreset(next);
        Require(IsSafeCameraPreset(p), "all shipped presets finite and bounded");
    }
    Require(cycle.Current() == CameraView::Chase,
            "four presses return to original view");
    const CameraPreset chase = GetCameraPreset(CameraView::Chase);
    const CameraPreset close = GetCameraPreset(CameraView::CloseChase);
    const CameraPreset hood = GetCameraPreset(CameraView::Hood);
    const CameraPreset high = GetCameraPreset(CameraView::High);
    Require(Near(chase.ArmLengthCm, 560.0) &&
            Near(chase.FieldOfViewDegrees, 90.0) &&
            chase.CameraLagEnabled && chase.CollisionProbeEnabled,
            "default camera reproduces original wide chase settings");
    Require(close.ArmLengthCm < chase.ArmLengthCm &&
            close.FieldOfViewDegrees < chase.FieldOfViewDegrees &&
            close.CameraLagSpeed > chase.CameraLagSpeed,
            "close chase genuinely changes framing and lag");
    Require(Near(hood.ArmLengthCm, 0.0) &&
            hood.ForwardOffsetCm > 200.0 &&
            !hood.CameraLagEnabled && !hood.CollisionProbeEnabled,
            "hood puts view ahead of chassis and bypasses boom probe");
    Require(high.ArmLengthCm > chase.ArmLengthCm &&
            high.FieldOfViewDegrees < chase.FieldOfViewDegrees &&
            high.PitchDegrees < chase.PitchDegrees,
            "high chase uses higher-angle longer-distance view");
    Require(std::string(CameraViewName(CameraView::Hood)) == "HOOD",
            "HUD view label grounded in shared camera policy");
    cycle.Next();
    cycle.Next();
    cycle.Reset();
    Require(cycle.Current() == CameraView::Chase,
            "reset clears previous camera selection");
    constexpr auto unknown = static_cast<CameraView>(255);
    Require(Near(GetCameraPreset(unknown).ArmLengthCm, 560.0) &&
            std::string(CameraViewName(unknown)) == "CHASE",
            "unsupported enum falls back to safe default");
    CameraPreset bad = chase;
    bad.ArmLengthCm = std::numeric_limits<double>::quiet_NaN();
    Require(!IsSafeCameraPreset(bad), "nonfinite boom length rejected");
    bad = chase;
    bad.PitchDegrees = 100.0;
    Require(!IsSafeCameraPreset(bad), "dangerous inverted camera rejected");
    bad = chase;
    bad.CameraLagEnabled = true;
    bad.CameraLagSpeed = 0.0;
    Require(!IsSafeCameraPreset(bad), "enabled lag must have positive speed");
    bad = chase;
    bad.FieldOfViewDegrees = 0.0;
    Require(!IsSafeCameraPreset(bad), "zero field of view rejected");
    bad = chase;
    bad.FieldOfViewDegrees = std::numeric_limits<double>::quiet_NaN();
    Require(!IsSafeCameraPreset(bad), "nonfinite lens rejected");
    bad = chase;
    bad.ForwardOffsetCm = std::numeric_limits<double>::infinity();
    Require(!IsSafeCameraPreset(bad), "infinite camera offset rejected");
    std::cout << "Camera preset and cycle policy passed.\n";
    return EXIT_SUCCESS;
}
