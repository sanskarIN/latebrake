#include "UI/TouchControlLayout.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

static void Check(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "Touch layout FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    using namespace latebrake;
    const auto layout = CalculateTouchControlLayout(1280.0, 720.0);
    Check(layout.Valid, "desktop-sized viewport produces touch zones");
    Check(layout.Steering.X > 0.0 && layout.Steering.Y > 0.0, "safe margins");
    Check(layout.Steering.X + layout.Steering.Width < layout.Throttle.X,
          "steering and pedals do not overlap horizontally");
    Check(layout.Throttle.Y + layout.Throttle.Height < layout.Brake.Y,
          "separate gas/brake pads have neutral gap");
    Check(layout.Brake.Y + layout.Brake.Height < 720.0, "brake stays inside viewport");

    const double steerMiddle = layout.Steering.X + layout.Steering.Width * 0.5;
    Check(HitTestTouchControl(layout, steerMiddle, layout.Steering.Y + 10.0) ==
          TouchControl::Steering, "steering press is detected");
    Check(HitTestTouchControl(layout, layout.Throttle.X + 20.0,
          layout.Throttle.Y + 20.0) == TouchControl::Throttle, "throttle press");
    Check(HitTestTouchControl(layout, layout.Brake.X + 20.0,
          layout.Brake.Y + 20.0) == TouchControl::Brake, "brake press");
    Check(HitTestTouchControl(layout, layout.Throttle.X + 20.0,
          (layout.Throttle.Y + layout.Throttle.Height + layout.Brake.Y) * 0.5) ==
          TouchControl::None, "gap is neutral");
    Check(HitTestTouchControl(layout, 15.0, 15.0) == TouchControl::None,
          "race telemetry region cannot accelerate");

    Check(TouchSteeringValue(layout, steerMiddle) == 0.0, "center means straight");
    Check(TouchSteeringValue(layout, layout.Steering.X) == -1.0, "left edge turns left");
    Check(TouchSteeringValue(layout,
          layout.Steering.X + layout.Steering.Width) == 1.0, "right edge turns right");
    Check(TouchSteeringValue(layout, -1000.0) == -1.0, "drag outside safely clamps");
    Check(TouchSteeringValue(layout, 100000.0) == 1.0, "right drag safely clamps");

    const auto portrait = CalculateTouchControlLayout(360.0, 640.0);
    Check(portrait.Valid && portrait.Steering.Y > 320.0, "portrait zones avoid top");
    Check(HitTestTouchControl(portrait, portrait.Brake.X + 1.0,
          portrait.Brake.Y + 1.0) == TouchControl::Brake, "portrait brake");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    Check(!CalculateTouchControlLayout(0.0, 720.0).Valid, "zero viewport fails closed");
    Check(!CalculateTouchControlLayout(239.0, 600.0).Valid, "undersize viewport fails closed");
    Check(!CalculateTouchControlLayout(nan, 720.0).Valid, "NaN viewport fails closed");
    Check(!CalculateTouchControlLayout(inf, 720.0).Valid, "infinite viewport fails closed");
    Check(HitTestTouchControl(layout, nan, 700.0) == TouchControl::None,
          "NaN touch cannot press buttons");
    Check(HitTestTouchControl(layout, inf, 700.0) == TouchControl::None,
          "infinite touch cannot press buttons");
    Check(TouchSteeringValue(layout, nan) == 0.0, "NaN steering input neutral");
    Check(TouchSteeringValue({}, 300.0) == 0.0, "invalid layout neutral");
    Check(HitTestTouchControl({}, 300.0, 400.0) == TouchControl::None,
          "invalid layout cannot press");

    std::cout << "Touch control layout tests passed.\n";
    return EXIT_SUCCESS;
}
