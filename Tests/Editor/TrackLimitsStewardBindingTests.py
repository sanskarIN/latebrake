"""Source-level contracts for real GameMode/vehicle/HUD track limits.

These are string-level wiring checks; they do not run genuine Unreal Engine.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]

def read(path):
    return (ROOT / path).read_text(encoding="utf-8")

class TrackLimitsStewardBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.policy = read("Source/Latebrake/Public/Track/TrackLimitsSteward.h")
        cls.footprint = read("Source/Latebrake/Public/Track/TrackLimitsFootprint.h")
        cls.wheel_layout = read("Source/Latebrake/Public/Vehicles/VehicleWheelLayout.h")
        cls.pawn = read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.mode_h = read("Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h")
        cls.mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        cls.hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.cmake = read("Tests/Standalone/CMakeLists.txt")
        cls.test_source = read("Tests/Standalone/TrackLimitsStewardTests.cpp")

    def test_real_game_mode_owns_race_clock_and_protects_session(self):
        self.assertIn('#include "Track/TrackLimitsSteward.h"', self.mode_h)
        self.assertIn("latebrake::TrackLimitsSteward TrackLimits;", self.mode_h)
        self.assertIn("TrackLimits.Update(", self.mode)
        self.assertIn("StewardTrackedPawn.Get() == StewardsDriver", self.mode)
        self.assertIn("StewardTeleportRevision == StewardsDriver->GetTeleportRevision()", self.mode)
        self.assertIn("StewardCheckpointSessionRevision == CheckpointSession.Current()", self.mode)
        self.assertIn("bStewardReady && bSameStewardSession", self.mode)
        self.assertIn("StewardsDriver->GetSpeedKmH()", self.mode)
        self.assertIn("StewardsDriver->GetActorRotation().Yaw", self.mode)
        self.assertIn("ChassisHeadingDegrees, ChassisSpeedKmH, LiveDelta, true", self.mode)
        self.assertIn("Race.AddTimePenalty(seconds)", self.mode)
        self.assertIn("StartLights.IsGreen()", self.mode)
        self.assertIn("HasLocalPlayerDriver()", self.mode)
        for required in ("Race.Reset();", "TrackLimits.Reset();",
                         "StewardTrackedPawn.Reset();",
                         "StewardTeleportRevision = 0;",
                         "StewardCheckpointSessionRevision = 0;"):
            self.assertGreaterEqual(self.mode.count(required), 2)

    def test_actual_visual_offsets_shared_with_referee_geometry(self):
        self.assertIn('#include "Vehicles/VehicleWheelLayout.h"', self.pawn)
        self.assertIn("VehicleVisualWheelLocalXY[0].X", self.pawn)
        self.assertIn("VehicleVisualWheelLocalXY[3].Y", self.pawn)
        self.assertIn("VehicleVisualWheelLocalZCm", self.pawn)
        self.assertIn("{122.0, -94.0}, {122.0, 94.0}", self.wheel_layout)
        self.assertIn("{-122.0, -94.0}, {-122.0, 94.0}", self.wheel_layout)
        self.assertIn("VehicleVisualWheelRadiusCm = 29.0", self.wheel_layout)
        self.assertIn("for (const CircuitPoint local : VehicleVisualWheelLocalXY)", self.footprint)
        self.assertIn("IsNearStarterRoadTireRadius(projected)", self.footprint)
        self.assertIn("ObserveStarterWheelFootprint(position, headingDegrees)", self.policy)
        self.assertIn("footprint.FullyOffPavement", self.policy)
        self.assertIn("ProjectedWheelsNearPavement", self.policy)
        self.assertNotIn("AddTimePenalty(", self.footprint)
        self.assertNotIn("SetActorLocation(", self.footprint)

    def test_hud_read_only_and_cross_platform_test_registered(self):
        self.assertIn("Mode->GetTrackLimits().Snapshot()", self.hud)
        self.assertIn("TRACK LIMIT WARNING", self.hud)
        self.assertIn("TRACK LIMITS  %d", self.hud)
        self.assertNotIn("AddTimePenalty(", self.hud)
        self.assertIn("NAME TrackLimitsSteward", self.cmake)
        self.assertIn("TrackLimitsStewardTests.cpp", self.cmake)
        self.assertIn("TestWheelProjectionAndTrackLimitsFairness", self.test_source)
        self.assertIn("TestActualRaceProgressClock", self.test_source)

    def test_suspension_does_not_rewrite_historical_strikes(self):
        self.assertGreaterEqual(self.mode.count("TrackLimits.Suspend();"), 4)
        self.assertIn("TrackLimits.Suspend();\n    SuspendBestLapGhost();", self.mode)
        self.assertIn("TrackLimitsRules::PenaltySeconds", self.policy)
        self.assertIn("if (!std::forward<ChargePenalty>(chargePenalty)", self.policy)
        self.assertIn("// A saved PB may change at finish", self.mode)

if __name__ == "__main__":
    unittest.main()
