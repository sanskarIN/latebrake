# Six original garage silhouettes — source and device QA

Six *fictional* offline garage configurations now have distinct **cosmetic
engine-primitive profiles** in the Unreal starter pawn. New IDs are appended,
not renumbered. They share the existing physically simulated chassis and
collision primitive: this is not six independently modeled production cars.

| Saved ID | Garage profile | Engine-primitive outline |
| ---: | --- | --- |
| 0 | Arrow | Balanced hood/deck, wingless body |
| 1 | Kestrel | Longer hood, shorter deck, low wide rear wing |
| 2 | Phantom | Short nose, long deck, higher narrow wing |
| 3 | Tempest | Extra-long nose, shallow cockpit, low spoiler |
| 4 | Glider | Very short front, longest tail, broad tall cockpit |
| 5 | Nova | Extended rear, broadest rear wing, balanced canopy |

These are original source-defined dimensions. No third-party car model,
manufacturing mark or car audio is embedded. Wings, hood, deck and supports
must remain collision-free and unsimulated. Existing first three IDs keep
their original tuning values and persistent ownership bits.

## Offline progression and save migration

- Profiles 3–5 cost **1,800 / 2,500 / 3,400** earned credits.
- The garage loops only among owned vehicles. The next purchase is the first
  locked saved ID; no free unlocking when loading a three-car save.
- Saved `OwnedVehicleMask` and `SelectedVehicleId` keep existing Unreal
  `int32` schema, with known vehicle bits masked to six. The starter bit is
  always restored, negative masks are rejected, and invalid/future IDs fall
  back to Arrow. The game still requires two identical purchase inputs
  within eight seconds to debit a car.
- Garage UI displays chosen profile number, *base* acceleration/grip/speed
  coefficients, next unlock cost and upgrades. These are not performance
  measurements from a packaged game.
- Career race garage locks and rollback-on-failed-profile-write still apply.

## Source verification

Run these from the repository root:

```sh
cmake -S Tests/Standalone -B build/garage-tests -DCMAKE_BUILD_TYPE=Release
cmake --build build/garage-tests --parallel
ctest --test-dir build/garage-tests -R 'GarageState|VehicleVisualRecipe|CareerGaragePolicy|CareerLaunchSelection|GaragePurchaseConfirmation' --output-on-failure
python -m unittest discover -s Tests/Editor -p '*Tests.py'
```

Source policies and binding tests are **not** Unreal/Chaos integration tests.

## Required licensed engine and installed game checks

1. On an authorized Unreal 5.8 Editor/Win64 install, load existing SaveGame
   containing only bits 0–2 and confirm the old car, bank and upgrades are
   preserved with IDs 3–5 still locked.
2. Purchase Tempest, Glider, and Nova through two-key confirmations, restart
   the game, and verify prices, bitmask, selection, persisted bank and no
   duplicate debit on further confirmation presses.
3. At 720p, 1080p, high-DPI and Android landscape safe areas, verify the
   garage page's lines remain legible and pagination accessible.
4. Compare actual chase/hood/side views for six distinct silhouettes. Check
   spoiler supports, wheel/body clipping, shadows, materials, and missing
   primitive assets. Verify garage selection never changes collision
   extents or unverified car-handling physics.
5. Exercise a career race and a failed SaveGame write: tuning must remain
   locked while the career attempt runs; a failed purchase/selection must
   restore the original car, garage mask and credits.

The original cooked circuit, physically tested vehicle handling, Win64/Android
package identity and installed graphical QA gates remain open in
[issue #148](https://github.com/sanskarIN/latebrake/issues/148).
