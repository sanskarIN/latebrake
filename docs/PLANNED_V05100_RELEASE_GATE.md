# Planned v0.5.100 source-only pre-release qualification (NOT PUBLISHED)

This document prepares a future LateBrake pre-release. **It is not a
published release, a pre-release authorization, a compiled Unreal game, a
Windows executable, an installer, an APK or a claim of UE5.8 qualification.**
The latest verified published release at the time of authoring is
[v0.5.99](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.99).

## Mainline candidate scope

- Preserve source-only mainline and the original project's immutable
  predecessor tags. Never label the unmerged PR #295 experimental braking
  branch or draft six-car racecraft PRs #346/#349 as shipped gameplay.
- Harden real human-versus-rival arcade drafting against impossible **vertical
  relative position jumps**, complementing the already shipped planar
  velocity-consistency, physical road support and obstruction checks.
- Use **measured Chaos Z velocity of each live car**; fail closed when either
  measurement is unavailable or invalid. The integrated result retains the
  0.18-second confirmation, +6% maximum drafting effect and shared +40%
  nitro/draft ceiling. Never change race progress or official lap/reward state.
- Add portable C++ motion, invalid-input and reacquisition regressions,
  Python tests asserting actual Unreal source binding, and update drafting QA.

## Exact-head pre-publication gate

1. Merge only reviewed changes with **passing exact-head** three-platform
   C++20 CTest, Python Editor source tests, PowerShell packaging policy tests,
   and offline release guard tests. Record the final merged SHA.
2. Run licensed UE5.8 Editor/UHT compile on a trusted owner-controlled machine.
   Exercise both cars on physical road support and check vertical samples,
   barrier traces, input/recovery reset and boost bounds in actual gameplay.
3. For a **playable** claim, additionally produce and independently test an
   original-circuit cooked installed Windows package and signed/hash-linked
   user QA at 30/60/120 FPS. Until then, the release must say **SOURCE-ONLY**.
4. Independently audit published v0.5.99 identity, current main protection,
   actual proposed tag target, the exact final release-note bytes, passing
   gates and zero attached binaries for a source-only publication.
5. Build and test a new narrowly scoped v0.5.100 publisher **only after**
   the final source SHA is known. Avoid reusing the v0.5.99-only publisher
   with a new version or implying that a draft release note is publication.
6. An owner must explicitly authorize publication of the **specific SHA**.
   Never retag, overwrite, or recreate the v0.5.99 release.

## Branch and source-scale boundaries

The portable research source merged after v0.5.99 is not automatically
integrated into the live Unreal driving loop. An audit of qualifying C++
lines should be run on the final main commit with
`python3 scripts/MeasureSourceScale.py --json`; no PR/branch counts are
additive to main, and code quantity does not imply playability.
Review standalone draft PRs against their actual base branches before
retargeting or merging. The separate future v0.6.0 source milestone
remains 200,000 unique qualifying production C++ lines.

## Ready-to-run source QA

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release --parallel
ctest --test-dir Tests/Standalone/build -C Release --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
node --test Tests/Release/ExperimentalSourceReleaseTests.cjs Tests/Release/MainlineSourceReleaseV0599Tests.cjs
```
