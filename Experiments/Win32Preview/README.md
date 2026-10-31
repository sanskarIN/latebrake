# LateBrake V5 — optional controller support in the standalone Win64 preview

**Development preview of an original Win32/GDI 2D racing experiment. NOT the Unreal Engine game.** No verified UE Editor compile, cooked levels, 3D assets, online multiplayer or Windows hardware testing is claimed.

## V5 development changes

- Optional Windows **XInput controller** support without hard-requiring the DLL: tries `xinput1_4.dll`, `xinput1_3.dll`, then `xinput9_1_0.dll`; keyboard and mouse remain available when none loads.
- Left analog stick steering with ±18% deadzone/remapped full travel; RT proportional acceleration and LT proportional braking; LB reverses; A triggers nitro; X performs penalized recovery.
- D-pad or left stick menu navigation; A selects, B returns, Start pauses/resumes. Analog menu navigation has a latch: no repeated selection changes from a held stick.
- Focus loss pauses racing, ignores background keyboard/controller input, and requires a *neutral controller* after focus regain or reconnect before allowing inputs. Holding A/Start/RT across a disconnect cannot unintentionally act when the controller returns.
- Controller status and control mappings appear in HUD/footer when connected.
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
