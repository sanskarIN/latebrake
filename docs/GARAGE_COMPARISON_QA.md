# Production garage: comparisons and upgrade price previews

**Status:** Unreal Engine 5.8 source-only development. Not a validated
render, installed Windows executable, or finished production UMG interface.

The real six-car Unreal Garage Canvas page now offers a comparison to the
currently **equipped car**, with signed theoretical tuning differences for
acceleration, grip and maximum-speed multipliers. In the desktop-expanded
view, each row states `VS EQUIPPED: ACC +12% | GRIP -6% | SPEED -2%`
for Kestrel when the starter Arrow is equipped with default upgrades.

These are ratios between the same GarageState multipliers; they are **not**
measured acceleration times, handling scores, 3D performance tests or race
positions. They never change the garage. Locked profiles may be inspected,
but not equipped or purchased from their individual displayed rows.
The first locked vehicle remains the only available Q purchase candidate.

Next to the current shared engine and grip levels, the desktop menu also
shows the next tier prices and whether the garage has enough credits, or
**MAX** if there is no further tier. This mirrors the existing
`GarageState::UpgradeCost` policy, not a second pricing authority.

## Validation included

- Pure C++20 policy tests compare both negative and positive deltas, all
  six cars, saved equipped selections and shared upgrades.
- Input guards reject invalid IDs, mixed-catalogue IDs, nonpositive,
  infinite/NaN, overflowing performance values, invalid credits/levels
  and unsupported upgrade types.
- Upgrade previews accurately display shortage/affordability/MAX, without
  changing ownership or credits.
- Unreal source binding confirms the menu reads the existing snapshot,
  uses only UI presentation, and retains the current purchase feedback.
- The existing paginated menu remains in control of overflow; compact
  layouts do not gain long comparison lines.

## Physical QA still required

Review on an actual authorized Unreal Editor/Win64 build: 720p/1080p,
ultrawide, high DPI, localization/wrapping, page scrolling, purchase
confirmations and failed-save rollbacks. An Android landscape device must
be tested separately. Issue #236 remains open. No game installer or release
artifact is produced by this change.
