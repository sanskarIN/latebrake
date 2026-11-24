#include "Track/RoundedCircuitRoadPlan.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void Require(bool okay, const char* reason) {
    if (!okay) { std::cerr << "Rounded circuit FAIL: " << reason << '\n'; std::exit(EXIT_FAILURE); }
}
bool Near(double a, double b, double eps=1e-7) { return std::abs(a-b) < eps; }
}

int main() {
    using namespace latebrake;
    static_assert(RoundedPhysicalRoadCount == 68);
    static_assert(RoundedCornerSegmentCount == 16);
    static_assert(RoundedHalfRoadWidthCm == 450.0);
    static_assert(RoundedHorizontalTangentCm == 2000.0);
    static_assert(RoundedVerticalTangentCm == 1000.0);
    Require(!PlannedRoundedPhysicalRoadPiece(-1).Valid &&
            !PlannedRoundedPhysicalRoadPiece(68).Valid,
            "invalid plan indices fail closed");
    for (int index=0; index<RoundedPhysicalRoadCount; ++index) {
        auto part=PlannedRoundedPhysicalRoadPiece(index);
        Require(IsValidRoundedPhysicalRoadPiece(part), "all 68 asphalt collision slabs valid");
        Require(Near(part.HeightCm + 50.0*part.ScaleZ, -1.0),
                "physical road tops align with the previous -1cm elevation");
        Require(Near(part.ScaleY, 9.0), "road width must be 900cm");
        Require(ContainsRoundedPhysicalRoadPoint(part,part.Center),
                "piece center belongs to its own physical road");
    }
    auto broken=PlannedRoundedPhysicalRoadPiece(0);
    broken.ScaleX=-1.0;
    Require(!ContainsRoundedPhysicalRoadPoint(broken,PlayerStart.Position),
            "invalid physical widths rejected");
    broken=PlannedRoundedPhysicalRoadPiece(0);
    broken.HeightCm+=5.0;
    Require(!IsValidRoundedPhysicalRoadPiece(broken),
            "floating asphalt is rejected");
    const double nan=std::numeric_limits<double>::quiet_NaN();
    const double inf=std::numeric_limits<double>::infinity();
    Require(!IsOnRoundedPhysicalRoad({nan,0.0}) &&
            !IsOnRoundedPhysicalRoad({0.0,inf}) &&
            !RoundedRoadCenterlineAt(nan).Valid &&
            !RoundedRoadCenterlineAt(inf).Valid,
            "nonfinite positions and route parameters rejected");

    for (const auto gate : CircuitGates) {
        Require(IsOnRoundedPhysicalRoad(gate),
                "all existing four scoring gates still lie on physical asphalt");
    }
    Require(IsOnRoundedPhysicalRoad(PlayerStart.Position) &&
            IsOnRoundedPhysicalRoad(RivalStart.Position),
            "existing player and rival grid spawns lie on physical asphalt");
    for (const auto pose : PlayerRecoveries) {
        Require(IsOnRoundedPhysicalRoad(pose.Position),
                "existing player recovery positions remain on asphalt");
    }
    for (const auto pose : RivalRecoveries) {
        Require(IsOnRoundedPhysicalRoad(pose.Position),
                "existing rival recovery positions remain on asphalt");
    }
    Require(!IsOnRoundedPhysicalRoad({0.0,0.0}) &&
            !IsOnRoundedPhysicalRoad({-6000.0,3000.0}) &&
            !IsOnRoundedPhysicalRoad({0.0,SouthY+1200.0}) &&
            !IsOnRoundedPhysicalRoad({EastX+1000.0,0.0}),
            "infield and outer grass are not asphalt");

    constexpr double pi=RoundedPi;
    constexpr CircuitPoint corners[]={
        {RoundedHorizontalTangentCm,-RoundedVerticalTangentCm},
        {RoundedHorizontalTangentCm,RoundedVerticalTangentCm},
        {-RoundedHorizontalTangentCm,RoundedVerticalTangentCm},
        {-RoundedHorizontalTangentCm,-RoundedVerticalTangentCm}
    };
    long edgeSamples=0;
    for (int corner=0;corner<4;++corner) {
        for (int step=0;step<=256;++step) {
            double theta=-pi/2.0+corner*pi/2.0+(step/256.0)*pi/2.0;
            for (double offset : {-RoundedHalfRoadWidthCm+35.0,
                                  -RoundedHalfRoadWidthCm+100.0,0.0,
                                  RoundedHalfRoadWidthCm-100.0,
                                  RoundedHalfRoadWidthCm-35.0}) {
                double r=RoundedArcRadiusCm+offset;
                Require(IsOnRoundedPhysicalRoad({
                    corners[corner].X+r*std::cos(theta),
                    corners[corner].Y+r*std::sin(theta)}),
                    "the full corner interior and road edges have NO gaps");
                ++edgeSamples;
            }
            if (step>20 && step<236) {
                for (double offset : {-RoundedHalfRoadWidthCm-90.0,
                                      RoundedHalfRoadWidthCm+90.0}) {
                    double r=RoundedArcRadiusCm+offset;
                    Require(!IsOnRoundedPhysicalRoad({
                        corners[corner].X+r*std::cos(theta),
                        corners[corner].Y+r*std::sin(theta)}),
                        "outside-edge/grass samples must NOT be physically paved");
                    ++edgeSamples;
                }
            }
        }
    }
    const auto start=RoundedRoadCenterlineAt(0.0);
    const auto wrap=RoundedRoadCenterlineAt(RoundedLapLengthCm);
    const auto back=RoundedRoadCenterlineAt(-RoundedLapLengthCm);
    Require(start.Valid && wrap.Valid && back.Valid &&
            Near(start.Pose.Position.X,-RoundedHorizontalTangentCm) &&
            Near(start.Pose.Position.Y,SouthY) &&
            Near(start.Pose.Position.X,wrap.Pose.Position.X) &&
            Near(start.Pose.Position.Y,back.Pose.Position.Y),
            "one-lap and negative wrap preserve position");
    for (int sample=0;sample<4096;++sample) {
        double s=RoundedLapLengthCm*sample/4096.0;
        auto now=RoundedRoadCenterlineAt(s);
        auto next=RoundedRoadCenterlineAt(s+1.0);
        Require(now.Valid && next.Valid &&
                IsOnRoundedPhysicalRoad(now.Pose.Position),
                "ALL 4096 centerline lap samples are on physical asphalt");
        double dx=next.Pose.Position.X-now.Pose.Position.X;
        double dy=next.Pose.Position.Y-now.Pose.Position.Y;
        double heading=now.Pose.HeadingDegrees*pi/180.0;
        Require(dx*std::cos(heading)+dy*std::sin(heading)>0.98,
                "forward tangent remains continuous across every bend");
    }
    std::cout << "Rounded circuit 68 pieces, " << edgeSamples
              << " edge samples and 4096 racing poses validated\n";
}