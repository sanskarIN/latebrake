"""Static Unreal wiring contract, not mobile rendering or device QA."""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class AndroidGhostTouchBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        def get(path):
            return (ROOT / path).read_text(encoding="utf-8")

        cls.layout = get("Source/Latebrake/Public/UI/TouchControlLayout.h")
        cls.policy = get("Source/Latebrake/Public/UI/GhostTouchPolicy.h")
        cls.pawn = get("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.hud = get("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.mode = get("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.cmake = get("Tests/Standalone/CMakeLists.txt")

    def test_geometry_shared_by_gameplay_and_hud(self):
        self.assertIn("TouchRect GhostView;", self.layout)
        self.assertIn("layout.GhostView.Contains(x, y)", self.layout)
        self.assertIn("TouchControl::GhostView", self.layout)
        self.assertIn("DrawPad(Pads.GhostView,", self.hud)
        self.assertIn('TEXT("GHOST ON") : TEXT("GHOST OFF")', self.hud)
        self.assertIn("Mode->IsBestLapGhostDisplayEnabled()", self.hud)
        self.assertIn(
            "latebrake::CalculateTouchControlLayout(Canvas->SizeX, Canvas->SizeY)",
            self.hud,
        )
        self.assertIn("latebrake::CalculateTouchControlLayout(Width, Height)",
                      self.pawn)

    def test_touch_action_fresh_and_fallible(self):
        self.assertIn('#include "UI/GhostTouchPolicy.h"', self.pawn)
        self.assertIn("case latebrake::TouchControl::GhostView:", self.pawn)
        self.assertIn("CanToggleGhostFromTouch({", self.pawn)
        self.assertIn("Mode && Mode->IsRaceStartGreen()", self.pawn)
        self.assertIn("Mode && Mode->HasLocalPlayerDriver()", self.pawn)
        self.assertIn("Mode && Mode->IsFocusLossGuardActive()", self.pawn)
        self.assertIn("ToggleBestLapGhost();", self.pawn)
        self.assertIn("TouchPressAdmission::NewPress", self.pawn)
        self.assertLess(self.pawn.index("if (!bAllowAcquire)"),
                        self.pawn.index("case latebrake::TouchControl::GhostView:"))
        self.assertIn("request.FreshPress", self.policy)
        self.assertIn("!request.MenuOpen", self.policy)
        self.assertIn("!request.FocusSuspended", self.policy)
        self.assertIn("!request.RaceFinished", self.policy)

    def test_never_modifies_lap_or_physics_authority(self):
        ghost_action = self.pawn.split(
            "case latebrake::TouchControl::GhostView:", 1
        )[1].split("case latebrake::TouchControl::MenuBrowse:", 1)[0]
        for unsafe in ("Race.Advance", "SaveRecords(", "RecoverTo(",
                       "ClearDriverInputs(", "SetPhysicsLinearVelocity",
                       "GhostRecorder.ResetRun", "AddForce("):
            self.assertNotIn(unsafe, ghost_action)
        self.assertIn("GhostDisplay.Toggle(bHumanInputAllowed)", self.mode)
        self.assertNotIn("GhostRecorder", self.policy)
        self.assertNotIn("AddForce(", self.policy)
        self.assertIn("NAME GhostTouchPolicy", self.cmake)

    def test_not_exposed_as_a_paused_menu_action(self):
        menu = self.pawn.split("if (Mode && Mode->IsMenuOpen())", 1)[1].split(
            "if (UGameplayStatics::IsGamePaused(this)", 1)[0]
        self.assertNotIn("TouchControl::GhostView", menu)
        self.assertNotIn("ToggleBestLapGhost", menu)


if __name__ == "__main__":
    unittest.main()
