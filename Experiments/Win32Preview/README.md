# LateBrake V4 — Windows 2D racing preview

**Experimental native Win32/GDI prototype. Not the Unreal Engine LateBrake game.** This is not a qualifying 3D racing release and not a verified Windows install. No network, accounts, or real multiplayer exist in this preview.

## Improvements over V3

- Fixed 120 Hz simulation steps with bounded catch-up after frame delays. Simulated controls no longer vary directly with display refresh rate.
- Smooth analog-style steering filter; high-speed directional assistance (toggle with **J**) for fewer abrupt turns. Preference is saved to the versioned `settings.dat` file and legacy V3 settings are read compatibly.
- **K** to recover a stranded car near its previous validated checkpoint, costing five seconds, damage, and nitro. Does not credit gates. Restart (**R**) resets the event instead.
- Six cars still differ by acceleration, top speed and grip. Locked cars are viewable but cannot drive until bought with local credits; even Home quick-start enforces ownership.
- Persistent best lap times per track, held separately from the existing V2/V3 career file, with a format marker, checksum, bounds checks and atomic-style replace.
- Live minimap with player position, local scripted rivals and next directional checkpoint; improved race HUD and help overlay.
- Improved finish breakdown, assisted/standard driving mode display, race goals, settings continuity and strict local-only progression.
- Synthetic tests of two-vs-one fixed simulation frames, invalid garage ownership, penalty recovery, wrong-way checkpoint rejection, lap file integrity, saved preference integrity and race progression.

### Controls

| Key | Function |
|---|---|
| W / Up | Accelerate |
| S / Down | Reverse |
| A / D or Left / Right | Steer |
| Space | Brake |
| Shift | Nitro |
| J | Steering assistance on/off (saved) |
| K | Recover with +5-second penalty |
| P | Pause/resume |
| R | Restart current event |
| F1 | Controls/help |
| H | High-contrast mode |
| Enter / arrows / mouse | Navigate menus and select |
| M / Escape | Home or back |

## Win64 installation and privacy

- Portable: run `LateBrakePreviewV4.exe`. No installer required.
- Setup: run `LateBrakeSetupV4.exe`. It asks permission to install for **your Windows account** under `%LOCALAPPDATA%\LateBrakePreviewV4`, creates an uninstall registration, and can launch the game. No admin elevation or downloads are required. Setup is unsigned; review the source and checksums first.
- Save data under `%LOCALAPPDATA%\LateBrakePreview\` uses `career.dat` (preserves older V2/V3 progression), `settings.dat` (V4 adds the steering-assist preference while reading valid V3 settings), and `lap-records-v4.dat` (new track PB file). Uninstall preserves career and records.
- No network, online multiplayer, advertising, tracking or background services. Your local files are not uploaded.

## Verification scope

`python VerifyPayload.py LateBrakePreviewV4.exe LateBrakeSetupV4.exe payload.inc` checks that both built binaries are real PE32+ AMD64 and that the *installer source* embeds exactly the game EXE. The game `--self-test` checks deterministic game-policy transitions with no save writes. Linux-native render tests use stubs to render screenshots from the **actual preview source**; they are NOT proof that an installed Win64 EXE draws or plays correctly.

Actual Windows gameplay, install/uninstall, performance, virus scanning and signing have **not** been independently qualified. The production UE 5.8 race project separately needs UHT/Editor compilation, original cooked level and installed Win64/Android QA ([#148](https://github.com/sanskarIN/latebrake/issues/148)). Multiplayer is future planned, tracked at [#233](https://github.com/sanskarIN/latebrake/issues/233).
