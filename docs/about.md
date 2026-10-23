# About Latebrake

**Latebrake — Race Beyond Limits** is a community-developed, open-source racing game project built with **Unreal Engine 5.8 and C++20**. Its first milestone is a small offline circuit racer. More sophisticated vehicles, presentation and modes are longer-term plans rather than advertised completed features.

## The goal

Build a source-accessible racing experience with satisfying controls, deterministic lap/checkpoint rules and a development process where contributors can verify their own changes. The first circuit uses Unreal Engine's built-in primitive shapes rather than shipping original vehicle art, commercial tracks or a finished world.

The repository separates portable engine-independent game rules from Unreal-specific actor, physics, input and HUD code. This lets GitHub Actions test the portable systems even though the hosted runners do not have a licensed Unreal Engine installation.

## A source-first prototype

| Area | Current state |
| --- | --- |
| Game engine | Unreal Engine 5.8 targets and C++20 source present; Editor/UHT build **unverified** |
| Vehicle | Physics-simulated primitive-mesh car, not a full Chaos Vehicles tire/suspension rig |
| Circuit | Procedural four-gate layout in C++; authentic project-owned `.umap` still requires licensed Editor authoring/cooking |
| Game modes | Prototype Circuit and Time Attack rules, experimental rival behavior |
| Progression | Offline fictional garage, upgrades, records, career and challenge catalogs |
| Platforms | Windows/Android are intended build targets, **no verified playable EXE or APK** |
| Multiplayer | Not implemented |
| Automated testing | Portable C++20 tests on Windows, Ubuntu and macOS; source/script checks |

## What we will not claim

Passing CMake, Python or packaging-policy tests does not prove that Unreal gameplay works. Pages deployments are **documentation builds** and are not evidence of a cooked game, installed Windows executable, Android APK, network service or certification.

Before a playable release, the project still needs genuine Unreal compilation, map authoring/cooking, packaged archives, deterministic startup/collision logs and installed driving/checkpoint/touch testing.

Read [current development status](development-status.md), [game design](GAME_DESIGN.md) and the [public roadmap](ROADMAP.md).
