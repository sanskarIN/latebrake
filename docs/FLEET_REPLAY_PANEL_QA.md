# Desktop experimental Results replay map

A wide desktop Results screen can show the exact frozen, recent six-car
observation window as a north-up original circuit map. Marker positions
and headings are interpolated only through complete retained frames.
Off-map cars are omitted, and the display is labelled sampled replay.

Use J and L to seek half a second, K to play/pause, H to select the next
retained highlight. The six map-panel buttons also allow focus cycling
and half/normal/double playback speeds. Click the timeline to seek.
Playback uses UI real time while the race is finished; it never restarts
the race or changes the official simulation clock. Leaving Results,
focus interruption, invalid evidence or a long UI hitch stops playback.

The panel is below the actual desktop navigation dock and outside the
protected text lane. Compact and touch layouts retain the paged text
review. Mouse input uses the exact presented geometry and frozen attempt,
not a stale viewport or a hidden map. Native QA must verify rendering,
font fit, input focus, resize, paused playback and the normal Results
restart action. This is 2D review, not a native cinematic camera.

### Direct car focus and sampled control readout

Click a uniquely nearest displayed car marker to select that observed slot and
pause the replay at the presented cursor. Ambiguous overlapping markers do not
select; use the existing FOCUS control instead. The focus row displays sampled
speed plus throttle, brake and signed steering command percentages (CMD), not
physical tire forces. Off-map actors retain sampled telemetry but cannot be
selected through a fabricated edge marker. Exact clip identity includes the
representative event clock, type, sampled speed and observation count.
Verify the expanded desktop panel and controls in the installed game; the
portable geometry and input tests do not qualify native canvas rendering.
