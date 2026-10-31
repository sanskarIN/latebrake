# Rival gate run-through driving — source-only gameplay QA

**Status:** The existing physical Unreal rival's C++ driving command now
keeps steering toward a paved aim point **through** its real next gate,
instead of declaring its distance-only waypoint "arrived" before
the racing gate plane is crossed. Not a UE 5.8/UHT compile, an authored
cooked circuit, an installed Windows game, or an Android APK.

## Real gameplay regression after directional gate scoring

The verified directional gate feature changed rival progress to require
a real forward **3D plane crossing**. But the existing
`PlanRivalTurn` still returned `{throttle=0, steering=0, arrived=true}`
when the car came within **440 cm** of the target. This could stop a
slow rival just *before* the real gate, preventing route advancement and
eventually forcing a needless recovery. The previous heading target was
also the gate center, encouraging the car to steer toward a point it
needed to drive *through*.

## Corrected physical rival path

- `RivalGateRunThroughPoint(nextGate)` computes a point **550 cm
  past** each of the four original circuit gates along the real inbound
  segment. All four points remain on the fixed circuit's paved road.
  It is a **steering target only**; actual gate geometry, timing,
  checkpoints, lap rewards and recovery positions do not change.
- The existing generic `PlanRivalTurn` now accepts an optional
  `allowProximityArrival` parameter, **true by default** to retain
  all existing distance-only callers. The real rival passes **false**.
- `ALatebrakeRivalCar::Tick` still uses real position, heading and
  planar speed for waypoint steering, plus corner anticipation, measured
  tire/surface/damage pace, player traffic and Chaos yaw moderation.
  Only the source of the next steering target and its proximity-arrival
  eligibility change.
- The earlier directional `RivalGateTrace.Sample` and
  `Route.ObserveVerifiedGate` still determine **when a rival lap may
  actually advance**. The new target cannot authorize a gate.
- A malformed real gate index is explicitly rejected by the rival:
  all transient steering/traffic/condition holds are cleared, the
  chassis is commanded to zero throttle/steer with braking, and it
  cannot drive toward the invalid `CircuitGate(-1)` origin fallback.
- The original pre-corner braking, lap completion, recovery teleports,
  player checkpoint timing, credits and player control remain intact.

## Numeric regression and Unreal source binding

`Tests/Standalone/RivalGateRunThroughTests.cpp` checks:

- Original default `PlanRivalTurn` behavior is unchanged.
- `allowProximityArrival=false` never returns `Arrived` based only
  on distance, including inside the old **440 cm** stop radius.
- All four steering targets point exactly **550 cm forward** through
  each correct gate and remain within the paved original layout.
- Low-speed approach/near/through/post-gate behavior keeps requested
  throttle and steering available without a false arrival event.
- The existing physical `CheckpointMotionTrace` accepts genuine
  forward swept crossings, independently of pilot proximity.
- High-speed corner braking and malformed-input fail-closed behavior
  remain in force, including invalid arrival radius.
- **200,000 deterministic approach samples** preserve finite and
  bounded steering/throttle, preexisting brake decisions and
  never claim a radial arrival for the real rival.

`Tests/Editor/RivalGateRunThroughBindingTests.py` verifies real
Unreal call order, invalid-index fail-closed behavior, the deliberate
no-proximity-arrival argument, directional route scoring and the
portable matrix registration. CI runs C++20 on Windows, Ubuntu and
macOS, Python editor source binding, release PowerShell checks and
MkDocs documentation.

## Installed gameplay acceptance still required

1. Build the actual UE 5.8 editor, UHT and game target with the
   owned, cooked circuit (not an imagined or placeholder map).
2. At each gate, drive an actual slow rival from 250cm before the
   plane. Verify it **does not halt** while still on the approach
   side, crosses forward once and then aims for the next gate.
3. Repeat at higher speeds and across frame hitches; inspect the
   physical Chaos steering/force response, corner overshoot,
   collision, traffic following and off-pavement safety.
4. Verify reverse, invalid-index, teleport, focus-loss, pause,
   restart, race finish and normal recovery cannot fabricate
   waypoint/lap progression.
5. Gather Win64 and Android installed-driving evidence for
   [playable release issue #148](
   https://github.com/sanskarIN/latebrake/issues/148) and
   [Android qualification issue #13](
   https://github.com/sanskarIN/latebrake/issues/13).

**Source-only CI passes do not certify a real playable race.**
