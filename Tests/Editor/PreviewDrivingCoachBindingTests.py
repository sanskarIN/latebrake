"""Ensure the actual 2D Win64 game uses the driving coach in physics and UI.

These are SOURCE-LEVEL integration contracts, not UE engine verification.
Portable policy tests and the built Windows EXE --self-test are separate.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]

def src(path):
    return (ROOT/path).read_text(encoding="utf-8")

class PreviewDrivingCoachBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app=src("Experiments/Win32Preview/LateBrakePreview.cpp")
        cls.policy=src("Experiments/Win32Preview/PreviewDrivingCoach.h")
        cls.tests=src("Tests/Standalone/PreviewDrivingCoachTests.cpp")
        cls.cmake=src("Tests/Standalone/CMakeLists.txt")
        cls.guide=src("Experiments/Win32Preview/TRIAL_QUICKSTART.md")

    def test_real_game_uses_current_track_geometry_not_fake_gate_arrow(self):
        self.assertIn('#include "PreviewDrivingCoach.h"', self.app)
        sample=self.app.split("static latebrake_preview::PreviewCoachResult sampleDriverCoach(",1)[1].split(
            "static latebrake_preview::RaceModePolicy selectedModePolicy()",1)[0]
        for marker in ("shapePos(c,s,&x0,&y0);",
                       "shapePos(c1,s1,&x1,&y1);",
                       "shapePos(c2,s2,&x2,&y2);",
                       "input.AimX=x1;input.AimY=y1;",
                       "input.TrackOffset=trackOffset(race.x,race.y);",
                       "input.Grip=carGrip[selectedCar];",
                       "latebrake_preview::EvaluatePreviewCoach(input)"):
            self.assertIn(marker,sample)
        for forbidden in ("saveProgress(","creditGate(","race.nextGate=",
                          "finishRace(","unlockSelectedCar("):
            self.assertNotIn(forbidden,sample)

    def test_assist_alters_real_steering_only_at_allowed_strength(self):
        physics=self.app.split("static void update(",1)[1].split(
            "static void drawTrack()",1)[0]
        self.assertIn("const auto coach=sampleDriverCoach(steer,true);",physics)
        self.assertIn("const float effectiveSteer=coach.Valid?coach.OutputSteer:steer;",physics)
        self.assertLess(physics.index("const auto coach=sampleDriverCoach(steer,true);"),
                        physics.index("race.smoothedSteer+=(effectiveSteer"))
        for marker in ("CoachAbs(out.OutputSteer)<0.35f",
                       "in.Speed>45.0f","!out.WrongWay",
                       "out.OutputSteer+correction"):
            self.assertIn(marker,self.policy)
        self.assertIn("if(assist){steeringAssist=!steeringAssist;saveSettings();",self.app)
        self.assertIn("manualCoach.OutputSteer!=0.0f",self.app)
        self.assertIn("selfTestStage=87;",self.app)

    def test_visible_guidance_and_offroad_priority(self):
        for marker in ("color(111,238,237)",
                       "step<11", "gameTime*6.0f",
                       "WRONG WAY / TURN AROUND",
                       "SLOW DOWN FOR CORNER",
                       "ROAD GUIDE: ON  [J]",
                       "ROAD GUIDE / GENTLE ASSIST",
                       "sampleDriverCoach(0.0f,false)"):
            self.assertIn(marker,self.app)
        self.assertLess(self.app.index('text(369,85,"OFF TRACK / LOW GRIP"'),
                        self.app.index('text(344,85,"WRONG WAY / TURN AROUND"'))
        self.assertIn("Road guide and steering feedback",self.guide)

    def test_portable_and_headless_executable_regressions(self):
        self.assertIn("PreviewDrivingCoachTests.cpp",self.cmake)
        self.assertIn("NAME PreviewDrivingCoach",self.cmake)
        self.assertIn("100000",self.tests)
        self.assertIn("s.AssistEnabled=false;",self.tests)
        self.assertIn("!c.AssistApplied",self.tests)
        self.assertIn("const auto activeCoach=sampleDriverCoach(0.0f,true);",self.app)
        self.assertIn("for(int tr=0;tr<4;tr++)",self.app)
        self.assertIn("no automatic driving",self.guide)

    def test_real_framebuffer_visual_regression_and_honest_speed_units(self):
        app=self.app
        for marker in (
            "selfTestStage=88;",
            "const int gi=(int)guideY*WIDTH+(int)guideX;",
            "if(frame[gi]!=color(111,238,237))return false;",
            "if(frame[gi]==color(111,238,237))return false;",
            "if(frame[74*WIDTH+332]!=color(115,38,40))return false;",
            'hint(161,544,"PX/S");',
        ):
            self.assertIn(marker,app)
        self.assertNotIn('hint(161,544,"KM/H");',app)
        self.assertIn("virtual canvas",self.guide)
        self.assertIn("pixels per second",self.guide)

if __name__=="__main__":
    unittest.main()
