# Real rival straight-line passing assist — source QA

**Status: Unreal player/rival source integration, NOT a playable,
verified or UE5.8-compiled game.** Only the existing original single
physical rival can attempt this manoeuvre.

## Gameplay improvement

The original AI slowed behind a slower player but never tried to
pass, leading to repeated courtesy-brake following and occasional
deadlocks. A new `RivalStraightPassAssist` can briefly steer toward a
second lane on the **existing original paved four-gate circuit**.

This is real steering input used by the Unreal AI actor, not a
decorative indicator. The current four checkpoint planes, lap clock,
career rewards and player-controlled vehicle stay unchanged.

The new limited assist:

- Requires the same **actual live player and rival Chaos actor
  positions, velocities, race eligibility and previously qualified
  tire, road and collision state** already used for safe driving.
- Only starts on a sufficiently long original paved straight if a slower player is
  **1600–2400 cm ahead**, the rival is closing by at least **300 cm/s**,
  both vehicles are largely aligned with the road and at least
  **2150 cm remain before the next corner**.
- New passes require **at least 1.8 seconds** before the measured
  closing motion reaches the conservative contact margin, and at least
  4200 cm of runway before the next corner. On the original circuit
  the two shorter north/south straights are excluded from passing.
- Selects the road lane **opposite the player's measured offset**;
  deterministic tie-breaking avoids oscillation. Lane aim remains
  **440 cm from the centreline**, inside the existing 800 cm road
  half-width. A local, 1000 cm ahead steering target causes an actual
  directional change rather than an imperceptible far-gate offset.
- Caps requested passing steering at **0.65 axis**; never increases
  throttle, directly modifies physics velocity or claims a gate.
- Keeps a **per-rival lane choice** until the rival has drawn more
  than 550 cm ahead, reaches a corner or six seconds have elapsed.
  Qualified pause, restart, recovery and race-finish branches reset it.
- Cancels the assist with normal braking if the player enters the
  selected pass lane. Invalid measurements fail closed. The original
  `YieldRivalToPlayer` collision-courtesy braking still runs AFTER
  the attempt and may reduce acceleration/activate ordinary braking.
  If it activates braking, the new `ResolveRivalPassTraffic` restores
  normal route steering and cancels the active passing lane state.

This is NOT claimed to be autonomous guaranteed collision avoidance,
multiplayer, official racing placement or a multi-opponent race.
Actual Chaos body orientation, wheel traction and road boundaries
still require evaluation with a real installed Windows game.

## Regression checks

`Tests/Standalone/RivalStraightPassAssistTests.cpp` checks
both long directional straights and safe refusal on short straights, right/left pass selection,
lateral approach and stable target, finished pass, unsafe traffic,
off-pavement/corner suppression, previous brake priority, malformed
physical inputs, focus interruption and 20,000 road observations.

`Tests/Editor/UnrealRivalStraightPassBindingTests.py` asserts
that the actual rival pawn supplies observed positions/velocities,
preserves all normal road/traffic/corner/grip filters and resets
passing state on invalid route, recovery, pause, restart or finish.

These are source and portable C++20 verification only.

## Actual UE 5.8 Win64 hardware acceptance requirements

1. Compile the actual game through UE5.8/UHT, cook the owned map and
   install the actual **3D** Win64 package. The separate experimental
   2D Windows preview is not valid evidence of this feature.
2. Follow the rival on all four straights. Put a slow player ahead
   at offsets of -150, 0 and +150 cm, including same-pace, near stopped
   and suddenly changing lanes.
3. Confirm an actual physical lane transition, plausible passing
   time, no wall/grass entry and return to the verified next gate.
4. Confirm no brake/steer oscillations, collisions or unqualified
   extra acceleration across 30/60/120 FPS.
5. Test both handling modes, worn tires, heat/damage, off-road,
   focus loss, pause/resume, restart, recovery, event finish and
   player pawn replacement. Measure race gate completion and stalls.
6. Review original pass-lane spacing relative to rendered chassis
   and collision geometry; tune/disable passing if two vehicles do
   not genuinely clear each other.

The **verified playable build** release gate
[#148](https://github.com/sanskarIN/latebrake/issues/148)
remains open.
