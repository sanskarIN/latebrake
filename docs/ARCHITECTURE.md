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
