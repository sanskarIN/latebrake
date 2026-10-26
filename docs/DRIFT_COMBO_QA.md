# v0.5.73 source work — Arcade drift combo acceptance

## Intended player feature

The real car's chassis velocities already feed
`DrivingSessionStats`. A new engine-independent
`DriftComboMeter` observes the same finite, bounded sample without
mutating the vehicle. It is a deliberately **fictional arcade score**,
not a measured competition record or a safety/driving lesson.

- No combo exists without sustained high-speed *in-game* lateral slip
  over a short contiguous qualification window. A single sudden skid,
  brake input or invalid frame earns nothing.
- A short loss of slip permits continuity **after the combo has qualified**;\n  any interruption before qualification resets the candidate. Sustained recovery to grip,
  pause, focus loss, paid recovery or physics invalidity ends and banks
  the current provisional combo without double counting.
- The live racing HUD shows the running combo and its current\n  time-based arcade scoring multiplier on spacious displays.
  The paged finished Results menu shows run-local total arcade points,
  combo count, best combo and qualified sample seconds, but ONLY if a
  real valid awardable race finish exists.
- The screen explicitly says **NO CREDITS**. These points do not go
  into career progression, saved records, unlocks, rewards, cloud
  accounts, race classification or any public leaderboard.
- Every race restart clears all combo statistics through the
  existing real `Car->ResetDrivingStats()` pathway; recovery does
  not reset legitimate points already earned in the current session.
- No new device controls, physics forces, source assets or third-party
  content are introduced.

## Source verification

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release \
  -R '^DriftComboMeter$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

Strict C++20 tests include no-combo quick slides, qualified sustained
combos, continued scoring, brief break versus real termination,
idempotent suspension, second combo after a restartable interval,
invalid delta/velocity and lateral speed, speed/ratio thresholds,
reset and the fact that unqualified samples never score.

Python source contracts verify the actual Unreal pawn observes only
after physics readiness and finite chassis velocity, suspends on
recovery and blocked driving, resets via real GameMode, and only
renders the arcade meter through the safe bounded UI. None of these
tests are UE game runtime or physical chassis playtests.

## Unfinished engine and real-device QA

- Compile real **Unreal Engine 5.8** Editor and Game targets through
  UHT/engine toolchain, cook the original LateBrake circuit and verify
  package identity with source SHA.
- Validate actually rendered meter on Win64 and Android across HUD
  sizes, pause/results pages, normal racing, source-set arcade/
  simulation handling modes, focus loss, restart and teleport.
- Ensure sustained virtual slip increments score predictably, common
  turns do not generate phantom combos and actual invalid/teleport
  motion breaks continuity. Check parked car, reverse and obstacle
  collision cases for falsely positive samples.
- Demonstrate a valid finished race before results claims are drawn;
  no credit balance change may be caused by arcade drift points.
- Attach actual screenshots, engine logs, package hashes, device
  models/versions and QA to [Playable-release gate #148](
  https://github.com/sanskarIN/latebrake/issues/148) as appropriate.

**v0.5.73 drift source is not yet engine-qualified, published, playable
or stable. No compiled Windows/Android game asset is available.**
