#include "AI/RivalStraightPassAssist.h"
#include "AI/RivalPassTrafficResolution.h"
#include "AI/RivalTrafficAwareness.h"
#include "AI/RivalFollowingBrakeHold.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "RivalPassGateRetry FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

// These are genuine world-space coordinates/velocities of the existing
// original four-gate circuit, not abstract invented extra rival cars.
latebrake::RivalPassSample Observe(
    int gate = 0, double rivalAlong = 550.0,
    double playerAhead = 1900.0, double rivalSide = 0.0,
    double playerSide = 0.0, double rivalSpeed = 2200.0,
    double playerSpeed = 1800.0)
{
    using namespace latebrake;
    const auto destination = CircuitGate(gate);
    const auto origin = CircuitGate(
        (gate + CircuitGateCount - 1) % CircuitGateCount);
    const double travelX = destination.X - origin.X;
    const double travelY = destination.Y - origin.Y;
    const double length = std::hypot(travelX, travelY);
    const double forwardX = travelX / length;
    const double forwardY = travelY / length;
    const double sideX = -forwardY;
    const double sideY = forwardX;
    return {1.0 / 60.0, gate,
        {origin.X + forwardX * rivalAlong + sideX * rivalSide,
         origin.Y + forwardY * rivalAlong + sideY * rivalSide},
        {origin.X + forwardX * (rivalAlong + playerAhead) +
             sideX * playerSide,
         origin.Y + forwardY * (rivalAlong + playerAhead) +
             sideY * playerSide},
        std::atan2(forwardY, forwardX),
        rivalSpeed * forwardX, rivalSpeed * forwardY,
        playerSpeed * forwardX, playerSpeed * forwardY,
        true, true};
}

latebrake::RivalTrafficObservation Traffic(
    const latebrake::RivalPassSample& sample,
    double safeBrakeDeceleration = 684.0)
{
    return {sample.Rival.X, sample.Rival.Y, sample.RivalHeadingRadians,
        std::hypot(sample.RivalVelocityX, sample.RivalVelocityY),
        sample.Player.X, sample.Player.Y,
        sample.PlayerVelocityX, sample.PlayerVelocityY,
        sample.Qualified, sample.RivalVelocityX, sample.RivalVelocityY,
        safeBrakeDeceleration};
}
} // namespace

