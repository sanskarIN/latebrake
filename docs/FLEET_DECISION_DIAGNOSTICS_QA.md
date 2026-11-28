# Experimental AI command diagnostics

The real rival Tick now records each of the twelve actual command stages,
from route planning through corner, difficulty, grip, surface, heat,
following, passing, immediate/predictive traffic, yaw and spin.
This is sampled request telemetry, not applied motor force or a collision
verdict. Every stage retains its existing control authority.

Only a fresh active-clock admission and the matching physical rival can
contribute, once per frame. The GameMode rechecks the accepted gate ordinal.
Missing observations, pause, recovery, epoch changes and malformed requests
break duration continuity. The summary integrates only adjacent qualified
samples and distinguishes braking, brake origin, queue, no-drive and
traffic-hold intervals. A change of brake origin is not double-counted.
No stage is attributed to an unsampled interval.

Live HUD counts represent current observed AI requests. Results retain
per-slot history only if independent route evidence agrees. The history
helps owner QA distinguish traffic holds from ordinary corner braking and
unqualified physical inputs. It changes no reward, rank or driving inputs.

Native QA: verify the actual six-car game, use a slower rival and corner
approach, compare the recorded requests with a real physics trace, pause,
recover and finish. Verify ordinary modes remain unaffected. Portable
tests establish source policy, not installed Unreal behavior.

## Exclusive no-drive context

For adjacent stationary/no-drive samples, attribute the observed interval
exactly once: road unqualified, traffic unqualified, traffic forecast,
traffic hold, queue, other brake, low throttle, or mixed endpoint context.
This deterministic precedence avoids counting overlapping conditions as
multiple seconds of lost time. The expanded Results UI reports the most
frequent sampled context by duration, not a causal diagnosis. A missing
sample cannot contribute to any bucket. The standalone coverage ratio
compares observed intervals to the elapsed race window; completed rivals
and absent samples may both reduce coverage. Teleport revision rollback
is refused even after a missing sample, and never lowers the accepted
revision baseline.

The Results fleet coverage denominator is five rivals multiplied by the
player's elapsed fee-free active clock. Only independently verified,
contiguous sampled intervals enter the numerator. One endpoint alone is
not a duration. This deliberately exposes incomplete observation rather
than normalizing missing rivals away; it is not a score for AI quality.

### Explicit command-stage completeness

Each rival command observation must contain all twelve stages, recorded once in
pipeline order. Missing, repeated, out-of-order or unknown stage identifiers
invalidate the entire trace; a default-initialized command is not evidence of
execution. Explicit zero commands remain admissible. A rejected native sample
cuts continuity and cannot be replaced in the same admitted frame, preventing
invented braking duration and false coverage. Standalone regressions exercise
every omitted and duplicate stage. Native UE gameplay remains unqualified.

### Sector-specific AI command review

The exact configured gates per lap define sector buckets. An interval enters
one bucket only if both adjacent, identity-bound endpoints report the same
absolute next-gate ordinal. Gate transitions are reported separately; even
identical modulo sectors on different laps are not stitched. Missing samples,
recovery and suspension still cut the journal's continuity. Results lists
observed-sector coverage and the sectors with the most sampled no-drive or
brake exposure among buckets with at least 0.5 s of observations. These are
not official sector times, estimates of lost time or a causal diagnosis.
Verify on a native six-car run that a missing frame, gate change and recovery
do not produce extra attributed seconds, and compact Results remain usable.
