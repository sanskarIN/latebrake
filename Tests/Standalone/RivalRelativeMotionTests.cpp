#include "Track/RivalRelativeMotion.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool yes, const char* message)
{
    if (!yes)
    {
        std::cerr << "RivalRelativeMotion FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b)
{
    return std::abs(a - b) < 1e-8;
}
} // namespace

int main()
{
    using namespace latebrake;
    const CircuitPoint player{0.0, SouthY};
    const auto snapshot = [&](double separation) {
        return BuildRaceMiniMapSnapshot(
            player, 0.0, true, 0, true,
            {separation * 100.0, SouthY}, true);
    };
    RivalRelativeMotionTracker tracker;
    const auto start = snapshot(30.0);
    Check(start.DirectRivalSeparationValid &&
          Near(start.DirectRivalSeparationMeters, 30.0),
          "full production radar supplies direct 30m physical geometry");

    Check(tracker.Update(start, 0.1, true).Trend == RivalRelativeTrend::None,
          "first sample establishes baseline, never invents velocity");
    for (double distance : {29.5, 29.0, 28.5})
    {
        Check(tracker.Update(snapshot(distance), 0.1, true).Trend ==
                  RivalRelativeTrend::None,
              "three intervals insufficient for a closing claim");
    }
    const auto closing = tracker.Update(snapshot(28.0), 0.1, true);
    Check(closing.Trend == RivalRelativeTrend::Closing &&
          Near(closing.DirectDistanceMeters, 28.0) &&
          Near(closing.SeparationChangeMetersPerSecond, 5.0),
          "sustained 5 m/s direct closing only after 0.4 seconds");

    Check(tracker.Update(snapshot(28.5), 0.1, true).Trend ==
              RivalRelativeTrend::None &&
          tracker.GetConfirmedTrend() == RivalRelativeTrend::None,
          "first sign reversal clears stale closing advice");
    for (double distance : {29.0, 29.5})
    {
        Check(tracker.Update(snapshot(distance), 0.1, true).Trend ==
                  RivalRelativeTrend::None,
              "opening requires its own sustained evidence");
    }
    const auto opening = tracker.Update(snapshot(30.0), 0.1, true);
    Check(opening.Trend == RivalRelativeTrend::Opening &&
          Near(opening.SeparationChangeMetersPerSecond, 5.0),
          "sustained separation growth reports opening not rival rank");
    Check(tracker.Update(snapshot(30.0), 0.1, true).Trend ==
              RivalRelativeTrend::None,
          "stationary distance immediately clears motion warning");

    // Rapid large displacement represents a discontinuity/teleport and
    // cannot produce a spurious closing estimate on the subsequent frame.
    tracker.Reset();
    tracker.Update(snapshot(30.0), 0.1, true);
    Check(tracker.Update(snapshot(1.0), 0.1, true).Trend ==
              RivalRelativeTrend::None &&
          Near(tracker.GetCandidateSeconds(), 0.0),
          "290m/s inferred separation jump rejected");
    Check(tracker.Update(snapshot(1.0), 0.1, true).Trend ==
              RivalRelativeTrend::None &&
          tracker.GetConfirmedTrend() == RivalRelativeTrend::None,
          "after rejected jump, next sample only restores baseline");

    for (double badDelta : {
        0.0, -0.1, 0.0001, 0.26,
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity()
    })
    {
        tracker.Update(snapshot(25.0), 0.1, true);
        Check(tracker.Update(snapshot(24.0), badDelta, true).Trend ==
                  RivalRelativeTrend::None &&
              tracker.GetCandidateSeconds() == 0.0,
              "bad, tiny or hitch time steps invalidate trend");
    }
    auto badDistance = snapshot(10.0);
    badDistance.DirectRivalSeparationMeters =
        std::numeric_limits<double>::quiet_NaN();
    auto badMarker = snapshot(10.0);
    badMarker.Rival.Valid = false;
    for (const auto& bad : {
        badDistance, badMarker,
        BuildRaceMiniMapSnapshot(player, 0.0, true, 0, false,
                                player, true),
        BuildRaceMiniMapSnapshot(player, 0.0, false, 0, true,
                                player, true),
        BuildRaceMiniMapSnapshot(player, 0.0, true, 0, true,
                                {6000.0, SouthY}, true)
    })
    {
        tracker.Reset();
        tracker.Update(snapshot(30.0), 0.1, true);
        tracker.Update(snapshot(29.0), 0.1, true);
        Check(tracker.Update(bad, 0.1, true).Trend ==
                  RivalRelativeTrend::None &&
              tracker.GetCandidateSeconds() == 0.0,
              "missing/ineligible/malformed/off-display rival clears motion");
    }
    tracker.Update(snapshot(30.0), 0.1, true);
    Check(tracker.Update(snapshot(29.0), 0.1, false).Trend ==
              RivalRelativeTrend::None &&
          tracker.GetCandidateSeconds() == 0.0,
          "race suspension always invalidates motion history");

    tracker.Reset();
    tracker.Update(snapshot(30.0), 0.1, true);
    Check(tracker.Update(snapshot(29.9), 0.1, true).Trend ==
              RivalRelativeTrend::None &&
          tracker.GetCandidateSeconds() == 0.0,
          "less than 2m/s radial change is noise, not approach");

    tracker.Reset();
    tracker.Update(snapshot(30.0), 0.1, true);
    for (int i = 1; i <= 8; ++i)
    {
        const auto next = tracker.Update(snapshot(30.0 - 0.25 * i),
                                         0.1, true);
        if (i >= 4)
        {
            Check(next.Trend == RivalRelativeTrend::Closing,
                  "2.5m/s sustained closing stays active");
        }
    }
    Check(Near(tracker.GetCandidateSeconds(), 0.4),
          "longer warning accumulation remains bounded");
    // High-refresh Unreal displays must still accumulate meaningful
    // 144Hz time deltas rather than resetting every (<10ms) frame.
    tracker.Reset();
    constexpr double RefreshDelta = 1.0 / 144.0;
    tracker.Update(snapshot(30.0), RefreshDelta, true);
    RivalRelativeMotionAdvice highRefresh;
    for (int frame = 1; frame <= 64; ++frame)
    {
        highRefresh = tracker.Update(
            snapshot(30.0 - 3.0 * RefreshDelta * frame),
            RefreshDelta, true);
    }
    Check(highRefresh.Trend == RivalRelativeTrend::Closing &&
          Near(highRefresh.SeparationChangeMetersPerSecond, 3.0),
          "144Hz samples report real 3m/s sustained closing");
    tracker.Reset();
    Check(tracker.GetConfirmedTrend() == RivalRelativeTrend::None &&
          tracker.GetCandidateSeconds() == 0.0,
          "restart/teleport/pawn replacement explicit reset");
    std::cout << "Rival relative-motion tests passed (portable C++20).\n";
    return EXIT_SUCCESS;
}
