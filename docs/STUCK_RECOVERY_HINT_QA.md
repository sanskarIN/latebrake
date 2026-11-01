# Low-motion recovery hint — source QA

## Scope

The production Unreal 5.8 Canvas HUD now has a **read-only** stuck-car
advisory. A local human must hold forward drive while a qualified physical
car moves slower than 0.8 m/s and stays within 1.5 metres of its initial
observation for a continuous four seconds. The HUD then displays the
existing **R / RECOVER** action (or the Android RECOVER touch pad), and
clearly states the existing successful-recovery time penalty of five seconds.

The hint does not automatically teleport, bypass ordered gates, award
credits, or edit persistence. A human pressing the existing recovery action
still goes through all existing placement/penalty verification.

## Source qualifications

- C++20: `ctest -R StuckRecoveryHint --output-on-failure` after configuring
  `Tests/Standalone` validates time threshold, motion resets, invalid
  numerical data, paused/unready frames, and successful retrigger.
- Python source-binding suite
  `python -m unittest discover -s Tests/Editor -p '*Tests.py'`
  verifies forward-throttle intent and the existing race/Pawn/teleport guards.
- Readiness is gated on the real local human, live green start, valid circuit,
  restart placement and physical rigid body. Pause, finished race, input
  release, frame stall or invalid speed clears the advisory.
- Recovery itself remains explicitly manual and may be refused when physical
  placement is unsafe. This source policy is **not** stuck detection validated
  on a device.

## Required installed gameplay checks

1. On a licensed UE 5.8 Win64 packaged game, hold forward throttle against
   a collision barrier for four continuous seconds; confirm the hint appears.
2. Release throttle, apply brake/handbrake, reverse, or drive a clear two
   metres; ensure the hint disappears and does not retain stale state.
3. Interrupt with menu/pause, OS focus loss, race restart, player pawn
   replacement and manual recovery; confirm another four qualified seconds
   are needed before any new hint.
4. Exercise Android touch GAS and RECOVER simultaneously on hardware. Verify
   the smaller HUD keeps lap, gate, race clock and other essential controls
   legible, and no stuck input survives OS backgrounding.
5. Verify recovery only charges the established five-second penalty on a
   successful legitimate relocation, never as a side effect of showing the
   hint. Record actual device, UE build, asset hash and logs per issue #148.

The open Windows preview experiment under `Experiments/Win32Preview` is
separate from this Unreal HUD feature. Source CI success does not prove
a cooked circuit, installed UI layout, performance, or a playable UE game.
