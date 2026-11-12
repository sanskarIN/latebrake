#include "Racing/PortableReplayDeltaTrace.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool condition, const char* description)
{
    if (!condition)
    {
        std::cerr << "PortableReplayDeltaTrace FAIL: " << description << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double x, double y)
{
    return std::abs(x - y) < 1.0e-9;
}
}

int main()
{
    using namespace latebrake;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    PortableReplayDeltaTrace reference;
    PortableReplayDeltaTrace challenger;
    Check(!reference.Append({0.0, 0.0}), "unconfigured rejects samples");
    Check(!reference.Reset(0.0) && !reference.Reset(nan) &&
          !reference.Reset(inf) && !reference.Reset(1.0e8),
          "course bounds fail closed");
    Check(reference.Reset(1000.0) && challenger.Reset(1000.0),
          "qualify course");
    Check(!reference.Append({1.0, 0.0}) &&
          !reference.Append({0.0, 1.0}), "exact zero origin");
    Check(reference.Append({0.0, 0.0}), "origin accepted");
    Check(!reference.Append({0.0, 0.0}) &&
          !reference.Append({-1.0, 1.0}) &&
          !reference.Append({nan, 1.0}) &&
          !reference.Append({100.0, nan}) &&
          !reference.Append({100.0, inf}) &&
          !reference.Append({1001.0, 2.0}),
          "invalid and repeated samples refused");
    Check(reference.Append({250.0, 20.0}) &&
          reference.Append({650.0, 50.0}), "ordered intermediate samples");
    Check(!reference.Append({250.0, 60.0}) &&
          !reference.Append({700.0, 50.0}),
          "strict time and distance monotonicity");
    Check(!reference.Complete(), "partial trace cannot compare");
    Check(!reference.Estimate(999.0).Valid &&
          !reference.Estimate(nan).Valid &&
          !reference.Estimate(-1.0).Valid,
          "no extrapolation or invalid distance");
    const auto between = reference.Estimate(450.0);
    Check(between.Valid && between.Interpolated &&
          between.LowerSampleIndex == 1 && between.UpperSampleIndex == 2 &&
          Near(between.EstimatedSeconds, 35.0),
          "binary search plus linear interpolation");
    const auto exact = reference.Estimate(250.0);
    Check(exact.Valid && !exact.Interpolated &&
          Near(exact.EstimatedSeconds, 20.0),
          "exact knots preserve time");
    Check(reference.Append({1000.0, 80.0}), "final split");
    Check(reference.Complete() && reference.Size() == 4,
          "complete verified path");
    Check(!reference.Append({1000.0, 81.0}),
          "no repeated finish point");
    Check(challenger.Append({0.0, 0.0}) &&
          challenger.Append({500.0, 45.0}) &&
          challenger.Append({1000.0, 85.0}), "opponent trace");
    auto delta = CompareReplayAtDistance(reference, challenger, 500.0);
    Check(delta.Valid && Near(delta.ReferenceSeconds, 38.75) &&
          Near(delta.ChallengerSeconds, 45.0) &&
          Near(delta.ChallengerBehindSeconds, 6.25),
          "positive means challenger behind");
    delta = CompareReplayAtDistance(reference, challenger, 1000.0);
    Check(delta.Valid && Near(delta.ChallengerBehindSeconds, 5.0),
          "finish timing agrees with full samples");
    Check(!CompareReplayAtDistance(reference, challenger, -1.0).Valid,
          "no comparison outside race");
    Check(challenger.Reset(900.0) &&
          challenger.Append({0.0, 0.0}) &&
          challenger.Append({900.0, 80.0}), "new circuit");
    Check(!CompareReplayAtDistance(reference, challenger, 700.0).Valid,
          "different courses never comparable");
    Check(!reference.Reset(-1.0) && reference.Size() == 0,
          "invalid reset clears stale sample state");
    PortableReplayDeltaTrace bounded;
    Check(bounded.Reset(128.0), "128 sample benchmark course");
    for (std::size_t i = 0; i < PortableReplayDeltaTrace::Capacity; ++i)
    {
        const double value = static_cast<double>(i);
        Check(bounded.Append({value, value * 0.5}),
              "capacity exactly bounded");
    }
    Check(bounded.Size() == PortableReplayDeltaTrace::Capacity &&
          !bounded.Append({128.0, 64.0}) &&
          !bounded.Complete(), "overflow rejected without mutation");
    std::cout << "Portable replay delta trace checks passed.\n";
}
