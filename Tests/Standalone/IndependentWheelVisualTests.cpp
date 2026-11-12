#include "Vehicles/WheelVisualMotion.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

void Require(bool pass,const char* why)
{
    if(!pass){std::cerr<<"IndependentWheelVisual FAIL: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
int main()
{
    using namespace latebrake;
    WheelVisualMotion wheels;
    Require(wheels.Advance({0.1,290.0,0.5,29.0}),"coasting frame accepted");
    const double coast=wheels.Current().RollDegrees;
    Require(std::abs(coast-180.0/3.14159265358979323846)<1.0e-9,"legacy phase retained");
    Require(wheels.ApplyCommittedBrakeRotation(1,12u,{0.0,0.0,0.0,0.5}),"committed rear-brake deltas accepted");
    Require(std::abs(wheels.CurrentCorner(0).RollDegrees-coast)<1.0e-9&&
            wheels.CurrentCorner(2).RollDegrees==0.0&&
            std::abs(wheels.CurrentCorner(3).RollDegrees-coast*0.5)<1.0e-9,
            "rear wheels can lock independently while unloaded front visuals coast");
    Require(wheels.CurrentCorner(0).FrontSteerDegrees==15.0&&
            wheels.CurrentCorner(2).FrontSteerDegrees==0.0,"rear wheels never receive front steering");
    Require(!wheels.ApplyCommittedBrakeRotation(1,12u,{0.0,0.0,0.0,0.5}),"receipt cannot apply angular delta twice");
    wheels.Advance({0.1,290.0,0.0,29.0});
    Require(!wheels.ApplyCommittedBrakeRotation(1,15u,{}),"stale receipt rejected after new frame");
    const auto before=wheels.CurrentCorner(0);
    Require(!wheels.ApplyCommittedBrakeRotation(2,15u,{0.0,0.0,0.0,std::numeric_limits<double>::quiet_NaN()}),
            "malformed late corner rejects entire visual correction");
    Require(wheels.CurrentCorner(0).RollDegrees==before.RollDegrees,"bad correction cannot partly alter corners");
    Require(wheels.ApplyCommittedBrakeRotation(2,15u,{}),"valid correction remains possible after rejected input");
    wheels.Reset();wheels.Advance({0.1,-290.0,-1.0,29.0});
    Require(!wheels.ApplyCommittedBrakeRotation(2,15u,{}),"restart does not reuse previous receipt");
    Require(wheels.ApplyCommittedBrakeRotation(3,15u,{-1.0,-1.0,-1.0,-1.0}),"reverse rotor deltas accepted");
    Require(wheels.CurrentCorner(0).RollDegrees<0.0,"modeled reverse wheel rotates backward");
    wheels.Suspend();
    Require(!wheels.ApplyCommittedBrakeRotation(4,15u,{}),"pause cannot animate a stale brake frame");
    Require(wheels.CurrentCorner(0).FrontSteerDegrees==0.0,"pause neutralizes steering");
    Require(wheels.CurrentCorner(999).RollDegrees==0.0,"bad corner index fails closed");
    std::cout<<"Independent wheel visuals: delivered angular deltas, lock, reverse and lifecycle passed\n";
}
