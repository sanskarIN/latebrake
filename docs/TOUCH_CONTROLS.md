# Touch controls (v0.4.1 source candidate)

> **Experimental Android-only on-screen touch interface.** This source has not
> been compiled with Unreal Engine 5.8, run on a phone, or packaged as an APK.
> Rectangles in the engine HUD are not native accessibility widgets.

## Layout and gestures

The HUD and the vehicle input dispatcher use the same engine-independent
`UI/TouchControlLayout.h` rectangles, scaled to the live viewport:

| Screen area | Action | Interaction |
| --- | --- | --- |
| Lower left | Steering | Hold and move sideways; center means straight |
| Lower right (upper pad) | GAS | Hold to accelerate |
| Lower right (lower pad) | BRAKE | Hold to brake |
| Upper right, first pad | PAUSE | Tap to pause; tap again to resume |
| Upper right, second pad | RESTART | Tap to restart the current event, also unpauses |
| Upper right, third pad | RECOVER | Tap to place the car near its next gate; +5s penalty |

Steering and pedals support two independent simultaneous fingers. Touches
that **begin outside** a control cannot activate it merely by moving over it.
The gap between gas and brake is neutral; a captured pedal finger crossing
into the gap releases both. Invalid touch coordinates neutralize held values.
The pad tint changes while steering/gas/brake is held. Touching race telemetry
does not activate the vehicle.

The input bindings for **initial presses and finger releases** execute while
paused. The PAUSE pad can request resume, while a GAS/STEER/menu finger
lifted during pause clears its physical-down tracking slot rather than
staying stuck until an unrelated OS focus reset. Release callbacks only
free ownership; they never trigger an extra action. A finger still down
when the menu is toggled stays latched until its genuine release, so
duplicate presses cannot replay PAUSE/RESTART/RECOVER. Android input
routing during pause still requires actual device verification.

## v0.5.43 manual PAUSE fallback (source-only, unqualified)

A standalone keyboard/gamepad or mobile PAUSE press now latches a game-level
pause **before** requesting Unreal `SetGamePaused(true)`. If the engine
continues ticking despite this request, the latch still blocks the racing
clock, ordered gates, recovery, player force/torque, rivals and driving touches.
The HUD shows PAUSED, and the mobile PAUSE pad remains highlighted. A genuinely
new PAUSE press can explicitly clear the fallback; dragging or reusing a
physically held finger cannot. Menu close and accepted restart also clear it.

**Required actual Win64/Android checks:** verify normal PAUSE/RESUME first;
instrument or deliberately simulate an engine world-pause refusal in an
authorized non-shipping Unreal build, confirm that the world may still tick
but the race clock, HUD, checkpoints, rival and car propulsion are frozen,
confirm a fresh second PAUSE press resumes, and validate no pre-pause
checkpoint sample can credit a gate after resuming. Repeat with app focus
loss and an Android PAUSE tap; foreground alone must not resume. Preserve
logs linked to the exact tested executable and source revision.

Portable C++20 and source-binding tests simulate the policy; they do
**not** demonstrate that Unreal or any phone has delivered these events.

## Engine/device acceptance checklist (NOT YET COMPLETED)

1. Compile `LatebrakeEditor` and the game target with a licensed Unreal 5.8
   installation and inspect compiler/UHT logs (including touch bindings).
2. On at least one landscape and one portrait Android viewport, check that
   all six pads appear in the expected positions without covering essential
   race information.
3. Hold steering plus GAS with separate fingers; move steering to each side.
   Release GAS and confirm the car stops accelerating without residual input.
4. Slide the GAS finger through the neutral gap and into BRAKE; confirm
   immediate release, braking and no unexpected reverse acceleration.
5. Start a touch outside the pads, slide into GAS and confirm the car does not
   accelerate until a **new press** begins inside GAS.
6. Tap PAUSE twice. Confirm physics stops, held inputs are cleared and play
   resumes without stuck steering or throttle.
