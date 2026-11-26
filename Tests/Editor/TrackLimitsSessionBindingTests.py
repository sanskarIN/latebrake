"""Actual original-circuit track limits binding; not native Unreal QA."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class TrackLimitsSessionBindingTests(unittest.TestCase):
    def test_actual_clock_and_continuity_precede_steward(self):
        mode = (ROOT / "Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp").read_text()
        tick = mode.split("void ALatebrakeRaceGameMode::Tick(",1)[1].split(
            "void ALatebrakeRaceGameMode::SuspendBestLapGhost(",1)[0]
        markers = ("const bool bActualClockAdvanced=latebrake::AdvancePlayerRaceClockIfReady(",
                   "const bool bStewardDriverValid=", "const bool bStewardContinuous=",
                   "TrackLimits.Observe(Race,", "StewardEvent==latebrake::TrackLimitsEvent::PenaltyApplied",
                   "SuspendBestLapGhost();")
        offsets = [tick.index(marker) for marker in markers]
        self.assertEqual(offsets, sorted(offsets))
        self.assertIn("StewardTeleportRevision==GhostDriver->GetTeleportRevision()",tick)
        self.assertIn("StewardCheckpointSessionRevision==CheckpointSession.Current()",tick)
        self.assertIn("LiveDelta,bActualClockAdvanced,bStewardContinuous",tick)
        self.assertEqual(mode.count("TrackLimits.Reset();"),2)
        for name in ("CloseMenuForRace","ToggleMenu","ToggleRacePause","PauseForAppInterruption"):
            self.assertIn("void ALatebrakeRaceGameMode::"+name+"()\n{\n    TrackLimits.Suspend();",mode)

    def test_fee_authority_and_read_only_hud(self):
        policy = (ROOT / "Source/Latebrake/Public/Track/TrackLimitsSession.h").read_text()
        self.assertEqual(policy.count("race.AddTimePenalty(seconds)"),1)
        self.assertIn("charged=race.GetTotalPenaltySeconds()-beforeFees",policy)
        self.assertIn("actualCount-snapshot.Penalties!=recoveryCount",policy)
        hud = (ROOT / "Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp").read_text()
        self.assertIn("Mode->GetTrackLimitsReview()",hud)
        self.assertIn("TRACK LIMITS | +5s INCLUDED IN TIME",hud)
        self.assertIn("Track fees +%.2fs | recovery fees +%.2fs (included)",hud)
        self.assertNotIn("TrackLimits.Observe(",hud)


if __name__ == "__main__":
    unittest.main()
