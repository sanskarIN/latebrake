# Real Unreal start-line gantry — 3/2/1/GO visual QA

**Status: source change, not a verified UE game.** The canonical
`RaceStartLights` still gates all actual race clock time, wheel forces,
player checkpoints and rival controls. A small original asset-free circuit
gantry is only a cosmetic representation of that already qualified phase.

## Planned physical installation

A new non-colliding gantry beside the original south-straight starter
grid is built from Unreal engine's BasicShapes, 3 red point lights and a
green GO point light. Three red lamps are visible with 3 seconds remaining,
then two, then one. A green light appears only during the existing short
GO phase. After the GO announcement, lights switch off, rather than
claiming a second start signal or holding a misleading permanent green.

A not-ready circuit, absent/unverified local driver, paused/menu/focus
loss, failed restart, or finished race turns off the dynamic lamps. In
game-state-pause scenarios the explicit mode interruption handlers also
hide them so the last green/red state cannot remain misleadingly visible.

All posts, beams, bulb meshes and lights have **no collision**. A mesh
failure must not change the authoritative nine collision-bearing
blocks, the four required checkpoints, road safety audit, car physics,
career saves, awards or AI progress.

## Portable automated checks

`StartGantryVisualTests.cpp` validates every phase and every failed
readiness combination, pause freezes, crossing the exact GO boundary,
racing expiration, invalid phases, and race restart. Python UE source
bindings verify the actual GameMode builds and refreshes the fixture
without creating a second countdown timer or bypassing pause guards.

## Mandatory real 5.8 Editor / installed QA

1. Run genuine UHT and compile the Editor/Game targets with UE 5.8.
2. Open the authentic cooked `/Game/Maps/LatebrakeCircuit`, examine
   the gantry alignment over the original start and its distance from
   vehicle collision, steering/touch inputs and the four gate planes.
3. Observe 3/2/1/GO with the game HUD in regular Circuit, solo Time
   Attack and career starts. Check first frame of GO and the transition
   to racing at different frame rates.
4. Pause or lose focus at every light phase, resume explicitly, restart,
   and trigger an unready grid/failed chassis state. No signal may skip
   ahead or falsely show GO while race timing is blocked.
5. Verify screenshots, point-light appearance in daylight and mobile,
   dynamic light performance, readability and colour accessibility.
6. Record exact source SHA, engine build, packaged Win64 EXE and user
   hardware evidence. This work does **not** close the genuine playable
   release gate [#148](https://github.com/sanskarIN/latebrake/issues/148).