int main()
{
    using namespace latebrake;
    const PilotCommand drive{0.72, 0.08, false, false};
    const PilotCommand originalBrake{0.0, 0.11, true, false};
    RivalStraightPassAssist rival;

    // A new legitimate pass is still permitted with adequate 19m of
    // same-straight separation and measured 4m/s closing speed.
    auto started = rival.Apply(drive, Observe());
    Require(started.Passing && rival.IsActive() &&
            !rival.IsRetryBlockedForGate(0) &&
            started.Command.Throttle == drive.Throttle,
            "original safe passing entry is unchanged");

    // The player's real vehicle slows abruptly: actual following traffic
    // policy (plus hysteresis) brakes, not merely a fabricated bool.
    const auto rapidlyClosing = Observe(
        0, 900.0, 900.0, -150.0, 0.0, 2450.0, 1100.0);
    const auto activePass = rival.Apply(drive, rapidlyClosing);
    Require(activePass.Passing,
            "active lane attempt exists before live emergency arbitration");
    const auto detected = YieldRivalToPlayer(
        activePass.Command, Traffic(rapidlyClosing));
    Require(detected.Brake && detected.Throttle == 0.0,
            "real collision courtesy guard requests emergency braking");
    RivalFollowingBrakeHold hold;
    const auto held = hold.Apply(detected, 1.0 / 60.0, true,
                                detected.Brake && !activePass.Command.Brake);
    const auto resolution = ResolveRivalPassTraffic(
        drive, activePass, held);
    Require(resolution.AbortPassing && resolution.Command.Brake &&
            resolution.Command.Throttle == 0.0 &&
            resolution.Command.Steering == drive.Steering,
            "collision brake restores original route steering");
    rival.AbortUntilGateChange();
    Require(!rival.IsActive() && rival.IsRetryBlockedForGate(0),
            "real brake abort retires only the current checkpoint's pass");

    for (int frame = 0; frame < 10000; ++frame)
    {
        // The player may now pull away; the previous unsafe lateral
        // trajectory must not be reinstated next frame at the same gate.
        const auto wouldRestart = rival.Apply(drive, Observe());
        Require(!wouldRestart.Passing && !rival.IsActive() &&
                rival.IsRetryBlockedForGate(0) &&
                wouldRestart.Command.Throttle == drive.Throttle &&
                wouldRestart.Command.Steering == drive.Steering &&
                !wouldRestart.Command.Brake,
                "10k subsequent safe-looking frames do not cause repeat weaving");
    }
    // Only route progress across an actually different verified next
    // gate clears the latch, not timers, steering noise or a player pause.
    const auto shortStraight = rival.Apply(drive, Observe(1));
    Require(!shortStraight.Passing &&
            !rival.IsRetryBlockedForGate(0),
            "new verified gate re-arms passing without inventing a short-straight pass");
    Require(rival.Apply(drive, Observe()).Passing,
            "a later eligible long straight can safely initiate a new pass");

    // The player actually crosses the chosen passing side; the
    // assistant brakes and also remembers why this gate was abandoned.
    rival.Reset();
    Require(rival.Apply(drive, Observe()).Passing,
            "lane-intrusion setup starts a valid manoeuvre");
    const auto intrusion = rival.Apply(
        drive, Observe(0, 650.0, 950.0, -280.0, -450.0));
    Require(!intrusion.Passing && intrusion.Command.Brake &&
            intrusion.Command.Throttle == 0.0 &&
            rival.IsRetryBlockedForGate(0) &&
            !rival.Apply(drive, Observe()).Passing,
            "blocked passing lane cannot re-arm on same straight");

    // An already-braking route must not secretly keep its old passing
    // lane; its next safe-looking frame cannot immediately re-enter.
    rival.Reset();
    Require(rival.Apply(drive, Observe()).Passing,
            "corner-brake setup starts a valid manoeuvre");
    const auto priorBrake = rival.Apply(originalBrake, Observe());
    Require(!priorBrake.Passing && priorBrake.Command.Brake &&
            priorBrake.Command.Steering == originalBrake.Steering &&
            rival.IsRetryBlockedForGate(0),
            "authoritative route/corner brake locks aborted same-gate pass");

    // The normal ahead/behind finish, not only emergencies, must
    // retire the present passing attempt until route progress.
    rival.Reset();
    Require(rival.Apply(drive, Observe()).Passing,
            "natural completion setup begins pass");
    const auto cleared = rival.Apply(
        drive, Observe(0, 850.0, -600.0, -420.0));
    Require(!cleared.Passing && !rival.IsActive() &&
            rival.IsRetryBlockedForGate(0),
            "finished clearance returns to route without same-gate retry");
    Require(!rival.Apply(drive, Observe()).Passing,
            "completed pass never repeats immediately");

    // Timing budget deliberately bounds lateral steering even with an
    // unrealistically long constant-speed observation. Once spent, a
    // new attempt must wait for route progression (not the next Tick).
    rival.Reset();
    Require(rival.Apply(drive, Observe()).Passing,
            "elapsed-duration setup begins pass");
    bool exhausted = false;
    for (int frame = 0; frame < 480; ++frame)
    {
        const auto step = rival.Apply(drive, Observe());
        if (!step.Passing)
        {
            exhausted = true;
            Require(!rival.IsActive() &&
                    rival.IsRetryBlockedForGate(0),
                    "six-second timeout retires current gate");
            break;
        }
    }
    Require(exhausted && !rival.Apply(drive, Observe()).Passing,
            "exhausted pass does not re-open on the next frame");

    // Any physical route/pavement rejection during an active attempt
    // is also a gate-local retirement, but a full race restart clears it.
    rival.Reset();
    Require(rival.Apply(drive, Observe()).Passing,
            "corner-runway setup begins pass");
    const auto corner = rival.Apply(
        drive, Observe(0, 4100.0));
    Require(!corner.Passing && rival.IsRetryBlockedForGate(0),
            "entering a turn cancels and retires this passing attempt");
    rival.Reset();
    Require(!rival.IsRetryBlockedForGate(0) &&
            rival.Apply(drive, Observe()).Passing,
            "actual restart/pause/recovery reset does not preserve prior gate state");

    // Invalid live physics still fails closed with brake authority and
    // removes any stale lane target. A fresh reset/recovery epoch may
    // later re-establish an entirely new attempt.
    auto invalid = Observe();
    invalid.DeltaSeconds = std::numeric_limits<double>::quiet_NaN();
    const auto rejected = rival.Apply(drive, invalid);
    Require(!rejected.Passing && rejected.Command.Brake &&
            rejected.Command.Throttle == 0.0 && !rival.IsActive() &&
            rival.IsRetryBlockedForGate(0) &&
            !rival.Apply(drive, Observe()).Passing,
            "corrupt live frame cannot restart passing on same gate");
    rival.Reset();
    rival.AbortUntilGateChange();
    Require(!rival.IsRetryBlockedForGate(0),
            "an idle controller cannot fabricate a retired gate");
    const auto notQualified = Observe();
    auto blocked = notQualified;
    blocked.Qualified = false;
    Require(rival.Apply(drive, blocked).Command.Brake &&
            !rival.IsRetryBlockedForGate(0),
            "unqualified race state cannot carry a ghost lane latch");

    std::cout << "Physical rival passing per-gate retry safety passed.\n";
    return EXIT_SUCCESS;
}
