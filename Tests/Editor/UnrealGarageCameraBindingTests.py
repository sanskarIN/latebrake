"""Actual draft native camera binding; source contracts, not UE rendering."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
def read(path):
    return (ROOT/path).read_text(encoding="utf-8")

class UnrealGarageCameraBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pawn=read("Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp")
        cls.header=read("Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h")
        cls.hud=read("Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp")
        cls.input=read("Config/DefaultInput.ini")
        cls.policy=read("Source/Latebrake/Public/Vehicles/OriginalVehicleShowroomCamera.h")
        cls.tests=read("Tests/Standalone/GarageShowroomPresentationTests.cpp")
        cls.cmake=read("Tests/Standalone/CMakeLists.txt")

    def test_cached_geometry_is_tied_to_committed_mesh(self):
        rebuild=self.pawn.split("void ALatebrakeCarPawn::RebuildOriginalVehicleVisual(",1)[1].split(
            "int32 ALatebrakeCarPawn::GetCommittedOriginalWheelParts()",1)[0]
        self.assertLess(rebuild.index("GarageCameraGeometry.Reset();"),
                        rebuild.index("OriginalVehicleSurfaceComposer{}.Build("))
        self.assertLess(rebuild.index("bOriginalVehicleVisualReady = true;"),
                        rebuild.index("GarageCameraGeometry = Prepared->CameraGeometry();"))
        self.assertIn("OriginalVehicleShowroomGeometry GarageCameraGeometry",self.header)
        self.assertIn("OriginalVehicleShowroomCamera GarageCameraFit",self.header)
        self.assertIn("FitPrepared(",self.policy)

    def test_native_view_override_is_read_only_and_fails_to_super(self):
        camera=self.pawn.split("void ALatebrakeCarPawn::CalcCamera(",1)[1]
        for marker in ("Super::CalcCamera(DeltaTime, OutResult);",
                       "bGarageCameraViewPresented = false;",
                       "GetCommittedOriginalWheelParts() != 4",
                       "GarageCameraGeometry.VehicleId() != GetPresentedGarageVehicleId()",
                       "!Mode->CanBrowseGarage()", "PC->GetViewportSize(Width, Height)",
                       "GarageCameraFit.FitPrepared(GarageCameraGeometry, Request)",
                       "MeshTransform.GetScale3D().Equals(FVector::OneVector, 0.001)",
                       "FRotationMatrix::MakeFromXZ(Forward, Up)",
                       "LineTraceSingleByChannel(", "OutResult.Location = Eye;",
                       "OutResult.FOV = static_cast<float>(Pose.HorizontalFovDegrees);",
                       "bGarageCameraViewPresented = true;"):
            self.assertIn(marker,camera)
        self.assertLess(camera.index("Super::CalcCamera("),camera.index("OutResult.Location"))
        self.assertLess(camera.index("LineTraceSingleByChannel("),camera.index("OutResult.Location"))
        for forbidden in ("OriginalVehicleSurfaceComposer", "CreateMeshSection(",
                          "Garage.Select(", "SaveRecords(", "AddForce(",
                          "SetPhysicsLinearVelocity(", "CameraViews.Next("):
            self.assertNotIn(forbidden,camera)

    def test_hud_and_camera_share_protected_desktop_lane(self):
        lane=read("Source/Latebrake/Public/UI/GarageShowroomLane.h")
        for marker in ("CalculateGarageShowroomLane(", "Request.CenterX = Lane.CenterX;",
                       "Request.HalfWidth = Lane.HalfWidth;",
                       "PriorPose.HalfWidth != Request.HalfWidth"):
            self.assertIn(marker,self.pawn)
        for marker in ("CalculateGarageShowroomLane(", "ShowroomLane.TextWidth",
                       "zone.Width = FMath::Min(zone.Width, lane.TextWidth)",
                       "LIVE 3D | F4 ORBIT"):
            self.assertIn(marker,self.hud)
        self.assertIn("CalculateHudMenuReadablePanel(",lane)
        self.assertIn("CalculateDesktopMenuMouseLayout(",lane)
        self.assertIn("touchUI||hud.Compact",lane)
        self.assertIn("DesktopApertures();",
                      read("Tests/Standalone/OriginalVehicleShowroomCameraTests.cpp"))

    def test_mouse_orbit_uses_same_guarded_path_and_measured_buttons(self):
        for marker in ("CalculateGarageOrbitButtons(", "HitTestMenuMouse(OrbitMouseX, OrbitMouseY)",
                       "car && car->CanSelectGarageCameraOrbit()",
                       "Car->CanSelectGarageCameraOrbit()", "GarageOrbitCaption(Index)"):
            self.assertIn(marker,self.hud)
        arbitration=read("Source/Latebrake/Public/UI/GaragePointerArbitration.h")
        self.assertIn("HitTestGarageOrbitButtons(orbit, x, y, true)",arbitration)
        self.assertIn("SelectGarageCameraOrbit(latebrake::GarageOrbitIndex(Action))",self.pawn)
        self.assertIn("Index == GarageCameraOrbitIndex",self.pawn)
        self.assertIn("wrong action fails closed",
                      read("Tests/Standalone/OriginalVehicleShowroomCameraTests.cpp"))

    def test_orbit_is_separate_from_race_camera_and_restores_on_exit(self):
        orbit=self.pawn.split("bool ALatebrakeCarPawn::CanSelectGarageCameraOrbit()",1)[1].split(
            "void ALatebrakeCarPawn::CalcCamera(",1)[0]
        for marker in ("Mode->CanBrowseGarage()", "!Mode->IsFocusLossGuardActive()",
                       "!Mode->IsManualRacePauseActive()",
                       "SelectGarageCameraOrbit((GarageCameraOrbitIndex + 1) % latebrake::GarageOrbitCount);",
                       "GarageCameraOrbitIndex = Index;",
                       "GarageCameraFit.Reset();"):
            self.assertIn(marker,orbit)
        self.assertNotIn("CameraViews.Next(",orbit)
        self.assertIn('ActionName="GarageOrbit",Key=F4',self.input)
        self.assertIn("GarageOrbitKey.bExecuteWhenPaused = true;",self.pawn)
        preview=self.pawn.split("void ALatebrakeCarPawn::UpdateGarageShowroomPresentation(",1)[1].split(
            "void ALatebrakeCarPawn::ClearDriverInputs()",1)[0]
        self.assertIn("if (!Plan.PreviewActive)",preview)
        self.assertIn("GarageCameraOrbitIndex = 0;",preview)
        self.assertIn("GarageCameraFit.Reset();",preview)
        self.assertIn("F4 GARAGE ANGLE %d/4",self.hud)
        self.assertIn("IsGarageCameraViewPresented()",self.hud)
        self.assertIn("AllRimsUseCommittedCameraGeometry();",self.tests)
        self.assertIn("add_latebrake_portable_research_test(OriginalVehicleShowroomCamera)",
                      self.cmake)

if __name__=="__main__":unittest.main()
