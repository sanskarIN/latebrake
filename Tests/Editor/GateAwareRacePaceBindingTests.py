"""Source binding only; no native UE or real driver evidence."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class GateAwareRacePaceBindingTests(unittest.TestCase):
    def test_projection_is_observation_only(self):
        source = (ROOT / "Source/Latebrake/Public/Racing/GateAwareRacePace.h").read_text(
            encoding="utf-8")
        for marker in ("ValidMeasuredLapTrace(reference)", "reference.FeeFree",
                       "ValidPrefix(prefix,next,completed+1,gates)",
                       "race.GetCurrentLapPenaltySeconds()",
                       "GatePaceStatus::CurrentSectorOverdue",
                       "AddWithinBound(remaining,predicted)"):
            self.assertIn(marker, source)
        for forbidden in ("PassCheckpoint(", "AddTimePenalty(", "SaveRecords(",
                          "SetActorLocation(", "Advance(", "CareerAward("):
            self.assertNotIn(forbidden, source)

    def test_hud_never_falls_back_after_reference_evidence_fails(self):
        hud = (ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp").read_text(
            encoding="utf-8")
        start = hud.index("ProjectGateAwareRacePace(Race)")
        end = hud.index("Race.GetMeasuredLapSplits().LatestGate()", start)
        section = hud[start:end]
        for marker in ("GatePaceStatus::Estimated", "GatePaceStatus::CurrentSectorOverdue",
                       "FINISH EST. UNAVAILABLE",
                       "!Race.GetMeasuredLapSplits().BestReference().Complete",
                       "ProjectLiveRacePace(Race)"):
            self.assertIn(marker, section)
        self.assertLess(section.index("GatePaceStatus::CurrentSectorOverdue"),
                        section.index("ProjectLiveRacePace(Race)"))
        self.assertLess(section.index("!Race.GetMeasuredLapSplits().BestReference().Complete"),
                        section.index("ProjectLiveRacePace(Race)"))


if __name__ == "__main__":
    unittest.main()
