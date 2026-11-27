# Experimental fleet slipstream qualification

This source feature extends the existing bounded arcade drafting mechanic
to the opt-in six-car circuit. Standard single-rival and career paths
retain the existing policy. The experimental implementation chooses one
real leader, never a stack of five boosts.

## Contract

- All five real rival identities, actual physical state and the locally
  controlled human must belong to the admitted experimental race frame.
- A candidate must be ahead, moving, sufficiently aligned, supported
  by physical road probes, unfinished and unobstructed, with the same
  speed, lateral-slip and distance checks as the established policy.
- Each eligible wake uses an independent real Visibility trace between
  the player and its exact leader. The other physical actors and circuit
  barriers are not excluded from that trace.
- A strongest candidate is chosen initially; an already qualifying
  leader remains selected while its wake remains valid. Replacement,
  recovery, actor teleport, stale frame, pause, changed race epoch,
  missing roster or relative-motion jump revokes confirmation.
- The new selected body requires its own continuous 0.18 second
  qualification. No previous leader's accumulated time is inherited.
- The selected bonus remains at most 6 percent before the existing
  shared nitro/drafting force allocation. The HUD reports the applied
  fraction after that allocation, not a requested or stacked bonus.

## Native Unreal Engine 5.8 acceptance checklist

1. Build the exact source candidate and launch the installed game.
2. Confirm all five AI bodies exist in the experimental circuit.
3. Follow each physically moving, road-supported AI in turn. Verify a
   single leader slot and no more than the ordinary drafting bonus.
4. Switch between two adjacent eligible rivals, recover either rival,
   pause/resume and restart. Check that old qualification is not reused.
5. Test a solid barrier, interposed third car, stationary/finished car,
   sideways-slipping car, opposite heading and airborne road support.
   Confirm that obstructed and physically ineligible wakes never apply.
6. Combine nitro and drafting and confirm the shared motor budget.
7. Verify single-rival, solo and career behavior remains unchanged.
8. Review compact and expanded HUD after interruption or race finish.

The implementation and portable tests are source-only evidence until
native cooking, package launch and physical gameplay have been tested.
