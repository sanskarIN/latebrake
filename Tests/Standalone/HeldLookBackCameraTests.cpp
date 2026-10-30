#include "Vehicles/HeldLookBackCamera.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool value, const char* message)
{
    if (!value)
    {
        std::cerr << "Held look-back camera FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    latebrake::HeldLookBackCamera camera;
    Require(!camera.IsHeld() && camera.SpringArmYawDegrees() == 0.0,
            "default chase direction stays forward");
    Require(!camera.Press(false) && !camera.IsHeld() &&
            camera.SpringArmYawDegrees() == 0.0,
            "menu, pause or invalid race press cannot latch lookback");
    Require(camera.Press(true) && camera.IsHeld() &&
            camera.SpringArmYawDegrees() == 180.0,
            "eligible held press requests exactly one rear-facing view");
    Require(camera.Press(true) && camera.SpringArmYawDegrees() == 180.0,
            "duplicate press never accumulates camera rotation");
    camera.Release();
    Require(!camera.IsHeld() && camera.SpringArmYawDegrees() == 0.0,
            "physical release restores forward view");
    camera.Release();
    Require(camera.SpringArmYawDegrees() == 0.0,
            "repeated release is harmless");
    Require(camera.Press(true), "fresh press is permitted after release");
    Require(!camera.Press(false) && camera.SpringArmYawDegrees() == 0.0,
            "paused/ineligible transition cannot preserve a stale hold");
    Require(camera.Press(true), "fresh authorized press is eligible again");
    camera.Reset();
    Require(!camera.IsHeld() && camera.SpringArmYawDegrees() == 0.0,
            "recovery/restart/focus reset always restores normal view");
    for (int index = 0; index < 20000; ++index)
    {
        Require(camera.Press(true) && camera.IsHeld(),
                "repeat held press remains deterministic");
        camera.Release();
        Require(!camera.IsHeld(), "every release clears the view");
    }
    std::cout << "Held look-back camera tests passed (no Unreal).\n";
    return EXIT_SUCCESS;
}
