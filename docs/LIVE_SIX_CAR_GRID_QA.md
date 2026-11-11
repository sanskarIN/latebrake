# Live six-car grid readiness — experimental Unreal QA

**Unreleased developer-only source. Not a verified UE5.8 build or
installed Windows executable.** Stacked on [PR #333](https://github.com/sanskarIN/latebrake/pull/333).
Do not insert into the frozen v0.5.98 source-only release candidate.

## Why a physically present actor is not a verified starting grid

The previous experimental GameMode accepted six registered, physics-ready
actors before advancing the race lights. A genuine Chaos chassis can have
moved **after** its spawn was verified: contact, surface responses, physical
shifts or measured velocity may carry a car into another grid cell before
GO. Static `StartGridPoses` geometry checks cannot establish the six
actors' positions at the actual count-down instant.

The new `IsExperimentalFleetReadyForStart` takes **live read-only**
`GetActorLocation`, `GetActorRotation().Yaw`,
`GetVelocity`, `GetUniqueID`, grid slot, registration and physical
readiness from the human, primary rival and each of the four actual
supplemental rivals. It feeds six real bounded snapshots to the portable
`EvaluateLiveSixCarStartingGrid` **on every qualified pre-GO tick**.
No invented, cached or static body observations authorize a new start.

A start is denied if any actual car:

- is missing or has no verified physical chassis;
- claims an incorrect grid slot, anonymous or duplicate actor token;
- has NaN/infinite/out-of-range pose/velocity;
- drifted farther than **125 cm** from its own authored slot;
- rotated beyond **18 degrees** from its own slot heading (modulo 360);
- moves in the XY plane faster than **250 cm/s (9 km/h)** before GO;
- falls/rises faster than **100 cm/s** vertically, or has a chassis centre
  more than **45 cm** from the flat prototype's actual south-asphalt
  top plus its **32.5 cm** physical half-height;
- lacks **three of four freshly sampled, near-horizontal WorldStatic
  chassis-ground traces** within the existing **115 cm** contact height,
  or has an invalid/overlong physics-frame delta;
- has a padded rotated chassis footprint off the original physical
  south-straight pavement or overlapping any actual barrier; or
- touches another car's rotated padded footprint, even when each car
  is *individually* close to its intended slot.

The pre-GO probe collector deliberately performs fresh WorldStatic
line traces on every eligible countdown tick, because the ordinary pawn
contact readout is unavailable before green and MUST NOT be reused.
It uses the existing 122/68 cm chassis-corner probe offsets, 280 cm
trace length and ground-normal/contact evaluator. Only traces whose
`Hit.GetActor()` is the exact **SouthStraight collision Actor** created
by `BuildCircuit` are admitted. An unrelated floor, a barrier, a
WorldStatic obstacle or a stale/missing road actor cannot impersonate
the intended original asphalt. This is source-level actor binding, **not**
evidence that a real cooked Unreal .umap or installed contact is correct;
live-world collision behavior still needs actual engine testing.
These are **experimental conservative pre-start thresholds**, not
measured car specifications or production race regulations. Failure
does not teleport actors, invent a fallback car, reset race state,
charge a penalty, alter a gate, save results or modify scoring.
`StartLights.Advance` is already gated by the complete physical
roster; this adds the indispensable **current pose** component.

Normal retail one-rival, career and solo Time Attack sessions do not
call the six-car validation. The separate terminal spawn-fault latch
and owner-approved restart policy in PR #333 remain authoritative.

## Automated source tests — NOT real Unreal

```bash
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --parallel 4
ctest --test-dir Tests/Standalone/build -R '^LiveSixCarStartingGrid$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'LiveSixCarStartingGridBindingTests.py' -v
```

The new strict portable C++20 test checks all six actual physical-slot
snapshots, each missing/invalid identity, duplicate live actor token,
nonfinite values, physically wrong car location/heading, fast pre-GO
car motion, heading wrap, fresh four-probe WorldStatic support, unrealistically
high/low chassis Z, vertical motion, stale contact frames, steep normals,
missing or distant supports and **pairwise overlap within individual slot
tolerances**. It also exercises 4,096 bounded grid-jitter combinations.
The Python test inspects the *real GameMode callsite* and verifies the
live Unreal samples reach this pure policy before GO.

Automated source geometry and static bindings do not replace a UE5.8
Editor/UHT/Game build or genuine Chaos vehicle behavior.

## Real UE5.8/Win64 acceptance to perform before a merge

1. Use an owner-controlled licensed UE5.8 Windows workstation and
   source-reviewed main-targeted integration PR to compile actual
   Editor/UHT and Development Game. This new draft feature itself must
   first be included in a newly reviewed exact-SHA integration candidate;
   **existing [PR #335](https://github.com/sanskarIN/latebrake/pull/335)
   remains frozen and does not include this subsequent PR**.
2. On the original cooked circuit in a Development build with
   `-LatebrakeExperimentalSixCarGrid`, inspect that six actual car
   bodies have unique identities and remain fully separated, on road
   and near their intended cells throughout countdown.
3. Deliberately move one physical car 125 cm away, rotate one by more
   than 18 degrees, push one above 250 cm/s before GO, and cause a
   padded overlap while all actors remain otherwise physics-ready.
   Verify countdown refuses GO and does not invent an unofficial
   finish, lap, recovery fee or result.
4. Test a hovering, falling, sunken or half-supported physical chassis
   while its XY/yaw remains valid. Confirm the original 3/2/1/GO lights
   cannot advance until a new frame has three valid real ground supports,
   safe chassis centre height and low vertical speed. Profile the 24
   additional WorldStatic line traces per countdown frame and confirm
   they do not create a hitch or ambiguous collision-surface hit.
   In an isolated development test, remove/disable SouthStraight while
   leaving generic WorldStatic ground or another collider: the six-car
   countdown must fail closed instead of admitting that false support.
5. Restore valid positions using an accepted new race restart.
   Verify physics then permits a genuine six-car countdown, and
   all actors can drive rather than merely being rendered.
6. Repeat ordinary one-rival Circuit, Time Attack and career both with
   and without the developer flag. Confirm unchanged retail starts,
   physics, clock and checkpoint scoring.
7. Measure 30/60/120 FPS pre-GO costs and inspect real collision/
   movement logs and any apparent jitter thresholds on Windows.

Keep [playable release issue #148](https://github.com/sanskarIN/latebrake/issues/148)
open until actual engine/installed package/required platforms are
independently qualified. No real Win64 installer or v0.5.98 release is
produced by this draft.
