# v0.5.75 race-start lights — source and engine QA

## Genuine new race-start behavior

The game now uses **one authoritative three-second start sequence**
with a finite, allocation-free portable timing policy. A qualified,
unpaused race displays **START LIGHTS 3 / 2 / 1 / GO!**. The actual
`ALatebrakeRaceGameMode` gates four real game systems:

- **Authoritative race clock:** each red frame returns zero elapsed race
  time. When a frame crosses GO, only the fraction *after* the third
  second reaches `RaceProgress`. Thus no premature lap clock or
  unearned active-timing evidence is fabricated.
- **Checkpoint and recovery:** the real checkpoint actor refuses and
  **clears directional motion sampling during red**, preventing a
  pre-green trajectory from spanning into the first green gate. The
  GameMode independently rejects scoring, and no charge/teleport
  is permitted before green.
- **Player controls and rival:** both player chassis propulsion and
  autonomous rival drive are disabled until the *same* green state.
  Neutralization continues for any failed restart-grid, missing
  circuit, pause, focus-loss or physics-readiness condition.
- **Pause/focus and restart:** the countdown does not advance while
  the menu/world is paused, focus is suspended, the original circuit
  is incomplete, the current pawn/verified restart grid is missing,
  or the event is finished. Accepted event restart resets all lights
  and race elapsed time. No SaveGame schema or credits change.

A short GO cue remains visible for approximately 0.85 seconds of
qualified running time. Unreasonably long (>0.5s) or invalid frames do
not consume start-sequence time; this protects against timer jumps,
though very low frame-rate devices need genuine performance testing.

## Portable and Unreal-source tests

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^RaceStartLights$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

`RaceStartLightsTests.cpp` links the **actual production**
`RaceProgress.cpp` and verifies 3→2→1→GO, pause/hitch exclusion,
fractional frame allocation on the GO boundary, post-green clock
growth, reset and zero elapsed seconds before the race starts.

`RaceStartLightsBindingTests.py` checks production Unreal C++ source
uses the *same* start gate to authorize GameMode clock, ordered
checkpoint, recovery, player tick, rival tick and Canvas HUD, not an
unlinked display-only animation.

## Essential Unreal and device acceptance (not yet performed)

1. Build with licensed UE 5.8 Editor/UHT and Game targets; author and
   cook the actual project-owned `LatebrakeCircuit.umap`; record hashes.
2. From Quick Race, Career and Time Attack, observe 3/2/1/GO pacing,
   both player and rival at the verified grid without premature motion.
   Trigger gates during red (if possible) and confirm no official
   gate/lap/credit or recovery charge is granted.
3. Interrupt during each light via menu, world pause, OS focus-loss
   and Android background/foreground. Countdown must freeze, never
   resume itself from a focus-lost world and never deliver hidden time.
4. Restart from mid-countdown, live race, paused view and Results.
   Check exactly one new sequence, real car placement and a newly
   zeroed race clock, with unchanged previous saved bests.
5. Test real frame hitches, high latency and 30/60/120Hz motion.
   Verify a GO-boundary frame accrues only its post-green fraction
   and engine tick order cannot give player or rival a head start.
6. Test Android portrait/landscape, split touch controls, high DPI,
   HUD clipping and SAVE FAILED warning reservation. The countdown
   row must not overlap physical steering/gas/brake regions.
7. Attach installed Win64/Android package SHA, UE logs, screenshots
   and controller/real gameplay results to
   [playable qualification #148](https://github.com/sanskarIN/latebrake/issues/148).

**This source milestone is NOT proof of real UE compilation, installed
playable packages, physical device quality or a stable game release.**
