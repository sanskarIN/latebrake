#include "Vehicles/CornerLookAheadCamera.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void Check(bool ok, const char* reason) {
    if (!ok) { std::cerr << "Corner camera: " << reason << '\n'; std::exit(1); }
}
bool Near(double x, double y, double eps=1.0e-9) {
    return std::abs(x-y) < eps;
}
}

int main() {
    using latebrake::CameraView;
    latebrake::CornerLookAheadCamera cam;
    Check(!cam.HasActiveSample(), "starts neutral");
    Check(Near(cam.Sample(0.0, 1.0, 1.0/60.0, true, false, CameraView::Chase), 0.0),
          "stationary car cannot swing camera");
    double first = cam.Sample(3600.0, 1.0, 1.0/60.0, true, false, CameraView::Chase);
    Check(first > 0.0 && first < 6.0, "steering creates bounded visual lead");
    double previous = first;
    for (int n=0; n<20000; ++n) {
        const double value = cam.Sample(3600.0, 1.0, 1.0/120.0, true, false, CameraView::Chase);
        Check(std::isfinite(value) && value >= previous && value <= 6.0,
              "long sessions bounded and monotonic");
        previous = value;
    }
    Check(Near(cam.Sample(3600.0, -1.0, 1.0/60.0, true, false, CameraView::Chase),
               6.0 - 12.0*(1.0 - std::exp(-(1.0/60.0)/0.24)), 1e-7),
          "reversing steering smoothly reverses lead");
    cam.Reset();
    double high = cam.Sample(3600.0, 1.0, 0.16, true, false, CameraView::High);
    Check(high > 0.0 && high < 4.0, "high chase stays within 4 degrees");
    Check(cam.Sample(3600.0, 1.0, .016, true, false, CameraView::Hood)==0.0 &&
          !cam.HasActiveSample(), "hood view remains static");
    cam.Sample(3600, 1, .016, true, false, CameraView::Chase);
    Check(cam.Sample(3600, 1, .016, true, true, CameraView::Chase)==0.0 &&
          !cam.HasActiveSample(), "look-back takes priority and resets lead");
    cam.Sample(3600, 1, .016, true, false, CameraView::Chase);
    Check(cam.Sample(3600, 1, .016, false, false, CameraView::Chase)==0.0 &&
          !cam.HasActiveSample(), "AI rival never acquires player camera");
    cam.Sample(3600, 1, .016, true, false, CameraView::Chase);
    Check(cam.Sample(-12, 1, .016, true, false, CameraView::Chase)==0.0 &&
          !cam.HasActiveSample(), "reverse drive resets view");
    Check(cam.Sample(std::numeric_limits<double>::quiet_NaN(), 1, .016, true,
                     false, CameraView::Chase)==0, "nonfinite speed resets");
    Check(cam.Sample(3600, std::numeric_limits<double>::infinity(), .016, true,
                     false, CameraView::Chase)==0, "nonfinite steering resets");
    Check(cam.Sample(3600, 1.5, .016, true, false, CameraView::Chase)==0,
          "invalid steering axis resets");
    Check(cam.Sample(3600, 1, .5, true, false, CameraView::Chase)==0,
          "frame hitch resets");
    Check(cam.Sample(3600, 1, .0, true, false, CameraView::Chase)==0,
          "invalid time resets");
    Check(cam.Sample(25000, 1, .016, true, false, CameraView::Chase)==0,
          "implausible physics speed resets");
    cam.Reset();
    const double fullA = cam.Sample(3600, 1, .10, true, false, CameraView::Chase);
    const double fullB = cam.Sample(3600, 1, .10, true, false, CameraView::Chase);
    cam.Reset();
    cam.Sample(3600, 1, .05, true, false, CameraView::Chase);
    const double splitA = cam.Sample(3600, 1, .05, true, false, CameraView::Chase);
    cam.Sample(3600, 1, .05, true, false, CameraView::Chase);
    const double splitB = cam.Sample(3600, 1, .05, true, false, CameraView::Chase);
    Check(Near(fullA,splitA) && Near(fullB,splitB),
          "easing unaffected by frame cadence");
    Check(cam.Sample(3600, 0, .08, true, false, CameraView::Chase) < splitB,
          "releasing steering recenters smoothly");
    cam.Reset();
    Check(!cam.HasActiveSample(), "race reset clears lead");
    std::cout << "Corner look-ahead camera C++ policy tests passed.\n";
    return 0;
}
