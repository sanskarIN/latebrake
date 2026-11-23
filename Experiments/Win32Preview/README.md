# LateBrake V5 — optional controller support in the standalone Win64 preview

**Development preview of an original Win32/GDI 2D racing experiment. NOT the Unreal Engine game.** No verified UE Editor compile, cooked levels, 3D assets, online multiplayer or Windows hardware testing is claimed.

## Latest trial build and Win64 canvas-input correctness

A **2D Windows trial artifact** can be built without Unreal by opening a
pull request that changes this experiment's source. The existing
`experimental-win32-preview.yml` GitHub workflow compiles the native
Windows x64 `LateBrakePreviewV5.exe` and `LateBrakeSetupV5.exe`,
runs isolated headless self-tests, validates actual PE32+ x64 binaries
and the installer's embedded game payload, and uploads the ZIP as an
**UNVERIFIED 2D Win64 trial**, not a release.

The trial ZIP now includes `TRIAL_QUICKSTART.md` plus a generated
`TRIAL_BUILD_SHA256.txt` containing both executable SHA-256 hashes,
the source PR head and CI checkout SHA. These are useful for identifying
the exact binary being tested; the runner's synthetic test **does not**
certify a real user-facing desktop game.

The original GDI viewport now delegates all letterbox/pillarbox mouse
mapping to `PreviewViewportMath.h`. Intermediate client coordinate
calculations use 64-bit integers, so extremely large resized windows
cannot overflow signed 32-bit hit-test products and activate unrelated
menu controls. The exact real Win32 `WM_MOUSEMOVE` and
`WM_LBUTTONDOWN` path uses this shared policy, not an isolated copy.
A portable strict C++20 `PreviewViewportMath` test covers normal
Windows resolutions, invalid rectangles, black bars, large-client
limits, 100,000 deterministic cases and 32-bit overflow boundaries.
The Windows GUI preview's `--self-test` also checks the maximum viewport
edge. See [trial quick start](TRIAL_QUICKSTART.md).

**These changes do not become UE5.8 features simply because the
source branches share a candidate PR.** Genuine Unreal gameplay
requires the separately licensed-engine Windows Editor/UHT and native
packaging QA described in the production build documentation.

## Twelve-car garage and per-vehicle appearance (draft 2D trial)

The standalone preview now offers **12 selectable fictional cars** in
two accessible pages, retaining the original six save IDs. Additional
cars require earned credits; existing progression still determines actual
race permissions, and different car profiles affect the 2D simulation.
The enlarged garage shows live per-car power/speed/grip and a visually
distinct original silhouette. Each model can independently choose
8 paint colors, clean/center/twin vinyl, and street/sport/track rims,
visible both in the showroom and on the actual 2D track. Choose with
the mouse or V/B/N keys. These are **cosmetic only** and never modify
physical handling or award credits.

A separate versioned `garage-appearance-v1.dat` is checksum validated,
bounded and written with a flushed atomic replacement; an unsuccessful
cosmetic save reverts the in-memory change rather than falsely showing
it as saved. No existing career.dat or settings.dat format was modified;
the existing 32-bit V2 ownership mask supports the appended twelve IDs,
but an older build that only understood six cars may not understand
new car IDs when downgrading. Existing Windows CI exercises a native
standalone game self-test and strict portable C++20 + source contracts.
No tested UE Engine 5.8 3D car models or mesh customization are implied.

## V5 development changes

- **Mode-correct rival sessions:** Time Attack (one lap) and Checkpoint Rush (two laps) now run as actual solo clock events; no AI cars are initialized, advanced, collision-tested or drawn in these modes. Circuit Race (three laps) and Rival Duel (two laps) retain three local scripted AI drivers. A per-attempt rival-count snapshot prevents menu selections from silently changing active physics, and the minimap explicitly labels solo/rival events. Rival Duel detects any rival that completes the required laps, rather than assuming a fixed AI slot is always fastest. Strict portable mode-policy tests and Windows synthetic self-tests cover this source change. This does not qualify real Windows hardware, AI quality or the Unreal game. [Mode-isolation QA](../../docs/PREVIEW_SOLO_EVENT_ISOLATION_QA.md).


- **Transactional career progression (source candidate):** race win credits/stars and garage-car unlock purchases now require a successful, flushed, atomic save before becoming accepted in memory. Failed writes restore previous money, wins, stars and ownership, with a clear **SAVE FAILED** Results message; caps and actual credited amounts match the V2 save format. Synthetic Windows self-tests inject success/failure without touching local player data. Real Windows disk-error QA is still required. [Save transaction QA](../../docs/WIN32_PREVIEW_SAVE_TRANSACTION_QA.md).

- **Directional checkpoint integrity:** the preview now requires the human car's actual forward movement segment to cross each ordered checkpoint plane from its upstream side, with both endpoints on the paved road. Merely sitting within a checkpoint radius, arriving on its far side, facing the right direction while stationary, or recovering to a checkpoint cannot grant a lap. A Windows headless self-test exercises all four gates over three laps, wrong-way attempts and non-crossings. This remains a 2D synthetic test, not a real driving/hardware qualification. [Gate crossing QA](../../docs/WIN32_PREVIEW_GATE_CROSSING_QA.md).

