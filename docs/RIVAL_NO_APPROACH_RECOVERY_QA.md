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

## Physical verified-frame continuity fix

The production `ObserveVerifiedGate` path now **rejects any physics
frame longer than 0.10 seconds**, even if the caller says a directional
plane was crossed. This is essential for the stationary recovery path:
previously a single finite eight-second renderer hitch at speed zero
could authorize an immediate supervised teleport. Such a frame is not
eight seconds of consecutive observed physical stalling.

Invalid position, speed or time also clears the real rival's stale
stationary and moving-stall accumulation. A new recovery requires
80 consecutive valid 0.10-second observations (or equivalent valid
smaller frames), using a 1-nanosecond cumulative tolerance to account
for non-exact decimal addition in IEEE-754 doubles. Gate credit and route advancement are blocked on a
hitch, while the real `CheckpointMotionTrace` still performs 3D
position, direction, speed and teleport-generation checking on the
ordinary frames. The separate historical `Observe` distance-only
portable interface remains backward-compatible; it does not drive
the actual Unreal rival. No automatic recovery or lap credit can
be based on an oversized verified physics tick.

The amended cross-platform tests check a hitch at the gate, a hitch
before stationary recovery, 79 clean frames interrupted by a long
frame, invalid infinite time, and fresh uninterrupted recovery.
In addition, the actual Unreal not-ready / pause / focus / menu branch
calls `Route.SuspendObservation()` to clear both stall windows.
Otherwise no route observation would occur during pause and 7.9 seconds
of earlier stationary or moving stall could carry into the first
resumed frame. The suspension preserves previously verified gate and
lap counts, recovery budget and any pending issued teleport ledger.
Native UE behavior remains unverified.
Unreal compilation and actual stalled-car recovery remain unverified
until native self-hosted UE5.8 playtesting.

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
   through the owner-controlled UE5.8 real-device validation workflow
   and record the specific PR head SHA and installed game evidence.

No source-only test proves installed gameplay.
