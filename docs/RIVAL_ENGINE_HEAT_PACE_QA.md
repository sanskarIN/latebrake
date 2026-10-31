# Rival engine-heat-aware cooldown pacing — source QA

**Status: source-only development beyond merged v0.5.86 candidate.**
No licensed Unreal Engine 5.8, UHT, cooked circuit, installed Win64
or physical Android playtest has been performed for this change.

## Feature

The physical AI rival now reads its **previous qualified base-pawn
engine heat fraction** after route planning, existing grip/surface/damage
risk policies, but before traffic yielding and the final spin guard.
The controller does not manufacture a target position, rival ranking,
engine temperature or track state.

At an observed engine fraction above 70% and planar speed at least
1,200 cm/s, the controller caps positive AI throttle monotonically
between the ordinary 0.85 planning maximum and a 0.30 hot floor.
The physically shared engine model still independently reduces actual
motor force during overheating. This second, optional pacing layer
simply eases throttle demand to create cooling opportunities; it never
changes steering or adds a service brake.

When no prior thermal sample exists after startup/recovery, it preserves
the existing qualified route command. Malformed physical inputs, frame
hitches and nonfinite data fail closed to zero throttle and braking.
Existing corner/traffic braking, directional gates, recovery, lap time,
career rewards and profile writes remain authoritative.

## Regression commands

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^RivalEngineHeatPace$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

C++20 tests cover warm/cold thresholds, cooldown behavior, brakes and
verified arrival priority, slow-speed launch recovery, invalid and
nonfinite input rejection, monotonicity, actual ArcadeEngineHeat
thermal evolution, and 200,000 bounded sample combinations. Python
source tests enforce the exact AI composition order and lack of
race-timing, economy or Unreal engine authority in the new policy.

## Required real gameplay acceptance

1. Build and cook the authored circuit in actual Unreal Engine 5.8
   with source/engine/map hashes and inspect the rival controller calls.
2. Drive multiple physical rival laps with real vehicle physics on
   an installed Win64 build; record thermal transitions, actual speeds,
   recovery attempts, braking and checkpoint eligibility.
3. Verify that low-speed starts and hill/dirt recovery do not stall
   the rival and that traffic collision avoidance always takes
   precedence. Compare lap completion and pace before/after this policy.
4. Run genuine packaged Android hardware tests if claiming mobile
   support; archive performance and input acceptance alongside issue #13.
5. Keep playable-release issue #148 open until authentic build and
   installed-game evidence exists.

Passing source-only CI cannot establish gameplay quality.
