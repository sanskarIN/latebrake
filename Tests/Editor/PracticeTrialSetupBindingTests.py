"""Source-level regression for temporary practice-only physical tuning."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
def read(path):
    return (ROOT / path).read_text(encoding="utf-8")

class PracticeTrialSetupBindingTests(unittest.TestCase):
    def test_launch_latches_before_physics_and_exits_restore_saved(self):
        mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        launch = mode.split("bool ALatebrakeRaceGameMode::StartQuickPractice(",1)[1].split(
            "latebrake::PracticeConfiguration ALatebrakeRaceGameMode::ReadPracticeConfiguration",1)[0]
        self.assertLess(launch.index("PracticeTrial.BeginPractice(Access)"),launch.index("ActivePractice = Preset"))
        self.assertLess(launch.index("ActivePractice = Preset"),launch.index("RestartRace();"))
        self.assertNotIn("SaveRecords(",launch)
        for first,last in (("void ALatebrakeRaceGameMode::SelectRaceEvent(","bool ALatebrakeRaceGameMode::StartQuickPractice("),
                           ("bool ALatebrakeRaceGameMode::StartCareerEvent(","void ALatebrakeRaceGameMode::")):
            segment=mode.split(first,1)[1].split(last,1)[0]
            self.assertLess(segment.index("PracticeTrial.EndPractice();"),segment.index("RestartRace();"))
        restart=mode.split("void ALatebrakeRaceGameMode::RestartRace()",1)[1].split(
            "void ALatebrakeRaceGameMode::RecoverPlayer(",1)[0]
        self.assertNotIn("PracticeTrial.BeginPractice(",restart)
        self.assertIn("ApplyGarageToPlayer();",restart)

    def test_tuning_and_history_share_effective_vehicle(self):
        mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        apply=mode.split("void ALatebrakeRaceGameMode::ApplyGarageToPlayer()",1)[1].split(
            "void ALatebrakeRaceGameMode::DismissGaragePalette()",1)[0]
        for token in ("GetActivePracticeTrial()", "Requested = Trial.Key",
                      "Performance = Trial.Performance","Requested.Selected, Look",
                      "PracticeTrialLook(Trial,GarageLooks)", "Car->SetGarageAppearance(Look)"):
            self.assertIn(token,apply)
        self.assertNotIn("Garage.Select(",apply)
        self.assertNotIn("Garage.Unlock(",apply)
        history=mode.split("latebrake::PracticeConfiguration ALatebrakeRaceGameMode::ReadPracticeConfiguration(",1)[1].split(
            "void ALatebrakeRaceGameMode::NotifyPracticeHandlingChange(",1)[0]
        self.assertIn("GetActivePracticeTrial().Key",history)
        self.assertIn("Car->GetSelectedVehicleId()!=Key.Selected",history)
        self.assertIn("Key==LastAppliedTuning",history)

    def test_trial_shortcuts_are_consumed_without_purchase(self):
        mode=read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        block=mode.split("bool ALatebrakeRaceGameMode::HandlePracticeTrialInput(",1)[1].split(
            "void ALatebrakeRaceGameMode::MenuNext()",1)[0]
        self.assertIn("Menu.Current()!=latebrake::MenuPage::QuickRace",block)
        self.assertIn("PracticeTrial.Edit(GetPracticeTrialAccess()",block)
        for forbidden in ("Garage.Select(","Garage.Unlock(","Garage.Upgrade(","SaveRecords(","ApplyGarageToPlayer("):
            self.assertNotIn(forbidden,block)
        pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        for action,call in (("NextCar","GarageNextVehicle"),("Toggle","GarageUnlockNext"),
                            ("NextEngine","GarageUpgradeEngine"),("NextGrip","GarageUpgradeGrip")):
            self.assertIn(f"if (Mode->HandlePracticeTrialInput(latebrake::PracticeTrialAction::{action}))return;\n"
                          f"        Mode->{call}();",pawn)
        self.assertIn("PracticeTrialAction::ChooseCar,VehicleId",pawn)

    def test_disclosure_and_registration(self):
        hud=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.assertIn("FREE TRIAL SETUP | NOT SAVED",hud)
        self.assertIn("UNOWNED CAR | PRACTICE ONLY",hud)
        self.assertIn("Q RETURNS TO SAVED GARAGE",hud)
        self.assertIn("GetActivePracticeTrial()",hud)
        self.assertIn("add_latebrake_portable_research_test(PracticeTrialSetup)",
                      read("Tests/Standalone/CMakeLists.txt"))

if __name__=="__main__": unittest.main()
