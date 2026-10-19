# Test plan

## Executable automated checks

Portable race rules: `cmake -S Tests/Standalone -B Tests/Standalone/build && cmake --build Tests/Standalone/build && ctest --test-dir Tests/Standalone/build --output-on-failure`.

The suite checks checkpoint ordering, duplicates, invalid indices, lap count, finish behavior, timer safety (including NaN/infinite deltas), frozen finish clock, restart/reset and invalid configuration clamping.

GitHub Actions matrix: Ubuntu, Windows and macOS. This validates portable C++ source, **not Unreal packaging**.

## Unreal integration test checklist (manual until installed)

- [ ] Unreal Header Tool generates classes, editor target builds without warnings/errors
- [ ] Correct default pawn appears at south straight; camera follows without distortions
- [ ] Starter car responds to W/S, A/D, space, R, T, C and handling mode buttons
- [ ] Pause/resume does not leave stuck throttle/brake input
- [ ] Car rests on visible collidable road; barriers do not block checkpoint circuit
- [ ] Only expected gate increments progress; gates cannot be bypassed/repeated for credit
- [ ] Three completed laps show finish, freeze timer and update best race
- [ ] Restart resets timer/gates without corrupting previous best
- [ ] Relaunch restores personal best and completed-races count
- [ ] Empty, corrupt and unwritable save slots are handled without crash
- [ ] Inputs/camera/UI are readable on supported desktop aspect ratios
- [ ] Packaged Windows build launches and can complete a race
- [ ] Android touch/control implementation and real-device test pass
- [ ] At least one release packaging workflow produces verifiable artifacts

Track the engine version, machine, GPU, driver, device temperatures, resolution, frame times, loading time, memory, and crash logs for every qualification.
