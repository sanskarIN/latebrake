# Transient handbrake — source-only prototype QA

The LateBrake force-based rigid-body car has a held **handbrake** input for
experimental controlled slides. It is separate from ordinary BRAKE.
Keyboard **Left Shift** and gamepad **right-stick click** hold the action;
releasing them returns to ordinary handling.

## Implemented source behavior

- Genuine local human driver only; the rival AI cannot activate it.
- While held, requested motor throttle is zero, lateral damping is reduced
  to preserve a controlled slide, and bounded forward resistance slows
  the prototype chassis. Sim-inspired and arcade modes have distinct
  lateral resistance. Simultaneously holding ordinary BRAKE preserves
  its stronger stopping demand.
- Long-frame resistance is still capped by FrameStableDampingRate, and
  nonfinite tuning or input cannot create nonfinite requested forces.
- Service BRAKE (Space/gamepad bottom face button) and Android touch
  BRAKE are unchanged; no additional mobile touch pad is introduced.
- One desktop speed HUD line optionally displays the transient handbrake
  state. No extra Android line obscures the existing touch controls.
- Pause, menu, focus loss, controller loss, restart and recovery clear
  the held state. No changes to lap awards, credits or saved records.

This is intentionally not a rear-wheel brake, tire lock, ABS, traction
control or suspension simulation; it uses the prototype's existing
force-based single rigid body.

## Portable tests

    cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
    cmake --build Tests/Standalone/build --config Release
    ctest --test-dir Tests/Standalone/build -C Release -R "^(HandbrakeDrivePolicy|FrameStableDamping)$" --output-on-failure
    python3 -m unittest discover -s Tests/Editor -p "HandbrakeBindingTests.py" -v

## Actual engine / device acceptance (not yet performed)

1. Compile Unreal Engine 5.8 Editor/Game through UHT and cook the
   owned circuit. Check that both new input keys exist on target devices.
2. Drive with Left Shift and right-stick-click at different speeds and
   handling modes. Check controller release and ordinary brake overlap.
3. Interrupt active handbrake with pause, menu, focus loss, recovery,
   unpossession, restart and invalid physics. Verify no lingering force.
4. Observe frame-time behavior and understeer/spin near track barriers.
   Tune from measured installed gameplay, not synthetic policy tests.
5. Verify desktop HUD clipping and unchanged mobile touch layout.

This branch does **not** produce or verify a Windows executable or Android
APK. Playable qualification issue #148 and Android device issue #13 must
stay open until real installed game evidence is available.
