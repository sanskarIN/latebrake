# LateBrake original 3D source models — OBJ inspection preview

This artifact is a viewable GEOMETRY EXPORT, NOT a running Unreal Engine
game, cooked map, wheel/steering animation, vehicle physics, or a replacement
for original Windows UE5.8/UHT/native performance QA.

Why: the full original Unreal 3D vehicle generator produces six car forms,
each with 3 true rim meshes. The plugin-independent geometry is validated
by C++20 on Windows/Linux/macOS but cannot be visually evaluated on that
basis alone. The export uses the production geometry and material IDs.

How to get a preview

1. Open the workflow Original 3D source car OBJ preview (NOT Unreal) on
   the draft PR branch:
   https://github.com/sanskarIN/latebrake/actions/workflows/original-3d-source-obj-preview.yml
2. Open its latest successful run for the exact reviewed PR #452 SHA.
   Download the ZIP artifact named UNVERIFIED-LateBrake-3D-source-OBJ-...
3. Extract it to a normal folder. It contains 18 pairs of .obj/.mtl files,
   named LateBrakeOriginalCar_0_Rim_0 to Car_5_Rim_2, a source receipt,
   and SHA256SUMS.txt.
4. In any trusted OBJ viewer, or Blender File > Import > Wavefront (.obj),
   open a file. Keep each OBJ and identically named MTL together so
   diffuse swatches load. Viewers may require enabling material preview.
5. Compare each car's STREET (rim 0, seven spokes), SPORT (rim 1, five
   spokes) and TRACK (rim 2, twelve spokes). All four wheels should exist
   and be separated as named groups from the static body.
6. Note screenshots and exact source SHA. Compare the shapes with real
   Unreal screenshots once a licensed 5.8 Editor/Win64 build exists.

File coordinates are Y-UP and metres, converted exactly from the
production source generator's (X,Y,Z)-centimetres:
OBJ (x,y,z)=(X,Z,-Y)/100. This is a right-handed transformation;
normals are mapped in the same way without changing triangle winding.
OBJ is static; the real Pawn handles steering and wheel rotation.

The 10 MTL color groups mirror the existing native source RGB palette
for each car's factory paint and wheel style. MTL diffuse swatches do not
implement Unreal materials, reflections, shadows, clearcoat, glass
transparency, lighting or physically correct rendering. A colored
OBJ is NOT visual proof of production game quality.

Run without GitHub (local development)

Compile with a C++20 compiler from the repository root:
  g++ -std=c++20 -O2 -I Source/Latebrake/Public Tools/OriginalVehicleObjPreview.cpp -o latebrake-obj-export

Then run:
  ./latebrake-obj-export output-cars

For the actual game use the separate PR #452 Unreal Editor/Win64
playtest workflow and acceptance guide. Do not submit exported files
as if they are a shipped Unreal executable.
