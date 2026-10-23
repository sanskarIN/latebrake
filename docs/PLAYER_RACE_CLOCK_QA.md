# LateBrake v0.5.10 — Local Player Race Clock Readiness

**Source development only. Not a game build, release, or engine qualification.**

## Source contract

The race clock must not accumulate elapsed time unless all of these hold:
a complete startup circuit, an unpaused world, an unfinished race, a
positive finite frame duration, and a valid local player
`ALatebrakeCarPawn` returned by the player's controller.

The existing v0.5.9 checkpoint scoring predicate still rejects
incomplete/paused/finished race progression. The checkpoint actor
already samples only an actual valid local player car. Driver loss
does not fabricate elapsed time, and resuming a real player continues
from the exact recorded clock rather than charging disconnected time.

## Strict portable checks

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release --output-on-failure
```

`PlayerRaceClockReadinessTests.cpp` links the game's real portable
`RaceProgress.cpp`, not a mock of the clock. These tests do not
exercise Unreal possession or actual HUD output.

## Genuine installed-game acceptance (pending)

1. Compile UE 5.8 editor/UHT and build a genuine map plus Win64 candidate.
2. With a valid track, verify a possessed player car advances elapsed
   race time while driving; pause freezes time and checkpoint awards.
3. In a disposable development build, temporarily remove the player
   pawn/possession during a running race. Verify the elapsed time is
   frozen and `PLAYER CAR NOT READY` appears in HUD/menu as applicable.
4. Restore a valid local pawn and verify the clock continues with no
   charge for time when no driver existed; finish/restart normally.
5. Repeat with a deliberately incomplete track, then the Android
   packaged candidate and real device input interruptions.
6. Keep build SHA, exact Unreal 5.8 version, installation/device
   information and raw logs with the private QA record.

Do not publish a playable EXE/APK until these engine/device steps and
all preceding readiness gates have been independently checked.
