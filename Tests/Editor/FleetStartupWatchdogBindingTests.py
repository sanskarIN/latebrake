"""Source contracts for experimental startup admission."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
class FleetStartupWatchdogBindingTests(unittest.TestCase):
    def test_watchdog_is_before_real_countdown(self):
        mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        tick=mode.split("void ALatebrakeRaceGameMode::Tick(",1)[1].split("\n}\n",1)[0]
        self.assertLess(tick.index("ObserveExperimentalFleet("),tick.index("StartLights.Advance("))
        self.assertIn("IsRestartPlacementReady() && bFleetReady",tick)
        fleet=read("Source/Latebrake/Private/Game/LatebrakeFleetRuntime.cpp")
        for marker in ("if (!IsExperimentalFleetRequested()) return true;",
                       "if (!StartLights.IsGreen())",
                       "FleetStartup.Observe(","if (Startup.ResetCountdown) StartLights.Reset()",
                       "FleetAttempt.ReportGridTimeout(FleetAttemptToken)",
                       "FleetStartup.IsCurrentPermission("):
            self.assertIn(marker,fleet)
        self.assertIn("IsFleetStartPresentationReady()",mode)
    def test_hud_does_not_advertise_go_during_fault(self):
        hud=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        a=hud.index("!Mode->IsFleetStartPresentationReady()")
        b=hud.index("switch (Mode->GetRaceStartPhase())",a)
        self.assertLess(a,b)
        for marker in ("FleetAttemptFaultName(Fleet.Fault)","FleetGridFailureName(Startup.Grid.Failure)",
                       "FLEET DRIVE SUSPENDED","GRID: %s | %.0fs left"):
            self.assertIn(marker,hud[a:b])
        self.assertIn("add_latebrake_portable_research_test(FleetStartupWatchdog)",
                      read("Tests/Standalone/CMakeLists.txt"))
if __name__=="__main__":unittest.main()
