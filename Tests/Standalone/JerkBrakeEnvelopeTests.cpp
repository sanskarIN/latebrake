#include "AI/JerkBrakeEnvelope.h"
#include "AI/JerkBrakeSpeedLimit.h"

#include <cstdlib>
#include <iostream>
#include <cmath>
#include <limits>

using namespace latebrake;

// Do not use NDEBUG-controlled assert: official CI tests Release builds
// with warnings-as-errors, and disabled assertions erase all evidence.
#define REQUIRE(condition) do { if (!(condition)) { \
    std::cerr << "JerkBrakeEnvelope FAIL " << __LINE__ \
              << ": " #condition << '\n'; \
    std::exit(EXIT_FAILURE); \
} } while (false)

int main()
{
    const JerkBrakeEnvelopeSettings settings{};
    const auto stopped=EvaluateJerkBrakeEnvelope(0,0,1000,settings);
    REQUIRE(stopped.Valid && !stopped.RequiresEmergencyBrake);
    REQUIRE(std::abs(stopped.RequiredGapCm-485.0)<1e-9);
    const auto contact=EvaluateJerkBrakeEnvelope(0,0,485,settings);
    REQUIRE(contact.Valid && contact.RequiresEmergencyBrake);

    // For identical speeds, no dynamic closing distance is invented.
    const auto matched=EvaluateJerkBrakeEnvelope(3000,3000,1000);
    REQUIRE(matched.Valid && matched.ClosingSpeedCmps==0);
    REQUIRE(matched.RampDistanceCm==0);

    // Jerk-limited braking must never undercut instantaneous full
    // deceleration for a positive closing speed.
    for(double speed=25;speed<=12000;speed+=125)
    {
        const auto r=EvaluateJerkBrakeEnvelope(speed,0,100000);
        REQUIRE(r.Valid);
        const double instantaneous=speed*speed/
            (2.0*settings.MaximumDecelerationCmps2);
        REQUIRE(r.RampDistanceCm+r.ConstantBrakeDistanceCm+1e-7
            >=instantaneous);
        REQUIRE(r.RequiredGapCm>=485+instantaneous);
        REQUIRE(r.StopDurationSeconds>=r.RampDurationSeconds);
        const auto before=EvaluateJerkBrakeEnvelope(
            speed,0,r.RequiredGapCm-0.01);
        const auto after=EvaluateJerkBrakeEnvelope(
            speed,0,r.RequiredGapCm+0.01);
        REQUIRE(before.Valid && before.RequiresEmergencyBrake);
        REQUIRE(after.Valid && !after.RequiresEmergencyBrake);
    }

    // Continuous at the ramp-to-constant-braking transition.
    const double transition=settings.MaximumDecelerationCmps2*
        settings.MaximumDecelerationCmps2/
        (2.0*settings.MaximumJerkCmps3);
    const auto left=EvaluateJerkBrakeEnvelope(transition-1e-5,0,1000);
    const auto right=EvaluateJerkBrakeEnvelope(transition+1e-5,0,1000);
    REQUIRE(left.Valid && right.Valid);
    REQUIRE(std::abs(left.RequiredGapCm-right.RequiredGapCm)<0.01);

    // A slower obstacle increases closing demand; a faster one does not.
    const auto slow=EvaluateJerkBrakeEnvelope(5000,1000,1000);
    const auto fast=EvaluateJerkBrakeEnvelope(5000,4500,1000);
    REQUIRE(slow.Valid && fast.Valid);
    REQUIRE(slow.RequiredGapCm>fast.RequiredGapCm);
    REQUIRE(!EvaluateJerkBrakeEnvelope(-1,0,1000).Valid);
    REQUIRE(!EvaluateJerkBrakeEnvelope(100,0,-1).Valid);
    REQUIRE(!EvaluateJerkBrakeEnvelope(100,0,
        std::numeric_limits<double>::infinity()).Valid);
    auto invalid=settings;
    invalid.MaximumJerkCmps3=0;
    REQUIRE(!EvaluateJerkBrakeEnvelope(100,0,1000,invalid).Valid);
    invalid=settings;
    invalid.ReactionSeconds=std::numeric_limits<double>::quiet_NaN();
    REQUIRE(!EvaluateJerkBrakeEnvelope(100,0,1000,invalid).Valid);
    // Inverse planner must never advise a speed whose forward gap is unsafe.
    const auto overlap=SolveJerkBrakeClosingSpeedLimit(485.0);
    REQUIRE(overlap.Valid && !overlap.HasSafeClosingSpeed);
    const auto invalidGap=SolveJerkBrakeClosingSpeedLimit(
        std::numeric_limits<double>::quiet_NaN());
    REQUIRE(!invalidGap.Valid && !invalidGap.HasSafeClosingSpeed);
    double previous=0.0;
    for(double gap=486.0;gap<=90000.0;gap+=137.0)
    {
        const auto limit=SolveJerkBrakeClosingSpeedLimit(gap);
        REQUIRE(limit.Valid && limit.HasSafeClosingSpeed);
        REQUIRE(limit.MaximumClosingSpeedCmps>=previous);
        REQUIRE(limit.MaximumClosingSpeedCmps>=0.0);
        REQUIRE(limit.MaximumClosingSpeedCmps<=20000.0);
        const auto safe=EvaluateJerkBrakeEnvelope(
            limit.MaximumClosingSpeedCmps,0.0,gap);
        REQUIRE(safe.Valid && !safe.RequiresEmergencyBrake);
        REQUIRE(limit.RequiredGapAtLimitCm==safe.RequiredGapCm);
        if(limit.MaximumClosingSpeedCmps<19999.0)
        {
            const auto faster=EvaluateJerkBrakeEnvelope(
                limit.MaximumClosingSpeedCmps+0.01,0.0,gap);
            REQUIRE(faster.Valid && faster.RequiresEmergencyBrake);
        }
        previous=limit.MaximumClosingSpeedCmps;
    }
    auto badLimitSettings=settings;
    badLimitSettings.MaximumDecelerationCmps2=-1.0;
    REQUIRE(!SolveJerkBrakeClosingSpeedLimit(1000,badLimitSettings).Valid);

}
