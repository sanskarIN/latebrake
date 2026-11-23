"""Source-level binding of real 2D Garage two-action spending consent.

This complements native Win64 synthetic --self-test; no user funds,
network service, engine build or true Windows manual QA is implied.
"""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def src(path):
    return (ROOT/path).read_text(encoding="utf-8")

class PreviewGaragePurchaseBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app=src("Experiments/Win32Preview/LateBrakePreview.cpp")
        cls.policy=src("Experiments/Win32Preview/PreviewGaragePurchaseConsent.h")
        cls.tests=src("Tests/Standalone/PreviewGaragePurchaseConsentTests.cpp")
        cls.cmake=src("Tests/Standalone/CMakeLists.txt")
        cls.guide=src("Experiments/Win32Preview/TRIAL_QUICKSTART.md")

    def test_only_confirm_path_can_debit_virtual_credits(self):
        method=self.app.split("static void unlockSelectedCar()",1)[1].split(
            "static void raceWithViewedOwnedCar()",1)[0]
        for marker in (
            "garagePurchase.Request(",
            "PreviewPurchaseChoice::Offer",
            "PreviewPurchaseChoice::Rejected",
            "PreviewPurchaseChoice::Confirm",
            "UNLOCK OFFER: U / CLICK AGAIN WITHIN 8 SEC",
            "credits-=carPrices[garageViewCar];ownedMask|=1<<garageViewCar;",
            "if(saveProgress())",
            "credits=oldCredits;ownedMask=oldMask;",
        ):
            self.assertIn(marker,method)
        self.assertLess(method.index("PreviewPurchaseChoice::Confirm"),
                        method.index("credits-=carPrices[garageViewCar];"))
        self.assertNotIn("selectedCar=garageViewCar;",method)

    def test_input_focus_page_view_and_help_cancel(self):
        for marker in (
            "if(garageViewCar!=cursor.Viewed)garagePurchase.Reset();",
            "garagePurchase.Reset();\n    page=target;",
            "if(help){garagePurchase.Reset();showHelp=!showHelp;return;}",
            "garagePurchase.Advance(dt);",
            "garagePurchase.PendingFor(garageViewCar)",
            "selfTestStage=90;",
            "garagePurchase.Advance(8.01f);",
            "garagePurchase.Reset(); // Same focus-loss cancellation",
        ):
            self.assertIn(marker,self.app)
        self.assertIn("if(msg==0x0008)",self.app)
        self.assertIn("garagePurchase.Reset();",self.app)

    def test_policy_is_no_crt_and_has_deterministic_test(self):
        self.assertIn("OfferLifetimeSeconds=8.0f",self.policy)
        self.assertIn("return PreviewPurchaseChoice::Offer;",self.policy)
        self.assertIn("return PreviewPurchaseChoice::Confirm;",self.policy)
        self.assertIn("250000",self.tests)
        self.assertIn("NAME PreviewGaragePurchaseConsent",self.cmake)
        self.assertIn("eight seconds",self.guide)

if __name__=="__main__":
    unittest.main()
