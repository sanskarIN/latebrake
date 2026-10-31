# Rival surface and impact-aware pace — source-only gameplay QA

**Status:** Implemented in the autonomous physical rival's C++ code.
Portable tests and source bindings are **not** an Unreal Engine 5.8/UHT
compile, a cooked original circuit, installed Windows gameplay, or an
Android device test.

## Motivation

The existing rival already observes genuine tire wear, speed, lateral
slip, corner geometry and the player's moving car. However the AI's
throttle/brake decisions previously did not explicitly account for the
same confirmed dirt multiplier or chassis-collision damage that reduces
the *physical* car's lateral damping. On dirt or after a severe contact,
the AI could continue requesting cornering speeds calibrated for
healthy pavement.

`RivalSurfaceDamagePace` is a conservative addition to the existing
physical driving path. It **does not** invent terrain information, alter
steering or change the official racing clock.

## Production integration

The subclass `ALatebrakeRivalCar::Tick` reads the *previous qualified
chassis frame's* measured `ArcadeSurfaceTraction` state and
`ArcadeCollisionDamage` grip multiplier. It runs the following pipeline:

1. Authoritative race start, real Chaos physics, valid local player and
   circuit gates.
2. Approximate rival waypoint progress, recovery and legitimate
   corner anticipation.
3. Tire/slip-aware `RivalGripPace` (existing).
4. **Confirmed off-pavement and impact-aware `RivalSurfaceDamagePace`**
   (new; may only reduce throttle or apply service braking).
5. Real player-position/velocity traffic-aware braking (existing).
6. Existing 0.18-second following-brake hold and
   `SetAutonomousControls` into the physical base pawn.

Every source of low-grip risk is independently bounded:

- Dirt must be **previously confirmed** by the existing pavement
  hysteresis, with its authoritative 0.72 lateral grip multiplier.
- Impact grip must come from previously accepted chassis contacts,
  remain between 0.856 and 1.0, and only triggers damage-aware
  corner caution when it is at most 0.95 and actual steering demand
  is at least 0.35.
- Invalid, stale or nonfinite samples **fail closed** to zero throttle
  and ordinary braking, with no invented acceleration.
- On real confirmed dirt the AI begins braking when measured speed
  exceeds its 1,300–1,900 cm/s corner target plus 180 cm/s.
  On severely damaged paved corners a separately bounded
  1,850–2,800 cm/s target applies.
- Short 0.12-second braking hysteresis avoids instant brake chatter.
  Low-speed recovery below 650 cm/s retains normal launch throttle,
  so a slow or stranded rival is not permanently blocked.

These are **source-level policy targets, not guaranteed real-world lap
speeds**: Unity-style scripted movement is not used; real Chaos forces,
track contact and driveability still need installed testing.

## Invariants and tests

`Tests/Standalone/RivalSurfaceDamagePaceTests.cpp` checks:

- healthy paved corner stays unchanged;
- confirmed fast dirt applies conservative braking;
- a short hold expires, bounded throttle returns and low-speed
  dirt recovery is not stuck;
- collision grip makes risky damaged turns more cautious, but a
  damaged straight remains free;
- earlier route braking and checkpoint arrival remain authoritative;
- stale road/damage states, nonfinite speed/slip, impossible grip,
  inconsistent dirt flags, malformed frame time and commands fail closed;
- **200,000 deterministic valid data combinations** preserve steering
  and never increase the rival's requested throttle.

The editor-source binding suite verifies the actual Unreal rival reads
the existing surface/impact objects after tire pace and before
player-traffic avoidance. Pause, restart, replay configuration, failed
or successful recovery and completion clear transient braking state.

## Still required before claiming this is playable

A licensed UE 5.8/UHT game build and real project-owned circuit cook;
Windows gameplay with a real AI rival recovering from grass/dirt,
damage and corners; player overtakes, real contact response, race reset,
frame-time behavior, win/loss results and progression; hardware Android
playtesting; and an evidence packet satisfying
[playable-release gate #148](https://github.com/sanskarIN/latebrake/issues/148).

No source-only CI success is a substitute for these tests.
