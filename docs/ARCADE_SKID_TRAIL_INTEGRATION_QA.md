# Actual arcade skid trails

Reconnects the existing portable skid geometry and ring pool (historical
draft #327) through the current actual arcade pawn, not the unmerged
four-spring physics candidate. A trace must hit a live collision-plan
asphalt component with its exact original transform and collision type.
Hit plane, full strip footprint, measured point velocity and probe
distance are verified. The arcade evidence path refuses any supplied
normal load or modeled slip. Marks are lateral-motion cosmetics only.

The optional world-space actor has a fixed 2048-slot ring, no collision,
no overlap or simulation, and a bounded expiry scan. A project-owned
rubber material is preferred if available; a verified global tint on
the stock material is the fallback. Neither source checks nor material
installation prove rendered pixels. Missing assets or failed updates
disable only the visual renderer.

Native acceptance: verify marks on paved drifting, no marks on grass,
walls, airborne or stationary bodies, no joins across recovery/pause,
all marks stay on the road, reset clears the ring, fallback visibility,
per-car budget at 30/60/120 FPS, and no change in handling or collision.
Check actual UE5.8 compile/UHT, Win64 cooking, GPU performance, and
installed gameplay separately. Historical renderer code was used as
a reference; do not count previously authored geometry as new work.
