"""Policy checks for safe original Editor-only PBR material authoring."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]
WRAPPER=(ROOT/"scripts/New-PreviewMaterials.ps1").read_text(encoding="utf-8")
GUIDE=(ROOT/"docs/UNREAL_PREVIEW_MATERIALS.md").read_text(encoding="utf-8")

class PreviewMaterialWorkflowTests(unittest.TestCase):
    def test_real_editor_and_fresh_receipt_are_required(self):
        for needle in (
            "UnrealEditor.exe", "Engine/Build/Build.version",
            "MajorVersion -ne 5", "MinorVersion -ne 8",
            "EngineAssociation -cne '5.8'",
            "CreatePreviewMaterials.py", "Remove-Item -LiteralPath $receiptPath",
            "if ($LASTEXITCODE -ne 0)",
            "if (-not (Test-Path -LiteralPath $receiptPath -PathType Leaf))",
        ):
            self.assertIn(needle,WRAPPER)
    def test_all_six_packages_are_verified_against_editor_bytes(self):
        for name in ("Road","Steel","CarPaint","Glass","Rubber","Emission"):
            self.assertIn("M_Latebrake"+name+"Pbr",WRAPPER)
        for needle in (
            "Get-FileHash -LiteralPath $file -Algorithm SHA256",
            "Length -ne [long]$item.bytes",
            "item.asset -cne $asset",
            "all_newly_compiled",
            "installed_cooked_game_validated -ne $false",
            "existing_unmodified_requires_visual_review",
        ):
            self.assertIn(needle,WRAPPER)
    def test_no_false_qa_promise_in_manual_instructions(self):
        self.assertIn("New-PreviewMaterials.ps1",GUIDE)
        self.assertIn("six actual Material",GUIDE)
        self.assertIn("No source test",GUIDE)
        self.assertIn("Unreal Editor 5.8",GUIDE)

if __name__=="__main__":
    unittest.main()
