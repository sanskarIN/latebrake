"""v0.5.62 radar mini-map Unreal-to-production C++ source contracts.

Source-level only: no UE/UHT compile, renderer, shipping game or installed
Win64/Android gameplay session is exercised here.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
MODE_H = ROOT / "Source/Latebrake/Public/Game/LatebrakeRaceGameMode.h"
MODE = ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp"
HUD = ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp"
HEADER = ROOT / "Source/Latebrake/Public/Track/RaceMiniMap.h"
RADAR = ROOT / "Source/Latebrake/Private/Track/RaceMiniMap.cpp"
TEST = ROOT / "Tests/Standalone/RaceMiniMapTests.cpp"
CMAKE = ROOT / "Tests/Standalone/CMakeLists.txt"


class RaceMiniMapBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mode_h = MODE_H.read_text(encoding="utf-8")
        cls.mode = MODE.read_text(encoding="utf-8")
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.header = HEADER.read_text(encoding="utf-8")
        cls.radar = RADAR.read_text(encoding="utf-8")
        cls.cpp_test = TEST.read_text(encoding="utf-8")
        cls.cmake = CMAKE.read_text(encoding="utf-8")

    def test_live_rival_world_position_verified_at_game_mode_boundary(self):
        self.assertIn("bool TryGetRivalRadarPosition(FVector& OutPosition) const;",
                      self.mode_h)
        method = self.mode.split(
            "bool ALatebrakeRaceGameMode::TryGetRivalRadarPosition(", 1
        )[1].split("int32 ALatebrakeRaceGameMode::GetRivalApproximateLaps()", 1)[0]
        self.assertIn("!IsValid(RivalCar)", method)
        self.assertIn("!RivalCar->IsRacePhysicsReady()", method)
        self.assertIn("RivalCar->GetActorLocation()", method)
        for axis in "XYZ":
            self.assertIn("FMath::IsFinite(Position." + axis + ")", method)
        self.assertLess(method.index("!IsValid(RivalCar)"),
                        method.index("RivalCar->GetActorLocation()"))
        self.assertLess(method.index("FMath::IsFinite(Position.Z)"),
                        method.index("OutPosition = Position;"))
        self.assertIn("return false;", method)
        self.assertIn("return true;", method)

    def test_hud_displays_real_positions_without_inventing_lap_ranking(self):
        for marker in (
            '#include "Track/RaceMiniMap.h"',
            "Mode->TryGetRivalRadarPosition(RivalWorldPosition)",
            "const bool bRivalRace = !bTimeAttack && Mode->HasRival();",
            "latebrake::BuildRaceMiniMapSnapshot(",
            "Car->GetActorRotation().Yaw",
            "Race.GetNextCheckpoint()",
            "Radar.Rival.Valid",
            "Radar.Player.Position.Valid",
            "Radar.DirectRivalSeparationValid",
            "RIVAL %.0fm DIRECT",
            "RACE RADAR / NORTH UP",
            "Radar.Player.DirectionX",
            "Radar.Player.DirectionY"
        ):
            self.assertIn(marker, self.hud)
        self.assertLess(self.hud.index("TryGetRivalRadarPosition("),
                        self.hud.index("BuildRaceMiniMapSnapshot("))
        self.assertLess(self.hud.index("BuildRaceMiniMapSnapshot("),
                        self.hud.index("if (Radar.Rival.Valid)"))
        self.assertNotIn("RankPlayer", self.radar)
        self.assertNotIn("Race.PassCheckpoint(", self.radar)
        self.assertNotIn("SaveRecords(", self.radar)

    def test_portable_policy_fails_closed_and_uses_original_circuit(self):
        for marker in (
            "IsCircuitGate(nextGate)",
            "CircuitGate(nextGate)",
            "NormalizeCircuitMap(position)",
            "std::isfinite(position.X)",
            "std::isfinite(playerHeadingDegrees)",
            "playerReady",
            "rivalReady",
            "rivalEnabled",
            "std::remainder(playerHeadingDegrees, 360.0)",
            "std::hypot(",
        ):
            self.assertIn(marker, self.radar)
        self.assertIn("DirectRivalSeparationMeters", self.header)
        self.assertIn("Rival", self.header)
        self.assertIn("Player", self.header)

    def test_tests_compile_the_production_implementation(self):
        for marker in (
            "RaceMiniMapTests.cpp",
            "Private/Track/RaceMiniMap.cpp",
            "NAME RaceMiniMap",
            "target_compile_features(latebrake_race_minimap_tests",
            "-Wpedantic -Werror",
        ):
            self.assertIn(marker, self.cmake)
        for marker in (
            "noRival", "invalidRival", "blocked",
            "badPlayer", "badYaw", "badRival",
            "overlapping", "DirectRivalSeparationMeters",
        ):
            self.assertIn(marker, self.cpp_test)

    def test_moving_to_legacy_clamped_rival_marker_is_detectable(self):
        mutant = self.radar.replace(
            "result.Rival = MakeMarker(rivalPosition);",
            "result.Rival = {true, 0.5, 0.5}; /* fake rival */"
        )
        self.assertNotIn("result.Rival = MakeMarker(rivalPosition);", mutant)
        self.assertIn("result.Rival = MakeMarker(rivalPosition);", self.radar)


if __name__ == "__main__":
    unittest.main()
