# Architecture — early foundation

## Runtime

- `ALatebrakeRaceGameMode`: owns track creation, persistent records and an engine-independent race-progress state object.
- `ALatebrakeCarPawn`: a starter body simulated by Unreal rigid-body physics; inputs produce forces, grip correction and yaw torque.
- `ALatebrakeCheckpoint`: sensor volumes that forward overlapping player car events to the game mode.
- `ALatebrakeRaceHUD`: draw-time presentation of speed and race state.
- `ULatebrakePlayerSave`: local player records stored via Unreal's SaveGame API.
- `latebrake::RaceProgress`: portable, deterministic, C++20 ordered-checkpoint/lap clock implementation.

The checkpoint rules are intentionally isolated from Unreal so CI compiles the same core logic without licensed engine tools. Engine and view code remains separate from this portable core.

## Runtime sequence

1. Unreal constructs the game mode and chooses a dynamically generated PlayerStart.
2. The default pawn is spawned and possessed by the local player controller.
3. The game mode creates its temporary circuit/markers from engine-built-in primitives.
4. Player input applies simulated rigid-body forces each tick.
5. Trigger overlaps call `RaceProgress::PassCheckpoint` in a strict sequence.
6. A complete third lap freezes race time; best result and completion count save locally.
7. HUD reads an immutable view of current race state.
8. Restart resets race and teleports the existing pawn to the circuit beginning.

## Engineering boundaries

- Current input mapping uses Unreal's supported legacy axis/action definitions for an assetless foundation. Move to Enhanced Input + remappable bindings for production.
- The starter controller relies on Chaos-backed rigid bodies, **not** the Chaos Vehicles wheeled movement system. Implement dedicated tires/suspension when original skeletal meshes and authored wheel setup exist.
- No multiplayer authority/security claims: game mode is currently designed for local single player only.
- Generated scenery is not a scalable world-streaming strategy. Replace with an original packaged map, level instances or World Partition as scope grows.
- Version and migrate saved profile data before introducing complex career/garage state.
- Keep offline core racing independent from future optional services.

## v0.2.0 event, AI and reward boundaries

- `latebrake::RaceEvents` is a portable, compile-time catalog; the Unreal game mode chooses event-specific lap counts and personal-best slots.
- `latebrake::RivalPilot` is a pure C++20 waypoint planner tested without Unreal; `ALatebrakeRivalCar` hosts its output and applies physics inputs.
- `ALatebrakeCheckpoint` checks the overlapping pawn is the real local-player pawn, not a rival, before crediting player race progress.
- `latebrake::RaceRewards` computes bounded nonfinancial offline credits, awarded once when the player's race transitions to finished. The SaveGame record stores accumulated credits.
- The rival is **experimental** and has no validated standings, collision avoidance, overtaking or synced race results.

## v0.3.0 garage domain

- `latebrake::GarageState` is an Unreal-independent C++20 domain model: immutable fictional vehicle definitions, owned bitmask, bounded credits, deterministic upgrades, validated snapshots and performance multipliers.
- `ALatebrakeRaceGameMode` is responsible for persistence and player-only application of garage tuning. Event completion feeds the garage's earned balance.
- `ALatebrakeCarPawn` accepts garage performance factors, restricting nonfinite inputs and applying them to its force-based controller. The prototype cabin silhouette is derived from a spec; no third-party car model is added.
- `ULatebrakePlayerSave` preserves the existing credits field and introduces optional garage properties with safe old-profile defaults.

No network payments, real-money entitlements or finalized garage UI are implemented.

## v0.3.1 validation boundaries

- `GarageState` handles negative ownership-mask recovery and transaction selection/quotes. The Unreal adapter publishes feedback but should not independently calculate prices or unlock order.
- `RecordValidation.h` is portable and bounds malformed saved race times plus saturates completion counts; Unreal SaveGame file IO remains untested by CI.
- `HudLayout.h` computes adaptive compact/normal layout measurements independently from Unreal; `ALatebrakeRaceHUD` formats game-facing text using those measurements.

