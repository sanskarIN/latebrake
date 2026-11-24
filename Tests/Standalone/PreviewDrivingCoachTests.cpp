#include "PreviewDrivingCoach.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool ok, const char* message)
{
    if (!ok) { std::cerr << "2D driver coach: " << message << '\n';
               std::exit(EXIT_FAILURE); }
}
latebrake_preview::PreviewCoachInput CleanSample()
{
    latebrake_preview::PreviewCoachInput s{};
    s.CarX=480.0f; s.CarY=510.0f;
    s.ForwardX=-1.0f; s.ForwardY=0.0f;
    s.TangentX=-1.0f; s.TangentY=0.0f;
    s.FutureTangentX=-0.95f; s.FutureTangentY=-0.31f;
    s.AimX=406.0f; s.AimY=493.0f;
    s.Speed=240.0f; s.ManualSteer=0.0f;
    s.TrackOffset=0.0f; s.TrackHalfWidth=0.23f;
    s.Grip=1.0f; s.OnRoad=true; s.AssistEnabled=true;
    return s;
}
void CoachBehaviors()
{
    using namespace latebrake_preview;
    auto s=CleanSample();
    auto c=EvaluatePreviewCoach(s);
    Check(c.Valid && c.Alignment>0.99f && c.Bend>0.2f &&
          c.RoadTurn==1 && !c.WrongWay,
          "track tangent and coming bend agree with player heading");
    Check(c.OutputSteer>0.0f && c.OutputSteer<0.26f &&
          c.AssistApplied,
          "gentle course-following assistance provides real correction");
    s.Speed=350.0f;
    c=EvaluatePreviewCoach(s);
    Check(c.BrakeForCorner && c.TargetSpeed<300.0f,
          "high speed toward a bend gives actionable brake warning");
    s.Speed=120.0f;
    c=EvaluatePreviewCoach(s);
    Check(!c.BrakeForCorner, "reasonable corner-entry speed avoids warning");
    s.Speed=240.0f;
    s.ForwardX=1.0f;
    c=EvaluatePreviewCoach(s);
    Check(c.WrongWay && !c.AssistApplied && !c.BrakeForCorner,
          "wrong direction is explicitly reported, never auto-steered");
    s.ForwardX=-1.0f;
    s.ManualSteer=-1.0f;
    c=EvaluatePreviewCoach(s);
    Check(c.OutputSteer==-1.0f && !c.AssistApplied,
          "maximum human steer has unconditional priority");
    s.ManualSteer=0.0f; s.AssistEnabled=false;
    c=EvaluatePreviewCoach(s);
    Check(c.OutputSteer==0.0f && !c.AssistApplied,
          "disabling assist genuinely restores manual-only turning");
    s.AssistEnabled=true; s.Speed=-100.0f;
    c=EvaluatePreviewCoach(s);
    Check(c.OutputSteer==0.0f && !c.AssistApplied && !c.WrongWay,
          "reversing is not mistaken for forward wrong-way driving");
    s.Speed=0.0f;
    c=EvaluatePreviewCoach(s);
    Check(c.OutputSteer==0.0f && !c.AssistApplied,
          "parked vehicle receives no motion assist");
    s.Speed=240.0f; s.OnRoad=false;
    c=EvaluatePreviewCoach(s);
    Check(c.Valid && !c.AssistApplied && !c.BrakeForCorner &&
          !c.WrongWay, "offroad feedback cannot impose a hidden autocorrect");
    s.OnRoad=true; s.TrackOffset=0.20f;
    c=EvaluatePreviewCoach(s);
    Check(c.OutputSteer>0.0f && c.OutputSteer<0.27f,
          "road-edge correction remains strictly bounded");
}

void MalformedReadings()
{
    using namespace latebrake_preview;
    auto s=CleanSample();
    s.TrackHalfWidth=0.0f;
    Check(!EvaluatePreviewCoach(s).Valid, "zero half-width invalid");
    s=CleanSample();s.Grip=0.0f;
    Check(!EvaluatePreviewCoach(s).Valid, "unknown grip invalid");
    s=CleanSample();s.ForwardX=s.ForwardY=0.0f;
    Check(!EvaluatePreviewCoach(s).Valid, "zero direction invalid");
    s=CleanSample();s.AimX=s.CarX;s.AimY=s.CarY;
    Check(!EvaluatePreviewCoach(s).Valid, "coincident aim invalid");
    s=CleanSample();s.TangentX=std::numeric_limits<float>::infinity();
    Check(!EvaluatePreviewCoach(s).Valid, "infinite track direction invalid");
    s=CleanSample();s.Speed=std::numeric_limits<float>::quiet_NaN();
    Check(!EvaluatePreviewCoach(s).Valid, "NaN race speed invalid");
    s=CleanSample();s.ManualSteer=std::numeric_limits<float>::quiet_NaN();
    const auto c=EvaluatePreviewCoach(s);
    Check(c.Valid && c.OutputSteer>=-1.0f && c.OutputSteer<=1.0f,
          "nonfinite steering never propagates into physics");
}
void DeterministicStress()
{
    using namespace latebrake_preview;
    unsigned int seed=0xb42892u;
    for (int i=0;i<100000;++i)
    {
        seed=1664525u*seed+1013904223u;
        auto s=CleanSample();
        s.Speed=static_cast<float>(seed%420u);
        s.ManualSteer=static_cast<float>(static_cast<int>(
            (seed>>12)%2001u)-1000)/1000.0f;
        s.TrackOffset=static_cast<float>(static_cast<int>(
            (seed>>4)%501u)-250)/1000.0f;
        s.Grip=0.8f+static_cast<float>(seed%600u)/1000.0f;
        s.AssistEnabled=(seed&1u)!=0u;
        s.ForwardX=static_cast<float>(static_cast<int>(
            (seed>>13)%2001u)-1000)/1000.0f;
        s.ForwardY=-0.5f;
        const auto c=EvaluatePreviewCoach(s);
        Check(c.Valid && std::isfinite(c.OutputSteer) &&
              c.OutputSteer>=-1.0f && c.OutputSteer<=1.0f &&
              std::isfinite(c.TargetSpeed) &&
              c.TargetSpeed>=150.0f && c.TargetSpeed<=320.0f &&
              c.Bend>=0.0f && c.Bend<=1.0001f,
              "100k mixed gamepad, speed, grip, edge and direction samples finite");
        if (std::abs(s.ManualSteer)>=0.35f || !s.AssistEnabled)
            Check(!c.AssistApplied &&
                std::abs(c.OutputSteer-s.ManualSteer)<0.00001f,
                "manual control unaffected by no-assist or hard steering");
    }
}
} // namespace
int main()
{
    CoachBehaviors();
    MalformedReadings();
    DeterministicStress();
    std::cout << "2D driving coach / steering assist policy passed.\n";
    return EXIT_SUCCESS;
}
