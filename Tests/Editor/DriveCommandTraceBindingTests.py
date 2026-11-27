"""Read-only native command capture and rendering contracts, not engine QA."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]

def read(path):
    return (ROOT / path).read_text(encoding="utf-8")

class DriveCommandTraceBindingTests(unittest.TestCase):
    def test_capture_is_after_physics_and_local_only(self):
        source = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        tick = source.split("void ALatebrakeCarPawn::Tick(float DeltaSeconds)", 1)[1]
        observe = tick.index("DriveTrace.Observe({")
        for marker in ("DriveTrace.BeginFrame", "NeutralizeMotionWhenDrivingBlocked",
                       "Chassis->AddForce(Forward * DriveForce * EffectiveThrottle",
                       "Chassis->AddTorqueInRadians", "SubmitSkidTrailFrame(SkidFrame)"):
            self.assertLess(tick.index(marker), observe)
        block = tick[observe-500:observe+650]
        for marker in ("TracePC->IsLocalController()", "TracePC->GetPawn()==this",
                       "Mode->HasLocalPlayerDriver()", "Mode->GetCheckpointSessionRevision()",
                       "GetTeleportRevision()", "RawThrottle,EffectiveThrottle",
                       "AssistedSteering,bServiceBraking,BrakeProfile.HandbrakeActive",
                       "GroundContact.HasDriveContact,bObservedMotorBranch"):
            self.assertIn(marker, block)
        self.assertIn("void ALatebrakeCarPawn::ClearDriverInputs()\n{\n    DriveTrace.Suspend();", source)
        header = read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        self.assertIn("DrivingStats.Reset(); DriveTrace.Suspend();", header)

    def test_draw_uses_only_fresh_observation_and_detailed_mode(self):
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        begin = hud.index("const bool bDriveReviewEligible")
        end = hud.index("const bool bTimeAttack", begin)
        block = hud[begin:end]
        for marker in ("HudTelemetryMode::Detailed", "Mode->IsRestartPlacementReady()",
                       "Mode->HasLocalPlayerDriver()", "Mode->IsRaceStartGreen()",
                       "!Mode->IsFocusLossGuardActive()", "!Mode->GetRace().IsFinished()",
                       "!UGameplayStatics::IsGamePaused(this)",
                       "Car->GetDriveCommandReview", "DriveCommandReview.Valid"):
            self.assertIn(marker, block)
        self.assertIn("DrawDriveCommandPanel(DriveCommandPanel,DriveCommandReview);", hud)
        draw = read("Source/Latebrake/Private/UI/LatebrakeDriveCommandHUD.cpp")
        self.assertIn("BuildHudDriveCommandPlot(Layout,Review)", draw)
        for forbidden in ("AddForce", "SaveRecords(", "SetPhysics", "SetAutonomousControls"):
            self.assertNotIn(forbidden, draw)
        cmake = read("Tests/Standalone/CMakeLists.txt")
        for suite in ("DriveCommandTrace", "HudDriveCommandPanel"):
            self.assertIn(f"add_latebrake_portable_research_test({suite})", cmake)

if __name__ == "__main__":
    unittest.main()
