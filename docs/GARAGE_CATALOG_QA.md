# Read-only six-car Garage catalogue — source QA

## What changed

The Unreal 5.8 Canvas Garage page previously described only the equipped
car and the next purchase. The page now shows **all six fictional cars** in
stable saved-ID order, including locked vehicles. Each row identifies
`EQUIPPED`, `OWNED`, `LOCKED / AFFORDABLE` or `LOCKED / NEED n` and
displays the original in-game-credit unlock cost.

A `Q NEXT` marker appears **only** on the first locked car: Q never buys an
arbitrary later row. An owned car is equipped by the existing E/SELECT
cycle. The three base performance coefficients now have a read-only
preview of **current shared Engine/Grip upgrades applied to each car**.
These are numerical tuning coefficients, not measured speed, lap time,
a visual 3D showroom, or a reliable predictor of handling.

The page retains its existing two-step spending gate (press Q/U/I
twice within eight seconds), SaveGame rollback, career-event garage
lock, and stable 0–5 vehicle IDs. A failed save remains an error. This
change does **not** introduce extra controls, new currency, third-party
assets, any new vehicle physics, or changes to the profile schema.

## Six-car career medal eligibility (source fix)

Career medal evaluation previously used a legacy **0–2** vehicle-ID guard
even after the Garage was expanded to six immutable vehicle IDs (0–5).
This meant Tempest, Glider and Nova could finish a genuine circuit with
valid ordered checkpoints but still be rejected for career medals.
The source-only fix now relies on the same authoritative
`GarageState::IsVehicle` range as car selection.

All six **owned and correctly equipped** cars can now earn normal medals
in any existing unrestricted career event. No events, scores, targets,
unlock costs, credit amounts or SaveGame fields are changed.
**Kestrel Mastery** still requires *exactly vehicle ID 1*.
Unrecognized/negative/future car IDs remain ineligible.

The real Unreal GameMode captures the `CareerStartedVehicleId` when
the race starts and submits an invalid `-1` sentinel if a different
garage car is selected before the finish. That anti-swap guard remains
unchanged; users cannot earn another car's vehicle-specific medal
through a mid-race selection mismatch. Awards are still based only on
the validated player's race clock and ordered gates, never AI timing.
Existing first-completion rewards remain one-time only and the career
save schema is unchanged.

`CareerSixCarEligibility` C++20 regression tests cover all six IDs,
out-of-range/corrupt IDs, Kestrel-only restrictions, one-time awards,
repeated runs and persisted medal reload. Source-binding tests verify
that the actual Unreal GameMode still supplies its captured starting
vehicle and rejects swaps. **Portable tests are not a licensed Unreal
Engine 5.8 build or an installed gameplay validation.**

## Small screens and accessibility

The existing menu detail pager is reused to expose each item within
its viewport-safe detail window. The menu title and instructions stay
pinned, and feedback from a staged or rejected garage transaction is
placed before the catalogue rows so it remains on the **first page**.
On compact layouts, each car is shown in a concise status row; on wider
layouts its tuned Acceleration / Grip / Speed multipliers get an extra
read-only row. Use keyboard Up/Down or Android menu UP/DOWN to navigate
details, not arrow keys to select a particular car. Touch UI, key
delivery and visuals remain unverified until testing on devices.

## Automated checks

```sh
cmake -S Tests/Standalone -B build/test -DCMAKE_BUILD_TYPE=Release
cmake --build build/test --parallel
ctest --test-dir build/test -R '^(GarageCatalogView|GarageState|GaragePurchaseConfirmation|MenuDetailPager)$' --output-on-failure
python -m unittest discover -s Tests/Editor -p '*Tests.py'
```

Portable C++20 tests exercise original and appended vehicle IDs, fresh
profiles, old three-car saves, all-owned saves, old/future/negative
ownership bits, corrupt selected IDs, missing/large credits,
affordability shortfalls, and the exact shared tuning formulas. Python
source contracts ensure that the actual Unreal HUD consumes the
read-only projected view and **cannot** buy, equip, award or save from
within its drawing method.

## Required real-engine/manual checks

1. Compile source with a licensed Unreal 5.8 Editor/UHT, cook the
   original map and install a true Win64 build (not the Win32 2D
   experimental preview).
2. Verify all six names, status labels, price/cost shortfalls and the
   Q NEXT marker at 720p, 1080p, and high DPI, including narrow
   Android landscape screens and physical touch safe areas.
3. Trigger an actual purchase confirmation and insufficient-credit
   response. Confirm the feedback is visible on the first details
   page without an unintended debit. Test failed local SaveGame
   write and app backgrounding.
4. Navigate forward and backward through every catalogue page without
   losing the pointer/menu focus; close/reopen must reset pagination.
5. Verify no new car automatically unlocks when importing a true
   older three-car profile, and repeated selection never changes
   current career race integrity or checkpoint scoring.

These source tests are not a substitute for an installed graphical QA
pass, UE compilation or real driving. Track the real playable-release
gate in [issue #148](https://github.com/sanskarIN/latebrake/issues/148)
and the production interface quality in
[issue #236](https://github.com/sanskarIN/latebrake/issues/236).