- **Unreleased pit service source feature:** a marked trackside bay on all four 2D circuits now lets a stationary car hold **E** (keyboard) or **Y** (controller) for 2.5 seconds to repair up to 40 damage and replenish up to 50 nitro. Race time and rivals keep advancing. Leave the bay to rearm; service never credits a checkpoint or modifies the career save schema. [Policy, synthetic tests and Windows QA](../../docs/PREVIEW_PIT_SERVICE_QA.md).

- Optional Windows **XInput controller** support without hard-requiring the DLL: tries `xinput1_4.dll`, `xinput1_3.dll`, then `xinput9_1_0.dll` **from Windows System32 only**, searches controller slots 0–3, and keeps keyboard/mouse available when none loads.
- Left analog stick steering with ±18% deadzone/remapped full travel; RT proportional acceleration and LT proportional braking; LB reverses; A triggers nitro; X performs penalized recovery.
- D-pad or left stick menu navigation; A selects, B returns, Start pauses/resumes. Analog menu navigation has a latch: no repeated selection changes from a held stick.
- Focus loss pauses racing, ignores background keyboard/controller input, and requires a *neutral controller* after focus regain or reconnect before allowing inputs. Holding A/Start/RT across a disconnect cannot unintentionally act when the controller returns.
- Controller status and control mappings appear in HUD/footer when connected.
- Resizable Windows game viewport now preserves the original 3:2 canvas with pillarboxing/letterboxing. Click and hover coordinates use the visible canvas only; black bars do not activate UI actions.
- Added synthetic input-level tests for axis extremes/deadzones, trigger scaling, button edges/held buttons, unplug/reconnect and focus re-entry. These do not constitute real controller hardware QA.
- Keeps V2 career saves and V3/V4 settings compatibility, twelve 2D vehicle profiles, four procedural tracks/events, rivals, 120Hz fixed-step simulation, checkpoint direction, race records and the personal installer.

## Controls

| Action | Keyboard | Xbox-compatible controller |
|---|---|---|
| Accelerate | W / Up | RT (analog) |
| Brake | Space | LT (analog) |
| Reverse | S / Down | LB |
| Steer | A / D, Left / Right | Left stick (analog) |
| Nitro | Shift | A (during race) |
| Recover (+5 sec) | K | X |
| Pit service (stopped in PIT bay, 2.5s hold) | E | Y |
| Pause/resume | P | Start |
| Menu selection | Arrows / Enter / mouse | D-pad/stick / A |
| Back | Escape / M | B |
| Driver guide | F1 | F1 |
| Steering assist | J | J |
| High contrast | H | H |
| Garage paint (next) | V | Mouse |
| Garage vinyl (next) | B | Mouse |
| Garage wheels (next) | N | Mouse |

The input fallback is designed for optional XInput devices; compatibility with specific gamepads, adapters, Bluetooth setups, trigger ranges, and Windows graphics/drivers **requires hands-on Windows testing**.

## Windows install and saved progress

- `LateBrakePreviewV5.exe` is the portable game.
- `LateBrakeSetupV5.exe` is the unsigned per-user installer: it embeds the same EXE bytes, installs to `%LOCALAPPDATA%\LateBrakePreviewV5`, registers an uninstaller and does not require admin elevation or network access.
- Saves stay under `%LOCALAPPDATA%\LateBrakePreview` and are intentionally preserved on uninstall. Source-level self-tests do not write to saves.
- Run `Build-Windows.ps1` in a VS x64 Native Tools PowerShell 7 shell to build from source on Windows, then `VerifyPayload.py` to compare the installer's embedded payload.
- Ordinary GitHub Actions MSVC checks test the self-test and package integrity only. They cannot verify graphics, controller hardware or human playability.

**Production goal:** An authenticated UE 5.8 Windows Editor/UHT build, original cooked circuit, inspected Win64 package and actual installed hardware tests remain required by issue #148. Xbox support in this 2D experiment is *not* an Unreal Engine Input Mapping integration; that requires separate production work.

## Diagnosing an empty or black preview window (source candidate)

The separate V5 **2D** program now reports failures to register or create
its desktop window in a visible Windows dialog (historical silent process
exit codes `5` and `6`). If a non-minimized window then fails to present
its GDI framebuffer on **45 consecutive attempts**, it shows a single
failure explanation and exits with code `26` instead of silently
continuing a blank surface forever. Minimized and zero-sized viewports
are deliberately ignored, and one successful GDI draw resets the counter.
The tested source policy does **not** detect a successful but incorrect
paint, inaccessible window behind another app, remote-desktop compositor
failure or missing Unreal gameplay content.

For **manual native Windows QA only**, run the unverified preview EXE in an
active Windows 11 desktop session, not merely in CI. Confirm the welcome
menu, visible original 2D track, resizable letterboxing, controls and
first lap. Minimize then restore; the app must not show a false graphics
failure. If the UI is blank, record whether there is a dialog, the process
exit status, Windows build, GPU/driver model, remote-session status,
window size, display scaling and the exact EXE SHA-256. Check Windows
Event Viewer or debugger output for `LateBrake V5` diagnostics.

GitHub's MSVC `--self-test` validates the 45-failure state machine without
opening any UI or reading/writing a player's profile. The Win64 build
job can confirm compilation and isolated synthetic logic, **not** desktop
visibility or an installed Unreal Engine 5.8 3D game. Do not publish a
playable-release claim from these results.
