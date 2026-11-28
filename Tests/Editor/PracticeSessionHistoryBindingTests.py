"""Session comparisons bind real practice starts, configurations and physical finishes."""
from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[2]


class PracticeSessionHistoryBindingTests(unittest.TestCase):
    def test_start_and_finish_authority(self):
        mode = (ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp").read_text()
        restart = mode.split("void ALatebrakeRaceGameMode::RestartRace()", 1)[1].split(
            "void ALatebrakeRaceGameMode::RecoverPlayer(", 1)[0]
        self.assertLess(restart.index("PracticeHistory.CancelAttempt();"),
                        restart.index("PracticeHistory.Begin("))
        self.assertLess(restart.index("RestartPlacement.OnRestartPlacementResult(bPlacedOnGrid)"),
                        restart.index("if (bPlacedOnGrid && IsPracticeRace())"))
        self.assertIn("PracticeHistory.Begin(ReadPracticeConfiguration(Car)", restart)
        finish = mode.split("const latebrake::FinishTimingEligibility FinishTiming", 1)[1]
        self.assertLess(finish.index("FinishTiming != latebrake::FinishTimingEligibility::Eligible"),
                        finish.index("PracticeHistory.Finish("))
        block = finish.split("if (IsPracticeRace())", 1)[1].split("// Read the live rival", 1)[0]
        self.assertIn("PracticeHistory.Finish(ReadPracticeConfiguration(PracticeDriver)", block)
        self.assertIn("return;", block)
        self.assertNotIn("SaveRecords(", block)

    def test_configuration_changes_are_not_laundered(self):
        mode = (ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp").read_text()
        self.assertIn("PracticeHistory.ObserveConfiguration(ReadPracticeConfiguration(GhostDriver)", mode)
        self.assertIn("LastTunedPawn.Get()!=Car", mode)
        self.assertIn("Key==LastAppliedTuning", mode)
        self.assertIn("Car->GetSelectedVehicleId()!=Garage.SelectedVehicle()", mode)
        self.assertIn("PracticeHistory.InvalidateConfiguration();", mode)
        pawn = (ROOT / "Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp").read_text()
        for method in ("SelectArcade", "SelectSimulation"):
            section = pawn.split(f"void ALatebrakeCarPawn::{method}()", 1)[1].split(
                "\nvoid ALatebrakeCarPawn::", 1)[0]
            self.assertIn("Mode->NotifyPracticeHandlingChange(this)", section)
        hud = (ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp").read_text()
        self.assertIn("SESSION COMPARISON UNAVAILABLE: CAR/TUNING/HANDLING CHANGED", hud)
        self.assertIn("SESSION ONLY | %u COMPARABLE FINISHES", hud)
        self.assertIn("Mode->GetPracticeComparison()", hud)
        cmake = (ROOT / "Tests/Standalone/CMakeLists.txt").read_text()
        self.assertIn("target_sources(latebrake_research_PracticeSessionHistory PRIVATE", cmake)


if __name__ == "__main__":
    unittest.main()
