# Garage and tuning — v0.3.0 experimental foundation

The garage is an **offline, in-game-credits-only** prototype. All vehicles are fictional: no third-party branded models, assets, audio, or manufacturer marks are shipped. These are handling and basic geometric silhouette presets on the same engine-primitive starter chassis, **not three fully modeled production cars**.

## Vehicle presets

| Profile | Credits to unlock | Acceleration | Lateral grip | Top speed | Starter shape |
| --- | ---: | ---: | ---: | ---: | --- |
| Arrow | Free | 1.00× | 1.00× | 1.00× | Baseline canopy |
| Kestrel | 600 | 1.12× | 0.94× | 0.98× | Longer, lower canopy |
| Phantom | 1,200 | 0.96× | 1.12× | 1.08× | Shorter, taller canopy |

These multipliers are applied to the starter force-based rigid-body controller. Physics tuning is **not** a calibrated Chaos Vehicles tire/suspension simulation.

## Earned credits and upgrades

Credits are awarded only when an offline event completes. Circuit events award 450 base credits; one-lap Time Attack awards 180 base. Faster times grant up to 150 bonus credits and a run with zero manual recoveries earns another 50. The balance is capped to prevent overflow. Purchases and upgrades require sufficient earned credits and cannot charge twice for an owned car.

Both Engine and Grip upgrades have three levels shared across the garage. Engine upgrade costs are 200, 350, 500 credits; Grip costs 150, 275, 400 credits. Engine upgrades add 6% acceleration and 2% top speed per level; Grip upgrades add 5% lateral assistance per level. The levels are not paid entitlements. Donations do not affect the garage.

## Prototype controls

| Key | Effect |
| --- | --- |
| Q | Unlock next locked fictional vehicle if credits permit |
| E | Select next owned vehicle |
| U | Purchase next Engine upgrade |
| I | Purchase next Grip upgrade |
| 3 / 4 | Circuit / Time Attack event |

The HUD shows selected vehicle profile, upgrade levels, credit balance and next unlock price. There is **no finished garage menu**. Gameplay control and mobile touch UI have not been playtested.

## Persistence and validation

The original Unreal SaveGame `TotalCredits` field remains the balance store for existing profiles. New optional fields preserve the owned vehicle bitmask, selected profile, engine level and grip level. The portable garage state repairs missing or invalid ownership, clamps upgrade levels, rejects invalid IDs and never allows negative balances or arithmetic overflow. Tests run in the GitHub C++20 matrix. **SaveGame serialization/migration must be checked in a real Unreal environment**.

## Next work

- Original authored vehicle meshes, collision and skeletal wheel assets, proper Chaos vehicle movement
- Garage menus with mouse/gamepad/touch navigation, confirmation and accessibility
- Vehicle-specific parts, physics balancing and reasonable racing-event difficulty
- Full Unreal Editor, Win64 and Android playtests

See [engineering status](STATUS.md) and [release qualification](https://github.com/sanskarIN/latebrake/issues/2).

## v0.3.1 transaction feedback and defensive migration

Garage purchases and selection now use the same portable `GarageState` lookup rules, rather than independent assumptions about car order in the Unreal game mode. The HUD displays whether a purchase or upgrade succeeded, lacked credits, or is already maxed. The UI also previews the next Engine and Grip upgrade prices (zero means fully upgraded).

Malformed negative ownership masks are treated as the starter-car-only state, avoiding the previous risk of granting every vehicle due to a sign bit. Valid positive known ownership bits remain preserved. The portable tests also check that old zero/missing profile fields do not remove the Arrow starter car.

These are code-level behaviors only; real Unreal SaveGame serialization, controller mappings and HUD rendering require manual editor/hardware verification.
