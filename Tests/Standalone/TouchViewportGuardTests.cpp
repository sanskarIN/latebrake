#include "UI/TouchViewportGuard.h"
#include "UI/TouchDriveState.h"
#include "UI/TouchPressTracker.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool ok, const char* what)
{
    if (!ok)
    {
        std::cerr << "Touch viewport FAIL: " << what << '\n';
        std::exit(EXIT_FAILURE);
    }
}
void CancelOnTransition(latebrake::TouchViewportGuard& view,
                        latebrake::TouchDriveState& drive,
                        int width, int height)
{
    if (latebrake::TouchViewportNeedsCancellation(view.Observe(width, height)))
    {
        drive.Clear();
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    TouchViewportGuard guard;
    TouchPressTracker presses;
    TouchDriveState drive;

    Require(!guard.HasBaseline(), "new viewport does not assume dimensions");
    Require(guard.Observe(0, 720) == TouchViewportStatus::Unavailable,
            "zero-size viewport is unavailable");
    Require(guard.Observe(100, 120) == TouchViewportStatus::Unavailable,
            "smaller than touch layout minimum is unavailable");
    Require(!guard.HasBaseline(), "invalid viewport cannot establish baseline");

    const auto wide = CalculateTouchControlLayout(1280, 720);
    const double gasX = wide.Throttle.X + wide.Throttle.Width * 0.5;
    const double gasY = wide.Throttle.Y + wide.Throttle.Height * 0.5;
    const double steerX = wide.Steering.X + wide.Steering.Width * 0.5;
    const double steerY = wide.Steering.Y + wide.Steering.Height * 0.5;
    Require(guard.Observe(1280, 720) == TouchViewportStatus::Ready &&
            guard.HasBaseline(), "first valid screen establishes baseline");
    Require(presses.Press(1), "gas finger down once");
    drive.Press(wide, 1, gasX, gasY);
    Require(presses.Press(2), "steering finger down once");
    drive.Press(wide, 2, steerX, steerY);
    Require(drive.ThrottleActive() && drive.SteeringActive(),
            "two fingers control car");
    CancelOnTransition(guard, drive, 1280, 720);
    Require(drive.ThrottleActive() && drive.SteeringActive(),
            "repeated same viewport leaves controls active");

    // The engine may never emit a touch-moved event while Android rotates.
    CancelOnTransition(guard, drive, 720, 1280);
    Require(!drive.ThrottleActive() && !drive.SteeringActive(),
            "valid orientation change instantly cancels both held controls");
    Require(presses.IsDown(1) && presses.IsDown(2),
            "viewport change must not forget still-physically-held fingers");
    Require(!presses.Press(1) && !presses.Press(2),
            "duplicate old finger presses cannot reacquire after rotation");
    const auto portrait = CalculateTouchControlLayout(720, 1280);
    drive.Move(portrait, 1, portrait.Throttle.X + 5, portrait.Throttle.Y + 5);
    Require(!drive.ThrottleActive(),
            "late touch movement after viewport change cannot reacquire gas");

    presses.Release(1);
    Require(presses.Press(1), "fresh release then press permits re-acquisition");
    drive.Press(portrait, 1, portrait.Throttle.X + 5, portrait.Throttle.Y + 5);
    Require(drive.ThrottleActive(), "new finger in new layout works");
    CancelOnTransition(guard, drive, 721, 1280);
    Require(!drive.ThrottleActive(),
            "even small resize neutralizes stale coordinates");
    Require(guard.Observe(721, 1280) == TouchViewportStatus::Ready,
            "unchanged new screen becomes stable");

    presses.Release(1);
    Require(presses.Press(1), "fresh press after resize accepted");
    const auto resized = CalculateTouchControlLayout(721, 1280);
    drive.Press(resized, 1, resized.Throttle.X + 5, resized.Throttle.Y + 5);
    Require(drive.ThrottleActive(), "resized pad accepts a new physical tap");
    CancelOnTransition(guard, drive, 0, 0);
    Require(!drive.ThrottleActive() && !guard.HasBaseline(),
            "temporary zero-sized surface clears touch and layout baseline");
    Require(presses.IsDown(1), "temporary zero size retains finger ownership");
    CancelOnTransition(guard, drive, 1280, 720);
    Require(!drive.ThrottleActive(),
            "restored surface does not spontaneously reacquire gas");
    Require(!presses.Press(1), "restored surface cannot replay held finger");

    guard.Reset();
    Require(!guard.HasBaseline(), "lifecycle reset drops stale geometry");
    Require(!TouchViewportNeedsCancellation(guard.Observe(1280, 720)),
            "first valid frame after app foreground establishes baseline");

    std::cout << "Touch viewport and finger ownership tests passed.\n";
    return EXIT_SUCCESS;
}
