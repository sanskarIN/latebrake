#pragma once

#include "AI/LiveSixCarStartingGrid.h"
#include "Garage/GarageAppearance.h"

#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake
{
// Before GO the standard blocked-drive path erases all rigid-body
// momentum. For the opt-in physical grid only, allow a downward component
// already measured on the body's real velocity to survive that reset.
// Do not add gravity, move a body, preserve lateral/rotational motion,
// authorize a race clock, or change pause/focus/recovery behavior.
struct FleetGridSettleSample final
{
    bool SessionEligible=false;
    int Slot=-1;
    CircuitPose Pose{};
    double HeightCm=0.0;
    double VelocityX=0.0;
    double VelocityY=0.0;
    double VelocityZ=0.0;
    double DeltaSeconds=0.0;
};

struct FleetGridSettleDecision final
{
    bool PreserveDownward=false;
    double VerticalVelocityCmS=0.0;
};

inline constexpr double FleetMaximumSettleHeightCm=240.0;
inline constexpr double FleetSettleBelowExpectedToleranceCm=5.0;
inline constexpr double FleetMaximumSettleDownSpeedCmS=800.0;

inline FleetGridSettleDecision PlanFleetGridSettling(
    const FleetGridSettleSample& sample) noexcept
{
    if (!sample.SessionEligible ||
        sample.Slot<HumanGridSlot || sample.Slot>=StartGridCapacity ||
        !IsValidStartGridPose(sample.Pose) ||
        !std::isfinite(sample.HeightCm) ||
        sample.HeightCm<LiveGridExpectedChassisCenterZCm-FleetSettleBelowExpectedToleranceCm ||
        sample.HeightCm>FleetMaximumSettleHeightCm ||
        !std::isfinite(sample.VelocityX) || !std::isfinite(sample.VelocityY) ||
        std::hypot(sample.VelocityX,sample.VelocityY)>LiveGridMaxPlanarSpeedCmS ||
        !std::isfinite(sample.VelocityZ) ||
        sample.VelocityZ>=0.0 || sample.VelocityZ<-FleetMaximumSettleDownSpeedCmS ||
        !std::isfinite(sample.DeltaSeconds) || sample.DeltaSeconds<=0.0 ||
        sample.DeltaSeconds>ChassisContactMaxFrameSeconds)
        return {};
    const auto planned=StartGridPoses[static_cast<std::size_t>(sample.Slot)];
    const double distance=std::hypot(sample.Pose.Position.X-planned.Position.X,
        sample.Pose.Position.Y-planned.Position.Y);
    const double yaw=std::abs(std::remainder(
        sample.Pose.HeadingDegrees-planned.HeadingDegrees,360.0));
    if (distance>LiveGridMaxPoseErrorCm || yaw>LiveGridMaxHeadingErrorDegrees ||
        !StartGridFootprintOnPavement(sample.Pose) ||
        StartGridFootprintTouchesBarrier(sample.Pose))
        return {};
    return {true,sample.VelocityZ};
}

// The six source vehicle IDs correspond to distinct cosmetic bodies,
// not independently simulated tire/engine classes. A roster is planned
// once per attempt from the player's selected appearance. Rivals never
// consume ownership, money or garage unlocks; every physics multiplier
// remains the existing neutral AI baseline.
struct FleetVisualEntry final
{
    int Slot=-1;
    int VehicleId=-1;
    GarageLook Look{};
};

struct FleetVisualPlan final
{
    bool Valid=false;
    std::array<FleetVisualEntry,StartGridCapacity> Cars{};
};

inline FleetVisualPlan BuildFleetVisualPlan(int playerVehicle,GarageLook playerLook) noexcept
{
    static_assert(VehicleCount==StartGridCapacity);
    static_assert(GaragePaintOptions>=StartGridCapacity);
    FleetVisualPlan plan;
    if (!GarageState::IsVehicle(playerVehicle) || !ValidGarageLook(playerLook))
        return plan;
    plan.Cars[0]={HumanGridSlot,playerVehicle,playerLook};
    for(int slot=1;slot<StartGridCapacity;++slot)
    {
        const int vehicle=(playerVehicle+slot)%VehicleCount;
        const auto paint=static_cast<std::uint8_t>((playerLook.Paint+slot)%GaragePaintOptions);
        const auto stripe=static_cast<std::uint8_t>((slot+1)%GarageStripeOptions);
        const auto wheels=static_cast<std::uint8_t>((slot+2)%GarageWheelOptions);
        plan.Cars[static_cast<std::size_t>(slot)]={slot,vehicle,{paint,stripe,wheels}};
    }
    plan.Valid=true;
    return plan;
}

inline bool ValidFleetVisualPlan(const FleetVisualPlan& plan) noexcept
{
    if (!plan.Valid) return false;
    for(int slot=0;slot<StartGridCapacity;++slot)
    {
        const auto& entry=plan.Cars[static_cast<std::size_t>(slot)];
        if (entry.Slot!=slot || !GarageState::IsVehicle(entry.VehicleId) ||
            !ValidGarageLook(entry.Look))
            return false;
        for(int other=0;other<slot;++other)
        {
            const auto& prior=plan.Cars[static_cast<std::size_t>(other)];
            if (prior.VehicleId==entry.VehicleId || prior.Look.Paint==entry.Look.Paint)
                return false;
        }
    }
    return true;
}
} // namespace latebrake
