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

## Twelve original cars and personal paint styling

Open **YOUR GARAGE** from the home screen. There are now **12 fictional
cars** arranged in two pages of six. Use the previous/next page buttons,
arrow keys, or click a car row. The first six cars preserve their old IDs;
the extra six have separate unlock prices and driving characteristics.
The garage shows actual acceleration, maximum speed and grip differences,
with a live top-down silhouette for the selected car. Your chosen car must
be unlocked with career credits before it can enter a race.

Customize each car independently in the garage. Click one of **8 paint
swatches**, choose **clean / center / twin** racing stripes, and choose
**street / sport / track** wheels. Alternatively press **V** to cycle paint,
**B** to cycle stripes, or **N** to cycle wheels. These are **free visual
changes only**, not performance upgrades or real 3D vehicle assets.
Changes appear on the showroom car and in the actual 2D race. Appearance
is saved to `%LOCALAPPDATA%\\LateBrakePreview\\garage-appearance-v1.dat`
without changing the existing career or UI settings files. Failed saves
roll back your cosmetic change and show a warning; a corrupt style file
is ignored safely. If this older trial sees a **newer saved cosmetic format**
(version greater than 1 with the recognized file identity), it keeps that
file read-only rather than replacing it with factory defaults. The Garage
shows **NEWER LOOK SAVE: READ ONLY / FILE PRESERVED**. Use the newer
compatible build to edit those looks; do not run both builds at once.
The automated synthetic test does not prove simultaneous-process file
locking, real Windows disk recovery or migration from a newer version. Older V5 builds may not recognize newly unlocked
car IDs above 5, so do not downgrade after unlocking cars 7–12.

## Focus and input safety

Switching away from the preview automatically pauses a running race. When
you return, **release held driving and menu keys for two input frames**
before pressing P (or using the paused menu) to resume. Holding throttle,
steering, Shift, Enter or another control while Alt-Tabbing back will not
drive or select a menu option until released. Clicking a button in an
inactive window only activates the game window on the first click; use
another deliberate click to activate that button. Controller reconnects
have a separate neutral-input guard. The preview never races in the
background.

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

## Road guide and steering feedback (experimental)

The optional **J** control now has an actual, bounded road-following
function in addition to its subtle steering-speed safety scaling.
**J ON** renders a short sequence of cyan centerline dots ahead of the
car, computed from the *selected circuit's real procedural road* (not a
shortcut pointing directly to a checkpoint). At road speed, a gentle
steering hint helps when you are not already steering strongly. **J OFF**
restores manual-only steering and hides the cyan guide dots. You retain
full steering priority, especially during deliberate hard turns;
there is no automatic driving. The next checkpoint has a distinct
pulsing marker.

Watch for **WRONG WAY / TURN AROUND** when moving forward against the
course direction and **SLOW DOWN FOR CORNER** when a preview-only local
curve estimate flags excessive approach speed. Off-road low grip takes
priority over those warnings. The speed advice is approximate, **not
a physical braking-distance guarantee**.

For useful playtest feedback, compare two laps with J ON and OFF on
all four courses. Report whether steering feels delayed, overcorrects,
understeers, fights player input or creates unjustified warnings.
Try reversing, stopping, driving off track, a fast corner, pause, Alt-Tab
and every controller steering position. Check that gate/lap counts,
rewards, opponents and your saved career never change merely because
the road guide is enabled. The native `--self-test` and portable
C++20 policy tests cover deterministic synthetic conditions, not real
controller latency, enjoyment, Windows display or 3D Unreal gameplay.
