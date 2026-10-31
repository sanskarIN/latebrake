# Original garage car silhouettes — source QA

Three fictional garage configurations now have different *cosmetic*
engine-primitive silhouettes in the Unreal starter pawn:

| Garage profile | Decorative shape |
|---|---|
| Arrow | Balanced hood and rear deck, clean wingless body |
| Kestrel | Longer hood, shorter rear deck, wide low spoiler |
| Phantom | Shorter hood, longer rear deck, raised narrower spoiler |

Cabin length and height remain based on the existing garage specifications.
The new hood, rear deck, wing and support components attach to the existing
chassis. They use only Unreal's built-in cube mesh with no third-party
art assets and no extra authoritative vehicle physics. Every new component
is decorative and must remain collision-free.

This is **not** a claim of licensed car design, production-quality original
mesh assets, finished rendering, vehicle customization, or playable builds.
Chassis force tuning and earned car ownership are unchanged.

## Source verification

```sh
python -m unittest discover -s Tests/Editor -p '*Tests.py'
cmake -S Tests/Standalone -B build/test
cmake --build build/test --parallel
ctest --test-dir build/test --output-on-failure
```

The portable `VehicleVisualRecipe` regression checks distinct body
proportions and invalid garage IDs. The editor binding source test verifies
that each decorative part is attached and the garage selects the recipe.

## Installed game acceptance

A UE 5.8 workstation must confirm all three earned garage cars visibly
differ in chase and showroom cameras; wing supports do not float, clip
or intersect the cabin; shadow cost is acceptable; no silhouette
changes chassis collision, wheel placement, spawn, recovery, handling,
checkpoint crossing, or speed. Test restarting, unlocking/changing
selected car and launching a career event.

An actual engine compile, owned map cook and installed Windows playtest
are still required by [issue #148](https://github.com/sanskarIN/latebrake/issues/148).
