# Observed arcade pit-service receipts

Source-only extension to the actual player pawn's service block. The
receipt logs before/after arcade model fractions only after the existing
hold completes. It does not control repairs, penalties, race records,
credit transactions or persisted vehicle state. Totals are cumulative
percentage points, not an implied current vehicle condition. The newest
eight service visits are retained; aggregate counts must match the real
service latch. Invalid or missing observations suppress the review until
the next race reset.

Native acceptance: complete a real service with chassis damage, tire
wear and depleted boost. Verify live feedback and results correspond to
actual model state; a partial repair/refill must not appear as full.
Repeat after leaving and returning to the bay, verify reset clears the
journal, and inspect compact and expanded HUD. Native UE/UHT, cook and
installed Win64 qualification are separate from portable tests.
