"""Source contract for the responsive, observational live radar."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class HudRaceRadarBindingTests(unittest.TestCase):
    def test_real_desktop_lane_and_asphalt(self):
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        for marker in ("CalculateHudRaceRadarLayout(", "RadarLayout.Panel.X-HorizontalBounds.Left",
                       "BuildHudRadarPresentation(", "GroupFleetRadar(FleetReview,0.075)",
                       "Presentation.Roads.Asphalt", "Presentation.Roads.Barriers",
                       "PlaceHudRadarLabels(RadarLayout,Presentation,Measures)",
                       "GetTextSize(Name,W,H,GEngine->GetSmallFont()", "Marker.Nose.Valid"):
            self.assertIn(marker, hud)
        policy = read("Source/Latebrake/Public/UI/HudRaceRadar.h")
        self.assertIn("CircuitCollisionBlocks", policy)
        self.assertIn("instruments.Panel", policy)
        self.assertIn("map.Y+(1.0-marker.V)*map.Height", policy)
        self.assertIn("HudRadarOverlaps(r,source.Markers", policy)

    def test_replay_heading_and_map_agree(self):
        replay = read("Source/Latebrake/Public/UI/FleetReplayPanel.h")
        self.assertIn("map.Y+(1.0-uv.V)*map.Height", replay)
        self.assertIn("car.Position.Y-length*std::sin", replay)
        self.assertIn("add_latebrake_portable_research_test(HudRaceRadar)",
                      read("Tests/Standalone/CMakeLists.txt"))

    def test_no_score_or_actor_authority(self):
        policy = read("Source/Latebrake/Public/UI/HudRaceRadar.h")
        for marker in ("SaveRecords(", "PassCheckpoint(", "AddTimePenalty(",
                       "SetAutonomousControls(", "SetActorLocation("):
            self.assertNotIn(marker, policy)


if __name__ == "__main__":
    unittest.main()
