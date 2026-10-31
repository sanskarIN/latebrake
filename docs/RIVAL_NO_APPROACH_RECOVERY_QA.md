# Rival high-speed no-approach recovery — source gameplay QA

**Development state:** Real engine-neutral C++20 route policy used by the
existing physical Unreal rival. Not a licensed Unreal Engine 5.8 build,
authored cooked circuit, packaged Windows game, APK, or installed driving
qualification.

## Problem

Before this change, `RivalRouteProgress` requested a supervised recovery
after **eight seconds below 80 cm/s**. A physically simulated rival can
be trapped without being nearly stationary: an AI chassis might slide
sideways along a barrier, spin in place, or circle a small area at
substantial speed. Because measured speed stayed above 80 cm/s, the old
stationary timer reset and no verified recovery was requested.

## Real gameplay integration

The actual `ALatebrakeRivalCar::Tick` already supplies the car's
measured world position, actual planar rigid-body speed and delta time
to `Route.Observe`, under the authoritative race, circuit and physics
readiness gates.

The route policy now tracks a **separate eight-second moving-stall
observation**. It may request the same already-audited recovery only when
all are true:

- Measured chassis speed is **450–10,000 cm/s**, so it genuinely is
  moving rather than using the original stationary recovery rule.
- Continuous source-valid frame times are at most 0.10 seconds.
- The distance to the actual current waypoint is finite, positive and
  no more than 50,000 cm.
- That distance **has not decreased by 260 cm** since the current
  observation's meaningful-progress baseline.

Every genuine 260 cm improvement resets the observation window, even if
the waypoint is not yet reached. Only distance toward the actual next
checkpoint is progress; high world speed, sideways sliding, reversing
and temporary movement away from the checkpoint are *not* progress.

The existing eight-second stationary recovery remains unchanged for
speeds under 80 cm/s. The 80–449 cm/s intermediate band is kept
conservative; it cannot trigger a high-speed loop recovery based on
ambiguous movement.

## Existing safeguards remain authoritative

- The same finite `MaxRecoveries` budget (100,000 attempts) is used by
  both stall mechanisms; this change adds **no** free teleport path.
- The request sets `RecoveryPending`, and both further route progress
  and duplicate requests remain blocked until the engine result arrives.
- Actual `TryRecoverTo` verifies the Unreal post-move placement and
  usable chassis. The route counts successful and rejected outcomes
  separately via `ReportRecoveryResult`.
- Every new route gate, race reset or event reconfiguration clears the
  moving-stall reference. Invalid or implausible data and frame hitches
  discard the reference; failed recovery demands an entirely new interval.
- Rival lap/waypoint bookkeeping is **approximate**, not authoritative
  player progression; no changes to player checkpoint, credits, career,
  records, race timing, rival steering or drive forces.

This is intentionally conservative: it may not rescue every real-world
stall, and a high-speed rival deliberately blocked by a vehicle could
eventually request a supervised recovery. Installed game testing must
assess that tradeoff rather than treating source results as gameplay
verification.

## Source regression suite

`Tests/Standalone/RivalNoApproachRecoveryTests.cpp` verifies no premature
teleport; 8-second continuous wall slides; oscillating no-progress paths;
legitimate 260cm approach; invalid frame and distance resets; intermediate
speeds; original stationary recovery; proper pending/outcome accounting;
reset, fresh waypoint and retry behavior. It also performs **400,000
deterministic observations** across normal approach and moving stalls.

`Tests/Editor/RivalNoApproachBindingTests.py` verifies the actual
Unreal rival's qualified position/speed/waypoint read, route invocation
and verified teleport feedback, without executing Unreal.

The portable C++ tests must pass on Windows, Ubuntu and macOS. This is
not a physical racing simulation: Chaos collision, AI path choice,
contact behavior, lap completion and driver experience are untested.

## Genuine Unreal acceptance still required

1. Compile full UE 5.8 UHT/Editor/game targets and cook the original
   `LatebrakeCircuit` map, without fabricating binaries or source receipts.
2. Test ordinary AI laps and corners: a rival genuinely approaching its
   target should **never** spend a recovery attempt.
3. In a private development build, stage a safe wall-slide or loop at
   high measured speed without waypoint approach. Confirm precisely one
   verified recovery request after eight valid seconds.
4. Verify pause, focus loss, frame hitches, invalid chassis, player traffic,
   failed teleports, pending state, new events and normal finish handling.
5. Validate actual installed Windows and Android performance and evidence
   under [playable game gate #148](
   https://github.com/sanskarIN/latebrake/issues/148).

No source-only test proves installed gameplay.
