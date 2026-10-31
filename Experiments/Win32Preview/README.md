# LateBrake V3 — personal Windows 2D racing preview

**Experimental, original Win32/GDI program. Not the Unreal Engine LateBrake game.**

This optional self-contained preview exists to test navigation, garage, racing
progression and HUD usability without waiting for a licensed UE 5.8 workstation.
It is **not** an Unreal build, representative physics simulator, online game,
verified playable release, or replacement for the production project's
Editor/UHT and installed-platform qualification.

## New in V3 (compared with V2)

- Rebuilt all menu surfaces with consistent hierarchy, selected-state borders,
  high-contrast cards, readable text, and contextual control hints.
- Full keyboard selection: Up/Down or Left/Right changes focus, Enter activates.
- Clickable Home actions, garage car list, track cards, event cards, resume,
  results actions, and optional garage purchase. Mouse pointer hover highlights.
- Integrated F1 controls overlay and `H` high-contrast palette toggle.
- New race HUD: lap, checkpoint, elapsed time, simulated speed, nitro, damage,
  best lap, track identity and four-step gate progress.
- Visible pre-race 3–2–1 countdown (racing time/physics do not advance before GO).
- Paused on window focus loss; pause and results now include clear actions.
- Store selected car, circuit, event and contrast preference in a versioned,
  check-summed local `settings.dat`. The **V2 career.dat format is unchanged**;
  winnings, credits and ownership from V2 carry into V3 when stored locally.
- No network code: future friend invites and online rooms are **not present**.

## Features that already existed in V2 and remain

- Six distinct stat configurations with credit-based unlocks and local career.
- Four mathematical 2D circuits / environments and four event types.
- Three scripted traffic rivals, nitro, basic impact damage, four ordered gates,
  lap timing and clean-run bonuses.
- Optional current-user Windows setup/uninstall registration; portable EXE.

## Run and use

1. Download the packaged V3 ZIP or compile from source.
2. Extract it completely, then run `LateBrakePreviewV3.exe` directly. Alternatively,
   run `LateBrakeSetupV3.exe` to install under your own `%LOCALAPPDATA%`.
3. On Home: arrows navigate and Enter activates the highlighted action. The
   menu buttons also work with mouse clicks. Select Garage, Tracks or Events.
4. In a race: `W/Up` accelerate; `S/Down` reverse; `A/D` or Left/Right steer;
   `Space` brake; `Shift` nitro; `P` pause; `R` restart; `M` home.
5. `F1` opens contextual help; `H` enables a high-contrast UI; `Esc` goes back.

For persistent data, the game uses `%LOCALAPPDATA%\LateBrakePreview\career.dat`
(the V2 format) and `settings.dat` for V3 UI preferences. The setup executable
installs itself and the preview under `%LOCALAPPDATA%\LateBrakePreviewV3` and
registers an entry in Windows Installed Apps. Uninstall preserves career data.
The setup has no network access or third-party downloads.

## Build on Windows

From a Visual Studio 2022 x64 Native Tools terminal with Python 3:

```powershell
pwsh ./Experiments/Win32Preview/Build-Windows.ps1
& ./Experiments/Win32Preview/out/LateBrakePreviewV3.exe --self-test
```

This command uses MSVC C++20 and the Windows SDK to build the freestanding
Win32/GDI executable. It then embeds those compiled EXE bytes into the
separate setup program. There is no Unreal SDK, DirectX game engine or
third-party runtime in this experimental build. Ordinary Windows CI can
compile and run the deterministic `--self-test` mode without claiming that
a person drove the game. `--self-test` is isolated from local saved progress.

**Observed testing so far:** portable source policy tests and native PE builds
passed in a non-Windows compiler environment. Frames were rendered through a
Linux test harness using the exact C++ canvas renderer. A true Windows MSVC
CI compile/logic check and real physical Windows GUI + install/uninstall QA
must be performed separately. Never call these source tests verified driving.

## Boundaries / future work

V3 is still **flat 2D** with elementary driving math, stylized colored car
shapes and no audio, tuning upgrades, damage repair, real traffic/network,
asset pipeline, complete gamepad support or controller remapping. No friends
invitation, matchmaking or real multiplayer exists. The production UE 5.8
repository must still satisfy issue #148 before calling it an installed
playable game; see issues #233 and #234 for multiplayer and Windows preview QA.
