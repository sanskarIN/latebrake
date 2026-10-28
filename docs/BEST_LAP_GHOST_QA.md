# Run-local best-lap ghost beacon — source QA

## Why this exists

A driving-line reference is more useful than another results statistic.
After the player successfully drives a complete **eligible, ordered-gate,
penalty-free** lap, LateBrake can replay the real observed position and yaw
from that faster lap as a **small floating non-colliding beacon** on later
laps of the **same attempt**. It is not an AI racer, a transparent car,
a persistent personal-best ghost, a network replay or a physical vehicle.
No third-party game art, server or AI API is involved.

The source provides an engine-independent fixed-memory C++20 recorder
(`Racing/BestLapGhost.h`) and an Unreal GameMode integration with the
existing car pawn, RaceProgress and guarded race-start clock.

## Acceptance and invalidation boundaries

- GameMode samples the actual possessed, physics-ready local player
  transform only after valid post-GO active race-clock advancement.
- Each lap starts independently at a near-zero clock; a late first
  sample cannot be interpreted as a full lap.
- Samples are stored at most about 10 times per second, up to 4,096
  frames and 360 seconds. Sampling never calls an API or allocates heap
  memory per tick. A long hitch, nonmonotone clock, malformed transform,
  impossible sampled jump or capacity exhaustion invalidates the
  **entire candidate lap**, not an existing complete best lap.
- Pawn replacement, focus/menu/pause session changes and the pawn's
  teleport/recovery revision invalidate that lap. The best completed
  lap may still be displayed after a later resumed clock, without
  crediting the interrupted attempt.
- Only `RegisterCheckpoint` after a real accepted final ordered gate
  can finalize a candidate. It requires the actual RaceProgress lap
  duration, an active clock update and no recovery penalty. A lap with
  only penalty time or no successful active time is rejected.
- A slower lap cannot overwrite the existing faster trace. Restart,
  new career event and initial BeginPlay clear the entire ghost state.
  No saved profile, career awards, records or leaderboard fields store it.
- Each render frame uses bounded binary time lookup and shortest-angle
  yaw interpolation. A ghost is hidden after its own best-lap duration
  rather than freezing indefinitely ahead of the current car.

## Current prototype rendering

Unreal spawns a single mobile `AStaticMeshActor` only after a valid
best lap. The marker uses the original engine cube art, scaled to a
small floating beacon and positioned about 130 cm above recorded
chassis pose. It has **no collision, overlap events, physics simulation,
navigation effect or dynamic shadow**. The marker is *not a full
translucent ghost car*. Material coloration is an optional fallback
and depends on the built-in engine material exposing a Color parameter;
without it the shape may appear in its default material. The desktop
HUD and Results also label it as **RUN ONLY** and **UNOFFICIAL**.

An actual translucent/outline vehicle model, user settings to disable
the feature, memory/performance profiling and real replay rendering
are later work. The fixed 2 x 4,096 double-precision pose buffers
are approximately 320 KiB per game mode even with no active ghost.

