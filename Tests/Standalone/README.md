# Portable C++20 test suite

The portable suite compiles `Source/Latebrake/Private/Racing/RaceProgress.cpp` from the game source rather than copying the implementation into test fixtures. This allows Linux, Windows and macOS CI to catch regressions in game-independent race rules without installing Unreal Engine.

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build
cmake --build Tests/Standalone/build --parallel
ctest --test-dir Tests/Standalone/build --output-on-failure
```

These checks **do not** validate physics feel, Unreal Header Tool generation, runtime map, input events, save APIs, cooked binaries or Android devices. See `docs/TESTING.md` for the Unreal-specific acceptance plan.
