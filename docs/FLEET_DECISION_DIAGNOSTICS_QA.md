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