## v0.3.2 input and live tuning boundaries

- `DriverInputSafety.h` owns portable normalization and throttle slew, shared by player and autonomous drivers; braking remains immediate.
- `ALatebrakeCarPawn` clears latent touch/axis commands on pause and recovery and prevents invalid frame-time/velocity values from entering force integration.
- `GarageTuningKey.h` derives the live physics revision from selected vehicle and engine/grip levels, excluding credits/ownership. Game mode caches the current weak player pawn plus key to avoid repetitive tuning setter work every tick.
- Failed local saves are flagged to the HUD and logged; they are **not silently treated as successful persistence**. Actual Unreal SaveGame IO and editor render testing remain outside standalone CI.

## v0.4.0 track definition and decoration boundary

- `Track/CircuitLayout.h` is the single portable source for four gate positions, starting poses, player and rival recovery points and asphalt coverage.
- `ALatebrakeRaceGameMode` spawns one temporary rectangular circuit using Unreal engine shapes and these shared gate definitions.
- `ALatebrakeRivalCar` uses the same gate and recovery coordinates for its deterministic waypoint pilot.
- `Track/TrackDecorations.h` plans cosmetic stripes, curbs and stands. The runtime spawns each with collision disabled, avoiding interference with car physics. Production performance work will consolidate them into instanced meshes.
- `Track/CircuitMap.h` translates world coordinates to clamped normalized positions; `ALatebrakeRaceHUD` draws the map on spacious desktop viewports.
- Race rules, checkpoints and lap counts remain unchanged and isolated in portable domain code.

These source-only improvements do not constitute engine compilation or a cooked map.

## v0.4.1 Android input presentation boundary

- `UI/TouchControlLayout.h` is pure C++20 and determines rectangles/hit tests in physical viewport pixels; `ALatebrakeRaceHUD` and `ALatebrakeCarPawn` both call it, avoiding drift between visible controls and actual triggers.
- Touch acquisition happens only on an initial press inside a pad. Subsequent movement can steer within the captured steering zone or change an existing pedal between GAS/BRAKE/neutral; no touch move can capture a new finger.
- In-game touch pause/restart/recover actions share existing player/race methods. Pause touches are configured to execute while paused; restart always returns the world to running state.
- Compact HUD omits verbose garage upgrade details to protect room for six on-screen pads.
- Portable CMake tests validate numeric and geometric behavior. The Unreal-specific touch input binding, rendering, UI legibility, pause handling, phone interruptions and hardware performance are explicitly **not** verified.

## v0.5.0 career and navigation layering

- `Career/CareerCatalog.h` contains a fixed immutable 12-ID event catalog:
  two three-round cups and six standalone objectives, using the existing
  four-gate track exclusively.
- `Career/CareerChallenges.h` validates authoritative player race facts and
  medal thresholds. It does not use `RivalRouteProgress` as standings.
- `Career/CareerProgress.h` stores versioned medal/active-event state,
  derives prerequisite eligibility and championship points, and grants
  first-medal/series-completion credits once on actual state transition.
- `ALatebrakeRaceGameMode` configures the existing `RaceProgress` lap
  count for the chosen career event, receives the final accepted checkpoint,
  records best lap and slowest lap/recoveries, then atomically packages both
  medals and garage credits for one SaveGame slot write.
- Legacy race/garage data is kept on migration; unknown future career schema
  disables writing rather than silently destroying newer save state.
- `UI/MenuNavigation.h` is a deterministic ten-page portable menu state
  machine. Engine game mode maps actions for quick race, career, challenges,
  championships, garage, settings, results and profile summary; Canvas HUD
  renders an **experimental**, unverified developer overlay, not a complete
  production UMG menu.
- Mobile lifecycle delegates release held steering/throttle/pedal captures
  on background/deactivation/foreground. The real Android callback and
  touch focus behavior still require device checks.

All source changes must pass portable CMake/CTest on three runner OSes.
This is **not** an Unreal Editor build or a playable packaged game.
