# Actual chassis rear-lamp alignment — source QA

The real stock Unreal Cube chassis has X half-length 190cm and Y
half-width 90cm after its 3.8x1.8x0.65 root scale. Previously the two
red point-light origins were at X=-385cm and the white reverse light
at X=-390cm—**195-200cm BEHIND** the actual bumper and farther than
their own 170cm attenuation radius. The red lamps were also outside
the car body width at Y=+-115cm.

The corrected source aligns red lamp origins at (-194,+-64,22)cm and
the white lamp at (-196,0,22)cm. They are 4-6cm beyond the true shell,
have the same low-cost shadow-free settings, and remain visual-only.
A shared C++20 layout defines chassis root scale, real half-extents
and the three attachments. The pawn uses that same root scale in its
collision chassis, decorative mesh compensation, dynamic body kit
and the lamp attachments so one change cannot silently detach them.

Strict C++ tests verify actual geometry, symmetric red pair, centered
reverse lamp, nonuniform Unreal parent-scale compensation and rejection
of the original detached/invalid layouts. Editor-only Python checks
verify original components and current brake, reverse, focus/reset
activation paths are still bound; neither test renders a scene.

**Not a photoreal light assembly.** There are still no emissive 3D
tail-lamp lenses, original PBR textures or hardware observations.
The genuine UE5.8 Editor/Win64 test must verify rear bumper placement,
red brake and white reverse illumination under day/dusk exposure and
chase camera, no clipping, and reasonable GPU cost. No C++ gameplay,
collision, user control, checkpoint, career or saved records changed.
