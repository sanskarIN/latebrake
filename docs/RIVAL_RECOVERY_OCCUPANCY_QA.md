# Physical rival recovery traffic-clearance QA

**DRAFT — source-level preflight only, not a verified Chaos collision.**

This candidate adds a *blocking* condition before the existing rival
`TryRecoverTo` in the original Unreal two-car race. It must not be
described as a functioning six-car physics or collision system.

## What the real code changes

- Use the currently possessed human pawn's **actual** Chaos body readiness,
  world-space XY location, heading, velocity and real actor identity.
- Before a rival recovery attempt, demand a valid player observation. Invalid,
  missing or nonfinite data **denies the teleport** rather than assuming the
  road is empty.
- Require the entire proposed 380 x 180 cm padded vehicle footprint (+45 cm
  on every side) to fit one of the original four physical asphalt slabs.
  Enforce existing barrier clearance too.
- Use a **continuous swept separating-axis** intersection between the
  static proposed landing and the player's short constant-velocity 0.4-second
  projected footprint. Collision at any point in the interval denies the
  proposed recovery, including overlap between rendered frame samples.
- A rejected preflight calls the existing
  `Route.ReportRecoveryResult(false)` exactly once, consumes the already
  issued finite recovery attempt and applies a safe brake command. It does
  not attempt any Unreal teleport. If clear, the existing independently
  verified `TryRecoverTo` still controls actual engine placement.
- The source API can accept up to five distinct real blockers (the human
  and four future rivals), rejects duplicate/missing actor tokens and never
  conjures phantom blockers. **Only one actual human blocker is connected
  by the live Unreal source today.** No additional AI pawn is spawned.
- No new world trace, collision shape, race score, player recovery fee,
  save schema, physics force or multiplayer authority is introduced.

This is a conservative linear-velocity *preflight*, not full dynamic
collision prediction. It does not model turning, jumps, suspension or
multi-car Chaos contacts, and cannot replace real installed gameplay QA.

## Standalone verification

    cmake -S Tests/Standalone -B Tests/Standalone/build
    cmake --build Tests/Standalone/build --parallel
    ctest --test-dir Tests/Standalone/build -R '^RivalRecoveryOccupancy$' --output-on-failure
    python3 -m unittest discover -s Tests/Editor -p 'RivalRecoveryOccupancyBindingTests.py' -v

Regression cases include static and moving overlap, fast cross-through,
moving-away motion, all four authored recovery locations, off-asphalt
footprints, malformed speed and identities, multiple physical blockers,
and 3,600 rotated-body samples.

## Real UE5.8 and installed Win64 acceptance required

1. Build UHT and Editor/Game targets from this exact commit using licensed
   Unreal Engine 5.8; cook and load the actual owner-authored map.
2. Physically park the human car over each rival recovery destination.
   Trigger a genuine stall recovery; verify no rival teleport occurs,
   exactly one failed attempt is recorded, and neither car is rewarded.
3. Drive the human car across a pending recovery spot at varying speeds,
   including low-speed stationary blocks and a fast approach. Verify the
   conservative sweep rejects potentially occupied landings while clear
   ones can still pass the real `TryRecoverTo` verification.
4. Test invalid player pawn, loss of physics simulation, pause, focus
   interruption, restart and event switches. No stale actor ownership or
   speculative empty road may authorize a teleport.
5. Measure real 30/60/120 FPS behavior and worst-frame cost. Audit actual
   forward and rear contact; the source forecast does not model jumps or
   changing heading. Record engine/build/map hashes, logs and gameplay video.
6. Extra AI cars must not be enabled until their *own* physical motion is
   included as separate blockers and independent route/traffic systems
   receive the same installed qualification.

Keep [playability gate #148](https://github.com/sanskarIN/latebrake/issues/148)
open. This policy is stacked on [six-slot grid PR #329](https://github.com/sanskarIN/latebrake/pull/329);
it does not alter the frozen v0.5.98 physics source candidate.
