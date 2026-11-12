import pathlib
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[2]
GAME=(ROOT/"Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp").read_text(encoding="utf-8")
BRIDGE=(ROOT/"Source/Latebrake/Private/Track/OriginalKerbUnrealPreviewBridge.cpp").read_text(encoding="utf-8")

class OriginalKerbUnrealPreviewBindingTests(unittest.TestCase):
    def test_opt_in_gate(self):
        self.assertIn("LatebrakeExperimentalKerbs",GAME)
        self.assertIn("SpawnOriginalKerbPreview(",GAME)
        self.assertIn("#if !UE_BUILD_SHIPPING",GAME)

    def test_five_collision_free_procedural_sections(self):
        self.assertIn("OriginalCircuitKerbComposer builder",BRIDGE)
        self.assertIn("builder.Validate(authored.Mesh)",BRIDGE)
        self.assertIn("PackKerbSections(",BRIDGE)
        self.assertIn("CreateMeshSection_LinearColor(",BRIDGE)
        self.assertIn("false,false); // No new physics collision.",BRIDGE)
        self.assertIn("SetCollisionEnabled(ECollisionEnabled::NoCollision)",BRIDGE)
        self.assertIn("SetActorEnableCollision(false)",BRIDGE)
        self.assertIn("SetCanEverAffectNavigation(false)",BRIDGE)
        self.assertNotIn("SetSimulatePhysics(true)",BRIDGE)
        self.assertNotIn("Race.Credit",BRIDGE)
        self.assertNotIn("RecordCollisionBlock",BRIDGE)

if __name__=="__main__":
    unittest.main()
