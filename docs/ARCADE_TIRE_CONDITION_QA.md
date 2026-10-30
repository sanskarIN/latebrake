# Arcade tire condition — source-only prototype and QA

**Source-only, not playable.** This is an arcade approximation of tire
condition based on **observed chassis lateral motion**, *not* actual tire
temperature, tire compound friction, wheel load, tire slip angle,
suspension or damage. Real Unreal Engine 5.8 / UHT / Win64 / Android
compilation and installed gameplay remain unverified.

## Driving behavior

- Both the human and existing CPU rival use the same per-pawn race-local
  tire condition policy. At every valid driving tick, the system reads
  measured planar and lateral chassis velocities, genuine brake demand
  and elapsed frame seconds; malformed values or a frame longer than
  100 ms freeze the policy without a fictitious wear update.
- The prototype begins at a neutral temperature of 42% and zero wear.
  Repeated lateral sliding increases calculated heat and wear. Brake
  use contributes to heat and wear. Ordinary straight-line travel
  gradually cools toward neutral, but never **repairs** wear.
- The computed condition scales **only the existing bounded lateral
  chassis damping**, never actual tire friction. Existing frame-stable
  force safeguards remain in charge of the final physics resistance.
  Grip multiplier is constrained to [0.68, 1.00]; zero, negative,
  infinite or arbitrary driving impulses are not introduced.
- Run-local wear is retained through *recoveries* and paused sessions,
  and resets only for a new race/event. Both player and retained rival
  have explicit race start reset hooks. No pits, tires, purchases,
  credits, official timing, lap records or persistent save fields
  are modified.
- Expanded desktop HUD shows **ARCADE GRIP** and **TIRE WEAR** after
  the authoritative race clock. Compact/touch HUD stays unchanged.

## Cross-platform source checks

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release --parallel
ctest --test-dir Tests/Standalone/build -C Release -R ArcadeTireCondition --output-on-failure
python3 -m unittest discover -s Tests/Editor -p ArcadeTireConditionBindingTests.py -v
```

The test exercises invalid frames, NaN/Infinity, impossible lateral
motion, pause retention, gradual wear, saturated heat, cooling,
5,000 deterministic stress sessions and race reset. Source binding tests
check live Chaos speed inputs, both driver types, bounded lateral
damping, reset ownership and mobile HUD exclusion. These **do not**
constitute an Unreal Engine build.

## Required real gameplay acceptance

1. Compile licensed UE 5.8 UHT, Editor and Game targets and cook the
   actual project-owned map; record exact source/engine provenance.
2. Compare identical three-lap sessions with clean straight driving
   versus repeated high-slip/drift driving. Verify natural, predictable
   traction changes and no sudden force reversal at 30/60/120 Hz.
3. Restart a race and verify both player and rival recover fresh
   condition. Recover only the player and verify wear is *not* repaired.
   Test Time Attack, career events, rival reconfiguration, ghost and
   saved record integrity.
4. Test pause, OS focus loss, large frame spikes, terrain collisions,
   zero-velocity and malformed-physics recovery paths. Watch for HUD
   stale data after interruptions and invalid wheels/scene objects.
5. Inspect compact mobile HUD, actual controller handling, frame-time,
   CPU and memory; retain real logs, hashes and repeatable test notes.

Until these gates and
[playable qualification #148](https://github.com/sanskarIN/latebrake/issues/148)
pass, the feature and its source archive are **not playable** and
must not be advertised as realistic tire physics.
