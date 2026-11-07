#include "Vehicles/ChassisWheelAnchors.h"
#include "Vehicles/SprungChassisDynamics.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool condition, const char* why)
{
    if (!condition)
    {
        std::cerr << "Chassis wheel geometry FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b, double tolerance = 1.0e-7)
{
    return std::isfinite(a) && std::isfinite(b) &&
           std::abs(a - b) <= tolerance;
}
latebrake::SuspensionFrame FlatSuspension()
{
    latebrake::SuspensionFrame frame;
    frame.PhysicsReady = true;
    frame.DeltaSeconds = 1.0 / 60.0;
    frame.MassKg = 1100.0;
    for (auto& c : frame.Corners)
    {
        c = {true, latebrake::SuspensionTargetRideDistanceCm, 1.0, 0.0};
    }
    return frame;
}
}

int main()
{
    using namespace latebrake;
    static_assert(ChassisWheelCount == 4);
    static_assert(ChassisWheelAnchors.size() == ChassisWheelCount);
    static_assert(ChassisProbeOffset(0).ForwardCm ==
                  ChassisVisualOffset(0, 0.0).ForwardCm);
    static_assert(ChassisProbeOffset(0).RightCm ==
                  ChassisVisualOffset(0, 0.0).RightCm);
    static_assert(ChassisProbeOffset(0).SpindleHeightCm == 0.0);

    // The world-scale mesh is a 3.8 x 1.8 x 0.65 chassis, but decorated
    // offsets are converted back to original centimeters on attachment.
    // Any unaccounted parent mesh scale would misalign the road ray and wheel.
    const double scale[] = {3.8, 1.8, 0.65};
    for (unsigned i = 0; i < ChassisWheelCount; ++i)
    {
        const auto& a = ChassisWheelAnchors[i];
        const auto probe = ChassisProbeOffset(i);
        for (double travel : {SuspensionVisualMinTravelCm, 0.0,
                              SuspensionVisualMaxTravelCm})
        {
            const auto visible = ChassisVisualOffset(i, travel);
            Require(Near(probe.ForwardCm, visible.ForwardCm) &&
                    Near(probe.RightCm, visible.RightCm) &&
                    Near(probe.SpindleHeightCm, 0.0),
                    "visual wheel and true Chaos probe must share unchanged XY");
            Require(Near(visible.SpindleHeightCm,
                         a.SpindleHeightCm + travel),
                    "wheel visual suspension travel must remain Z-only");
            Require(Near((visible.ForwardCm / scale[0]) * scale[0],
                         probe.ForwardCm) &&
                    Near((visible.RightCm / scale[1]) * scale[1],
                         probe.RightCm) &&
                    Near((visible.SpindleHeightCm / scale[2]) * scale[2],
                         visible.SpindleHeightCm),
                    "scaled Unreal parent must preserve authored centimeter wheel coordinates");
        }
        Require(Near(std::abs(a.ForwardCm), ChassisHalfWheelbaseCm) &&
                Near(std::abs(a.RightCm), ChassisHalfTrackCm) &&
                Near(a.SpindleHeightCm, ChassisWheelSpindleHeightCm),
                "all four actual contact probes must match one wheel geometry");
    }

    // The 29 cm visual wheel radius and the 29 cm chassis-relative spindle
    // height put a resting tire bottom 58 cm below the chassis. The spring
    // target is 59 cm: an intentional 1 cm cosmetic clearance on flat road.
    Require(Near(SuspensionTargetRideDistanceCm -
                    (ChassisWheelVisualRadiusCm -
                     ChassisWheelSpindleHeightCm), 1.0),
            "static wheel bottom must be almost at the spring's actual road");

    // A rotated chassis must keep left/right paired with the same true
    // world-basis spring origins, rather than sampling world-axis offsets.
    for (double yawDegrees : {0.0, 37.0, 90.0, 178.0, 271.0})
    {
        const double r = yawDegrees * 3.14159265358979323846 / 180.0;
        const double fx = std::cos(r), fy = std::sin(r);
        const double rx = -fy, ry = fx;
        double sumX = 0.0, sumY = 0.0;
        for (unsigned i = 0; i < ChassisWheelCount; ++i)
        {
            const auto probe = ChassisProbeOffset(i);
            const auto visual = ChassisVisualOffset(i, 10.0);
            const double physicalX = fx * probe.ForwardCm +
                                     rx * probe.RightCm;
            const double physicalY = fy * probe.ForwardCm +
                                     ry * probe.RightCm;
            const double visualX = fx * visual.ForwardCm +
                                   rx * visual.RightCm;
            const double visualY = fy * visual.ForwardCm +
                                   ry * visual.RightCm;
            Require(Near(physicalX, visualX) &&
                    Near(physicalY, visualY),
                    "yaw-rotated spring contacts must remain under visual wheels");
            sumX += physicalX;
            sumY += physicalY;
        }
        Require(Near(sumX, 0.0) && Near(sumY, 0.0),
                "symmetric force origins must not create spurious net torque");
    }

    // Source spring force composition at the four shared XY anchors. A
    // perfectly level 1100 kg car creates no net roll/pitch moment; a
    // genuine front-left raised road contact gives an actual nonzero
    // chassis pitch/roll moment, not a decorative bump animation.
    const auto resting = CalculateSprungChassis(FlatSuspension());
    Require(resting.Valid && resting.ContactCount == 4,
            "four real springs must support the nominal flat reference");
    double roll = 0.0, pitch = 0.0;
    for (unsigned i = 0; i < ChassisWheelCount; ++i)
    {
        const auto probe = ChassisProbeOffset(i);
        const double f = resting.Corners[i].UpForce;
        roll += probe.RightCm * f;
        pitch -= probe.ForwardCm * f;
    }
    Require(Near(roll, 0.0, 0.01) && Near(pitch, 0.0, 0.01),
            "equal spring loads must balance pitch and roll around the center");

    auto uneven = FlatSuspension();
    uneven.Corners[0].DistanceCm -= 5.0;
    const auto raised = CalculateSprungChassis(uneven);
    Require(raised.Valid && raised.ContactCount == 4 &&
            raised.Corners[0].UpForce > resting.Corners[0].UpForce,
            "a raised front-left road edge increases its physical spring load");
    roll = 0.0;
    pitch = 0.0;
    for (unsigned i = 0; i < ChassisWheelCount; ++i)
    {
        const auto probe = ChassisProbeOffset(i);
        roll += probe.RightCm * raised.Corners[i].UpForce;
        pitch -= probe.ForwardCm * raised.Corners[i].UpForce;
    }
    Require(std::isfinite(roll) && std::isfinite(pitch) &&
            roll < -1.0 && pitch < -1.0,
            "the shared wheel layout must create appropriate real bump torque");

    std::cout << "Four-corner wheel geometry and spring alignment passed.\n";
    return EXIT_SUCCESS;
}
