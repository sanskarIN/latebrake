# Win32 Preview V5 installer safety — source and Windows CI candidate

This document covers the **separate native Win32/GDI 2D experiment**, not
the Unreal Engine 5.8 game. There is no real game/installer acceptance claim.

## What this changes

The optional per-user setup now:

1. Accepts `--uninstall`, `--final-remove`, and `--self-test` only
   as an **exact argument after the executable path**. Directory names
   containing these words cannot accidentally activate a removal path.
   Unknown command arguments are rejected instead of silently installing.
2. Displays `5.0.0-preview` as the installed V5 program version instead
   of the stale V4 version.
3. Requires the destination to be a real directory and rejects a redirected
   reparse-point install directory, including junctions.
4. Writes `LateBrakePreviewV5.exe.new` via Windows `CREATE_NEW`, fully
   writes and flushes it, and stages the setup copy separately. Existing
   installed game bytes are not truncated during a failed staging attempt.
5. Replaces the setup copy before replacing the game via same-directory
   `MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH` and registers the
   uninstaller only after both replace operations succeed.

**Limits:** Multi-file replacement is not an all-or-nothing filesystem
transaction. If the second rename fails, the old game remains, but the
setup helper may already have changed. Manually close running game/setup
programs before reinstalling. A crashed installation may leave a `.new`
file that must be reviewed and manually removed before retrying.
No installer integration testing has been performed on a human Windows PC.

## CI acceptance (not a GUI installation)

- The Windows preview workflow compiles an actual native PE32+ game and
  embedded installer with the MSVC toolchain.
- The game's synthetic self-test checks its own racing/navigation logic.
- The setup's **headless `--self-test`** validates exact argument parsing
  (including misleading directory names) without editing user data.
- CI runs the setup self-test against an isolated `LOCALAPPDATA` path and
  checks it wrote no files.
- Existing `VerifyPayload.py` checks the generated executable and the
  bytes embedded into setup.
- Unverified Windows artifacts are retained for **14 days** for private
  manual GUI QA; CI artifacts are not GitHub Release game downloads.

## Manual Windows checks still required

- Run setup from a normal quoted install path and from a folder with
  `--uninstall` in its **directory name**. Neither may delete files.
- Install on a clean local profile; inspect Installed Apps version/name,
  exact installed PE bytes, and launch behavior.
- Reinstall over a closed previous build; compare the installed EXE hash to
  the CI original. Confirm old career saves remain intact.
- Keep the game EXE running while attempting upgrade. A failed replace must
  leave a runnable prior game, and errors must be clearly displayed.
- Interrupt staging, simulate unwritable directory/leftover `.new`
  files, and verify no installed executable is truncated.
- Uninstall from Installed Apps, verify removal of program files and
  uninstall registration, and verify **saved local career data survives**.
- Capture Windows version, tested user-account permissions, logs/screenshots,
  and the exact artifact SHA256 before labeling the installer verified.

For the **actual Unreal Engine game**, separately satisfy
[playable release gate #148](https://github.com/sanskarIN/latebrake/issues/148).
This change is a *source candidate*, not a playable release.
