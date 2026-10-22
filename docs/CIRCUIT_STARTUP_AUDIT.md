# LateBrake v0.5.6 — Circuit startup geometry audit

**GitHub source-only pre-release. This is NOT a verified playable game or an executable distribution.**

## Problem fixed in the source

Previous LateBrake GameMode startup logging treated **four checkpoint
Actor spawns** as enough to mark the game `LATEBRAKE_STARTUP_READY`.
Although scenery was logged separately, a failed ground, road or
barrier Actor creation could go unreported as a startup blocker.
A race may not award progress on an incomplete collision course.

## Source files and contracts

| File | Responsibility |
| --- | --- |
| `Source/Latebrake/Public/Track/CircuitCollisionPlan.h` | Defines the nine original collision-bearing block transforms and semantic materials in pure C++20 |
| `Source/Latebrake/Public/Track/CircuitStartupAudit.h` | Fixed-memory counting policy for exact 9 collision blocks, 4 unique checkpoints, 7 groups and 93 instances |
| `Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp` | Instantiates original track Actors from the portable plan, logs startup markers and refuses checkpoint awards if incomplete |
| `Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h` | Maintains nonpersistent local `bCircuitReady` state |
| `Tests/Standalone/CircuitCollisionPlanTests.cpp` | Verifies exact original geometry source data and safe invalid indices |
| `Tests/Standalone/CircuitStartupAuditTests.cpp` | Tests positive construction, missing/extra blocks, duplicate/wrong gates and incomplete scenery |
| `scripts/Verify-RuntimeStartupLog.ps1` | Validates textual startup marker grammar, singleton counts and ordering |
| `scripts/Test-PackagingPolicy.ps1` | Ensures synthetic missing/extra/wrong-order actor reports fail; positive fixture checks syntax only |

Unreal still uses the standard engine primitive Cube for these
prototype Actors; no Epic asset, third-party code or executable is
bundled in the source repository. The original centimetre transforms
and style groups are preserved.

## Markers in a genuine v0.5.6-or-later runtime log

The required ordered lines include:

```text
LATEBRAKE_SCENERY_READY groups=7 instances=93 collision=off
LATEBRAKE_GEOMETRY_READY collision=9/9
LATEBRAKE_STARTUP_READY map=LatebrakeCircuit checkpoints=4/4
```

A successful parse means that **source-created Actor counts were logged**.
It does not certify a real installed Unreal game or working collision,
scene rendering, lap progression, save games, controls or frame times.
Synthetic parser fixtures must never be offered as genuine evidence.

## Still required before any playable pre-release

- Compile actual UE **5.8** Editor/UHT/game C++ targets and fix
  implementation-specific engine issues on the licensed toolchain.
- Create and cook an original serialized `/Game/Maps/LatebrakeCircuit`
  level using Unreal Editor and verify the installed world.
- Generate authentic Win64/Android candidates with recorded build SHA,
  toolchain, source map/package and per-file hashes.
- Observe actual ground, road, wall and checkpoint collisions while
  driving with keyboard/gamepad/touch under 30/60 FPS and longer hitches.
- Exercise recovery, restart, lap order, career rewards, save/load,
  rotation/input cancellation, HUD and Android interruption on devices.
- Capture fresh single-session runtime logs, independently verify
  startup markers, and attach qualified human QA evidence to the
  relevant open GitHub issues.

The published v0.5.6 source archive is **not** a game build. Genuine
Unreal-device acceptance remains open and is required before a playable release.
