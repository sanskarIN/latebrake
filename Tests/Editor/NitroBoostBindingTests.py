"""Source-level integration contracts, NOT an Unreal Engine build."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class NitroBoostBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy = read("Source/Latebrake/Public/Vehicles/ArcadeNitroBoost.h")
        cls.pawn_h = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.config = read("Config/DefaultInput.ini")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.manual = read("docs/NITRO_BOOST_QA.md")

    def test_player_only_press_has_real_race_and_physics_guards(self):
        press = self.pawn.split("void ALatebrakeCarPawn::NitroPressed()", 1)[1].split(
            "void ALatebrakeCarPawn::NitroReleased()", 1)[0]
        for guard in (
            "IsLocallyControlled()", "Cast<APlayerController>(GetController())",
            "Mode->IsRaceStartGreen()", "Mode->IsCircuitReadyForGameplay()",
            "Mode->IsRestartPlacementReady()", "!Mode->IsMenuOpen()",
            "!UGameplayStatics::IsGamePaused(this)",
            "!Mode->IsManualRacePauseActive()", "!Mode->IsFocusLossGuardActive()",
            "!Mode->GetRace().IsFinished()", "IsRacePhysicsReady()",
            "NitroBoost.Press(bEligible)",
        ):
            self.assertIn(guard, press)
        self.assertIn("NitroBoost.Release();", self.pawn.split(
            "void ALatebrakeCarPawn::NitroReleased()", 1)[1].split(
                "void ALatebrakeCarPawn::Tick(", 1)[0])

    def test_live_physics_and_speed_cap_are_bounded(self):
        tick = self.pawn.split("void ALatebrakeCarPawn::Tick(", 1)[1].split(
            "void ALatebrakeCarPawn::ApplySignalLights(", 1)[0]
        for source in (
            "!IsRacePhysicsReady()", "const FVector Velocity = Chassis->GetPhysicsLinearVelocity()",
            "const float LongitudinalSpeed = FVector::DotProduct(FlatVelocity, Forward)",
            "if (!FMath::IsFinite(Speed)",
            "NitroBoost.Step(", "bEffectiveBraking",
            "static_cast<double>(EffectiveThrottle)",
            "static_cast<double>(LongitudinalSpeed)",
            "NitroDrive.ForceMultiplier", "NitroDrive.SpeedCapMultiplier",
            "FMath::FInterpConstantTo(", "Chassis->AddForce(",
            "FlatVelocity.GetClampedToMaxSize(ActiveSpeedCap)",
        ):
            self.assertIn(source, tick)
        self.assertLess(tick.index("!IsRacePhysicsReady()"), tick.index("NitroBoost.Step("))
        self.assertLess(tick.index("NitroBoost.Step("), tick.index("Chassis->AddForce("))
        self.assertIn("NitroBoost.Suspend();", tick)
        ai = self.pawn.split("void ALatebrakeCarPawn::SetAutonomousControls(", 1)[1].split(
            "void ALatebrakeCarPawn::SetGarageTuning(", 1)[0]
        self.assertNotIn("NitroBoost.Press(", ai)

    def test_reset_interruption_and_paused_release(self):
        self.assertIn("#include \"Vehicles/ArcadeNitroBoost.h\"", self.pawn_h)
        self.assertIn("latebrake::ArcadeNitroBoost NitroBoost;", self.pawn_h)
        self.assertIn("GetNitroBoost() const", self.pawn_h)
        self.assertIn("NitroBoost.Reset();", self.pawn_h)
        self.assertIn("CurrentNitroSpeedCapScale = 1.f;", self.pawn_h)
        clear = self.pawn.split("void ALatebrakeCarPawn::ClearDriverInputs()", 1)[1].split(
            "void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()", 1)[0]
        self.assertIn("NitroBoost.Suspend();", clear)
        self.assertIn("CurrentNitroSpeedCapScale = 1.f;", clear)
        bind = self.pawn.split("void ALatebrakeCarPawn::SetupPlayerInputComponent(", 1)[1]
        self.assertIn('TEXT("Nitro"), IE_Pressed', bind)
        self.assertIn('TEXT("Nitro"), IE_Released', bind)
        self.assertIn("NitroRelease.bExecuteWhenPaused = true;", bind)
        self.assertIn('+ActionMappings=(ActionName="Nitro",Key=B)', self.config)
        self.assertIn('+ActionMappings=(ActionName="Nitro",Key=Gamepad_LeftThumbstick)',
                      self.config)

    def test_hud_policy_and_source_qa_are_honest(self):
        self.assertIn("Car->GetNitroBoost().ChargeFraction()", self.hud)
        self.assertIn("Car->GetNitroBoost().IsActive()", self.hud)
        self.assertIn("!bCompact", self.hud)
        self.assertIn("ArcadeNitroBoostTests.cpp", self.cmake)
        self.assertIn("NAME ArcadeNitroBoost", self.cmake)
        self.assertIn("cxx_std_20", self.cmake)
        self.assertIn("Unreal Engine 5.8", self.manual)
        self.assertIn("not a playable", self.manual.lower())
        for forbidden in (
            "AddForce(", "SaveGame", "AwardCredits(", "PassCheckpoint(",
            "RaceProgress", "UCLASS(", "UE_LOG(",
        ):
            self.assertNotIn(forbidden, self.policy)


if __name__ == "__main__":
    unittest.main()
