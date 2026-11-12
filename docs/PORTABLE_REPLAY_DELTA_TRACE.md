# Portable replay delta trace: tested timing foundation

This source-only component computes timing differences between two
**pre-measured** laps on the **same** immutable course. It never drives a car,
plays a ghost, saves an official record, samples the actual Unreal spline, or
authorizes a time-attack reward.

- Header: \`Source/Latebrake/Public/Racing/PortableReplayDeltaTrace.h\`.
- Build target: \`PortableReplayDeltaTrace\` in strict Windows, macOS and
  Linux portable C++20 CTest.
- Bounded 128-sample storage; \`Reset\`, \`Append\`, \`Estimate\`, \`CompareReplayAtDistance\`.
- Reject nonfinite data, out-of-order distance/time, wrong course length,
  incomplete laps, out-of-bounds queries and sample overflow.
- Distances use metres and elapsed time uses seconds; timing interpolation
  is **only** a piecewise linear estimate, not an engine-reported split.
- Positive difference means the challenger is behind at that distance.
- No heap allocation, dynamic filesystem writes, gameplay state mutation,
  live ghost rendering, Unreal dependency, or public release qualification.

Future integration requires authentic distance projection, measured timestamps,
player eligibility and live native Win64 Unreal validation; it is not included
in this PR.
