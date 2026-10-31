# Arcade driving force composition — source regression and real-engine QA

**Status: development test coverage only.** The latest public release
remains v0.5.85 SOURCE-ONLY. This regression is **not an installed Unreal 5.8 test**
and does not constitute an EXE/APK, cooked circuit or hardware qualification.

## Why this integration test matters

Several independent engine-neutral policies already influence the same
Chaos rigid-body car: throttle corner assist, tire heat/wear, asphalt-edge
traction, service brake fade, physical chassis collision damage, nitro and
qualified rival drafting. Their individually bounded outputs must compose
in a safe order.

The new **ArcadeDriveComposition** standalone C++20 test exercises real
production policy headers together and mirrors the three AddForce motor
paths without changing engine physics, gameplay, economy or saved data.

For a valid, forward-driving frame, the intended shared motor budget is:

    applied force / healthy base force
      = qualified throttle × valid surface motor factor
        × retained impact motor factor × (1 + allocated nitro + draft)

Here allocated nitro + draft is at most **0.40**. The existing allocator
prioritizes finite nitro charge. Confirmed dirt reduces the entire motor
budget to **78%**; impact damage reduces it to no less than **79%**.
The original corner assist never increases positive throttle. The real
pawn must not drive after an invalid/blocked chassis or surface sample.
Reverse and braking keep their existing separate guards.

The existing lateral resistance scale uses real tire condition, surface
traction and damage multipliers, all bounded at or below one. Service
brake heat only affects service braking and never grants motor power.

## Automated checks

- The portable C++20 test covers baseline road power, combined nitro
  and draft, horizontal collision damage, confirmed dirt, combined grip,
  invalid physics/surface inputs, pause impact suspension, race reset,
  and **200,000** deterministic combined policy samples.
- The Python binding suite checks production chassis validation, ordered
  source observation, all three shared motor scales, brake fade branch,
  and reset/suspension lifecycle.
- No runtime force values or source physics are changed.

From the repository root:

    cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
    cmake --build Tests/Standalone/build --parallel
    ctest --test-dir Tests/Standalone/build --output-on-failure -R ArcadeDriveComposition
    python3 -m unittest discover -s Tests/Editor -p '*Tests.py'

## Required real-device acceptance

1. Compile licensed Unreal Engine 5.8 and UHT, cook the owned race map
   and package actual Win64 and Android candidates.
2. Verify all three motor paths in actual forward driving with nitro,
   real rival drafting, confirmed dirt and chassis damage simultaneously.
3. Check service brake heat, parking-brake floor, coasting, and recovery
   across pause, focus interruption, race restart and failed physics.
4. Repeat with Android touch, keyboard/gamepad and rival AI; compare
   actual 30/60/120-fps installed driving and physics traces.
5. Record engine build, source/map/package hashes and startup/QA logs.

**This does not close the playable-game qualification gate.**
