# Changelog

All notable verified changes will be recorded here. Version labels below describe source development, **not published releases**.

## Unreleased — v0.1.1 stabilization branch

### Added
- Per-sector, current lap, last lap, in-race fastest-lap timing and HUD readout.
- Saved all-time fastest completed lap (backward-compatible default field).
- Direction-checked circuit checkpoint overlaps with a minimum entry speed.\n- Recovery clock penalty and post-finish/invalid-penalty rejection.
- Regression tests for wrong-way gates, lap splits and timer overflow.

### Validation
- Portable C++20 checks can run without Unreal Engine; full editor/gameplay/platform builds still require qualification.
- This branch must not be advertised as v0.1.1 prerelease before v0.1.0 packages are verified and published.

## Unreleased — v0.1.0 development

### Added
- Unreal Engine 5.8 C++20 project, source module and build targets.
- Physics-backed starter pawn with keyboard/gamepad bindings, two driving presets, brake, recovery and camera toggle.
- Runtime-generated circuit, four checkpoint gates, three-lap rule state machine and racing HUD.
- Persistent local personal-best record and completion counter.
- Pause/resume input and experimental left-steer/right-pedal touch handling (device-unverified).
- Engine-independent C++20 regression tests and GitHub Actions matrix.
- Project licensing, build instructions, architecture and status documentation.

### Known limitations
- Not compiled inside Unreal Editor, packaged or device-tested.
- No original packaged level, production vehicle meshes, dedicated wheel/suspension simulation or polished/tested touch UI.
- No v0.1.0 release or downloadable executable/APK available.
