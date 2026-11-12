# Portable circuit grip profile

Source-only \`PortableCircuitGripProfile.h\` models a fully-covered lap
station -> authored dry/wet grip profile as a future diagnostic tool.
Every contiguous interval must be specified explicitly, without gaps or
overlap; no invented default asphalt replaces missing authored track data.

- Bounded 64 sections, deterministic binary search and fail-closed input.
- Explicit wetness, rubber and debris adjustments return a bounded
  dimensionless estimate, not a confirmed tire force.
- Portable C++20 CTest exercises exact boundaries, malformed data, no
  extrapolation and 8,000 mid-wetness lookups.
- Zero Unreal/Chaos integration: the result is **not** applied to the
  existing player's tires, any AI actor, asphalt slab, collision, rain,
  season or official qualifying lap.
- Future integration needs proven UE5.8 collision surface sampling,
  actual track authored layout and installed Win64 game testing.
