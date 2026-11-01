# Experimental Unreal race-local pit service (source milestone)

This is a **production Unreal codepath**, unlike the separate Win32/GDI
preview experiment. It is still a source milestone, **not** a qualified
Unreal Engine 5.8 build, polished animated service garage, official racing
regulations, or a tested Windows/Android release.

## Existing flat circuit and in-game controls

- The prototype pit is a stationary bay on the **inner south paved
  shoulder**, centered at Unreal XY **(0, -1300 cm)**; the south-straight
  racing centerline is Y=-1800 cm. It uses existing pavement and does not
  create a new pit-lane road or new track mesh.
- During an eligible live race, steer towards the inner shoulder while
  slowing to **at most 120 cm/s (4.32 km/h)**. No throttle and no held
  nitro may accompany service. Hold **P** (keyboard) or **Y/top face
  button** (Xbox-style gamepad) for **2.5 continuous advancing seconds**.
- A service succeeds only when chassis impact damage, arcade tire wear,
  or nitro charge actually needs attention. A completed stop repairs
  **up to 25 percentage points** of race-local chassis damage, fits the
  baseline synthetic tire condition, and restores **up to 40 percentage
  points** of nitro charge while cooling boost heat.
- The bay is a 250 cm radius area. To do another stop, **drive out of
  the bay and return**. Releasing P/Y, moving too quickly, pressing the
  throttle, or holding nitro clears progress. A pause/menu/focus loss
  clears partial progress. A new race resets stop count.
- A non-compact desktop HUD readout shows distance to the bay within
  15 metres, current service hold percentage and stop count. It is
  intentionally omitted from compact mobile HUD layouts pending a
  proper touch-native pit control and real device QA.

The Y/top face button is contextual: the menu already uses it for
confirmation when open. An extra guard in `MenuActivate` prevents the
closed-menu input handler from erasing a live pit-service hold.

## Important limitations

- The visual bay has no authored sign, safety barrier, pit crew animation,
  sound or custom `.umap` geometry yet. The HUD is initial wayfinding,
  **not a finished visual pit lane**.
- This cannot repair damage outside the bay, create car ownership, alter
  unlock costs, award checkpoint credits, change record files, teleport
  the driver, or pause the official racing clock. Race-only chassis/
  tire/nitro state is not persisted to `SaveGame`.
- Physics and input are still the experimental UE rigid-body controller,
  not fully qualified Chaos wheel simulation.
- Unreal 5.8 Editor compilation, packaged Windows running, physical
  gamepad/keyboard service and competitive rival clearance are **not
  established by synthetic CI**. Do not release as playable-qualified.

## Tests

```sh
cmake -S Tests/Standalone -B build/pit-test
cmake --build build/pit-test --parallel
ctest --test-dir build/pit-test --output-on-failure
python -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

`RacePitServiceTests` exercises invalid/paused samples, timing, button
releases, physical position, throttle, nitro arbitration and exit/reentry.
`RacePitVehicleServiceTests` tests the actual arcade damage, tire and
nitro service functions. `UnrealPitServiceBindingTests` checks source
wiring and the contextual gamepad Y behavior. These are portable/source
contracts only.

## Required engine and actual device QA

1. Compile genuine UE 5.8 Editor/Win64 with UHT and launch a real generated
   circuit map; ensure there are no compilation errors or actors missing.
2. Visually confirm that inner south shoulder is reachable, paved and
   clear of checkpoint gates and AI centerline. Hold P at less than
   4.32 km/h without throttle. Run the same sequence on gamepad Y.
3. Create genuine damage and spent nitro in a race, then verify one 2.5s
   pit completion restores only the bounded amount and race clock
   continues normally. Check synthetic tire condition without claiming
   real tires were fitted.
4. Confirm keyboard Enter and controller Y still activate the open
   desktop menu, and Y does not cancel driving commands when the menu
   is closed. Test pause/focus loss, menu changes, teleport recovery
   and restart while holding the pit key.
5. Try to exploit repeats from inside the same bay; ensure there is no
   second service until the chassis exits and re-enters. Check no saved
   garage/career data changes; exercise AI rivals in the same area.
6. Confirm the HUD text does not collide with the existing instrument
   panel on 720p, 1080p, ultrawide and mobile. Add real track signage,
   a mobile input and visual polish before treating this as a game
   feature ready for a published playable build.

Track genuine game UX qualification in
[issue #236](https://github.com/sanskarIN/latebrake/issues/236)
and authentic release evidence in
[issue #148](https://github.com/sanskarIN/latebrake/issues/148).
