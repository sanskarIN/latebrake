# LateBrake V5 — Windows 2D trial quick start

**This is an experimental Windows x64 2D game and installer, NOT a trial
of the Unreal Engine 5.8 3D racing game.** It has an original top-down
racing view, not the planned 3D cars/tracks or production AI. GitHub
Actions compiles and self-tests the binaries but cannot prove that they
display or drive correctly on your PC.

## What is inside this trial archive

- `LateBrakePreviewV5.exe` — standalone portable native Windows 10/11
  64-bit racing experiment. You can run it without installing.
- `LateBrakeSetupV5.exe` — optional unsigned per-user installer. It
  installs only the standalone 2D prototype, not the UE project.
- `TRIAL_BUILD_SHA256.txt` — exact source identity and both binary
  hashes generated during the Windows build, not a release signature.
- This guide.

## First test on Windows 10/11

1. Open the GitHub Actions workflow for the associated PR and download
   its `UNVERIFIED-LateBrake-V5-2D-Win64-...` artifact ZIP.
2. Extract all files to a normal local folder, such as Downloads. Read
   the receipt and verify SHA-256 if you want to independently confirm
   which EXE was built (PowerShell:
   `Get-FileHash .\LateBrakePreviewV5.exe -Algorithm SHA256`).
3. Run `LateBrakePreviewV5.exe` directly. You do **not** need to run
   the optional installer. This is an unsigned development program;
   do not bypass a security warning if you cannot verify/trust it.
4. Test the first menu, choose a car and track, then start Circuit
   Race, Time Attack, Rival Duel and Checkpoint Rush. Solo modes should
   not show scripted opponents.
5. Drive with W/Up to accelerate, A/D/Left/Right to steer,
   Space to brake, S/Down to reverse, Shift for nitro and K to recover.
   P pauses; F1 opens the guide. On supported Xbox-compatible
   controllers the right trigger accelerates and left stick steers.
6. Resize the window from wide landscape to square/tall. Clicking
   black bars around the gameplay canvas must never activate menus.
   Test 100%, 125% and 150% Windows DPI scaling if available.
7. Try optional `LateBrakeSetupV5.exe` only after the portable run.
   A real installation, uninstall and saved-career persistence test
   must be performed on your own Windows account.

## Share actionable bug evidence

Record Windows version, GPU, display scaling, screen resolution, input
device, selected mode, exact reproduction steps, screenshots, and the
SHA-256 from `TRIAL_BUILD_SHA256.txt`. Note if the process exits or if
you get a black window. Do not upload your career save or other private
files without checking their contents.

## Limits

The archive does not contain a cooked Unreal map, UE5.8 executable,
real 3D vehicle assets, native Chaos physics, multiplayer, a verified
Windows install, or a signed commercial release. The full Unreal
candidate requires a licensed UE5.8 Windows self-hosted runner and
separate Editor/UHT, cook, packaged Win64 and hands-on game QA. This
trial cannot certify those features.

Source: https://github.com/sanskarIN/latebrake
