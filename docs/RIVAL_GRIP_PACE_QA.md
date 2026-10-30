# Rival grip-aware corner pace — source-only QA

**Source-only prototype. Not a verified Unreal Engine 5.8 game build.**
The existing AI follows the same directional circuit waypoints; this
change does not create an overtaking system, a separate racing line, actual
per-wheel tire dynamics, authoritative opponent standings or a pit strategy.

## Gameplay behavior

- The autonomous rival already receives the exact same race-local
  chassis-derived arcade tire heat and wear as the player. The new
  `RivalGripPace` uses its **previous valid physics-tick grip**, current
  real chassis planar/lateral velocities, steering demand and delta time.
- After existing route steering/corner anticipation and **before**
  existing player-aware traffic braking, a grip-risk governor can
  request ordinary braking or decrease throttle. The policy may never
  increase drive, alter steering, bypass a waypoint or weaken a brake.
- A turn requires **speed at least 11 m/s**, steering magnitude at least
  **0.22**, and either **3%+ loss of available arcade grip** or measured
  lateral slip of at least **12%**. Stronger slip, tighter steering and
  worn tires lower a conservatively bounded target pace. If actually
  overspeed by more than 1.5 m/s, ordinary braking takes priority.
- A **0.12-second maximum brake hold** dampens brake/release chatter
  while a cornering hazard subsides. It resets on race restart, pilot
  reconfiguration, route completion, recovery and blocked/paused physics.
- Malformed or stale velocities, grip, control commands or frame deltas
  fail closed (no motor demand, safe braking) and clear transient state.
  Missing tire telemetry also prevents the AI from relying on invented
  grip samples.
- The existing player-aware rival following, event countdown, four-gate
  route recovery, damage, lap clock, credits, car garage, nitro,
  drafting and physical chassis collision remain separate authorities.

## Portable and source-binding verification

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release --parallel
ctest --test-dir Tests/Standalone/build -C Release -R '^RivalGripPace$' --no-tests=error --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'RivalGripPaceBindingTests.py' -v
```

The portable tests cover valid grip-limited straight/corner/launch cases,
brake hysteresis, already-braking and arrived routes, time hitches,
nonfinite telemetry, invalid controls and 5,000 race-session reset cycles.
Python source tests enforce ordered integration with the original
corner anticipation, live tire condition, traffic awareness and rival
control path. These are not Chaos physics simulations.

## Mandatory installed-game acceptance

1. In licensed **Unreal Engine 5.8**, compile actual UHT and Game/Editor
   code; cook the authentic owned race map and package a real candidate.
2. Drive a complete multi-lap race on Win64. Observe the actual AI at
   straight entries, high-speed corners, cold and worn race-session tires,
   right/left approaches, player traffic and final waypoint.
3. Confirm low grip causes only slower corner-entry pace, never
   arbitrary steering or acceleration. Compare at 30, 60 and 120 FPS;
   check for stuck/braking loops and realistic pit-free completion.
4. Check enemy route/recovery, interrupted focus, pause, failed
   physics, stopped player, restart and race-end handling. The AI must
   not acquire additional official wins, rewards or save privileges.
5. Record exact build SHA, engine version, map, package hash, logs,
   controller/keyboard evidence and failures before claiming gameplay.

This is **source-only** until UE compile, map cook and installed-device
testing are documented on
[playable gate #148](https://github.com/sanskarIN/latebrake/issues/148).
