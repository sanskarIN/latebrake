#include "Vehicles/VehicleSignalLights.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "VehicleSignalLights FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}
int main()
{
    using latebrake::SelectVehicleSignalLights;
    using latebrake::VehicleSignalSample;
    const auto blocked = SelectVehicleSignalLights({false, true, -400.0});
    Require(blocked.RedRearIntensity == 0.0 &&
            blocked.WhiteReverseIntensity == 0.0 &&
            !blocked.BrakeIlluminated && !blocked.ReverseIlluminated,
            "menu/focus/start invalidity turns both signals off");

    const auto stopped = SelectVehicleSignalLights({true, false, 0.0});
    Require(stopped.RedRearIntensity > 0.0 &&
            stopped.WhiteReverseIntensity == 0.0 &&
            !stopped.BrakeIlluminated && !stopped.ReverseIlluminated,
            "valid idle uses a dim tail lamp, never invented brake/reverse");

    const auto forwardBrake = SelectVehicleSignalLights({true, true, 1800.0});
    Require(forwardBrake.BrakeIlluminated &&
            forwardBrake.RedRearIntensity > stopped.RedRearIntensity &&
            forwardBrake.WhiteReverseIntensity == 0.0,
            "real BRAKE input illuminates red rear signal at forward speed");

    const auto stationaryBrake = SelectVehicleSignalLights({true, true, 0.0});
    Require(stationaryBrake.BrakeIlluminated &&
            stationaryBrake.RedRearIntensity == forwardBrake.RedRearIntensity,
            "brake lamps are controlled by held braking, not only deceleration");

    const auto reverse = SelectVehicleSignalLights({true, false, -250.0});
    Require(reverse.ReverseIlluminated &&
            reverse.WhiteReverseIntensity > 0.0 &&
            !reverse.BrakeIlluminated &&
            reverse.RedRearIntensity == stopped.RedRearIntensity,
            "reverse requires actual backward chassis velocity");

    const auto reverseBrake = SelectVehicleSignalLights({true, true, -500.0});
    Require(reverseBrake.BrakeIlluminated && reverseBrake.ReverseIlluminated &&
            reverseBrake.RedRearIntensity == forwardBrake.RedRearIntensity,
            "brake and reverse lamps may legitimately illuminate together");

    Require(!SelectVehicleSignalLights({true, false, -129.0}).ReverseIlluminated &&
            SelectVehicleSignalLights({true, false, -130.0}).ReverseIlluminated,
            "reverse threshold has deterministic exact boundary");

    for (double invalid : {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        15000.1, -15000.1
    })
    {
        const auto lights = SelectVehicleSignalLights({true, true, invalid});
        Require(lights.RedRearIntensity == 0.0 &&
                lights.WhiteReverseIntensity == 0.0 &&
                !lights.BrakeIlluminated && !lights.ReverseIlluminated,
                "malformed/implausible motion must never latch lights");
    }

    for (int cm = -15000; cm <= 15000; cm += 37)
    {
        for (bool brake : {false, true})
        {
            const auto v = SelectVehicleSignalLights(
                VehicleSignalSample{true, brake, static_cast<double>(cm)});
            Require(std::isfinite(v.RedRearIntensity) &&
                    std::isfinite(v.WhiteReverseIntensity) &&
                    v.RedRearIntensity >= 0.0 &&
                    v.RedRearIntensity <= 95.0 &&
                    v.WhiteReverseIntensity >= 0.0 &&
                    v.WhiteReverseIntensity <= 70.0 &&
                    v.BrakeIlluminated == brake,
                    "sampled motion always yields bounded light intensity");
        }
    }

    const auto resumed = SelectVehicleSignalLights({true, false, 0.0});
    Require(resumed.WhiteReverseIntensity == 0.0 &&
            !resumed.BrakeIlluminated &&
            resumed.RedRearIntensity == stopped.RedRearIntensity,
            "brake/reverse never stay latched across a new neutral sample");

    std::cout << "Vehicle signal-light source policies passed.\n";
    return EXIT_SUCCESS;
}
