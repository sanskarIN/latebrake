# Six-car launch + predictive traffic integration QA

**Experimental DRAFT / SOURCE-ONLY. A passing portable test does not prove Unreal Engine 5.8 Editor/UHT, cooked original assets, installed Win64 gameplay, real Chaos ground contact or target FPS.**

## Provenance

- Shared experimental physical-fleet base: PR #333 at 469c32302a7c4ae69c6fd6a14f1119933ce85f74.
- Pre-green exact physical launch validation: PR #337 at 3e8d0bda0c8ef9e90a86379e440cf632a956e9a2.
- After-green six-car predictive traffic: PR #338 at b1540472ccaa4dd1755170775744694b917049a7.
- The current integrated PR is not the original frozen UE qualification PR #335, does not update protected main or v0.5.98, and cannot authorize release publication.

## Safety boundary

The pre-green GameMode samples six actual physical chassis and checks XY/yaw/velocity, vertical position, fresh four-corner WorldStatic contact, and exact original SouthStraight collision actor identity. Failure to observe a single real body or sufficient ground support denies GO.

After green, each actual AI samples five unique real other actors. The existing short-horizon traffic check remains authoritative. The additional 1.4-second bounded OBB prediction can further reduce throttle and request braking. The supervisor tracks exact real actor tokens, brake capability estimated from already accepted tire/heat/surface/damage observations, and bounded hazard hysteresis. It must never invent steering, change player controls, teleport, award laps, save progress or perform publishing.

## Portable tests

The combined strict C++20 target (SixCarAggregateSafety) verifies that the same six actor tokens are valid across the pre-green and after-green policies; a missing body fails both, insufficient ground support blocks green, and actor replacement triggers new braking authority. Independent CTest targets still cover both separate policies. Static Python contracts verify the actual source callsite composition and release isolation.

Run complete CTest and editor source contracts on every supported CI platform. Passing counts must be observed on the exact candidate commit rather than copied from sibling PRs.

## Licensed engine acceptance required before merge

1. Record and review the exact new aggregate source HEAD and toolchain identity; do not overwrite the previously frozen PR #335 evidence.
2. Run authorized UE5.8 UHT, Editor and Game builds. Treat compile failures as blockers; source-only tests cannot catch every engine API mismatch.
3. Cook the owned original circuit and drive a genuine installed Win64 build; verify the real six chassis bodies, SouthStraight contact and 3/2/1/GO transitions.
4. Test absent, duplicated, floating, displaced, crashing and recovered cars. No unsafe start, duplicate actor credit or silent fleet fallback is allowed.
5. Test real post-green opposing/crossing/closing traffic, braking heat, worn tires and grip variation, while checking steering and player scoring remain unchanged.
6. Profile actual 30/60/120 FPS with all six cars, four traces per vehicle during the countdown, and the after-green collision forecast. Collect logs and reproducible evidence.
7. Regress ordinary single-rival Circuit, solo Time Attack, career, pause, focus loss and restarts. Keep this developer feature non-shipping.

Issue #148 remains open until all playable-engine acceptance gates pass. Production-code counts exclude tests, docs, workflows, generated or duplicate code, and must not add overlapping PR totals.
