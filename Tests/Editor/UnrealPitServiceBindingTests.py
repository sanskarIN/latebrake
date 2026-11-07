"""Source-level Unreal pit integration contracts, not a UE/Chaos playtest."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class UnrealPitServiceBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.ini = read("Config/DefaultInput.ini")
        cls.policy = read("Source/Latebrake/Public/Racing/RacePitService.h")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")

    def test_pit_controls_and_race_reset_are_bound_to_player_pawn(self):
        self.assertIn('#include "Racing/RacePitService.h"', self.header)
        self.assertIn("latebrake::RacePitService PitService;", self.header)
        self.assertIn("PitService.Reset(); bPitServiceHeld = false;", self.header)
        self.assertIn('TEXT("PitService"), IE_Pressed', self.pawn)
        self.assertIn('TEXT("PitService"), IE_Released', self.pawn)
        self.assertIn("PitRelease.bExecuteWhenPaused = true;", self.pawn)
        self.assertIn('ActionName="PitService",Key=P', self.ini)
        self.assertIn(
            'ActionName="PitService",Key=Gamepad_FaceButton_Top', self.ini)

    def test_service_only_after_actual_race_and_physics_readiness(self):
        tick = self.pawn.split(
            "void ALatebrakeCarPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "\nvoid ALatebrakeCarPawn::", 1)[0]
        self.assertLess(tick.index("!Mode->IsRaceStartGreen()"),
                        tick.index("PitService.Observe({"))
        self.assertLess(tick.index("!latebrake::CanApplyPlayerDrive({"),
                        tick.index("PitService.Observe({"))
        self.assertLess(tick.index("if (!IsRacePhysicsReady())"),
                        tick.index("PitService.Observe({"))
        self.assertIn("Mode->HasLocalPlayerDriver()", tick)
        self.assertIn("IsLocallyControlled()", tick)
        self.assertIn("PitService.Observe({", tick)
        self.assertIn("static_cast<double>(SurfacePosition.X)", tick)
        self.assertIn("static_cast<double>(Speed)", tick)
        self.assertIn("NitroBoost.IsHeld()", tick)
        self.assertIn("CollisionDamage.RepairAtPit()", tick)
        self.assertIn("TireCondition.ReplaceAtPit()", tick)
        self.assertIn("NitroBoost.RefillAtPit()", tick)
        self.assertNotIn("SaveRecords(", tick)

    def test_controller_menu_confirm_cannot_cancel_closed_menu_pit_hold(self):
        confirm = self.pawn.split(
            "void ALatebrakeCarPawn::MenuActivate()", 1)[1].split(
            "void ALatebrakeCarPawn::MenuMousePressed()", 1)[0]
        self.assertIn("!Mode || !Mode->IsMenuOpen()", confirm)
        self.assertLess(confirm.index("!Mode || !Mode->IsMenuOpen()"),
                        confirm.index("ClearDriverInputs();"))
        self.assertLess(confirm.index("ClearDriverInputs();"),
                        confirm.index("Mode->MenuActivate();"))
        clear = self.pawn.split(
            "void ALatebrakeCarPawn::ClearDriverInputs(", 1)[1].split(
            "void ALatebrakeCarPawn::", 1)[0]
        self.assertIn("bPitServiceHeld = false;", clear)
        self.assertIn("PitService.Suspend();", clear)

    def test_hud_is_read_only_and_does_not_steal_touch_space(self):
        self.assertIn("Car->GetPitService()", self.hud)
        self.assertIn("Pit.ProgressFraction()", self.hud)
        self.assertIn("PIT SERVICED", self.hud)
        self.assertIn("!bCompact && VerticalPlan.MaxTelemetryLines >= 28", self.hud)
        self.assertIn("RacePitServiceTests.cpp", self.cmake)
        self.assertIn("NAME RacePitService", self.cmake)

    def test_visual_beacon_does_not_modify_physical_circuit_authority(self):
        marker = self.mode.split(
            "// Visual-only pit beacon", 1)[1].split(
            "int32 SpawnedCheckpoints", 1)[0]
        self.assertIn("latebrake::RacePitBay.X", marker)
        self.assertIn("latebrake::RacePitBay.Y + 265.0", marker)
        self.assertIn("ECollisionEnabled::NoCollision", marker)
        self.assertIn("SetGenerateOverlapEvents(false)", marker)
        self.assertNotIn("Audit.RecordCollisionBlock(", marker)
        self.assertNotIn("SpawnBlock(", marker)
        self.assertIn("LATEBRAKE_GEOMETRY_READY", self.mode)

    def test_pit_remains_inside_existing_circuit_not_rival_centerline(self):
        self.assertIn("RacePitBay{0.0, -1300.0}", self.policy)
        self.assertIn("IsOnPavedSurface({x, y})", self.policy)
        self.assertIn("sample.PlanarSpeedCmPerSecond > 120.0", self.policy)
        self.assertIn("RacePitServiceHoldSeconds = 2.5", self.policy)
        self.assertIn("Armed = false;", self.policy)
        self.assertIn("if (!InBay)", self.policy)


if __name__ == "__main__":
    unittest.main()
