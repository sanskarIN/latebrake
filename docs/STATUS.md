# Engineering status

**Milestone:** v0.1.0 pre-release candidate development (**not release-ready**)

**Date:** 2026-10-19

## Implemented in repository source

- UE 5.8 C++ game/editor target and module.
- Physics-rigid-body car pawn, desktop controls, braking, reverse, recover, two handling presets, camera-distance toggle, pause, and initial two-finger touch-region bindings.
- Procedural starter circuit built from runtime engine primitive references.
- Four ordered gates and three-lap finite race state machine.
- HUD with speed, race clock, lap/gate status and personal best.
- Local USaveGame storage of the best race time and completion count.
- Portable race-progression tests and a cross-platform GitHub Actions workflow.
- Core open-source and contributor documentation.

## Verified

- RaceProgress C++20 logic compiles and its standalone test passes with a local GNU C++ compiler.
- GitHub Actions portable C++20 test workflow has reported success on the branch's earlier pushed commits across its OS matrix; reconfirm against the final head of this branch.

## Not verified / missing

- **No Unreal 5.8 gameplay code compilation, PIE test or cooked build.**
- No native Windows executable, Android APK/AAB or Linux package.
- No authored vehicle rigs, Chaos Vehicles suspension/wheel implementation or realistic drivetrains.
- No original production-quality 3D vehicle/track/lighting/FX/audio assets.
- No authored .umap; runtime startup currently relies on a built-in engine entry map.
- Touch-region bindings added in C++ but **not tested on an Android device**; visual touch buttons, tilt controls and mobile performance testing remain missing.
- No AI opponent, traffic, career, garage, event browser, world exploration or multiplayer.
- No verified engine build failures because an Unreal build has not been attempted here.

## Acceptance gate for v0.1.0

1. Compile and open project in a real UE 5.8 development setup.
2. Confirm spawn, physics, camera, keyboard/gamepad controls and collision/visibility.
3. Validate first gate, order enforcement, lap count, time, finish screen and restart.
4. Validate save/reload and invalid/corrupt-slot handling.
5. Author a distributable project map and test real Win64 and Android packaging (including mobile controls).
6. Record actual hardware test observations, resolutions and limitations.
7. Publish pre-release only with real working artifacts and honest release notes.

The portable unit-test workflow is **not** a substitute for these gates.
