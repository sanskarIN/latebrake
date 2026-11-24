
# Unreal 5.8 Canvas menu readability: draft QA

The earlier menu printed text directly against bright or visually busy
3D tracks. The menu now gains a dark translucent panel behind the left
text lane, with a contrasting header and accent rail.

The pure source policy HudMenuReadablePanel.h bounds the contrast area
within the real font-measured text lane, away from desktop mouse
buttons and the Android touch-steering strip. The same panel includes
the reserved SAVE FAILED baseline. This cannot change purchases,
menu hit tests, race input, scores, player saves or gameplay physics.

Portable C++ tests cover normal resolutions, small and portrait windows,
100000 deterministic screen dimensions, invalid layout parameters,
and absence of a panel once the menu closes.

UNVERIFIED native Unreal visual acceptance: verify all menu pages at
900x600, 1280x720 and 1920x1080, daylight/night/rain backdrops, 125
percent DPI, Android pads, actual mouse buttons, long wrapped strings,
save warning visibility, and no obstruction of the garage schematic.
Real UE5.8 UHT, Editor, cooked Win64 and physical QA are still required.
