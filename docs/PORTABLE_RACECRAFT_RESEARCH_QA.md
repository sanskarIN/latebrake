# Portable Racecraft, Telemetry and Original Audio — integration gates

**Status: source-only C++ research integration. NOT a verified UE5.8 game,
not installed gameplay, and not a playable pre-release.**

This integration isolates portable libraries originally developed on the
[experimental source branch at commit `78075f03`](https://github.com/sanskarIN/latebrake/commit/78075f03d46cf534dc80674d8586de8a6378c7ef)
and tests them against the current, protected `main`. It copies no historical
unverified native Unreal `Private/` implementation, physics binding,
original-map mutation or release-publishing workflow. The original branch
and draft PR #349 remain intact.

## What is deliberately added

- **69 portable Racecraft headers:** validated physical frame/actor evidence,
  material and tire force models, four-contact prediction, aerodynamic and
  drivetrain calculations, steering and braking envelopes, collision/passing
  forecasting, official-state isolation, pit-crew and incident evidence,
  and original six-car replay/camera research.
- **32 compatible portable Telemetry headers:** read-only vehicle samples, finite
  numeric validation, lap and sector review, tire/fuel/brake analysis, replay
  sampling and export, strategy forecasts, driver coaching and session
  consistency checks.
- **3 portable Audio headers:** original waveform synthesis, physical
  race audio staging and bounded engine sound-mix calculations.
- **4 original portable geometry headers:** typed, self-contained asphalt,
  kerb, Armco crash-barrier and six-car prototype body-surface mesh
  generation, with separate deterministic geometry fixtures. No meshes
  are uploaded, rendered or cooked in Unreal by these classes.

These are **domain models**, and are not enabled inside
`ALatebrakeCarPawn`, `ALatebrakeRivalCar`, `ALatebrakeRaceGameMode`, or
the actually installed vehicle/camera/audio systems by this PR. A model
named `PhysicalSixCar...` is **not** evidence that six native Chaos
vehicles, engine audio output, recordings or real maps are operational.
No player credits, race clock, checkpoints, save authority or network
functions are permitted to read these prototype outputs without
separate validated integration.

## Minimum independently inspectable tests

`Tests/Standalone/CMakeLists.txt` registers **22 real behavior suites**,
each built with MSVC `/W4 /WX` or Clang/GCC
`-Wall -Wextra -Wpedantic -Werror`, plus **4 compile-all header targets**
covering all 69 Racecraft, 32 Telemetry, 3 Audio and 4 geometry headers. The source
tool must show **all** the old 177+ CTest targets passing, including the
new ones; a header merely existing in `Source` is not a successful
test or native Unreal integration.

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release --parallel 4
ctest --test-dir Tests/Standalone/build -C Release --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
python3 scripts/MeasureSourceScale.py --json
```

The exact-source status is independently gated on Windows, Ubuntu and
macOS in GitHub Actions. PowerShell release-policy fixtures, offline
no-write publication guard and MkDocs QA must still pass. The old
independent draft PR may have had CI, but its CI result **is not**
substituted for new, protected-main integration tests.

## Size accounting without double-counting

The prior protected main snapshot `c1752b3a79de4aca389df9fc224e98c7326c87e3`
measured **18,506** nonblank/noncomment game-source C++ lines in
**183** files. After removing 19 incompatible stacked telemetry declarations, staged
commit `354e02ab64550c68e7fe52e74515d9ffec3585cc` measured **69,093**
on the PR branch, or **50,587 additional qualifying lines**. The four later, separately tested geometry builders add genuine source;
verify their contribution from the final exact-head source audit instead of
extrapolating from their physical-line count. Treat this as the *branch* count
until the final merge is completed and the same method runs on the new
protected `main` commit. Do not count test/fixture code, comments,
README text, generated content or copied external third-party sources,
and do not add PR LOC twice to the mainline count.

**Scope caveat:** the imported portable code is previously authored, newly integrated
portable experimental source, **not** a claim of equivalent new lines of
functionality verified in the runnable game. 
The second-generation strategy telemetry was deliberately held back because
its globally named coaching types collide with the independently developed
first-generation driver-coaching API. Its source-only test passing alone
would have concealed this ODR problem. The remaining research-only overtake
commitment policy uses its own nested namespace to avoid collisions with
the other autonomous passing planner.

The 200,000-line
`v0.6.0` source milestone and release-playability gate are distinct.

## Required real Unreal Engine owner qualification before using in gameplay

1. On an isolated UE5.8 / UHT Windows 11 workstation, build the
   protected-main Editor and Game targets and cook the original circuit.
2. Design thin read-only sensor adapters with specific actor/session
   revisions, real Chaos transforms, per-wheel grounded contact and
   physical track/road provenance. Never synthesize missing sensor values.
3. Run individual subsystem opt-in integration behind a versioned
   toggle. Fail closed on missing motion, invalid sensor coherence,
   stale frame times or a changed actor/session identity.
4. Verify six real cars (if ever enabled) against independent collision
   boundaries, braking, pit entry, track limits, actual powertrain
   state and original map topology; measure genuine engine logs and
   30/60/120 FPS installed behavior.
5. Validate any original sound synthesizer against real audio output
   samples/device underruns and copyright-safe custom assets; pure
   numerics are not a shipped sound system.
6. Test race restarts, focus pause, recovery/teleport, offline saves,
   career timing and failure paths without allowing these models
   to grant credits or official lap results.
7. Repeat native qualification for every platform and publish actual
   Win64 installer / Android APK only after verified Cook/UAT receipts,
   manifest integrity and installed launch evidence.

Until then, the latest public pre-release remains
[v0.5.99](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.99),
a **zero-asset SOURCE-ONLY preview**. This PR does not create new tags,
release assets, CI secrets, executable downloads or completed release QA.
