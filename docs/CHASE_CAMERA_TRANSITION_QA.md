# Chase-view camera transition — source and installed-game QA

**Status:** mainline-port candidate of earlier draft PR #328; not a filmed Unreal Engine 5.8 playtest.
This changes only the human spring-arm camera; it does **not** improve
vehicle physics, tire grip, road collision, car art or release playability.

## Behavior and safe scope

- C cycles existing CHASE, CLOSE CHASE, HOOD, HIGH CHASE views.
- The three **chase** views use a 0.32-second cubic smoothstep for spring-arm
  distance, pitch and forward/vertical socket offset. FOV, spring-arm collision
  probing and camera lag policy continue to come from the selected preset.
- Entering/leaving **HOOD** snaps immediately, preventing an intermediate
  arm passing through the car body; rear-view press/release is also immediate.
- Repeated chase changes start from the last rendered pose. A pause, app focus
  loss, recovery or invalid/overlong tick ends the unfinished blend.
- This is deliberately **presentation-only**. No scoring, control, save data,
  rival AI, or Chaos forces read this state.

## Portable checks

Run the strict stand-alone test suite and the source-binding Python fixture:

    cmake -S Tests/Standalone -B Tests/Standalone/build
    cmake --build Tests/Standalone/build --parallel
    ctest --test-dir Tests/Standalone/build -R '^ChaseCameraTransition$' --output-on-failure
    python3 -m unittest discover -s Tests/Editor -p 'ChaseCameraTransitionBindingTests.py' -v

The C++20 contract checks 30/60/120 FPS equivalence, exact target completion,
interrupted view changes, 10,000 rapid repeated switches, finite input bounds,
pause cancellation and invalid frames. The Python fixture checks the real
pawn's binding text, **not** C++ or UHT compilation.

## Required trusted UE5.8 and installed Win64 qualification

1. Compile project using the actual licensed UE5.8 editor, UHT and Game
   target on the candidate commit. Source-only CI cannot validate native
   symbols, assets or the actual camera.
2. In an installed cooked original circuit, cycle each adjacent camera
   pair; confirm the hood has no intermediate clipping, and that chase
   views ease without a teleport or rendering through collision geometry.
3. Change views twice during a single blend at 30, 60 and 120 FPS; verify
   displayed spring-arm framing is continuous and the new target is reached.
4. Hold/release rear view during a transition. Pause and resume, lose/regain
   focus, restart, recover/teleport and finish; verify no stale transitional
   pose or yaw lead remains after control returns.
5. Simulate camera component loss and regained ownership, including local
   controller handoff: confirm no pre-interruption animation resumes.
6. Drive six distinct garage profiles through corners and across uneven
   road geometry; inspect spring-arm collision probe, follow-camera FOV and
   camera lag. Record worst-frame times and capture footage.
7. Repeat in an AI rival session: no remote/playerless pawn should animate
   a human camera; never infer vehicle suspension from visual ease.

Installed-playability qualification remains outstanding until the
owner has actual compile, package and physical playback evidence. Portable unit tests and
string-binding tests cannot substitute for video or measured installed QA.
