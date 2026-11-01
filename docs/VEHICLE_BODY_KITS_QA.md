# Six original 3D body-kit variants — Unreal source milestone

This change improves the **existing Unreal Engine 5.8 pawn**. It is
separate from the experimental Windows GDI preview and does **not** turn
these primitive meshes into finished production car assets.

## What changes

The garage already contains six fictional cars with append-only save IDs.
These continue to use the same physical Chaos rigid-body chassis and
existing performance/tuning policy. The decorative body parts attached to
the chassis gain seven additional, collision-free primitive elements:

- A front splitter with model-specific width.
- Paired side skirts with model-specific lengths.
- A rear diffuser with model-specific width.
- An optional raised hood scoop.
- Optional paired rear fins.

The existing cabin, hood, rear deck, wing and wheel animations remain
unchanged. The new parts attach to the same root and compensate its
nonuniform scale. They never become a collision shape or overlap sensor.

| Garage ID | Car | Body kit | Visible identity |
| --- | --- | --- | --- |
| 0 | Arrow | STREET | Balanced trim, no scoop or fins |
| 1 | Kestrel | SPRINT | Wider front and small scoop |
| 2 | Phantom | GT | Long side blades with rear fins |
| 3 | Tempest | TRACK | Widest front and raised scoop/fins |
| 4 | Glider | TOURING | Extended skirts and restrained aero |
| 5 | Nova | APEX | Broad rear diffuser, scoop and tall fins |

The existing read-only garage catalogue labels the body kit in its
expanded desktop detail rows. Locked and saved vehicle ownership is **not**
modified. Invalid vehicle IDs fall back to the starter visual design.

## Six independent garage paint identities (unreleased source development)

Each existing fixed garage ID also has a separate, engine-neutral and
immutable original paint recipe. The body / trim / canopy RGB fields
are bounded [0, 1] and do not add saved paint purchases or customize
ownership. The expanded garage menu labels each car's recipe:

| Car | Paint name | Visual intent |
| --- | --- | --- |
| Arrow | SKY BLUE | Blue body with pale trim and dark canopy |
| Kestrel | EMBER ORANGE | Orange body with graphite trim |
| Phantom | ELECTRIC VIOLET | Violet body with cool pale trim |
| Tempest | RACING SCARLET | Red body with charcoal aero |
| Glider | SEA GLASS | Teal body with pale highlights |
| Nova | SOLAR YELLOW | Yellow body with dark accents |

The real Unreal pawn creates dynamic material instances only during
`BeginPlay` for its built-in mesh components and reuses them when the
selected garage car changes. It writes the conventional vector parameter
`Color` on the body, canopy and collision-free kit components. No
materials are created in the Unreal class-default constructor or
per-frame driving tick. It retains the mesh's original material parent,
avoiding intentional edits to physical-material properties.

**Rendering caveat:** this is still an **unverified material integration**
until an actual UE 5.8 editor session confirms the loaded built-in
primitive parent material exposes a vector parameter named `Color`.
A missing parameter may leave one or more parts in their default
material even while the garage HUD correctly labels the paint. If so,
author and verify a project-owned material with an exposed `Color`
parameter before promoting this from source-only to a playable feature.
Do not claim verified fully colored 3D cars from source tests alone.

`VehiclePaintRecipe` standalone C++ tests validate all six names,
bounded palettes, contrast between body / accent / canopy, corrupted
vehicle-ID fallback, and deterministic repeated selection.
`UnrealVehiclePaintBindingTests` ensures that the pawn's updates are
bound only to live actor creation / garage switching and do not change
Chaos forces, SaveGame, unlocks or rewards.

## Performance and gameplay isolation

Seven new components are allocated **once per car actor construction**.
They use built-in engine cube primitives already loaded for the original
car; they do not load custom meshes, shaders, textures or marketplace
assets. All new parts have **NoCollision**, overlap events off, physics
simulation off and dynamic shadows off. Geometry and visibility only
change on garage selection, not in the per-frame chassis tick. No game
reward, speed, damage, collision, checkpoint, or SaveGame code is added.

## Automated verification

```sh
cmake -S Tests/Standalone -B build/vehicle-body-kits
cmake --build build/vehicle-body-kits --parallel
ctest --test-dir build/vehicle-body-kits --output-on-failure
python -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

`VehicleBodyKit` validates the six immutable shape identities, finite
dimension limits, optional-part flags, invalid-ID fallback and repeated
selections. `UnrealVehicleBodyKitBindingTests` checks the actual pawn
component declarations, attachment, collision exclusion, visible variant
wiring and garage readouts. These source tests **do not compile Unreal**.

## Mandatory real UE5.8/Windows QA

1. Build the **actual UE 5.8 UHT/Editor/Game targets**, author/load the
   real original circuit map and cook a Windows candidate. Do not
   substitute standalone CMake tests for this step.
2. Start an actual session, inspect all six models from front/rear/
   side/top cameras, and confirm the rear fins, scoop and trim genuinely
   appear in installed rendering. The engine default primitive material
   may not supply the contrast needed; original materials will follow.
3. Unlock/equip each car and cycle back to Arrow. Ensure parts disappear
   when disabled, correct tuning is applied and no car-specific
   mesh configuration persists after swapping.
4. Drive into walls and around rivals; confirm only the original chassis
   responds to contacts, no new stuck collisions or unexpected tire
   behavior occurs, and lap time/credits/save format remain compatible.
5. Measure actual CPU/GPU frame times, camera clipping and visual
   density on Windows and Android. Verify these cosmetic primitives
   do not excessively reduce frame rate or overlap HUD elements.

This is **source-level visual progress**, not an artist-authored sports
car mesh, working multiplayer, a tested installer, or a qualified
playable Unreal build. Genuine playable-release evidence remains tracked
by [#148](https://github.com/sanskarIN/latebrake/issues/148) and
production UI quality by [#236](https://github.com/sanskarIN/latebrake/issues/236).
