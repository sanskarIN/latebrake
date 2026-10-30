# Physical-rival slipstream drafting — source-only racing feature

**SOURCE-ONLY C++20 and Unreal source integration; not playable.**
No Unreal Engine 5.8/UHT compilation, real cooked original circuit or
installed Windows/Android driving qualification is claimed. This is
a bounded **arcade drafting effect**, not aerodynamic simulation,
multiplayer positioning or a source of official race results.

## Design and source behavior

- Only the **locally controlled human** in a green-light, eligible race
  can benefit. The opponent must be the real existing and physics-ready
  moving rival; time attack (without rival), a missing/finished AI car,
  pause, menu, invalid placement, race finish, focus loss and invalid
  rigid-body physics never create phantom draft bonuses.
- The player and rival must travel forward at least 12 m/s and 10 m/s
  respectively. The relative geometry must be within **7–38 m ahead**,
  less than **3.8 m lateral** and **2.3 m vertical** separation, with
  heading alignment above 0.94 and forward throttle at least 30%.
- Real measured relative speeds/positions/directions are supplied to
  an engine-free production C++20 policy. If the player closes more
  than 10 m/s faster than the rival, or either brakes/reverses/moves
  improperly, boost immediately clears.
- Drafting must remain continuously eligible for **0.18 seconds** before
  granting help; an invalid frame, stale/missing rival, teleport or
  input interruption clears the entire confirmation window.
- A qualified position yields at most **+6% additive motor force**.
  Greater distance or lateral separation reduces the benefit. The
  existing baseline motor force, frame-stable damping, top-speed cap,
  steering, service/hand brakes, car damage/condition and AI controller
  remain authoritative. This does **not** alter saved career data,
  credits, lap time clocks, legitimate gate awards or official records.
- When nitro and drafting overlap, the independent bonuses share a
  single **40% total extra motor-force cap**. Nitro has priority because
  it consumes limited charge. Drafting can contribute only the remaining
  share (as little as 2% when nitro grants its full 38%). The HUD shows
  the **actually granted** drafting percentage, not the unrestricted
  geometric eligibility estimate.
- [Cross-mechanic budget tests](../Tests/Standalone/ArcadeBoostArbitrationTests.cpp)
  reject nonfinite inputs and excessive additive force.

- An optional **SLIPSTREAM** active readout appears only in the
  expanded desktop race HUD. Compact phone controls and authoritative
  race rows remain unchanged.

## Build-independent source checks

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release --parallel
ctest --test-dir Tests/Standalone/build -C Release -R SlipstreamDraftBoost --output-on-failure
python3 -m unittest discover -s Tests/Editor -p SlipstreamDraftBindingTests.py -v
```

Tests reject malformed/time-hitch samples, NaN/Infinity position and
motion, reverse/too-distant/oncoming rival, wrong lane, vertical
separation, denied driver states, braking, low speeds, invalid headings,
and over-fast approach. They also check a 5,000-run stress sample, all
real racer/unreal binding guards and reset paths. This verifies only
portable logic and static Unreal-source wiring.

## Required real-engine and installed-game tests

1. Compile licensed UE 5.8 targets with actual UHT; cook an original
   circuit and record exact editor/package/engine/source SHA256 hashes.
2. Test sustained drafting behind the real AI on straight segments
   with measured gap and heading. Confirm extra acceleration is
   repeatable but bounded at both 30/60/120 FPS and different garage
   configurations. Verify driving feel and stability.
3. Check starting grid, countdown, menu, manually failed pause,
   focus loss, vehicle recovery, fresh restart, opposing headings,
   AI route completion and Time Attack. No phantom boost may survive.
4. Compare control/input handoff, standalone engine car physics,
   motor-force application and normal steering/braking on Win64.
   Confirm no collision or vehicle penetration is needed to draft.
5. Verify this cannot affect legitimate lap/checkpoint validation,
   game saves or credit awards; preserve genuine engine logs and
   observed hardware results. Test Android rendering separately.

Until verified on installed packages and
[playable milestone #148](https://github.com/sanskarIN/latebrake/issues/148)
is qualified, this remains an unverified **source-only** feature.
