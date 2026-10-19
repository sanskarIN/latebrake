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

## Observed CI result

As of 2026-10-19, commit `f8723f7937faf9503734719f9a1aeaacf640727b` passed the Ubuntu, Windows and macOS portable C++20 jobs: https://github.com/sanskarIN/latebrake/actions/runs/39221329905 . Later commits require their own verification.

## v0.1.1 regressions

Tests now cover current/last/best lap, sector splits, a slower finish that does not overwrite the fastest lap, invalid checkpoint sequences, timer overflow, invalid directional crossings and minimum speed checks. Unreal-specific QA must test the actual car overlap velocity, save migration, HUD layout and lap recording.

Recovery penalty assertions verify positive additions, rejection of NaN/infinity/negative durations and frozen finish clock behavior. In Unreal, verify that a manual recovery adds five seconds, does not count a skipped gate, and that restart clears the penalty along with the run.

## v0.2.0 portable and engine qualification

Additional CTest targets: RivalPilot (heading/turn, overspeed braking, wraparound, invalid inputs) and RaceRewards (event thresholds, clean recovery bonus, invalid inputs), along with the existing lap and event catalog tests.

Unreal-specific checklist:
- [ ] Compile new `ALatebrakeRivalCar` reflection class in UE 5.8 and validate actor initialization
- [ ] Circuit mode spawns exactly one rival and restart reinitializes its pilot
- [ ] Time Attack mode removes the rival and resets to one lap
- [ ] Keys 3 and 4 select modes with independent best times
- [ ] Rival overlaps never advance player checkpoints or trigger player results
- [ ] Player gate order and required directional speed still work
- [ ] Credits awarded once, survive save/reload and ignore unearned wins
- [ ] Android viewport labels, gamepad and mobile touch are tested on real devices

## v0.2.1 additional regression coverage

The portable RivalRoute suite checks waypoint wrapping, approximate lap completion, invalid distance/speed/delta rejection, stuck recovery after 8 seconds, movement reset of the stuck timer, full finish lockout and route reset. SteeringFilter tests cover bounded steering steps, reversals, invalid frame deltas, out-of-range steering inputs, NaN, overflow and convergence. RivalPilot additionally tests early corner braking and negative speed rejection.

Unreal validation still required:
- [ ] Rival consumes planned commands before base physics and recovers without resetting its route
- [ ] Stuck detection works near barriers, with no oscillating recovery loop
- [ ] Rival approximate lap display never claims an authoritative finish position
- [ ] New steering responsiveness is driveable at varied frame rates on real devices
