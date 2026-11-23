"""Real Unreal Sun/Sky Settings-only visual toggling, not Editor shader QA."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def src(p):return (ROOT/p).read_text(encoding="utf-8")
class UnrealCircuitLightingBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.gm=src("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.h=src("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.pawn=src("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.ini=src("Config/DefaultInput.ini")
        cls.hud=src("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.cmake=src("Tests/Standalone/CMakeLists.txt")
        cls.test=src("Tests/Standalone/CircuitLightingPreviewTests.cpp")
    def test_updates_same_actual_engine_lights(self):
        self.assertIn("RuntimeSun = GetWorld()->SpawnActor<ADirectionalLight>()",self.gm)
        self.assertIn("RuntimeSky = GetWorld()->SpawnActor<ASkyLight>()",self.gm)
        self.assertIn("ApplyCircuitLightingPreview();",self.gm)
        section=self.gm.split(
            "void ALatebrakeRaceGameMode::ApplyCircuitLightingPreview()",1
        )[1].split("void ALatebrakeRaceGameMode::BuildCircuit()",1)[0]
        for marker in ("RuntimeSun->SetActorRotation(", "RuntimeSun->GetLightComponent()->SetIntensity(",
                       "RuntimeSun->GetLightComponent()->SetLightColor(",
                       "RuntimeSky->GetLightComponent()->SetIntensity(",
                       "RuntimeSky->GetLightComponent()->SetLightColor(",
                       "CanChangeCircuitLighting(","bDuskLightingPreview = !bDuskLightingPreview;"):
            self.assertIn(marker,section)
        for invalid in ("SaveRecords(","SetMassOverrideInKg(","AddForce(",
                        "SpawnActor<", "EarnCredits("):
            self.assertNotIn(invalid,section)
    def test_settings_only_and_not_persisted(self):
        self.assertIn("Mode->GetMenuPage()!=latebrake::MenuPage::Settings",self.pawn)
        self.assertIn("Mode->ToggleCircuitLightingPreview();",self.pawn)
        self.assertIn("DuskView.bExecuteWhenPaused = true;",self.pawn)
        self.assertIn('ActionName="CircuitLightingPreview",Key=F3',self.ini)
        self.assertIn("F3: TOGGLE DAY/DUSK",self.hud)
        self.assertIn("IsDuskLightingPreview()",self.h)
        self.assertIn("bDuskLightingPreview = false;",self.h)
        load=self.gm.split("void ALatebrakeRaceGameMode::SaveRecords()",1)[1]
        self.assertNotIn("bDuskLightingPreview",load)
        self.assertIn("NAME CircuitLightingPreview",self.cmake)
        self.assertIn("250000",self.test)
if __name__=="__main__":
    unittest.main()
