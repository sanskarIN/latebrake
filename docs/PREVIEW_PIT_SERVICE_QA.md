# Experimental Win32 V5 pit-service QA

**Scope:** The standalone 2D Win32/GDI preview only. This source patch does not
implement an Unreal Engine 5.8 pit lane, multiplayer, a cooked circuit,
hardware-qualified controller support, or a verified Windows installer.

## Gameplay contract

- A labelled **PIT** marker is projected from the same procedural circuit
  geometry as the four checkpoint gates on each selected track.
- Stop inside the marked circle, release gas/reverse/nitro, and hold **E**
  (keyboard) or **Y** (Xbox-compatible XInput controller) for **2.5 seconds**
  of active, unpaused 120 Hz simulation time.
- A completed service reduces accumulated arcade collision damage by up to
  **40 points** and adds up to **50 nitro points**, both clamped to existing
  0–100 bounds. No credits or bonus awards are granted by servicing.
- Normal race time continues to advance. Rivals continue moving, and their
  existing finish/loss conditions still apply. This is strategic lost time,
  not a free race pause.
- The car is stationary during active service; the repair cannot silently
  credit a checkpoint. A second service requires driving out of the bay
  before returning. Entering a menu, restarting, or finishing resets or
  disables the transient service state.
- Attempts outside the bay, at speed >=16 simulated units, under throttle,
  reverse or nitro, with a full-health/full-nitro car, or without a held
  service control cannot progress the timer.
- Saved career data, record file formats, race awards, and unlock prices
  are unchanged. The Results page reports the number of completed stops
  for that race, not a persistent racing statistic.

## Reproducible synthetic check (no real racing qualification)

On a Windows x64 machine with MSVC/SDK and Python, run:

```powershell
./Experiments/Win32Preview/Build-Windows.ps1
$exe = (Resolve-Path './Experiments/Win32Preview/out/LateBrakePreviewV5.exe').Path
$p = Start-Process -FilePath $exe -ArgumentList '--self-test' -Wait -PassThru
if ($p.ExitCode -ne 0) { throw "Synthetic test failed: $($p.ExitCode)" }
python ./Experiments/Win32Preview/VerifyPayload.py `
  ./Experiments/Win32Preview/out/LateBrakePreviewV5.exe `
  ./Experiments/Win32Preview/out/LateBrakeSetupV5.exe `
  ./Experiments/Win32Preview/out/payload.inc
```

The existing `experimental-win32-preview.yml` workflow runs the same
game-level self-test and checks that the embedded game bytes match the portable
EXE. The self-test now covers wrong location/drive inputs, high-speed rejection,
timed repair, one-stop latch, drive-out rearm, no checkpoint credit, and rivals
continuing their simulation.

## Required manual Windows QA

1. Drive each of the four tracks and confirm the PIT marker is visible,
   reachable and not mistaken for a directional checkpoint.
2. Try E/Y near the bay at high speed, with full throttle and while paused;
   none should service the car.
3. Take collision damage or use nitro; park in the bay, hold E or Y for
   2.5 seconds, then confirm damage/nitro gauges change by bounded amounts.
4. Confirm that leaving the bay permits a second visit, but holding the key
   continuously inside the bay does not award repeated instant repairs.
5. Observe rival movement, race time, finish/fail rules, and your next gate
   during the stop. Resume driving and verify lap count and save integrity.
6. Test window focus loss, controller disconnect/reconnect, keyboard fallback,
   resized windows and the unsigned per-user installer/uninstaller.
7. Record the exact source commit, Windows build details, SHA256 EXE hashes
   and findings. Do not label an untested artifact as playable-qualified.

**Exit gate:** Source self-tests and manual QA are separate; neither
qualifies the UE game. Keep production playable gate #148 open until real
Unreal Editor/cook/package and physical-device observations exist.
