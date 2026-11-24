#include "Racing/BestLapGhost.h"

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
        std::cerr << "BestLapGhostYawContinuity FAIL: "
                  << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

latebrake::LapGhostPose Pose(double time, double x, double yaw)
{
    // True Unreal-world centimetre pose units, no fictional score source.
    return {time, x, 500.0, 180.0, yaw};
}

void CompleteReference(latebrake::BestLapGhost& ghost)
{
    for (int i = 1; i <= 50; ++i)
    {
        Require(ghost.Record(Pose(i / 10.0, 2.0 * i, 0.0), true),
                "source-controlled real-time reference trace accepted");
    }
    Require(ghost.FinishLap(5.08, true, true),
            "first penalty-free ordered lap can still create a best");
}
} // namespace

int main()
{
    using namespace latebrake;
    static_assert(BestLapGhost::MaxObservedYawRateDegreesPerSecond >=
                  1500.0);
    static_assert(BestLapGhost::YawJitterAllowanceDegrees >= 10.0);
    BestLapGhost ghost;
    CompleteReference(ghost);
    Require(ghost.HasBest() && ghost.GetBestSeconds() == 5.08,
            "verified previous best exists before invalid samples");

    // Static XYZ but near-instant 180-degree yaw is a broken transform
    // or lost angular-velocity observation, not a trustworthy ghost.
    Require(ghost.Record(Pose(0.10, 0.0, 0.0), true),
            "start fresh candidate heading witness");
    Require(!ghost.Record(Pose(0.10 + 1.0 / 120.0, 0.0, 180.0),
                          true) &&
            !ghost.IsCandidateAllowed() &&
            ghost.GetCandidateCount() == 0,
            "instant stationary yaw flip poisons only current ghost");
    LapGhostPose displayed;
    Require(ghost.Evaluate(2.5, displayed) &&
            ghost.GetBestSeconds() == 5.08 &&
            std::isfinite(displayed.YawDegrees),
            "invalid new heading preserves the previously completed best");
    Require(!ghost.FinishLap(3.0, true, true) &&
            ghost.GetBestSeconds() == 5.08,
            "broken candidate cannot replace a legitimate faster best");

    // The sparse keyframe decision must not hide two fast contradictory
    // live heading observations. Both observations fit before the next
    // 10Hz storage key, so this specifically protects the source trace.
    ghost.StartLap();
    Require(ghost.Record(Pose(0.100, 0.0, 0.0), true),
            "live short-frame yaw baseline");
    Require(ghost.Record(Pose(0.104, 0.0, 18.0), true) &&
            ghost.GetCandidateCount() == 1,
            "one moderate unstored heading update is accepted");
    Require(!ghost.Record(Pose(0.108, 0.0, 45.0), true) &&
            !ghost.IsCandidateAllowed() &&
            ghost.GetCandidateCount() == 0,
            "second unstored excessive yaw delta cannot hide in one stored key");

    // Unreal heading wraps around +180 and -180. Shortest-signed-angle
    // arithmetic must not flag an ordinary 2-degree seam crossing.
    ghost.StartLap();
    Require(ghost.Record(Pose(0.100, 0.0, 179.0), true),
            "start near positive yaw seam");
    Require(ghost.Record(Pose(0.104, 0.0, -179.0), true) &&
            ghost.Record(Pose(0.108, 0.0, -176.0), true) &&
            ghost.GetCandidateCount() == 1,
            "wrap boundary and following motion remain safe without overrecording");
    Require(ghost.Record(Pose(0.200, 1.0, -160.0), true) &&
            ghost.GetCandidateCount() == 2,
            "proper 10Hz key still accepted after yaw seam crossing");

    // At lower observation rates the cap is deliberately generous to
    // distinguish a sampled arcade spin from an impossible instant
    // rotation. This is not used to limit the car's actual angular speed.
    ghost.StartLap();
    Require(ghost.Record(Pose(0.100, 0.0, 0.0), true) &&
            ghost.Record(Pose(0.250, 0.0, 180.0), true) &&
            ghost.IsCandidateAllowed(),
            "sparsely observed rapid arcade spin not blindly rejected");

    const int frameRates[] = {30, 60, 120, 240};
    for (const int fps : frameRates)
    {
        BestLapGhost run;
        const int frameCount = fps * 2;
        for (int frame = 0; frame <= frameCount; ++frame)
        {
            const double elapsed = 0.05 +
                static_cast<double>(frame) / static_cast<double>(fps);
            const double x = static_cast<double>(frame) * 40.0 /
                static_cast<double>(fps);
            // Frequent benign yaw changes also cross the -180/+180
            // discontinuity in both directions without changing speed.
            const double yaw = std::remainder(
                6.0 * static_cast<double>(frame), 360.0);
            Require(run.Record(Pose(elapsed, x, yaw), true),
                    "genuine continuous heading trace accepted at target FPS");
        }
        Require(run.GetCandidateCount() >= 8 &&
                run.GetCandidateCount() <= 24,
                "30-240FPS live observation still uses bounded 10Hz storage");
        Require(run.FinishLap(2.10, true, true) && run.HasBest(),
                "valid frame cadence yields official completed best lap");
        LapGhostPose evaluated;
        Require(run.Evaluate(1.05, evaluated) &&
                std::isfinite(evaluated.X) &&
                std::isfinite(evaluated.YawDegrees) &&
                std::abs(evaluated.YawDegrees) <= 180.0,
                "replay interpolates physically plausible wrapped heading");
        Require(!run.Evaluate(2.50, evaluated),
                "ghost stops at best time, no infinite visual persistence");
    }

    // Reject 20,000 distinct microscopically short-frame heading snaps.
    // This must not allocate during Unreal Tick, leak earlier best
    // results, or accept yaw flips simply because position did not move.
    for (int attempt = 0; attempt < 20000; ++attempt)
    {
        ghost.StartLap();
        Require(ghost.Record(Pose(0.10, 0.0, 0.0), true),
                "repeated yaw stress baseline accepted");
        const double dt = 0.001 +
            0.001 * static_cast<double>(attempt % 4);
        const double degrees = attempt % 2 == 0 ? 179.0 : -179.0;
        Require(!ghost.Record(Pose(0.10 + dt, 0.0, degrees),
                              true) &&
                !ghost.IsCandidateAllowed() &&
                ghost.GetCandidateCount() == 0 &&
                ghost.GetBestSeconds() == 5.08,
                "20k high-FPS visual yaw snaps cannot become a best ghost");
    }
    ghost.ResetRun();
    Require(!ghost.HasBest() && ghost.GetBestCount() == 0,
            "official run reset still deletes stale visual ghost history");

    std::cout << "Continuous real-frame yaw and replay ghost tests passed.\n";
    return EXIT_SUCCESS;
}
