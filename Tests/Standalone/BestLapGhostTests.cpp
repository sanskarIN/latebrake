#include "Racing/BestLapGhost.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace
{
void Check(bool yes, const char* message)
{
    if (!yes)
    {
        std::cerr << "BestLapGhost FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

latebrake::LapGhostPose Pose(double t, double x, double yaw = 0.0)
{
    return {t, x, 500.0, 180.0, yaw};
}

void RecordLap(latebrake::BestLapGhost& recorder, double lap,
               bool active = true, bool noPenalties = true,
               double distanceMultiplier = 10.0)
{
    const int count = static_cast<int>(std::floor(lap * 10.0));
    for (int i = 1; i <= count; ++i)
    {
        const double sec = i / 10.0;
        Check(recorder.Record(Pose(sec, sec * distanceMultiplier), true),
              "valid 10Hz frame accepted");
    }
    recorder.FinishLap(lap, active, noPenalties);
}
}

int main()
{
    using latebrake::BestLapGhost;
    using latebrake::LapGhostPose;
    BestLapGhost g;
    Check(!g.HasBest(), "no imaginary ghost before complete lap");

    for (int i = 1; i <= 120; ++i)
    {
        Check(g.Record(Pose(i / 10.0, i * 9.0), true), "record first lap");
    }
    Check(g.FinishLap(12.0, true, true), "completed timed lap becomes new best");
    Check(g.HasBest() && g.GetBestCount() >= 100 &&
          g.GetBestSeconds() == 12.0, "best duration and poses stored");
    LapGhostPose replay;
    Check(g.Evaluate(6.25, replay) &&
          std::abs(replay.X - 562.5) < 1e-6 &&
          replay.Seconds == 6.25, "subframe interpolation is stable");
    Check(g.Evaluate(0.0, replay) && replay.Seconds > 0.0,
          "playback before first key uses first valid pose");
    Check(g.Evaluate(11.98, replay) && replay.X > 1000.0,
          "last interval is accessible without extrapolation");
    Check(!g.Evaluate(12.1, replay) &&
          !g.Evaluate(std::numeric_limits<double>::quiet_NaN(), replay),
          "no ghost after best lap finishes or at invalid time");

    // A slower lap may replay the existing ghost but must never overwrite it.
    RecordLap(g, 15.0);
    Check(g.GetBestSeconds() == 12.0, "slower lap cannot overwrite best");
    RecordLap(g, 10.0);
    Check(g.GetBestSeconds() == 10.0, "faster eligible lap replaces best");
    Check(g.Evaluate(5.0, replay) &&
          std::abs(replay.X - 50.0) < 1e-6,
          "replay uses upgraded best, not superseded lap");

    // Recording pauses do not permanently delete already completed best.
    Check(g.Record(Pose(0.10, 1.0), true), "resume candidate start");
    g.InvalidateCurrentLap();
    Check(!g.Record(Pose(0.20, 2.0), true),
          "a poisoned lap cannot restart mid-lap");
    Check(!g.FinishLap(8.0, true, true),
          "interrupted or teleported lap must not become best");
    Check(g.HasBest() && g.GetBestSeconds() == 10.0,
          "unverified candidate does not delete prior ghost");
    RecordLap(g, 9.0, false, true);
    Check(g.GetBestSeconds() == 10.0, "penalty-only/non-active lap rejected");
    RecordLap(g, 9.0, true, false);
    Check(g.GetBestSeconds() == 10.0, "recovery-penalty lap rejected");

    g.ResetRun();
    Check(!g.HasBest() && g.GetBestCount() == 0,
          "new race never inherits a prior event's ghost");
    Check(!g.Record(Pose(1.0, 1.0), true),
          "first observed frame too late cannot invent clean full lap");
    g.StartLap();
    Check(!g.Record(Pose(0.10, 1.0), false),
          "blocked race sample poisons current candidate");

    g.StartLap();
    Check(g.Record(Pose(0.10, 1.0), true), "new lap re-arms recorder");
    Check(!g.Record(Pose(0.80, 8.0), true),
          "long world-pause/physics hitch breaks a lap");
    g.StartLap();
    Check(g.Record(Pose(0.10, 1.0), true), "start strict time check");
    Check(!g.Record(Pose(0.10, 1.0), true),
          "repeated or non-advancing clock is not a valid new sample");
    g.StartLap();
    Check(g.Record(Pose(0.10, 1.0), true), "new lap re-arms after hitch");
    Check(!g.Record(Pose(0.20, 4000.0), true),
          "unexplained teleport-sized spatial jump is rejected");

    // The previous 10Hz recorder skipped the displacement check when
    // a valid Unreal tick arrived less than 0.09 seconds after a key.
    // A very short-lived teleport could escape into a later clean key.
    g.StartLap();
    Check(g.Record(Pose(0.10, 0.0), true), "subframe ghost baseline");
    Check(!g.Record(Pose(0.14, 3000.0), true) &&
          !g.IsCandidateAllowed() && g.GetCandidateCount() == 0,
          "sub-10Hz teleport cannot bypass physical pose validation");
    Check(!g.Record(Pose(0.20, 0.0), true),
          "returning to the former key cannot rehabilitate poisoned lap");
    Check(!g.FinishLap(8.0, true, true),
          "a hidden out-and-back teleport never earns a clean lap");

    g.StartLap();
    Check(g.Record(Pose(0.10, 0.0), true), "begin ordinary high-FPS lap");
    for (int frame = 1; frame <= 8; ++frame)
    {
        const double t = 0.10 + frame * 0.01;
        Check(g.Record(Pose(t, frame * 10.0), true) &&
              g.IsCandidateAllowed() && g.GetCandidateCount() == 1,
              "valid short-time Unreal frames preserve 10Hz keyframe budget");
    }
    Check(g.Record(Pose(0.20, 100.0), true) &&
          g.GetCandidateCount() == 2,
          "normal high refresh rate still stores sparse ghost keyframes");
    Check(g.Record(Pose(0.21, 115.0), true) &&
          g.IsCandidateAllowed() && g.GetCandidateCount() == 2,
          "short subframe road motion remains accepted but unstored");

    // Rapidly repeated tiny-time samples cannot hide an impossible
    // teleport behind the last accepted 10Hz keyframe. No heap, file,
    // clock or scoring authority is involved in this stress check.
    for (int round = 0; round < 20000; ++round)
    {
        g.StartLap();
        Check(g.Record(Pose(0.10, 0.0), true),
              "deterministic subframe spatial baseline");
        const double t = 0.11 + 0.001 * (round % 50);
        Check(!g.Record(Pose(t, 6000.0), true) &&
              !g.IsCandidateAllowed() && g.GetCandidateCount() == 0,
              "20k sub-10Hz spatial jumps must invalidate candidates");
    }

    for (const LapGhostPose& invalid : {
        Pose(0.1, std::numeric_limits<double>::quiet_NaN()),
        Pose(0.1, std::numeric_limits<double>::infinity()),
        Pose(0.1, 500001.0),
        {0.1, 1.0, 2.0, 3.0, std::numeric_limits<double>::quiet_NaN()},
        {-0.1, 1.0, 2.0, 3.0, 0.0}
    })
    {
        g.StartLap();
        Check(!g.Record(invalid, true), "invalid pose or clock rejected");
    }

    g.StartLap();
    Check(g.Record(Pose(0.1, 1.0, 179.0), true), "yaw wrap first key");
    Check(g.Record(Pose(0.2, 2.0, -179.0), true), "yaw wrap second key");
    for (int i = 3; i <= 10; ++i)
    {
        Check(g.Record(Pose(i / 10.0, i * 1.0, -179.0), true),
              "longer wrapped heading sequence");
    }
    Check(g.FinishLap(1.01, true, true), "valid complete yaw lap");
    Check(g.Evaluate(0.15, replay) &&
          std::abs(std::abs(replay.YawDegrees) - 180.0) < 0.01,
          "heading interpolates shortest way through 180 degrees");

    // Explicit two-run exhaustion test, bounded by max recording time.
    g.ResetRun();
    g.StartLap();
    Check(g.Record(Pose(0.1, 1.0), true), "start extreme candidate");
    Check(!g.Record(Pose(BestLapGhost::MaxLapSeconds + 0.1, 2.0), true),
          "minutes-long unbounded lap never consumes arbitrary memory");

    std::cout << "Best-lap ghost source policies passed.\n";
    return EXIT_SUCCESS;
}
