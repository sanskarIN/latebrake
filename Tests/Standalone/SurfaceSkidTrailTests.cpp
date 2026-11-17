#include "Vehicles/SurfaceSkidTrail.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
void Require(bool pass, const char* why)
{
    if (!pass) { std::cerr << "SurfaceSkidTrail FAIL: " << why << '\n'; std::exit(EXIT_FAILURE); }
}
latebrake::SurfaceSkidFrame Frame(double x)
{
    latebrake::SurfaceSkidFrame f;
    f.DeltaSeconds=1.0/60.0;f.DrivingAllowed=true;
    for(unsigned i=0;i<4;++i)
    {
        auto& c=f.Contacts[i];c.Supported=true;c.PavedRoad=true;c.SurfaceKey=10;
        c.Position={x,static_cast<double>(i)*60.0,0.0};c.Normal={0.0,0.0,1.0};
        c.ForwardCmPerSecond=3000.0;c.LateralCmPerSecond=1500.0;c.NormalLoadNewtons=2695.0;
    }
    return f;
}
void Basis(const latebrake::SkidStrip& s)
{
    using namespace latebrake;
    Require(s.Valid&&std::abs(SkidLength(s.Forward)-1.0)<1.0e-9&&
        std::abs(SkidLength(s.Right)-1.0)<1.0e-9&&std::abs(SkidLength(s.Up)-1.0)<1.0e-9,
        "fitted basis vectors are independently unit length");
    Require(std::abs(SkidDot(s.Forward,s.Right))<1.0e-9&&
        std::abs(SkidDot(s.Forward,s.Up))<1.0e-9&&std::abs(SkidDot(s.Right,s.Up))<1.0e-9&&
        SkidDot(SkidCross(s.Forward,s.Right),s.Up)>1.0-1.0e-9,
        "fitted decorative strip basis is orthogonal and right handed");
}
}
int main()
{
    using namespace latebrake;
    auto flat=MakeSkidStrip({0,0,0},{100,0,0},{0,0,1},{0,0,1},1.0);
    Basis(flat);
    Require(std::abs(flat.Center.Z-0.24)<1.0e-12&&flat.LengthCm==100.5&&flat.WidthCm==18.0,
        "flat strip uses centimetres and a small explicit road-surface lift");
    const auto slopeNormal=SkidVector{-0.3,0,1}*(1.0/std::sqrt(1.09));
    const auto slope=MakeSkidStrip({0,0,0},{100,0,30},slopeNormal,slopeNormal,0.5);
    Basis(slope);
    Require(std::abs(SkidDot(slope.Center,slopeNormal)-0.24)<1.0e-10&&
        std::abs(slope.LengthCm-(std::sqrt(10900.0)+0.5))<1.0e-9,
        "inclined strips use measured plane length instead of a horizontal approximation");
    const auto bank=MakeSkidStrip({0,0,0},{100,0,0},{0,0.6,0.8},{0,0.6,0.8},1.0);
    Basis(bank);
    Require(std::abs(bank.Right.Z+0.6)<1.0e-10,"banked strip width follows the genuine surface plane");
    Require(!MakeSkidStrip({0,0,0},{100,0,20},{0,0,1},{0,0,1},1.0).Valid,
        "large unobserved height discontinuity cannot create a floating road bridge");
    SurfaceSkidTrail stepTrail;
    stepTrail.Advance(Frame(0));
    auto stepFrame=Frame(700);
    for(auto& c:stepFrame.Contacts)c.Position.Z=16.0;
    Require(stepTrail.Advance(stepFrame).Count==0,
        "piece subdivision cannot disguise an unsupported sixteen-centimetre road-height jump");
    SurfaceSkidTrail trail;
    Require(trail.Advance(Frame(0)).Count==0,"new contact phase seeds without inventing a prior point");
    auto batch=trail.Advance(Frame(100));
    Require(batch.Valid&&batch.Count==4,"four independent supported skidding points emit strips");
    for(unsigned i=0;i<batch.Count;++i)Basis(batch.Strips[i]);
    batch=trail.Advance(Frame(316));
    Require(batch.Count==8,"high speed at low frame rate resamples distance into bounded pieces");
    batch=trail.Advance(Frame(1016));
    Require(batch.Count==16,"worst accepted jump remains within the sixteen-strip frame budget");
    auto teleport=Frame(1116);teleport.TeleportRevision=1;
    Require(trail.Advance(teleport).Count==0,"teleport revisions cannot connect decorative tire trails");
    auto next=Frame(1216);next.TeleportRevision=1;next.Contacts[0].SurfaceKey=11;
    Require(trail.Advance(next).Count==3,"a new surface identity breaks only its own wheel continuity");
    auto shoulder=Frame(1316);shoulder.TeleportRevision=1;
    for(auto& c:shoulder.Contacts)c.PavedRoad=false;
    Require(trail.Advance(shoulder).Count==0,"grass or an untagged world-static hit cannot receive asphalt marks");
    next=Frame(1416);next.TeleportRevision=1;
    Require(trail.Advance(next).Count==0,"return from an ineligible surface starts new anchors");
    next=Frame(1516);next.TeleportRevision=1;
    for(auto& c:next.Contacts){c.LateralCmPerSecond=0;c.ModeledSlipAvailable=true;c.ModeledBrakingSlip=-0.6;}
    Require(trail.Advance(next).Count==4,"committed modeled longitudinal slip can independently request visual marks");
    auto invalid=next;invalid.DeltaSeconds=0.2;
    Require(!trail.Advance(invalid).Valid&&trail.Advance(next).Count==0,
        "rejected frame hitches break every corner's visual continuity");
    const double nan=std::numeric_limits<double>::quiet_NaN();
    invalid=next;invalid.Contacts[2].Normal.Z=nan;
    Require(trail.Advance(invalid).Count==0,"malformed per-corner vectors cannot stamp bad geometry");
    for(double fps:{30.0,60.0,120.0})
    {
        trail.BreakContinuity();double covered=0.0;unsigned strips=0;
        for(unsigned step=0;step<static_cast<unsigned>(fps*3.0);++step)
        {
            auto f=Frame(3000.0*static_cast<double>(step)/fps);f.DeltaSeconds=1.0/fps;
            auto b=trail.Advance(f);Require(b.Valid&&b.Count<=16,"distance sampling stays bounded at each FPS");
            for(unsigned i=0;i<b.Count;++i){covered+=b.Strips[i].LengthCm-0.5;++strips;}
        }
        Require(strips>0&&covered>4.0*(9000.0-201.0)&&covered<=36000.0,
            "three seconds of contact displacement is covered without duplication or frame-rate-length growth");
    }
    std::mt19937 rng(0x5b9810);std::uniform_real_distribution<double> u(0.0,1.0);
    for(unsigned n=0;n<100000;++n)
    {
        const double nx=(u(rng)-0.5)*0.8,ny=(u(rng)-0.5)*0.8;
        const SkidVector normal{nx,ny,std::sqrt(1.0-nx*nx-ny*ny)};
        const double angle=u(rng)*6.283185307179586;
        SkidVector direction{std::cos(angle),std::sin(angle),
            -(nx*std::cos(angle)+ny*std::sin(angle))/normal.Z};
        direction=direction*(1.0/SkidLength(direction));
        const SkidVector start{(u(rng)-0.5)*100000.0,(u(rng)-0.5)*100000.0,(u(rng)-0.5)*1000.0};
        const double distance=1.0+178.0*u(rng);
        const auto s=MakeSkidStrip(start,start+direction*distance,normal,normal,u(rng));
        Basis(s);
        Require(std::abs(s.LengthCm-distance-0.5)<1.0e-7&&
            std::abs(SkidDot(s.Center-start,normal)-0.24)<1.0e-7,
            "100k independently constructed planes retain tangent length and fixed surface lift");
    }
    std::cout<<"Skid trails: plane geometry, contact/teleport lifecycle, FPS coverage and 100000 plane samples passed\n";
}
