"""Whole-look preset integration contracts, not native rendering proof."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class GarageLookPresetsBindingTests(unittest.TestCase):
    def test_existing_storage_and_atomic_look(self):
        appearance = read("Source/Latebrake/Public/Garage/GarageAppearance.h")
        direct = read("Source/Latebrake/Public/Garage/GarageDirectAppearance.h")
        for marker in ("GaragePresetOptions = 6", "static_assert(ValidGarageLookPresets())",
                       "look = GarageLookPresets[static_cast<std::size_t>(choice)].Look",
                       "static constexpr int SaveVersion = 1",
                       "GarageAppearanceState::SerializedBytes == 18u"):
            self.assertIn(marker, appearance)
        self.assertIn("next=GarageLookPresets[static_cast<std::size_t>(request.Choice)].Look", direct)
        self.assertIn("!SameGarageLook(current, request.ExpectedLook)", direct)

    def test_existing_authority_and_rollback_are_reused(self):
        mode = read("Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp")
        block = mode.split("void ALatebrakeRaceGameMode::GarageCycleAppearance(", 1)[1]
        block = block.split("void ALatebrakeRaceGameMode::GarageChooseAppearance(", 1)[0]
        for marker in ("RejectGarageMutationIfBlocked()", "GaragePurchaseConfirm.Reset()",
                       "CanEditGarageDuringRace", "bFutureSaveSchema",
                       "CanStyleGalleryVehicle", "PreviewGarageDirectAppearance",
                       "GarageLooks.Choose(Viewed, layer, Direct->Choice)",
                       "GarageLooks = Before", "SyncGarageShowroomPreview()",
                       "GarageLookChoice(Look,layer)"):
            self.assertIn(marker, block)
        self.assertEqual(block.count("SaveRecords();"), 1)
        self.assertNotIn("Garage.Unlock(", block)
        self.assertNotIn("Garage.Select(", block)

    def test_palette_cycle_and_full_look_preview(self):
        nav = read("Source/Latebrake/Public/UI/GaragePaletteNavigation.h")
        glyph = read("Source/Latebrake/Public/UI/GarageAppearanceGlyph.h")
        hud = read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        self.assertIn("case GarageLookLayer::Wheels: return GarageLookLayer::Preset;", nav)
        self.assertIn("case GarageLookLayer::Preset: return GarageLookLayer::Paint;", nav)
        self.assertIn("GarageLookLayer::Stripe || layer==GarageLookLayer::Preset", glyph)
        self.assertIn("CHOOSE FULL LOOK", hud)
        self.assertIn("CYCLE CHOOSER TO FULL LOOK PRESETS", hud)
        self.assertIn("PresentedGaragePalette.ChoiceVisible", hud)
        self.assertIn("Mode->GarageChooseAppearance({Session.VehicleId,Session.Look,Session.Layer,Choice})", hud)
        self.assertIn("add_latebrake_portable_research_test(GarageLookPresets)",
                      read("Tests/Standalone/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
