# Experimental Unreal race-local pit service (source milestone)

This is a **production Unreal codepath**, unlike the separate Win32/GDI
preview experiment. It is still a source milestone, **not** a qualified
Unreal Engine 5.8 build, polished animated service garage, official racing
regulations, or a tested Windows/Android release.

## Unreleased Android PIT HOLD touch input (source development)

A new dedicated on-screen **PIT HOLD** rectangle occupies the previously
unused middle row, at relative viewport coordinates X=43%–66% and
Y=40%–50%. It does **not** overlap GAS, BRAKE, STEER, GHOST, CAM,
menu/navigation or recovery pads. The HUD draws the same shared
`TouchControlLayout` rectangle used for input hit testing.

Only an original physical finger-down in that rectangle during a live
ready post-GO local-player race can acquire the pit touch owner. That
same finger must remain held inside the pad. A release, movement out,
malformed input, viewport rotation/resize, pause, menu, lost focus,
restart or physics-blocked driver state cancels ownership. Moving back
into the pad never reacquires without a new finger-up/down. The
independent `TouchPressTracker` retains original physical-down latches,
so duplicated events and unrelated steering/pedal fingers cannot
trigger extra service. Keyboard P and gamepad Y retain their bindings.

An active PIT touch only supplies the **held request** to the
existing physical `RacePitService` state machine. It does not
silently bypass actual speed (at most 120 cm/s), measured position,
neutral throttle, unheld nitro, service-need or 2.5-second continuous
hold requirements; race and reward rules are unchanged. The visible
pad remains advisory if the chassis/race is not eligible. All features
are **source-only until genuine Android on-device multi-touch tests**.

Manual Android QA must cover 360×640 portrait and landscape, safe-area
cutouts, rotation during a hold, two simultaneous steering/pedal
fingers plus PIT, release outside pad, focus loss, system dialogs,
race finish/restart and zero unintended pit-service actions. Capture
physical on-device footage/logs and actual package hashes. Cross-platform
C++ tests and Python source checks cannot substitute for that evidence.

## Visible pit-bay pavement outline (unreleased source development)

The existing overhead pit beacon identifies the general inner shoulder, but
alone does not tell a driver where the **250 cm radius service zone**
actually starts and ends. A new source-level visual ring contains
**eight separated mint-colored pavement dashes**, arranged around the same
`RacePitBay` coordinate used by the physical service state machine.
Each short dash is centered approximately 190 cm from the pit center and
its full length stays inside the **actual 250 cm service radius**.

The Unreal `BuildCircuit` source attempts to render these as **one actor
with one instanced cube-mesh component**, not eight independent collision
actors. Physics collision is `NoCollision`, overlap events and navigation
effects are disabled, and dynamic shadows are off. The existing nine
audited collision blocks, four gate/checkpoint actors, timed service
eligibility, lap rewards and saved games are untouched. Marker failure
is logged as an optional visual failure and cannot change track
readiness or create false official race results.

`PitBayPaint` standalone C++ tests verify all dashes are finite,
distinct, actually on paved serviceable road, wholly within the
physical service zone, and deterministic across repeated planning.
`UnrealPitBayPaintBindingTests` checks the production Unreal source
for no collision, no navigation and absence of gate/race mutation.

**Actual UE 5.8 visual QA is still required**: inspect whether the
built-in engine cube's parent material supports the requested `Color`
vector parameter, ensure the short ground stripes visibly sit above
the existing south asphalt, drive through them with no impulse or
surface-grip changes, and confirm performance on Windows/Android.
If the material lacks this parameter, author an original material before
describing them as colorful in a finished game. No installed executable
or authentic Unreal render has been produced by these source tests.

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

The non-compact **RACE RADAR / NORTH UP** also displays a small `PIT`
symbol projected from the **same `RacePitBay` XY coordinates** used by
the physical service eligibility policy, so there is no separately
estimated wayfinding location. It remains a static decorative marker
and does not change eligibility, time, rewards or checkpoints. Radar
is intentionally hidden on compact windows and mobile HUDs.

The Y/top face button is contextual: the menu already uses it for
confirmation when open. An extra guard in `MenuActivate` prevents the
closed-menu input handler from erasing a live pit-service hold.

## Important limitations

- The generated circuit includes an **optional, non-colliding colored
  pit beacon** on the inside edge near the bay and the HUD provides initial
  wayfinding. There is still no authored text sign, safety barrier, pit
  crew animation, sound, custom `.umap` geometry or finished visual pit lane.
  Its absence must never claim track collision/startup readiness or affect
  lap timing. Real rendering and contrast require live Unreal QA.
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
2. Visually confirm that the colored nonblocking pit beacon appears and
   the north-up desktop radar displays a PIT marker inside the south
   circuit line. Confirm the point remains aligned with the physically
   serviceable inner shoulder, clear of checkpoint gates and AI centerline. Confirm the beacon cannot stop or damage the car. Hold P at less than
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
