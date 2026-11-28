# Detailed HUD command trace

This session-only display reads completed local player physics ticks. It
does not measure wheel forces or change driving. Switch HUD density to
Detailed in Settings. On a sufficiently tall desktop layout, the new
panel occupies space between the existing race radar and instruments.
On smaller or touch layouts it falls back to text in the ordinary
telemetry queue; primary race information retains its existing priority.

The four-second window plots time-weighted filtered and moderated
normalized throttle/steering, independent service and handbrake request
fractions, and actual admitted ground-support state. Empty history is
not filled with fabricated zeros. A rejected, skipped or interrupted
tick, changed controller, race or teleport revision restarts the trace.
The trace is bounded to 1024 samples, so very high tick rates may provide
less than four seconds. Displayed coverage describes observations only.

Native checks still required: verify fonts and overlay fit in Unreal at
720p, 1080p and ultrawide resolutions; test keyboard, controller and
touch controls; observe pause, focus-loss, recovery and restart; check
negative throttle/steering, service and handbrake, and loss of support.
The standalone suite and editor source tests do not certify these.

Mint envelopes preserve the actual minimum and maximum moderated commands
within each observed bin. They are not uncertainty bounds or predicted
motion. Compact text reports mean and peak differences between filtered
and moderated commands in percentage points, over the observed window.
