# Windows 2D preview — career save transaction integrity

**Scope:** V5 native Win32/GDI standalone 2D experiment, **not** the
official Unreal Engine 5.8 LateBrake game. Source and synthetic tests
do not establish a playable or installed Windows release.

## Fixed failure

Previously the preview modified in-memory career credits, event wins,
stars and purchased-car ownership even if `career.dat` could not be
written or its temporary file could not replace the previous profile.
The Results screen then claimed that progress had been saved.

## New behavior

- All race awards are staged against a snapshot of existing money, win
  count and stars. A valid full write, flushed file, closed handle and
  successful atomic replacement of `career.dat` are required before
  the UI acknowledges an awarded win.
- A failed save restores **all three** in-memory values and shows
  **SAVE FAILED** and zero applied reward. The physical race remains
  complete and may be replayed, but is not claimed as a saved career
  victory. An attempted finished-race replay cannot award twice.
- Garage purchases use the same commit-or-rollback approach. On failure
  both the paid credits and ownership mask are restored, with explicit
  failure feedback instead of a false unlock confirmation.
- Credit rewards stop at 500,000, and career stars at 100,000, matching
  the existing V2 `career.dat` validation. The Results page reports
  the *actual* credited amount (possibly less at the cap), not an
  impossible full nominal reward.
- File format, user save location and older V2 progress compatibility
  are unchanged. The old file is preserved on failed write/flush/rename
  where the underlying Windows filesystem honors the atomic replacement.

## Automated checks

The existing native Windows experimental workflow runs `--self-test`
with a synthetic success/failure toggle that bypasses **all** profile
file I/O. It asserts that failed race saves do not change credits, wins,
stars, reward display or acknowledgement; failed purchases do not
deduct credits or unlock a car; successful operations still work;
repeated finish/unlock calls do not double-award; and high-bound
records remain within the save format.

Cross-platform Python source checks enforce that the actual native
preview source calls the commit/rollback branches and that CI isolates
the self-test from user career data.

## Required real Windows QA

On a private disposable Windows user profile, play a valid race and
purchase a car, verify `career.dat` survives relaunch, and compare
pre/post state. Repeat with intentionally unwritable or full storage,
confirm that credits/ownership return to their old values, that the UI
shows the failure and that the previous career file is still readable.
Test replacement of an existing file, antivirus/file locks, power-loss
recovery limitations, and simultaneous launches separately.

Do not change permissions on someone else's live career data.
A build-only Windows job cannot verify the filesystem failure mode or
the installer. Keep experimental 2D qualification in
[issue #234](https://github.com/sanskarIN/latebrake/issues/234) and
the official Unreal playable milestone in
[issue #148](https://github.com/sanskarIN/latebrake/issues/148).
