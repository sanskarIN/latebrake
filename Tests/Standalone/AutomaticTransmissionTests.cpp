#include "Vehicles/AutomaticTransmission.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool pass, const char* message)
{
    if (!pass)
    {
        std::cerr << "AutomaticTransmission FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double x, double y) { return std::abs(x - y) < 1.0e-9; }
}

int main()
{
    using namespace latebrake;
    static_assert(AutomaticTransmission::ForwardGearCount == 4);
    static_assert(AutomaticTransmission::UpshiftRatios[0] >
                  AutomaticTransmission::DownshiftRatios[0]);
    static_assert(AutomaticTransmission::UpshiftRatios[1] >
                  AutomaticTransmission::DownshiftRatios[1]);
    AutomaticTransmission box;
    Require(!box.Readout().Valid && box.Readout().Gear == 1,
            "no fictitious measured engine RPM before a live frame");
    auto drive = [&](double speed, double throttle = 1.0,
                     double dt = 0.05, double cap = 6500.0,
                     bool arcade = true, bool ready = true)
    {
        return box.Advance({dt, speed, throttle, cap, arcade, ready});
    };
    const auto launch = drive(0.0);
    Require(launch.Valid && launch.Gear == 1 && launch.EstimatedRpm == 900 &&
            Near(launch.PropulsionFactor, 1.0) && !launch.Shifting,
            "idle-to-launch starts with usable first gear");
    auto firstShift = drive(1600.0);
    Require(firstShift.Valid && firstShift.Gear == 2 && firstShift.Shifting &&
            firstShift.PropulsionFactor > 0.0 &&
            firstShift.PropulsionFactor < 0.25,
            "crossing first upshift threshold produces real short power cut");
    for (int i = 0; i < 5; ++i) firstShift = drive(1600.0);
    Require(firstShift.Gear == 2 && !firstShift.Shifting &&
            firstShift.PropulsionFactor <= 1.0 &&
            firstShift.PropulsionFactor > 0.85,
            "shift recovers bounded propulsion without retriggering");
    auto gear3 = drive(3300.0);
    Require(gear3.Gear == 3 && gear3.Shifting,
            "second upshift follows live chassis speed");
    for (int i = 0; i < 5; ++i) drive(3300.0);
    auto gear4 = drive(4900.0);
    Require(gear4.Gear == 4 && gear4.Shifting,
            "fourth gear available at the last speed band");
    for (int i = 0; i < 5; ++i) gear4 = drive(4900.0);
    Require(gear4.Valid && gear4.Gear == 4 && !gear4.Shifting &&
            gear4.EstimatedRpm >= 900 && gear4.EstimatedRpm <= 7200,
            "estimated RPM bounded and not presented as real engine sensor");
    auto down = drive(3900.0);
    Require(down.Gear == 3 && down.Shifting,
            "speed drop triggers measured-velocity downshift");
    for (int i = 0; i < 5; ++i) drive(3900.0);
    down = drive(2400.0);
    Require(down.Gear == 2 && down.Shifting,
            "middle downshift uses hysteresis");
    for (int i = 0; i < 5; ++i) drive(2400.0);
    down = drive(800.0);
    Require(down.Gear == 1 && down.Shifting,
            "final downshift restores launch gear");
    for (int i = 0; i < 5; ++i) drive(800.0);

    const auto engineBrake = drive(800.0, -1.0);
    Require(engineBrake.Gear == 1 && Near(engineBrake.PropulsionFactor, 1.0),
            "opposite pedal remains ordinary engine braking at forward speed");
    const auto reverse = drive(10.0, -1.0);
    Require(reverse.Gear == -1 && reverse.Shifting &&
            reverse.PropulsionFactor < 0.25,
            "reverse engages only after near-stop");
    for (int i = 0; i < 5; ++i) drive(-500.0, -1.0);
    const auto backing = drive(-500.0, -1.0);
    Require(backing.Gear == -1 && !backing.Shifting &&
            backing.PropulsionFactor > 0.0 &&
            backing.PropulsionFactor <= 1.0,
            "reversing produces bounded propulsion");
    const auto forwardBrake = drive(-500.0, 1.0);
    Require(forwardBrake.Gear == -1 && Near(forwardBrake.PropulsionFactor, 1.0),
            "forward pedal opposes reverse motion without premature shift");
    const auto first = drive(-30.0, 1.0);
    Require(first.Gear == 1 && first.Shifting,
            "forward gear engages after reverse speed nearly stops");

    box.Reset();
    Require(!box.Readout().Valid && box.Readout().Gear == 1,
            "restart clears old gear/RPM/shift history");
    const auto nearTop = drive(6400.0);
    Require(nearTop.Gear == 2 && nearTop.Shifting,
            "one frame never jumps three gears when launched very fast");
    const auto invalid = drive(std::numeric_limits<double>::quiet_NaN());
    Require(!invalid.Valid && !box.Readout().Valid &&
            box.Readout().Gear == 1,
            "nonfinite physics fails closed and clears stale readout");
    Require(!drive(1000.0, 1.0, 0.5).Valid &&
            !drive(1000.0, 1.01).Valid &&
            !drive(1000.0, 1.0, 0.05, -20.0).Valid &&
            !drive(1000.0, 1.0, 0.05, 6500.0, true, false).Valid,
            "frame hitch, bad input, cap and ineligible gameplay are denied");

    box.Reset();
    for (int i = 0; i < 3000; ++i)
    {
        const double speed = static_cast<double>((i * 61) % 11501) - 1800.0;
        const double throttle = i % 7 == 0 ? -1.0 : 1.0;
        const auto output = drive(speed, throttle, 0.01);
        Require(output.Valid &&
                (output.Gear == -1 || (output.Gear >= 1 && output.Gear <= 4)) &&
                std::isfinite(output.PropulsionFactor) &&
                output.PropulsionFactor >= 0.0 &&
                output.PropulsionFactor <= 1.0 &&
                output.EstimatedRpm >= 900 && output.EstimatedRpm <= 7200,
                "3000 direction and speed changes keep valid bounded output");
    }
    std::cout << "Four-speed automatic transmission transitions validated.\n";
    return EXIT_SUCCESS;
}
