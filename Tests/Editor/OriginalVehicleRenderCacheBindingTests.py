"""Bounded CPU cache source contracts; no GPU/Unreal package claims."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(path):
    return (ROOT/path).read_text(encoding="utf-8")

class OriginalVehicleRenderCacheBindingTests(unittest.TestCase):
    def test_preparation_only_on_synchronous_cache_miss(self):
        pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        rebuild=pawn.split("void ALatebrakeCarPawn::RebuildOriginalVehicleVisual(",1)[1].split(
            "int32 ALatebrakeCarPawn::GetCommittedOriginalWheelParts()",1)[0]
        for mark in ("OriginalRenderCache.Acquire(",
                     "{VehicleId,static_cast<int>(GetPresentedGarageLook().Wheels)}",
                     "OriginalPreparedVehicleRender::Prepare(",
                     "generated.Mesh, shading, animated", "Prepared->Packed()",
                     "GarageCameraGeometry = Prepared->CameraGeometry();",
                     "Part->CreateMeshSection("):
            self.assertIn(mark,rebuild)
        self.assertLess(rebuild.index("OriginalRenderCache.Acquire("),
                        rebuild.index("OriginalVehicleSurfaceComposer{}.Build("))
        self.assertLess(rebuild.index("Prepared->Packed()"),
                        rebuild.index("Part->CreateMeshSection("))
        self.assertLess(rebuild.index("bOriginalVehicleVisualReady = true;"),
                        rebuild.index("GarageCameraGeometry = Prepared->CameraGeometry();"))
        self.assertIn("OriginalRenderCache.Reset();",pawn.split(
            "void ALatebrakeCarPawn::EndPlay(",1)[1].split("void ALatebrakeCarPawn::UnPossessed(",1)[0])
        tick=pawn.split("void ALatebrakeCarPawn::Tick(",1)[1].split(
            "void ALatebrakeCarPawn::ApplyWheelVisualPose()",1)[0]
        self.assertNotIn("OriginalRenderCache.Acquire(",tick)
        policy=read("Source/Latebrake/Public/Vehicles/OriginalVehicleRenderCache.h")
        self.assertIn("OriginalRenderCacheCapacity=3u",policy)
        self.assertIn("BuildOriginalIndexedRender(mesh,shading,animated)",policy)
        self.assertIn("camera.Build(mesh)",policy)
        self.assertIn("std::make_unique<const OriginalPreparedVehicleRender>",policy)

    def test_diagnostics_do_not_claim_gpu_benchmark_or_authority(self):
        hud=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        settings=hud.split("case latebrake::MenuPage::Settings:",1)[1].split(
            "case latebrake::MenuPage::Profile:",1)[0]
        for mark in ("GetOriginalRenderCacheStats()", "CPU MESH CACHE:",
                     "NOT VRAM", "#if !UE_BUILD_SHIPPING"):
            self.assertIn(mark,settings)
        self.assertIn("add_latebrake_portable_research_test(OriginalVehicleRenderCache)",
                      read("Tests/Standalone/CMakeLists.txt"))
if __name__=="__main__":unittest.main()