7. Tap RESTART while paused and while driving. Confirm progress, penalties and
   rival resets and that the new race runs unpaused.
8. Tap RECOVER while racing; verify a single 5-second penalty and that repeated
   physical input works. Verify RECOVER cannot act after race completion.
9. Verify multi-touch release/cancel and focus-loss behavior on real devices.
   If OS interruption creates stuck fingers, implement explicit cancel/focus
   cleanup before any playable release.
10. Measure HUD legibility, touch-target size, thermal performance, frame time
    and input latency on representative low- and high-end Android hardware.

## Portable-only tests

Run:

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release --output-on-failure
```

`TouchControlLayout` checks bounds, hit-testing, steering clamping,
portrait layouts, safe button separation and malformed numeric input across
portable C++20 builds. These tests do **not** compile Unreal's touch dispatcher,
exercise the actual HUD render path, or replace physical device QA.

## v0.5.29 source development — neutralize residual commands

The original touch owner state already stopped reporting GAS or steering
after a finger-up or malformed input. However, the real Pawn applies
`SmoothedThrottle` and `SmoothedSteering` in its physics tick. Their old
values could remain nonzero after touch ownership was canceled, briefly
applying motor force or steering torque after the player stopped commanding
it.

`UI/TouchResidualSafety.h` now binds touch cancellation to those actual
applied commands. The Unreal `TouchReleased` callback zeros the released
finger's filtered output before freeing its physical press slot; malformed
touch coordinates cancel only the affected owner. An owned pedal dragged
from GAS to the neutral gap or BRAKE now cuts residual acceleration, while
exiting the steering pad vertically zeros residual steering. Separate
steering/GAS fingers do not cancel one another; a valid owned finger can
move back to GAS or steering without invoking a discrete action.

Portable C++20 regression fixtures check these transitions and the next
physics-step input; a source-contract Python test checks the real Pawn's
callback integration. Neither can confirm installed Unreal input delivery
or actual rigid-body forces. Test these cases on Android under
[issue #13](https://github.com/sanskarIN/latebrake/issues/13).

## Future accessibility work

Add configurable left/right-handed pad placement, larger minimum target
sizes, explicit mobile garage/event selection, touch joystick affordances,
screen reader labels, remappable controls, and production UMG widgets.
No such settings or features are claimed in this milestone.

## v0.5.0 source additions (unverified on Android)

- Two career pads (NEXT/CAREER) appear in the racing HUD.
- In the paused development menu, NEXT advances panels, SELECT activates,
  BROWSE cycles eligible career/challenge IDs, and CLOSE returns to driving.
- The menu suppresses driving touch dispatch entirely; touches on GAS or
  BRAKE must not affect the vehicle while menu focus is active.
- Lifecycle delegate hooks clear captured touch/analog controls on app
  deactivate, background and foreground transitions; unpossession/endplay
  also clear controls and remove listener registrations.

Physical-device testing of orientation changes, Android back/home,
notifications, fingers released while suspended, repeated foreground
activation, and touch overlap/legibility remains **required**. Do not close
issue #13 on the strength of portable geometry tests.

## Development menu pause reconciliation

When the development menu is open, keyboard Escape/gamepad Pause now
requests menu closure rather than directly resuming the game behind the
menu. On Android, the menu's CLOSE pad continues to use the menu route.
This is a source-level fix and **requires Unreal and real phone tests**
for actual pause delivery while the game is paused.

## Controller-level Android setup (unreleased v0.5.1)

The race GameMode now explicitly instantiates
`ALatebrakePlayerController`. Its Android-only BeginPlay activates
game-only input and removes Unreal's overlapping default virtual joystick,
while enabling touch events for the pawn's own drawn, multi-finger pads.
Windows keyboard and gamepad handling are unchanged.

**Required real-device checks:** thumb steering plus simultaneous pedal,
finger release after leaving a pad, opening the prototype menu with fingers
held, pause/resume, background/foreground lifecycle neutralization, small
screen layout and touch re-acquisition after focus returns. Android code
still has **not** been compiled or played on hardware in this branch.

## v0.5.1 development: held-finger ownership safety (not device-qualified)

`UI/TouchDriveState.h` now owns all driving finger capture state.
The Unreal pawn forwards press, move, release, menu and lifecycle events to
that engine-independent state machine rather than keeping separate mutable
steering and pedal flags. A held finger can **never** turn into an unrelated
new press by dragging across a restart, recovery or pause button.

- Only a **new press** inside the steering pad or a pedal pad can acquire
  that control. Dragging in from empty telemetry or the menu cannot acquire.
- Two different fingers can steer and accelerate/brake concurrently. Other
  fingers cannot steal or release an active drive control.
- Steering clamps on horizontal drags; leaving the steering pad's vertical
  band immediately neutralizes its axis. Releasing the finger frees ownership.
- One captured pedal finger can slide `GAS → neutral gap → BRAKE` or back.
  The neutral gap and any other UI region immediately release acceleration.
- Invalid NaN/infinite coordinates cancel the affected finger. A globally
  invalid viewport layout cancels **both** held controls on the first
  observed touch event after a resize or orientation change.
- Menu ownership, the standalone pause state and app foreground/background
  interruptions neutralize captured drive input; late touch moves after
  cancellation cannot reacquire anything without a new press.
- On the Android HUD, held control highlights use the **same** shared
  capture state as the physics update; they are not simulated indicators.

Portable `TouchDriveStateTests.cpp` validates capture, ownership, multi-finger
input, horizontal steering, release, invalid touch events, neutral drag,
menu-action drag avoidance and cancellation on all three CI operating systems.
**It does not mean that actual Android devices or Unreal touch bindings
have been compiled, installed or tested.**

## v0.5.1 development: single-fire discrete touch actions (not device-qualified)

The separate portable `TouchPressTracker` records **all** initial touch indices,
including touches beginning outside visible pads. An existing held finger cannot
fire a second PAUSE, RESTART, RECOVER or career/menu action if Unreal dispatches
another `IE_Pressed` event before the corresponding release. Moving an action
finger to another button likewise cannot act without a physical release/new
press. Two different fingers remain independent. Driving input clears caused by
menus, restarts or recovery do **not** clear this tracker; real touch releases
do. App lifecycle interruption/unpossession resets the press tracker in case
the OS discarded release events.

Portable `TouchPressTrackerTests.cpp` tests duplicate action suppression,
independent finger ownership, invalid indices and interruption reset. This
does **not** verify Unreal's exact mobile press/release routing.

On real Android hardware: hold a finger on RECOVER long enough to receive
repeat callbacks and verify that **only one** 5-second penalty is assessed;
drag it over RESTART/PAUSE; verify no action until physical release.
Repeat with a second finger, a PAUSE/menu transition, and an Android
background/foreground event.

First real Android package acceptance checks should include: two physical
fingers held together, a second/third finger trying to steal a pedal,
moving one pedal finger across the neutral gap, dragging from gas over
PAUSE without activating the button, dragging steering vertically away
from the pad, rotating/resizing the viewport, background/foreground
transitions and opening/closing the menu during held input.

## v0.5.1 viewport/orientation ownership safeguard (unreleased)

A phone can rotate, enter split-screen/multi-window, or resize its render
surface **without moving any of the fingers already held on screen**. The
original pixel coordinates then refer to obsolete GAS/BRAKE/STEER pads.

The portable `TouchViewportGuard` compares successive viewport dimensions.
On Android the car pawn calls it **before physics** in `Tick` and also from
the touch dispatcher (which can still receive input while the game is
paused). Any real change in valid viewport width or height immediately
clears both held steering and pedal ownership. Missing/too-small
viewports also neutralize the driving controls. Same-size frames preserve
the current controls.

Crucially, a viewport change does **not** reset
`TouchPressTracker`: old physical fingers remain marked down until an
actual release. This prevents duplicate RECOVER, PAUSE, RESTART or menu
actions from the same held finger, and prevents late touch movement from
automatically reacquiring acceleration. The driver must lift and press
again to control the car in the new screen layout. Genuine app lifecycle
focus changes separately reset the press tracker because the OS may have
discarded touch-up events.

## v0.5.2 post-release input filter correction (source only)

The original viewport guard removed GAS/STEER finger ownership on
rotation/resizing, but did not reset `SmoothedThrottle` and
`SmoothedSteering` in the car's rigid-body control. Those slew-limited
signals could still apply acceleration or turning torque for several
physics ticks after the old finger was invalidated.

Now a single engine-independent `NeutralizeTouchOnViewportChange`
function is called by both the Android pre-physics Tick check and the
touch dispatcher. On a changed/invalid viewport it clears the
`TouchDriveState` owner **and** zeroes the two smooth filters, so the
next physics update cannot reuse pre-rotation drive commands. A stable
viewport does not interrupt currently held controls; legitimate fresh
presses can acquire the new layout after release. The separate
`TouchPressTracker` still prevents duplicate discrete actions from
the previous held finger.

When the local Android player controller disappears, the pawn resets
all raw drive axes and filters as well; otherwise an old analog gamepad
value could rebuild throttle on the next frame.

**Additional real-device checks:** hold full GAS and steering at speed,
rotate without releasing fingers, inspect applied force/torque on the
next physics tick; there should be no new throttle/steering commands
from the old touches (vehicle momentum will still carry the car).
Repeat with transient invalid window, an external gamepad, lost
controller/possession, pausing and returning to the application.
Portable C++20 tests confirm only the command policy; physical motion,
Unreal integration and Android devices are not yet verified.

### Installed Android QA — still required

- [ ] Hold steering and GAS, rotate from landscape to portrait; both
  commands must neutralize **without needing a moved event**.
- [ ] Return to landscape, change screen bounds using windowing/insets,
  or trigger a transient zero-sized viewport: no old gas input persists.
- [ ] While a finger remains down across the resize, duplicated Pressed
  or delayed Moved events must not reacquire or activate race actions.
- [ ] Lift and press in the new layout: steering, gas and brake work
  independently and menu controls still accept fresh taps.
- [ ] Test rotation while paused or inside a career/garage menu; no held
  driving axis should carry into an unpaused race.
- [ ] Measure FPS and frame time on target phones; the extra viewport
  query is source only until real UE 5.8 / Android validation.

These invariants have portable C++20 regression coverage but **no
installed Unreal build or device evidence**. Keep issue #13 open.

## Focus-loss race suspension (next source milestone, not device-qualified)

On application **deactivation/background**, the actual player pawn now clears
analog and touch drive commands and asks GameMode to **pause the active race**.
An engine-independent `AppFocusRaceGuard` also latches suspension
independently of Unreal's `SetGamePaused` result. This avoids accumulating
race time, accepting checkpoint crossings, charging RECOVER penalties or
advancing the rival route if a platform keeps ticking after losing focus.

A foreground notification clears OS-stale held inputs, **but never
automatically resumes** the race. Only a deliberate PAUSE/resume, menu CLOSE
or accepted race RESTART returns gameplay to an active state. Menu-open and finished races do not acquire a redundant focus-loss latch.
For an already-paused race, the latch still prevents OS-level automatic
foreground resumption from restarting the clock; no duplicate world-pause
request is made. **Every NEW focus-loss latch advances the checkpoint
session epoch, even if the OS paused Unreal before the focus delegate fired.**
Repeated deactivation/background callbacks are ignored and do not advance
it again. A fresh checkpoint epoch prevents a crossing bridged between
pre-loss and resumed positions.

The portable `FocusLossTransition` decision distinguishes a newly latched
race whose Unreal world is already paused from a newly latched race still
needing a world-pause request. This distinction fixes the older case where
an already-paused world caused GameMode to return before invalidating the
checkpoint epoch. It is a source-level fix until genuine UE runtime tests.

**Additional v0.5.38 installed QA:** pause using the OS before the gameplay
deactivation delegate runs; verify the first post-resume gate sample cannot
score and a fresh approach/crossing can. Repeat deactivation callbacks and
confirm they cannot create extra transitions. Test the same scenarios when
the engine pause request fails. Record the exact packaged revision and
installed-device traces under issues #31 and #13.

**Real-device acceptance still needed:** with a genuine UE 5.8 Win64 and
Android package, hold GAS + steering while switching applications, opening
notification shade, rotating, pressing Home, and returning. Confirm the
clock, rival and checkpoints stop while backgrounded; verify PAUSE resumes
once without latched controls; verify no race action replays from an old
finger. Also test duplicate deactivation/foreground callbacks, menu-open
deactivation, and interrupted SaveGame operations. Portable source tests
cannot confirm actual OS event delivery or in-engine pausing.

### v0.5.39 source-only pre-release: physics and touch on failed OS pause

If the app deactivates but Unreal fails to pause the world, the race-suspension
latch also zeros the player's throttle, steering, filtered axes, and physical
chassis linear/angular velocities on deactivation and surviving pawn ticks.
New touches while suspended cannot drive, recover, restart or select career;
only a fresh PAUSE press can invoke an intentional manual resume. Existing
finger-down tracking rejects duplicate PAUSE notifications.

**Installed QA still required:** while moving at nonzero speed, force the
Unreal pause request to fail on an authorized engine build, switch out of the
app, and verify both that the car does not travel a meaningful distance while
the timer is frozen and that no throttle/steering or action taps are accepted.
Return, tap PAUSE once, verify the first resumed movement has no stale gas or
turning command, and verify genuine new steering/throttle presses work.
Repeat for already-paused focus loss, duplicate OS events and OS auto-unpause.
Check an installed Win64 candidate and physical Android hardware; the source
contracts and portable policy tests cannot substitute for this evidence.

## v0.5.69 navigable paused menu details (source-only)

When a menu is open, the top-right pads normally labelled
**RESTART** and **RECOVER** in racing are drawn as **UP** and
**DOWN**. A newly admitted press scrolls the *detail pages*
instead of restarting or charging a recovery; the existing NEXT,
SELECT, BROWSE and CLOSE menu pads keep their original roles.
While driving, RESTART and RECOVER retain their racing behavior.
Keyboard **Up/Down** arrows control detail pages; **PageUp/PageDown**
continue to switch menu categories.

The title is pinned and a numbered page notice explains when
additional results are available. Source regression tests check
pause-eligible dispatch and bounded UI-only state; physical Android
gesture delivery and readability still require licensed UE 5.8
runtime/device testing.

## Experimental touch steering center-noise filter (next source cycle)

The center 8% of the **normalized steering-pad half-width** now maps to
neutral steering. This is not a literal 8% of screen width. A captured
finger can move outside the band and receive a continuous, symmetric
rescaled response to full left/right lock, then return to neutral while
a separate GAS finger remains held. GAS/BRAKE, one-shot action hit areas,
capture ownership, pause/focus handling and gamepad's independent deadzone
are unchanged. The filter has no stored input state and performs no physics
or reward calculations. Returning to the neutral band also immediately
clears the owned smoothed steering output; otherwise a recently held turn
could leave residual applied torque while the thumb appears centered.

Strict C++20 portable tests cover both signs, a dense monotonicity sweep,
exact full lock, invalid inputs and two-finger gas/steering transitions.
A Python source contract checks that Unreal's press and move paths use the
same shared policy. This is **not** measured Android input latency,
thumb comfort, motor/steering performance or UE compilation.

On actual Android devices, test: press and jitter within the center band
while holding GAS with another finger (car should travel straight);
drag just past either edge of the band and back; reach both full locks;
release/rotate/pause while accelerating; and verify no unintended
torque, button overlap or stuck input. Evaluate both portrait and landscape
sizes and collect hardware feedback before tuning 8% or claiming the
feature device-qualified.
