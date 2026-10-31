# Rival directional checkpoint gates — source-only gameplay QA

**State:** Production source code wired into the existing physically
controlled Unreal Engine 5.8 rival. These source tests do **not**
compile licensed UE 5.8/UHT, verify an actual cooked circuit, produce a
Windows EXE or Android APK, or certify the game as playable.

## Problem

Previously, the approximate autonomous rival route advanced its next
waypoint whenever the car was within **440 cm** of that waypoint. This
could count a rival lap after approaching the gate from behind, driving
past it in reverse, or remaining near the checkpoint without a real
forward crossing. At high speed the rival could also cross the actual
gate plane between frames while finishing outside that 440 cm sphere,
making the AI erroneously revisit the gate.

## Real chassis integration

The existing player checkpoint actors already use
`CheckpointMotionTrace` and `EvaluateCircuitGatePlane` to require a
genuine forward 3D gate-plane crossing. The rival now owns its own
`CheckpointMotionTrace RivalGateTrace` member. On every authorized
physics tick, `ALatebrakeRivalCar` samples its **real actor X/Y/Z**,
**actual world-planar chassis velocity**, and **correct next gate index**.

Only a `RivalGateTrace.Sample(...)` result of true can advance the
rival's route through `Route.ObserveVerifiedGate(...)`. The check
retains all existing requirements from the player gate geometry:

- Correct **forward** movement and forward plane transition, not just
  heading/position or being inside the checkpoint marker.
- Crossing within **500 cm** of the gate's lateral center.
- Crossing inside the gate's **380 cm** vertical half-height around
  the original marker center at Z=38.
- At most **1,200 cm** of actual displacement between accepted
  consecutive samples.
- Finite position and world velocity; invalid physics cannot preserve
  a previous crossing baseline across the bad frame.
- Ordered waypoint index, with no duplicate lap credit while a
  physical recovery is pending or the route has completed.

A genuine forward segment can be counted even if its **final position
is outside the old 440 cm radial test**. This avoids missing an actual
crossing on a longer physical frame.

The trace is reset on race reset/configuration, race pause/focus/menu
denial, accepted waypoint advancement, recovery attempt, and completed
route. It never bridges a teleport or session discontinuity.

The earlier engine-independent distance-only `RivalRouteProgress::Observe`
method remains intact for legacy portable callers, but the **actual
Unreal rival no longer uses it for waypoint credit**. It calls the
stricter `ObserveVerifiedGate` method. Real distance and speed still
feed the existing low-speed and moving-no-approach recovery detectors.

The rival's route remains an **approximate AI diagnostic**, not a
player-authoritative result. Player checkpoint timing, credits, race
awards, ghost data, career progression, physical steering, and saving
are not affected.

## Source-level qualification

The new `RivalDirectionalGateTests.cpp` compiles the **actual
production** route policy and the shared real player gate geometry
without importing Unreal. It verifies:

- A complete two-lap series of correct gate crossings, with no
  completion before the required last gate.
- A genuine crossing that finishes **500 cm beyond** the gate still
  advances the rival.
- Near-center contact without crossing, moving backward, wrong-way
  physical velocity, out-of-lane/too-high passage, and teleport-sized
  jumps cannot award a gate.
- A stale trace cannot survive an explicit race/recovery reset.
- No route progression is possible while a recovery is unresolved;
  the existing verified-versus-rejected accounting remains intact.
- **200,000 deterministic physical gate-crossing pairs** check
  correct sequential advancement and no first-frame scoring.

`RivalDirectionalGateBindingTests.py` inspects the actual Unreal
source binding: true measured physics, correct next waypoint,
`RivalGateTrace.Sample` before route progress, no old
distance-only shortcut, verified recovery, and explicit resets.
The portable test matrix must pass on Windows/Ubuntu/macOS, along
with Unreal-source Python checks and the MkDocs build.

## Genuine device QA still needed

1. Build actual UE 5.8 UHT/Editor/game targets and cook the owned
   starter circuit, with a real map and engine receipts.
2. Drive an actual rival through each clockwise/counterclockwise
   approach as appropriate for the prescribed four-gate route.
   Check the true world velocity, plane normal, and Z height.
3. Cross valid gate planes between long frame samples; verify one
   ordered advancement even when the final car location is outside
   the old waypoint proximity radius.
4. Test arriving near the gate without crossing, reverse traversal,
   off-lane and airborne maneuvers, pause, recovery teleport, focus
   loss, race restart, traffic and frame hitches. Confirm no
   early/phantom rival finish.
5. Run physical installed Windows and Android sessions and record
   frame/pacing/lap counters and source/cook fingerprints under
   [playable qualification gate #148](
   https://github.com/sanskarIN/latebrake/issues/148).

**Source-level CI alone does not prove a real Unreal race works.**
