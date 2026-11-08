#include "Track/WorldScaleRacePacePlan.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Require(bool condition, const char* msg)
{
    if (!condition) { std::cerr << "WorldScaleRacePace FAIL: " << msg << '\n';
                     std::exit(EXIT_FAILURE); }
}
bool Close(double a,double b,double tol)
{ return std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<=tol; }
double WrapDistance(double a,double b,double lap)
{ const double d=std::abs(a-b);return std::min(d,lap-d); }
}
int main()
{
    using namespace latebrake;
    static_assert(WorldScalePaceStations==512);
    const auto plan=BuildWorldScalePacePlan();
    Require(plan.Valid && plan.EstimatedLapSeconds>25.0 &&
            plan.EstimatedLapSeconds<180.0,
            "finite candidate 1.70km lap and braking plan");
    const double ds=WorldScalePaceStepCm;
    const double brake=2.0*WorldScalePaceBrakeAccelCmps2*ds;
    const double motor=2.0*WorldScalePaceDriveAccelCmps2*ds;
    int straights=0,curves=0,braking=0,accelerating=0;
    double fastestStraight=0.0,slowestCurve=WorldScalePaceMaxSpeedCmps;
    for(unsigned i=0;i<WorldScalePaceStations;++i)
    {
        const auto& a=plan.Nodes[i];
        const auto& b=plan.Nodes[(i+1)%WorldScalePaceStations];
        const auto original=WorldScaleCenterlineAt(a.DistanceCm);
        Require(original.Valid && IsOnWorldScaleRoad(a.Position) &&
                Close(original.Pose.Position.X,a.Position.X,1e-6) &&
                Close(original.Pose.Position.Y,a.Position.Y,1e-6),
                "all 512 route stations use the exact 9m physical circuit");
        Require(a.TargetCmps>=WorldScalePaceMinSpeedCmps &&
                a.TargetCmps<=a.LocalLimitCmps+1e-7 &&
                std::isfinite(a.CurvaturePerCm) &&
                a.CurvaturePerCm>=0.0 && a.CurvaturePerCm<0.00015,
                "finite speed under curvature ceiling");
        const double v=a.TargetCmps*a.TargetCmps;
        const double next=b.TargetCmps*b.TargetCmps;
        Require(v<=next+brake+0.02 && next<=v+motor+0.02,
                "every adjacent node, including lap seam, is brakeable");
        if(a.CurvaturePerCm<1e-11)
        {++straights;fastestStraight=std::max(fastestStraight,a.TargetCmps);}
        if(a.CurvaturePerCm>0.00011)
        {
            ++curves;slowestCurve=std::min(slowestCurve,a.TargetCmps);
            Require(v*a.CurvaturePerCm<=
                    WorldScalePaceLateralAccelCmps2+0.02,
                    "actual 80m radius bend never exceeds lateral budget");
        }
        if(a.TargetCmps>b.TargetCmps+0.5)++braking;
        if(a.TargetCmps+0.5<b.TargetCmps)++accelerating;
    }
    Require(straights>140 && curves>80 && braking>20 && accelerating>20 &&
            fastestStraight>3800.0 &&
            slowestCurve>1900.0 && slowestCurve<3300.0,
            "distinct long straights and anticipated 80m corner slowing");
    for(double invalid:{std::numeric_limits<double>::quiet_NaN(),
                         std::numeric_limits<double>::infinity(),
                         -std::numeric_limits<double>::infinity()})
        Require(!SampleWorldScalePace(plan,invalid).Valid &&
                std::isnan(WorldScaleSampleCurvature(invalid,100.0)),
                "nonfinite distance never provides valid pace");
    Require(std::isnan(WorldScaleSampleCurvature(0.0,-1.0)),
            "invalid derivative window refused");
    const WorldScalePacePlan empty{};
    Require(!SampleWorldScalePace(empty,0.0).Valid,
            "empty, unqualified plan cannot authorize speed");
    for(unsigned i=0;i<8192;++i)
    {
        const double s=WorldScaleCircuitLapCm*i/8192.0;
        const auto a=SampleWorldScalePace(plan,s);
        const auto b=SampleWorldScalePace(plan,s+WorldScaleCircuitLapCm*3.0);
        const auto c=SampleWorldScalePace(plan,s-WorldScaleCircuitLapCm);
        Require(a.Valid&&b.Valid&&c.Valid&&
                Close(a.TargetSpeedCmps,b.TargetSpeedCmps,1e-5)&&
                Close(a.TargetSpeedCmps,c.TargetSpeedCmps,1e-5)&&
                Close(a.Pose.Position.X,b.Pose.Position.X,1e-4),
                "8192 repeated-lap speed samples remain wrap-safe");
    }
    // The speed model is a continuous speed-vs-distance reference.
    // Repeated laps and values one micrometre either side of each knot
    // must never change the brake target abruptly due to rounding.
    for (unsigned i = 0; i < WorldScalePaceStations; ++i)
    {
        const double knot = i * WorldScalePaceStepCm;
        const auto before = SampleWorldScalePace(plan, knot - 1.0e-5);
        const auto after = SampleWorldScalePace(plan, knot + 1.0e-5);
        const auto exact = SampleWorldScalePace(plan, knot);
        Require(before.Valid && after.Valid && exact.Valid &&
                Close(before.TargetSpeedCmps, exact.TargetSpeedCmps, 0.001) &&
                Close(after.TargetSpeedCmps, exact.TargetSpeedCmps, 0.001),
                "all 512 speed interpolation knots and start seam are continuous");
    }
    int projections=0;
    for(unsigned i=0;i<1024;++i)
    {
        const double s=WorldScaleCircuitLapCm*(i+0.31)/1024.0;
        const auto pose=WorldScaleCenterlineAt(s);
        Require(pose.Valid,"centreline exists for projection test");
        const double rad=pose.Pose.HeadingDegrees*WorldScalePaceRadPerDegree;
        for(double lateral:{-300.0,0.0,300.0})
        {
            const CircuitPoint car{pose.Pose.Position.X-std::sin(rad)*lateral,
                                   pose.Pose.Position.Y+std::cos(rad)*lateral};
            const auto projected=ProjectWorldScaleRoute(plan,car);
            Require(projected.Valid&&projected.InsidePhysicalCandidateRoad &&
                    Close(projected.SignedLateralCm,lateral,12.0)&&
                    Close(projected.CentrelineSeparationCm,std::abs(lateral),12.0)&&
                    WrapDistance(projected.LapDistanceCm,s,
                                 WorldScaleCircuitLapCm)<35.0 &&
                    projected.SuggestedSpeedCmps>0.0,
                    "hypothetical car follows real 9m road and signed lateral");
            ++projections;
        }
    }
    Require(projections==3072,"all 3072 projected car poses covered");
    const auto first=ProjectWorldScaleRoute(
        plan,WorldScaleCenterlineAt(0.0).Pose.Position);
    const auto last=ProjectWorldScaleRoute(
        plan,WorldScaleCenterlineAt(WorldScaleCircuitLapCm-2.0).Pose.Position);
    Require(first.Valid && last.Valid &&
            WrapDistance(first.LapDistanceCm,last.LapDistanceCm,
                         WorldScaleCircuitLapCm)<10.0,
            "nearest-segment search includes correct closure at start/finish");
    for(CircuitPoint bad: {CircuitPoint{0.0,0.0},
                          CircuitPoint{1.0e10,0.0},
                          CircuitPoint{std::numeric_limits<double>::quiet_NaN(),0.0}})
        Require(!ProjectWorldScaleRoute(plan,bad).Valid,
                "infield or invalid physics cannot claim route progress");
    Require(!ProjectWorldScaleRoute(empty,plan.Nodes[0].Position).Valid,
            "invalid plan denies route projection");
    std::cout<<"WorldScaleRacePace: 512 stations, 8192 wrap checks, "
                "3072 nearby projections passed (source-only).\n";
    return EXIT_SUCCESS;
}
