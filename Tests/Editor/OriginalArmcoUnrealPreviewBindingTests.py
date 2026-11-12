import pathlib
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[2]
GAME=(ROOT/"Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp").read_text(encoding="utf-8")
BRIDGE=(ROOT/"Source/Latebrake/Private/Track/OriginalArmcoUnrealPreviewBridge.cpp").read_text(encoding="utf-8")
SOURCE=(ROOT/"Source/Latebrake/Public/Track/OriginalArmcoBarrierMesh.h").read_text(encoding="utf-8")

class OriginalArmcoUnrealPreviewBindingTests(unittest.TestCase):
    def test_opt_in_production_boundary(self):
        self.assertIn("LatebrakeExperimentalArmco",GAME)
        self.assertIn("SpawnOriginalArmcoPreview(",GAME)
        self.assertIn("#if !UE_BUILD_SHIPPING",GAME)
        self.assertIn("BuildOriginalCircuitPaintPreview",GAME)

    def test_noncolliding_guardrail_remains_source_trusted(self):
        self.assertIn("OriginalArmcoBarrierMeshBuilder builder;",BRIDGE)
        self.assertIn("builder.ValidateMesh(built.Mesh)",BRIDGE)
        self.assertIn("PrepareOriginalBarrierSections(",BRIDGE)
        self.assertIn("CreateMeshSection_LinearColor(",BRIDGE)
        self.assertIn("false, // Never create alternate collision",BRIDGE)
        self.assertIn("SetCollisionEnabled(ECollisionEnabled::NoCollision)",BRIDGE)
        self.assertIn("SetActorEnableCollision(false)",BRIDGE)
        self.assertIn("SetCanEverAffectNavigation(false)",BRIDGE)
        self.assertIn("OriginalBarrierSteelColor",BRIDGE)
        self.assertNotIn("SetSimulatePhysics(true)",BRIDGE)
        self.assertNotIn("RecordCollisionBlock(",BRIDGE)
        self.assertNotIn("Race.Credit",BRIDGE)

    def test_original_collision_wall_identity_never_changes(self):
        self.assertIn("PlannedCollisionBlock(",SOURCE)
        self.assertIn("OriginalBarrierFirstBlock=5",SOURCE)
        self.assertIn("OriginalBarrierLastBlock=8",SOURCE)
        self.assertIn("ArmcoInsidePhysicalBarrier(",SOURCE)
        self.assertIn("OriginalCollisionPlanMatches()",SOURCE)
        self.assertIn("NoOfficialLapAuthority",SOURCE)

if __name__=="__main__":
    unittest.main()