## Portable/source verification

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^BestLapGhost$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'BestLapGhostBindingTests.py' -v
```

Portable C++ tests cover first/last playback, interpolation and yaw wrap,
best replacement, slower lap rejection, menu/recovery invalidation,
invalid coordinates and race-clock input, penalties, non-active laps,
max-duration and restart. Source binding tests verify the real player
pawn, same authoritative clock, ordered checkpoint transition, transient
actor, zero collision and lack of saved score authority.

## Unreal 5.8 and installed Windows/Android QA still required

1. Compile the genuine Unreal Editor/Game through UHT, cook the
   original circuit, and install a Development game package.
2. Drive a valid first lap, then confirm a single small marker follows
   the prior trace at proper speed without clipping into track or
   obscuring braking/gates. Test 0/low frame rate and different
   camera positions.
3. Drive a faster second lap, slower third lap, then replay only while
   a new valid lap is running. Note that the default three-lap circuit
   may require starting another event to inspect future refinements;
   the ghost intentionally does not persist across restart.
4. Pause/resume, minimize/reactivate, recover/teleport, restart, finish,
   change event and replace/respawn pawn. Verify an interrupted lap
   never becomes the best ghost and the beacon clears whenever needed.
5. Verify race clock, checkpoints, career credits, garage, SaveGame,
   player-rival contact, AI and drive forces are unchanged.
6. Profile renderer/performance on Win64 and multiple real Android
   phones. A functioning CMake binary is not the packaged game.
   Archive genuine installed test observations and logs.

**Status: source-only, unverified visual gameplay experiment.**

## Optional post-v0.5.79 desktop ghost visibility control

A distinct source development branch adds press-only **G** keyboard
and **left gamepad face-button** bindings. While the real race clock/input are active and the player car
is verified, pressing G hides or restores the floating visual marker.
The recording policy and existing in-memory fastest lap are retained.
Menu/pause/focus-lost/finished/invalid-player commands are refused.
The feature is not a race-speed modifier and does not overwrite a
personal-best result, saved preference or garage state.

The current implementation supports desktop keyboard and gamepad;
there is no Android touch-region button, settings persistence or
translucent full-car ghost has been added. Test G/gamepad toggling during an
actual installed Win64 race with a valid best lap and after pause,
menu, OS focus loss, recovery and restart. Verify the marker vanishes
immediately without needing another Tick and reappears only after
explicit re-enable and valid replay conditions.

## Android ghost visibility pad — next-cycle source candidate

The non-colliding best-lap beacon can be hidden/shown during an eligible
active race using **G** on keyboard, the left face button on gamepad, or
the new on-screen **GHOST ON / GHOST OFF** pad on Android. The pad sits
directly below RECOVER in the right action column, with a neutral gap
before GAS. It does not displace steering, gas, brake, pause, recovery,
restart or the existing CAM pad. The same pure C++20 rectangle definition
feeds hit testing and Unreal HUD drawing.

Only a fresh physical touch-down in the dedicated rectangle is eligible.
Moving a held gas/steering finger into the pad, duplicate presses and
touches while the menu, countdown, finish or OS focus guard owns input
must not toggle the beacon. The GameMode independently rechecks the
authoritative green start, actual human driver and pause/focus state.
Hiding a beacon never deletes an eligible best-lap trace, and the
preference never changes physics, checkpoints, lap times, credits or saves.
A touch release merely frees its physical-down identity.

Portable QA:

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^(GhostTouchPolicy|TouchControlLayout|GhostDisplayPreference)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'AndroidGhostTouchBindingTests.py' -v
```

**Real Android qualification still required:** confirm readable/high-DPI
landscape and portrait positions, OS touch event delivery, multi-finger
gas/steer plus GHOST interactions, pause/focus interruption, and whether
the pad obstructs HUD or track view on low-resolution devices. Also
confirm the beacon is never physically collidable and that input never
updates scoring or saved records. This is source-only; no APK or real
device observation is claimed.

## Next-cycle read-only ghost separation — source candidate

The expanded **non-compact** HUD may show **GHOST DIST** in meters or feet
(based on the player's existing speed unit setting). This is the 3D distance
between the real player's chassis and the recorded best-lap pose evaluated at
the **same elapsed lap time**. It is **not** a lap time delta, signed race
lead/lag, racing classification or official result. The marker's decorative
130 cm vertical rendering offset is deliberately excluded from the distance.

The independent, bounded C++20 computation rejects NaN/infinity, out-of-world
coordinates and separation greater than 250 m. Unreal GameMode additionally
requires a visible, actually spawned beacon, valid chassis and circuit,
completed prior lap, active GO race, and no menu, focus loss, pause or finish.
No value is stored, persisted or forwarded into car physics, timing, rewards,
career progress or ghost pose recording. Compact Android screens keep their
official timing and control rows rather than adding this optional text.

**Required real UE/installed acceptance before playable claims:** use an
authored cookable track and qualified Win64 build, complete a clean full lap,
confirm live ghost distance follows a physical player moving relative to the
beacon, and verify both metric/imperial displays. Toggle ghost OFF, pause,
switch to menu, lose focus and finish: the number should disappear immediately
or on the next eligible HUD paint as appropriate. Check that teleports,
off-track recovery, invalid physics and a missing beacon cannot display a
stale distance, and that the timing/results/profile are unchanged. Run the
same visual/layout test on real Android hardware. Portable policy and source
contract tests are **not** UE compilation or installed gameplay QA.
