import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp").read_text(encoding="utf-8")
HEADER = (ROOT / "Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h").read_text(encoding="utf-8")
RACING = (ROOT / "Source/Latebrake/Public/Track/RacingLineLayout.h").read_text(encoding="utf-8")
DETAIL = (ROOT / "Source/Latebrake/Public/Track/CircuitSurfaceDetail.h").read_text(encoding="utf-8")

class OriginalCircuitPaintRuntimeBindings(unittest.TestCase):
    def test_opt_in_only_nonshipping_circuit_paint(self):
        self.assertIn("void BuildOriginalCircuitPaintPreview(UStaticMesh* Mesh);", HEADER)
        self.assertIn("LatebrakeExperimentalCircuitPaint", SOURCE)
        self.assertIn("#if !UE_BUILD_SHIPPING", SOURCE)
        self.assertIn("BuildOriginalCircuitPaintPreview(DecorationMesh);", SOURCE)
        self.assertIn("FParse::Param(FCommandLine::Get()", SOURCE)

    def test_geometry_must_be_entirely_supported_by_actual_asphalt(self):
        self.assertIn("IsValidRacingLinePaintFootprint(Mark)", SOURCE)
        self.assertIn("IsSurfaceDetailOnPhysicalAsphalt(Detail)", SOURCE)
        self.assertIn("RacingLineAsphaltCoveredAreaCm2", RACING)
        self.assertIn("PlannedCollisionBlock(", DETAIL)
        self.assertIn("Guides!=latebrake::RacingLineMarkCount", SOURCE)
        self.assertIn("Edges!=latebrake::CircuitSurfaceEdgeCount", SOURCE)
        self.assertIn("RubberMarks!=latebrake::CircuitRubberCount", SOURCE)
        self.assertIn("Holder->Destroy();", SOURCE)

    def test_preview_meshes_have_no_collision_or_game_authority(self):
        function=SOURCE.split("void ALatebrakeRaceGameMode::BuildOriginalCircuitPaintPreview(",1)[1].split("void ALatebrakeRaceGameMode::BuildCircuit()",1)[0]
        self.assertIn("SetCollisionEnabled(ECollisionEnabled::NoCollision)", function)
        self.assertIn("SetGenerateOverlapEvents(false)", function)
        self.assertIn("SetCanEverAffectNavigation(false)", function)
        self.assertIn("SetCastShadow(false)", function)
        self.assertNotIn("SetSimulatePhysics(true)", function)
        self.assertNotIn("RecordCollisionBlock", function)
        self.assertNotIn("Race.Reset()", function)
        self.assertNotIn("Race.Credit", function)

if __name__ == "__main__":
    unittest.main()
