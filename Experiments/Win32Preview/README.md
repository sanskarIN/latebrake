# LateBrake V5 — optional controller support in the standalone Win64 preview

**Development preview of an original Win32/GDI 2D racing experiment. NOT the Unreal Engine game.** No verified UE Editor compile, cooked levels, 3D assets, online multiplayer or Windows hardware testing is claimed.

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
- Keeps V2 career saves and V3/V4 settings compatibility, six vehicle tuning profiles, four procedural tracks/events, rivals, 120Hz fixed-step simulation, checkpoint direction, race records and the personal installer.

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

The input fallback is designed for optional XInput devices; compatibility with specific gamepads, adapters, Bluetooth setups, trigger ranges, and Windows graphics/drivers **requires hands-on Windows testing**.

## Windows install and saved progress

- `LateBrakePreviewV5.exe` is the portable game.
- `LateBrakeSetupV5.exe` is the unsigned per-user installer: it embeds the same EXE bytes, installs to `%LOCALAPPDATA%\LateBrakePreviewV5`, registers an uninstaller and does not require admin elevation or network access.
- Saves stay under `%LOCALAPPDATA%\LateBrakePreview` and are intentionally preserved on uninstall. Source-level self-tests do not write to saves.
- Run `Build-Windows.ps1` in a VS x64 Native Tools PowerShell 7 shell to build from source on Windows, then `VerifyPayload.py` to compare the installer's embedded payload.
- Ordinary GitHub Actions MSVC checks test the self-test and package integrity only. They cannot verify graphics, controller hardware or human playability.

**Production goal:** An authenticated UE 5.8 Windows Editor/UHT build, original cooked circuit, inspected Win64 package and actual installed hardware tests remain required by issue #148. Xbox support in this 2D experiment is *not* an Unreal Engine Input Mapping integration; that requires separate production work.
