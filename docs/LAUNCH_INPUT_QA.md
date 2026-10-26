# v0.5.76 launch input onset — source and device QA

## Presentation-only gameplay feature

After the real 3/2/1/GO green-light gate has opened, the player pawn
may record the **first forward motor-input command** applied within
the first 120 seconds of official racing time. This is a voluntary,
race-local, source-observed indicator, not an official reaction-time
measurement or a verdict on driving skill.

- The actual Unreal `ALatebrakeCarPawn::Tick` first enforces the
  verified circuit, authoritative green light, correct restart grid,
  focus/pause and physics validity, and confirms its real player pawn
  identity. Rival/AI pawns never record a human measurement.
- A **positive effective throttle of at least 0.20**, without active
  braking, can latch the authoritative `RaceProgress` elapsed seconds
  exactly once. Negative/reverse, neutral, braking, invalid, nonfinite
  or out-of-window samples never write a value.
- An already held accelerator at green may result in **0.00 seconds**,
  which describes the first accepted applied input, not personal
  reflex speed. The metric does NOT prove that tires accelerated or
  the engine physically moved the car.
- The existing bounded Results pager shows
  `FIRST FORWARD INPUT AFTER GO: … (NO BONUS)` only when the event has
  a genuinely eligible finished race and a valid sample exists.
  Missing data is omitted rather than fabricated.
- Existing `ResetDrivingStats` resets the onset for every valid new
  Quick Race, career attempt and replay; pause/resume and recovery do
  not overwrite a valid earlier in-run observation.
- Neither official timing, driver speed, tire force, position,
  scoring, career awards, credits nor saved player data is modified.

## Source and portable tests

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release \
  -R '^LaunchInputOnset$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

The strict C++20 test links the *real* `RaceProgress.cpp` and verifies
no invented input, wrong player, brakes, reverse, subthreshold,
nonfinite/out-of-window readings, stable one-shot capture at official
race time, reset and zero-time input held at GO. Unreal-source
contracts check real physics readiness, actual pawn identity,
GameMode race clock binding and the paged finished Results path.

## Genuine engine and installed-device acceptance (outstanding)

- Build real authorized UE 5.8 Editor/UHT and Game targets, cook the
  source-owned original circuit, and install authentic Win64/Android
  candidates with exact source/cooked-map/package hashes.
- Observe actual keyboard, Xbox/PlayStation style gamepads and
  two-finger Android throttle/brake around 3/2/1/GO. Confirm no
  pre-green or rival-onset input can be reported.
- Compare displayed onset with the official game race clock for
  held-throttle-at-GO, delayed throttle, braking, pause/focus loss,
  recovery and multiple race restarts. Treat physics tick ordering
  as an engine QA question until directly measured.
- Ensure narrow/tall/higher-DPI mobile Results pages still paginate
  the text without colliding with SAVE FAILED and touch pads.
- Capture UE logs, actual device model, screenshots/QA and verified
  build/map provenance under [playable gate #148](
  https://github.com/sanskarIN/latebrake/issues/148).

**This is source development, not a verified UE runtime feature, human
reflex evaluation, released executable game or stable game version.**
