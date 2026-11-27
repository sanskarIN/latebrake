# Experimental six-car matched-gate timing

This source candidate extends the opt-in experimental fleet HUD and finish
review. It compares independently accepted physical gate receipts at the
same gate ordinal on the common active race clock with player penalties
removed. It is not a race ranking, official gap, wall-clock lap record,
award or shipping feature.

## Evidence and retention

- Bind all six actor identities and the exact course shape once per attempt.
- Accept only the next contiguous positive gate ordinal per physical actor.
- Reject a foreign actor, repeated gate, uninitialized attempt and invalid
  or non-increasing active clock without changing the accepted stream.
- Compare each pair only at its most recent shared retained gate. Never
  subtract clocks from different gates. A full-fleet summary requires
  independent retained receipts for the same gate from all six actors.
- A bounded 257-sample ring per car retains a maximum 256-gate lap and its
  preceding boundary when the samples are still present. Overwritten
  receipts are unavailable, never substituted by a newer array entry.
- The existing live physical route review must agree with each stream's
  gate count; rival last-gate clocks must also match the independent ledger.
- +/-0.25 seconds is an uncertain sampled clock window, not a certified tie.
- The finish snapshot is captured before experimental permissions are
  revoked. No later callbacks modify that published snapshot.

## Native acceptance checklist (not yet performed)

1. Build the exact candidate using the licensed Unreal Engine and UHT.
2. Launch the installed Windows experimental six-car circuit. Confirm
   the default one-rival and solo/career paths remain unchanged.
3. Confirm no matched timing is shown before both cars accept a gate.
4. Observe rivals ahead and behind the human at different checkpoints.
   Verify each row labels the exact shared gate, not either car's next gate.
5. Confirm equal sampled clock values display CLOSE rather than rank or a
   precise winning margin. Compare a separated common-gate observation.
6. Pause, resume, recover and restart. Confirm no old-attempt timing survives
   and that fee-free active clock evidence never includes recovery penalties.
7. At the genuine player finish, confirm the frozen rows reflect only the
   previously observed receipts and experimental results save no awards.
8. Test compact and expanded HUD, Results clipping, long race gate labels,
   controller navigation and window resizing. No installed rendering is
   claimed by the portable unit tests.

The standalone tests cover malformed receipts, asynchronous gate streams,
cohort comparison, retention boundaries and maximum supported gate counts.
Editor Python tests assert source binding order only, not native behavior.
