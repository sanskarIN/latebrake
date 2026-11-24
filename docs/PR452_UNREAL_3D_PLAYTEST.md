# LateBrake PR #452 — real Unreal 5.8 Windows playtest

**For the project owner/tester. Current state: SOURCE CANDIDATE, NOT
a packaged or human-verified 3D game.** This document covers the
existing production six-car Unreal C++ racing prototype in
[PR #452](https://github.com/sanskarIN/latebrake/pull/452), not the
separate twelve-car native Win32 2D experiment.

## What is testable in this PR

- The real UE Pawn's generated six-car body meshes and ten material slots,
  with a safe visible legacy fallback when a tintable surface is missing
- Six direct numbered VIEW-only buttons in the paused Garage, and
  separate explicit equip and protected spend actions
- Eight color choices (F5), clean/center/twin stripes (F6), and
  **three different 3D wheel shapes**: STREET seven spokes,
  SPORT five broad spokes and TRACK twelve narrow spokes (F7);
  F8 restores the viewed owned car's look
- Two actual Unreal front-facing spotlights driven by the existing
  accepted-race signal, plus Settings **F3 DAY/DUSK** current-session
  light preview
- Development Settings readout: **LIVE CAR MESH**, exact
  **PAINT PARAM**, and **FRONT BEAMS REQUESTED**, each explicitly a
  source observation rather than a measured GPU rendering pass

The generated meshes are not professional licensed 3D automobile assets,
and this source does not implement Chaos wheel suspension, real-world
racing multiplayer, or a verified full 3D release.

## Prerequisites to test the actual 3D game

You need Windows x64, a working GPU/driver, a genuinely installed
**Unreal Engine 5.8** with UnrealEditor.exe, Build.bat and RunUAT.bat,
the correct Visual Studio MSVC/Windows SDK toolchain, **Git + Git LFS**
and **PowerShell 7**. The normal GitHub-hosted CI cannot install/cook
Unreal or provide a real 3D game EXE.

### Path A: authorized owner-managed GitHub runner

If you already have a *trusted* self-hosted Windows x64 runner labeled
`unreal-5-8`, an isolated licensed UE5.8 installation, and its
`LATEBRAKE_UNREAL_ROOT` environment variable:

1. Inspect the exact current changes and 40-character head SHA on
   [PR #452](https://github.com/sanskarIN/latebrake/pull/452).
2. On the main-branch version of
   [Manual trusted Unreal 5.8 PR validation](https://github.com/sanskarIN/latebrake/actions/workflows/validate-draft-unreal-pr.yml),
   choose **Run workflow** and **main**, enter `pr_number=452`,
   `expected_head_sha=<the full CURRENT PR head SHA>` and
   `validation=Editor`. Wait for actual UHT/Editor execution and
   inspect logs; a queued job with no runner is not validation.
3. Only if that genuine Editor compile succeeds, run the workflow
   again against the SAME reviewed head with `validation=Win64`.
   This must compile the Editor, author/cook an actual binary map
   using UE, and create a source/engine/map-hashed cooked archive.
4. Download the workflow's short-lived **UNVERIFIED-UE58-Win64-PR452**
   artifact (normally 3-day retention). Keep its complete directory
   together, validate its manifest with the existing source scripts,
   and install/run it on a separate Windows test machine.
5. Even a green Editor/cook job **does not** qualify on-device
   steering, car visuals, framerate, sound, saves or 3D gameplay.
   Use the acceptance table below and preserve the exact source SHA.

Do not enable automatic execution of PR code on your trusted Unreal
runner, run unknown fork changes, or merge to obtain test permission.
The workflow explicitly authenticates the same-repository owner head.

### Path B: local isolated Windows 11 PC (no self-hosted runner)

Review the PR diff *including scripts* before running any unmerged code.
Use a fresh, disposable Git clone, not your main local working copy:

```powershell
git clone https://github.com/sanskarIN/latebrake.git latebrake-pr452-test
cd latebrake-pr452-test
git lfs install
$Expected = Read-Host 'Paste CURRENT full lowercase 40-hex PR #452 head SHA'
if ($Expected -cnotmatch '^[0-9a-f]{40}$') {
    throw 'Provide the currently reviewed PR head, not a release tag'
}
git fetch --no-tags origin pull/452/head
if ($LASTEXITCODE -ne 0) { throw 'Could not fetch the current PR head' }
$Actual = (git rev-parse FETCH_HEAD).Trim()
if ($Actual -cne $Expected) {
    throw "PR head changed: expected $Expected; actual $Actual"
}
git switch --detach $Expected
if ($LASTEXITCODE -ne 0 -or (git rev-parse HEAD).Trim() -cne $Expected) {
    throw 'Source checkout does not match the reviewed SHA'
}
git lfs pull
if ($LASTEXITCODE -ne 0) { throw 'Git LFS content unavailable' }
git status --short
```

Inspect unexpected status output rather than discarding edits or
bypassing source-provenance checks. Run this **only after** UE5.8 and
the required Windows build tools are installed, changing EngineRoot
to the actual UE5.8 directory:

```powershell
pwsh ./scripts/Build-WindowsPlaytest.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8' -Launch
```

This executes the real Editor/UHT, binary map authoring, UAT
BuildCookRun and package hashing. On success, it prints a complete
`Latebrake.exe` candidate directory and launches via
`Run-WindowsPlaytest.ps1` to capture a private runtime log and a
source-bound receipt. Keep the **entire cooked candidate directory**.
No source CI, static ZIP or unsigned 2D setup can stand in for that
Unreal packaging step. Do not upload personal logs or private system
information without reviewing them.

## On-device acceptance matrix

Mark each row PASS / FAIL / NOT TESTED and note the exact game build
SHA, platform and screenshots. This is a manual witness list, **not
automatically verified performance or release authorization**.

| Test | What you must observe |
| --- | --- |
| True 3D startup | Real cooked Unreal executable and four-gate circuit render and respond, without an invisible or non-drivable player |
| Mesh fallback | Development Settings shows `SOURCE MESH COMMITTED` with detected tint name; if it shows `LEGACY CUBE FALLBACK`, record that material/mesh failure rather than marking render success |
| Six original cars | Browse all six fixed IDs in Garage; locked previews never change equipped car or charge credits |
| Explicit equip | Select ENTER only on an owned car; compare returned actual race pawn to the intended equipped ID |
| Paint and stripes | F5 cycles eight paints; F6 cycles clean/center/twin stripes; inspect roof, panels and front/back at night and day |
| Three real rim geometries | F7 switches STREET 7, SPORT 5, TRACK 12 spokes across all four visible wheels; 3D model changes, not only material color |
| Animated source wheels | With 3D source mesh committed, drive/reverse/steer and verify all four original wheels physically appear to spin, fronts visibly yaw, pause freezes rotation and recovery resets phase; ensure tires remain centered under the car |
| Save and reload | Changed owned car's paint/stripes/rims persist after close/relaunch; future-save schema is not overwritten |
| Headlight direction | During an accepted active race, two front spotlights point forward onto the actual road and match the visible car bumper; check right-hand and left-hand turns |
| Rear light geometry | In DAY and DUSK test brake and reversing for both generated 3D source car and legacy Cube fallback; the red/white light sources should align near each car's true rear bumper, not remain buried inside the longer car |
| Pause/focus | Pause, open menus and Alt-Tab; headlights command off and controls do not remain latched; no unintended racing in background |
| Dusk visual evaluation | On Settings press F3; actual sun and sky visibly change to DUSK; F3 again returns DAY; F3 must not change credits or save file |
| Native source diagnostics | Development Settings reports fallback/source mesh, detected paint parameter, requested front beams and `3D WHEEL RIG: 4/4 COMPONENTS` if every source wheel was initialized; compare every value with actual pixels and physical rotation |
| Race and rival | Real acceleration/braking, collision, GO countdown, sequential checkpoints, lap/finish and offline rival still work |
| Display and performance | 900x600, 1280x720, 1920x1080, ultrawide where available; observe readable HUD, mouse hitboxes and frame-time/latency on your actual GPU |
| Correct exit evidence | Preserve real runtime log, source/engine/cook manifest and user QA notes; no unexpected post-run changes to sealed package |

The source code may pass all portable tests and still fail Unreal
material compilation or have poor gameplay graphics/handling.
Do not mark a row PASS based solely on CI. Compare captured screenshots
for each rim, car and time of day rather than trusting a debug label.

## QA evidence helper already in this repository

After an actual local game launch and private runtime receipt, the
existing review script prepares a user-editable, hash-bound checklist:

```powershell
python ./scripts/Review-WindowsPlaytest.py init --receipt 'D:\LateBrakePlaytestLogs\YOUR-SESSION.json' --output 'D:\LateBrakeQA\manual-checklist.json'
```

Then edit actual observations only and run:

```powershell
python ./scripts/Review-WindowsPlaytest.py evaluate --receipt 'D:\LateBrakePlaytestLogs\YOUR-SESSION.json' --checklist 'D:\LateBrakeQA\manual-checklist.json' --output 'D:\LateBrakeQA\manual-review.json'
```

Create your QA output directory first and keep it **outside** the
fingerprinted game folder. The evaluator deliberately reports
`SELF_REPORTED_WIN64_QA_UNVERIFIED`, not a playable certification.
Also record any visual-specific findings from the matrix above.
Do not publish a public game build or mark PR #452 ready for merge
until genuine UE Editor, cook, installed Windows and visual/handling
evidence are all reviewed.

## Quick option: separate 2D Windows experiment

For a no-UE trial, use the most recent successful
[Experimental LateBrake V5 Win64 preview](https://github.com/sanskarIN/latebrake/actions/workflows/experimental-win32-preview.yml)
artifact on the PR and follow
[2D trial quickstart](../Experiments/Win32Preview/TRIAL_QUICKSTART.md).
Its 12 top-down cars and setup EXE are NOT the six-car UE5.8 racing game;
a successful trial here cannot qualify 3D performance, shaders, wheels
or native race physics.

## Atomic car-selection + rim-style visual handoff

The former GameMode update first changed the selected car using
`SetGarageTuning`, which built its 3D body with the **previous**
car's `CurrentGarageLook.Wheels`, then delivered its actual saved look
using `SetGarageAppearance`. If the rim styles differed, this could
generate the native procedural car/4-wheel set **twice** on one equip
and briefly show a mismatched saved look.

The GameMode now passes the **selected vehicle's own validated saved
appearance** into the same tuning call. The real Pawn commits
`GarageVehicleId` and `CurrentGarageLook` first, then runs *at most
one* original 3D source-mesh rebuild and one palette update, according
to the engine-independent `GarageVisualTransition` policy. Pure
paint/stripe choices still only recolor/reposition materials; a
rim-only style change still rebuilds exactly once; tuning-only
upgrades change bounded performance multipliers without redoing
visual geometry. Invalid/future IDs fail closed to starter+factory
look. The 18-byte appearance SaveGame schema, ownership, purchase
authority, physics root and scoring logic are unchanged.

**Manual Unreal acceptance:** equip a newly owned car whose saved
TRACK rims differ from the currently equipped STREET rims. On the
first frame the car and all four wheel shapes must match the new
car's saved style; no factory-rim flash, incomplete material, or
double-rebuild hitch. Repeat with same rim style but different car,
paint-only changes, upgrades, reload and rollback after failed save.
Source-only tests cover every car/car/rim/rim/paint/stripe
transition, idempotence, invalid IDs and invalid saved styles, not
actual UHT/Editor/Windows frame-time certification.

## Debug a double-build hitch while equipping a car

The DEVELOPMENT-only Settings page now reports:
`3D SOURCE BUILDS: X ATTEMPTS / Y READY`. These counters change only
when `RebuildOriginalVehicleVisual` actually starts constructing its
five-part source car and once after all source materials are allocated
and the body/four wheels are committed. Attempts that hit missing
materials or fail mesh validation never count as READY. These are
**source-method counters**, not a profiler, GPU proof or a 3D game
verification receipt; they saturate instead of overflowing.

Owner acceptance sequence:
1. After normal player startup, inspect the initial attempts/ready.
2. Equip another **owned** vehicle whose wheel style differs: return
   to Settings and verify the attempt count grew by **one**, not two.
3. Change only F5 paint or F6 stripes on the equipped car: verify no
   additional mesh attempts.
4. Change F7 rims: expect exactly **one** new source attempt.
5. Change only engine/grip upgrade: do not reconstruct source mesh.
6. Force a missing tint/material in an isolated developer copy:
   attempts may rise without a ready build; legacy car stays visible.
7. Confirm the HUD report never claims actual GPU/hardware quality.

The source test covers saturation, 300,000 accounting transitions,
failure paths and no fake committed source mesh. Actual UE5.8/cooked
Win64 test and screenshots still required.

## Verify the actual built rim identity, not only the selected button

Development Settings also shows `LIVE 3D RIM: STREET / SPORT /
TRACK` using the *authored rim style of the last successfully
committed original vehicle mesh*. Failed material creation and
fallback geometry explicitly report `FALLBACK / NO SOURCE MESH`
instead of repeating the desired UI choice. The value resets before
each build, so a failed new-car equip cannot falsely report the
previous car's successful wheel style. It still cannot prove that the
GPU drew those triangles or that the physical wheels behave well.

After changing an owned car's rim style and equipping it, compare
this readout with the selected Style indicator, the 4/4 source-wheel
readiness, the build-attempt counters and **actual screenshots**.

## Live browsed-car 3D showroom (visual only, still needs real GPU QA)

The old Garage catalogue drew the browsed car as a Canvas schematic
while the full source-generated Unreal Pawn kept showing the
authoritatively equipped vehicle. Browsing a locked car did not change
the actual body in the world. This source change now applies the
VIEWED car's original 3D body, four non-colliding wheels, saved
paint, stripes and rim shapes as a transient cosmetic preview.

The Pawn keeps independent authoritative GarageVehicleId and
CurrentGarageLook values. Preview presentation cannot change owned
vehicles, selected driving identity, physical forces, saved credits,
career results or the SaveGame appearance schema. The GameMode
updates the preview synchronously when browsing the six showroom
cars, changing menus, equipping or styling a vehicle. A locked car
remains explicitly locked and cannot be customized or driven.

Owner testing sequence:

1. Start a normal race, then open Garage and browse all six cars.
   Check the live 3D body changes in the background along with the
   text catalogue; if the fallback cube appears, report it honestly.
2. Browse a locked car without unlocking. Confirm that car can be
   VIEWED but is still not owned or equipped for the next race.
3. Browse an owned parked car, change paint, stripe and rim. Verify
   these visually change the viewed car, but the driving car's
   current performance and selected ID remain unchanged.
4. Equip a different owned car. Confirm selection/save authority is
   handled by the normal equipped-car flow, not by just browsing.
5. Leave Garage with PAGE navigation or TAB close. The selected
   car must be restored before the world is unpaused for driving.
6. Repeat with denied/failed appearance writes, quick-car hotkey,
   focus suspensions, active-race pause and recovery/restart.
7. In Garage confirm the LIVE SHOWROOM diagnostic reports VIEWED CAR
   and identifies SOURCE 3D COMPONENTS versus LEGACY CUBE FALLBACK.
   These are source observations, not shader or framerate proof.

This is an asset-light source integration rather than a finished UMG
showroom scene. A genuine Unreal 5.8 Editor/UHT compile, cooked
Win64 executable, visual screenshots and owner gameplay verification
are still required. Pure C++ policy tests cover menu enter/exit and
the full six-car, eight-paint, three-stripe, three-rim state space.

## Garage navigation and partial-mesh failure regression

- On desktop, PREVIOUS PAGE, NEXT PAGE and CLOSE must stay usable when
  EQUIP is disabled for an already equipped, locked, read-only or
  career-restricted vehicle. Try both page directions from the Garage
  using the mouse, then return and select an eligible owned vehicle.
  Disabled EQUIP must still reject the click. Test at 900x600,
  1280x720 and 1920x1080; keyboard/gamepad navigation remains separate.
- Numbered quick-view buttons must only change the visual cursor.
  Portable mutation tests reject oversized schematic counts, malformed
  or detached drawing pieces, invalid actions and ambiguous overlap.
- In an isolated developer copy, simulate a missing procedural body or
  wheel component AFTER a successful five-part source render; rebuild
  with a different vehicle/rim. Every surviving source mesh must become
  hidden and cleared before the legacy car becomes visible. Check the
  missing component at each of the five positions, including the last
  wheel. No floating source wheels, double-rendered body, collision
  activation or stale committed-rim readout is acceptable. Restore the
  intact configuration and confirm a later valid rebuild can commit.

These are source changes and manual acceptance instructions; actual
Unreal Editor/Windows rendering of injected component faults has not
been verified by the source tests.

## Deferred showroom initialization

In an isolated UE developer test, initialize a new pawn with a valid
viewed car/rim before BeginPlay, different from the equipped car.
The first source build, primitive fallback shape and material palette
must use the same viewed ID and look. Verify one initial mesh attempt,
no extra attempt on an unchanged showroom sync, and correct equipped
vehicle restoration before driving. Source policy tests cover all
six-by-six identities and three viewed rim choices. Native QA remains
required; the source check cannot reproduce Unreal spawn ordering.

## Accurate sequential unlock budget in Garage

On a new profile with 3,400 earned credits, browse Nova. Its direct
sticker price is 3,400 CR, but Q must buy Kestrel, Phantom, Tempest,
Glider and Nova in order: 9,500 CR total, 6,100 CR short. Verify the
viewed route summary and row both show the total shortage, the Q target
is still Kestrel, and browsing does not spend or unlock. A partially
owned save must skip already-owned prerequisites in the displayed total.
After an actual confirmed purchase the route changes with the authoritative
snapshot. A failed save must restore the prior route and balance. Future
profile/read-only and focus restrictions remain unchanged; budget OK is
not transaction permission. All numbers refer only to earned game credits.

## Dedicated source-framed Garage camera (draft native binding)

The current track-world Garage preview uses the exact committed original
body/rim vertices. The cached points are prepared on a successful mesh
commit and fitted only when the view/aspect changes. The ordinary
Unreal camera result is preserved unless the local player's Garage is
open, the committed mesh identity and all four wheel components match,
the viewport is valid and a visibility trace does not block the view.

- Press F4 in Garage to cycle four angles. Check actual viewport framing
  at 900x600, 1280x720, 1920x1080, ultrawide and portrait.
- Change rims and cars, including a locked car. Check actual geometry
  remains visible. F4 must not increment mesh build counters.
- Verify F4 outside Garage or during focus/manual suspension is inert.
- Close the Garage and confirm the prior driving camera preset resumes;
  C/RB retains its existing driving-camera behavior.
- In a developer copy remove a material or wheel component, alter render
  scale, or park behind track geometry. The normal camera must remain
  available rather than forcing a malformed or obstructed preview.
- The source HUD reports a successfully returned camera view, not GPU
  pixels. Confirm the paused PlayerCameraManager actually updates on F4,
  resize and browsing; if not, report the native lifecycle blocker.

This is not a separate studio map or a verified Unreal build. UHT,
Editor, cooked Win64 and real owner acceptance remain mandatory.

### Desktop unobstructed 3D lane

At 900x600, 1280x720, 1920x1080 and ultrawide resolutions, verify the left detail panel wraps without covering the car, the right controls and six schematic selectors remain reachable, and all four F4 viewpoints remain inside the center lane. Resize across the compact threshold and reopen Garage; no stale camera framing or wheel hit zone. Hover over the car and scroll: it must not page details; scrolling the left text lane must. The native camera may fall back if the track obstructs its translated eye. Confirm the live 3D label is absent on fallback. Android and compact layouts retain original framing and controls. Source tests are not proof of actual UE rendering.

### Direct orbit selection

Click each of the four buttons beneath the preview area, then press F4 and confirm the same index is cycled. The selected label follows the actual requested angle even when the obstruction fallback is active. Disabled controls during focus loss or manual pause cannot switch views. Clicking gaps, the scene itself, or controls on another menu page must not switch the camera. Verify the controls do not cover the projected car at laptop and ultrawide sizes; verify the regular race camera is unchanged after closing Garage.

### Surface shading and tangent-frame QA

Inspect all six cars and all three rim types in front, rear and side views under available native track lighting. Body contours and curved tire surfaces should show smoother lighting, while glass boundaries, hard trim, lamp housings and rim details retain their shape. Verify there are no black seams, inverted normals, shading cracks, wheel-pivot changes or new collision sections. OBJ source preview exports use the same normals as native mesh uploads; their materials are not a substitute for UE PBR or GPU tests. Compare paint changes without additional mesh rebuilds. Keep the fallback primitive car when the source shading or material contract fails.
