#pragma once
#include "Racecraft/RacecraftMath.h"
#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
inline constexpr int PhysicalFleetCapacity = 6;
inline constexpr int PhysicalWheelCount = 4;
inline constexpr double MaximumFrameTimeSeconds = 0.10;
inline constexpr double MaximumVerticalTravelCm = 6000.0;

enum class RacePhase : std::uint8_t
{
    Dormant, Grid, Countdown, Green, Caution, Paused,
    RedFlag, Finished, Aborted
};
enum class ContactSurface : std::uint8_t
{
    Unknown, Asphalt, PaintedKerb, Grass, Gravel,
    WetAsphalt, StandingWater, Wall, PitConcrete
};
enum class DrivetrainLayout : std::uint8_t
{
    Unknown, FrontWheelDrive, RearWheelDrive, AllWheelDrive
};
enum class VehicleClass : std::uint8_t
{
    Unknown, Sports, Touring, Prototype, Rally, Formula
};
enum class TireCompound : std::uint8_t
{
    Unknown, Hard, Medium, Soft, Intermediate, Wet
};
enum class CarHealth : std::uint8_t
{
    Unknown, Qualified, Damaged, Unstable, Unsupported, Retired
};
enum class SnapshotFault : std::uint8_t
{
    None, Uninitialized, MissingSession, WrongPhase,
    DuplicateActor, StaleTime, InvalidPose, InvalidSpeed,
    InvalidControls, InvalidContact, UnsupportedGeometry,
    RaceClockMismatch, EnergyDiscontinuity
};

struct RaceIdentity final
{
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    std::uint64_t Frame = 0;
    std::uint32_t SessionRevision = 0;
    int Slot = -1;
    bool EngineObserved = false;
    bool BodySimulating = false;
    bool OriginalTrack = false;
};
struct WheelObservation final
{
    Vec3 ContactWorldCm{};
    Vec3 ContactNormal{0.0, 0.0, 1.0};
    double SuspensionCompressionCm = 0.0;
    double SuspensionTravelCm = 25.0;
    double AngularVelocityRadS = 0.0;
    double RadiusCm = 34.0;
    double NormalLoadN = 0.0;
    double SlipRatio = 0.0;
    double SlipAngleRadians = 0.0;
    double TemperatureC = 90.0;
    double WearFraction = 0.0;
    double PressureKpa = 200.0;
    double SurfaceGrip = 1.0;
    ContactSurface Surface = ContactSurface::Unknown;
    bool PhysicalContact = false;
    bool ContactVerified = false;
    bool Driven = false;
    bool Steerable = false;
};
struct ChassisObservation final
{
    Pose2 PlanarPose{};
    Vec3 WorldPositionCm{};
    Vec3 VelocityCmS{};
    Vec3 AccelerationCmS2{};
    Vec3 AngularVelocityRadS{};
    double PitchRadians = 0.0;
    double RollRadians = 0.0;
    double MassKg = 1250.0;
    double YawInertiaKgM2 = 1800.0;
    double FrontTrackCm = 160.0;
    double RearTrackCm = 160.0;
    double WheelbaseCm = 270.0;
    double CenterOfMassHeightCm = 55.0;
    double FrontStaticLoadFraction = 0.53;
    double ChassisDamageFraction = 0.0;
    double AeroDamageFraction = 0.0;
    double RideHeightCm = 12.0;
    CarHealth Health = CarHealth::Unknown;
};
struct DriverControls final
{
    double Throttle = 0.0;
    double Brake = 0.0;
    double Steering = 0.0;
    double Handbrake = 0.0;
    double Clutch = 0.0;
    int GearRequest = 0;
    bool NitroRequest = false;
    bool RecoveryRequest = false;
    bool PitRequest = false;
    bool HumanControlled = false;
};
struct PowertrainObservation final
{
    double EngineRpm = 0.0;
    double EngineTorqueNm = 0.0;
    double WheelTorqueNm = 0.0;
    double GearRatio = 0.0;
    double FinalDriveRatio = 0.0;
    double ClutchEngagement = 1.0;
    double EngineTemperatureC = 90.0;
    double GearboxTemperatureC = 80.0;
    double FuelFraction = 1.0;
    double BatteryFraction = 1.0;
    double MotorPowerKw = 0.0;
    double SpeedLimiterCmS = MaximumPhysicalSpeedCmS;
    int EngagedGear = 0;
    DrivetrainLayout Layout = DrivetrainLayout::Unknown;
    bool EngineRunning = false;
    bool GearPhysicallyEngaged = false;
};
struct TrackObservation final
{
    double DistanceAlongLapCm = 0.0;
    double UnwrappedDistanceCm = 0.0;
    double LateralDeviationCm = 0.0;
    double RoadHalfWidthCm = 800.0;
    double SurfaceFriction = 1.0;
    double WetnessFraction = 0.0;
    double BankAngleRadians = 0.0;
    double SlopeRadians = 0.0;
    double CurvaturePerCm = 0.0;
    double VisibilityFraction = 1.0;
    int NextGate = 0;
    int LapIndex = 0;
    int SectorIndex = 0;
    bool OnOriginalPavement = false;
    bool PhysicalWallClearance = false;
    bool ForwardGateQualified = false;
};
struct RaceContext final
{
    RacePhase Phase = RacePhase::Dormant;
    double ClockSeconds = 0.0;
    double SessionSeconds = 0.0;
    double FrameDeltaSeconds = 0.0;
    double StartLightSeconds = 0.0;
    int RequiredLaps = 3;
    int CompletedLaps = 0;
    int RequiredGateCount = 4;
    bool EngineGameModeReady = false;
    bool TimerAuthoritative = false;
    bool ResultFinalized = false;
};
struct PhysicalVehicleFrame final
{
    RaceIdentity Identity{};
    ChassisObservation Chassis{};
    std::array<WheelObservation, PhysicalWheelCount> Wheels{};
    DriverControls Controls{};
    PowertrainObservation Powertrain{};
    TrackObservation Track{};
    RaceContext Race{};
    double TimeSeconds = 0.0;
    bool PhysicsQualified = false;
    bool ChassisOnWorldStatic = false;
};
struct FleetFrame final
{
    std::array<PhysicalVehicleFrame, PhysicalFleetCapacity> Members{};
    int Count = 0;
    std::uint64_t Session = 0;
    std::uint32_t Revision = 0;
    double TimeSeconds = 0.0;
    bool EngineRosterVerified = false;
    bool OriginalMapVerified = false;
};
struct BoundedCommand final
{
    double ThrottleCeiling = 1.0;
    double BrakeFloor = 0.0;
    double SteeringCeiling = 1.0;
    double SpeedCeilingCmS = MaximumPhysicalSpeedCmS;
    bool RequireStop = false;
    bool Valid = false;
    bool RestrictionOnly = true;
};
struct PhysicalGuardReceipt final
{
    SnapshotFault Fault = SnapshotFault::Uninitialized;
    int Slot = -1;
    int OtherSlot = -1;
    std::uint64_t AcceptedFrame = 0;
    double TimeSeconds = 0.0;
    double ObservedSpeedCmS = 0.0;
    double SafeSpeedCeilingCmS = 0.0;
    bool Trustworthy = false;
    bool EverChangedPlayerControls = false;
    bool EverChangedOfficialClock = false;
};
inline bool ValidFrameDelta(double seconds) noexcept
{
    return Finite(seconds) && seconds > 0.0 &&
        seconds <= MaximumFrameTimeSeconds;
}
inline bool PhysicalActorIdentityValid(RaceIdentity id) noexcept
{
    return id.Session != 0 && id.Actor != 0 &&
        id.Slot >= 0 && id.Slot < PhysicalFleetCapacity &&
        id.EngineObserved && id.BodySimulating && id.OriginalTrack;
}
inline bool ValidDriverControls(DriverControls c) noexcept
{
    return Finite(c.Throttle) && c.Throttle >= 0.0 &&
        c.Throttle <= 1.0 && Finite(c.Brake) &&
        c.Brake >= 0.0 && c.Brake <= 1.0 &&
        Finite(c.Steering) && c.Steering >= -1.0 &&
        c.Steering <= 1.0 && Finite(c.Handbrake) &&
        c.Handbrake >= 0.0 && c.Handbrake <= 1.0 &&
        Finite(c.Clutch) && c.Clutch >= 0.0 &&
        c.Clutch <= 1.0 && c.GearRequest >= -1 &&
        c.GearRequest <= 12;
}
inline bool ValidChassisGeometry(ChassisObservation c) noexcept
{
    return Finite(c.PlanarPose) && Finite(c.WorldPositionCm) &&
        Finite(c.VelocityCmS) && Finite(c.AccelerationCmS2) &&
        Finite(c.AngularVelocityRadS) && Finite(c.PitchRadians) &&
        Finite(c.RollRadians) && Finite(c.MassKg) &&
        c.MassKg >= 200.0 && c.MassKg <= 12000.0 &&
        Finite(c.YawInertiaKgM2) && c.YawInertiaKgM2 > 0.0 &&
        Finite(c.FrontTrackCm) && c.FrontTrackCm > 80.0 &&
        Finite(c.RearTrackCm) && c.RearTrackCm > 80.0 &&
        Finite(c.WheelbaseCm) && c.WheelbaseCm > 120.0 &&
        Finite(c.CenterOfMassHeightCm) &&
        c.CenterOfMassHeightCm >= 5.0 &&
        c.CenterOfMassHeightCm < c.WheelbaseCm &&
        Finite(c.FrontStaticLoadFraction) &&
        c.FrontStaticLoadFraction > 0.2 &&
        c.FrontStaticLoadFraction < 0.8 &&
        Finite(c.ChassisDamageFraction) &&
        c.ChassisDamageFraction >= 0.0 &&
        c.ChassisDamageFraction <= 1.0 &&
        Finite(c.AeroDamageFraction) &&
        c.AeroDamageFraction >= 0.0 &&
        c.AeroDamageFraction <= 1.0 &&
        Length(c.VelocityCmS) <= MaximumPhysicalSpeedCmS &&
        Length(c.AccelerationCmS2) <= 10000.0;
}
inline bool ValidWheelObservation(WheelObservation w) noexcept
{
    return Finite(w.ContactWorldCm) &&
        Finite(w.ContactNormal) &&
        Finite(w.SuspensionCompressionCm) &&
        Finite(w.SuspensionTravelCm) &&
        w.SuspensionTravelCm > 0.0 &&
        w.SuspensionTravelCm < 100.0 &&
        Finite(w.AngularVelocityRadS) &&
        std::abs(w.AngularVelocityRadS) < 2000.0 &&
        Finite(w.RadiusCm) && w.RadiusCm > 10.0 &&
        w.RadiusCm < 100.0 &&
        Finite(w.NormalLoadN) &&
        w.NormalLoadN >= 0.0 && w.NormalLoadN <= 300000.0 &&
        Finite(w.SlipRatio) && std::abs(w.SlipRatio) <= 10.0 &&
        Finite(w.SlipAngleRadians) &&
        std::abs(w.SlipAngleRadians) <= Pi &&
        Finite(w.TemperatureC) && w.TemperatureC >= -80.0 &&
        w.TemperatureC <= 350.0 &&
        Finite(w.WearFraction) && w.WearFraction >= 0.0 &&
        w.WearFraction <= 1.0 &&
        Finite(w.PressureKpa) && w.PressureKpa >= 0.0 &&
        w.PressureKpa < 1000.0 &&
        Finite(w.SurfaceGrip) && w.SurfaceGrip >= 0.0 &&
        w.SurfaceGrip <= 4.0;
}
inline bool ValidTrackObservation(TrackObservation t) noexcept
{
    return Finite(t.DistanceAlongLapCm) &&
        Finite(t.UnwrappedDistanceCm) &&
        Finite(t.LateralDeviationCm) &&
        Finite(t.RoadHalfWidthCm) && t.RoadHalfWidthCm > 0.0 &&
        Finite(t.SurfaceFriction) && t.SurfaceFriction >= 0.0 &&
        t.SurfaceFriction <= 4.0 && Finite(t.WetnessFraction) &&
        t.WetnessFraction >= 0.0 &&
        t.WetnessFraction <= 1.0 &&
        Finite(t.BankAngleRadians) &&
        std::abs(t.BankAngleRadians) <= Pi * 0.5 &&
        Finite(t.SlopeRadians) &&
        std::abs(t.SlopeRadians) <= Pi * 0.5 &&
        Finite(t.CurvaturePerCm) &&
        std::abs(t.CurvaturePerCm) < 0.10 &&
        Finite(t.VisibilityFraction) &&
        t.VisibilityFraction >= 0.0 &&
        t.VisibilityFraction <= 1.0 &&
        t.LapIndex >= 0 && t.LapIndex <= 100000 &&
        t.NextGate >= 0 && t.NextGate < 256 &&
        t.SectorIndex >= 0 && t.SectorIndex < 256;
}
inline bool ValidPowertrainObservation(PowertrainObservation p) noexcept
{
    return Finite(p.EngineRpm) && p.EngineRpm >= 0.0 &&
        p.EngineRpm <= 30000.0 &&
        Finite(p.EngineTorqueNm) &&
        std::abs(p.EngineTorqueNm) <= 20000.0 &&
        Finite(p.WheelTorqueNm) &&
        std::abs(p.WheelTorqueNm) <= 200000.0 &&
        Finite(p.EngineTemperatureC) &&
        p.EngineTemperatureC >= -80.0 &&
        p.EngineTemperatureC <= 1200.0 &&
        Finite(p.GearboxTemperatureC) &&
        p.GearboxTemperatureC >= -80.0 &&
        p.GearboxTemperatureC <= 1200.0 &&
        Finite(p.FuelFraction) && p.FuelFraction >= 0.0 &&
        p.FuelFraction <= 1.0 && Finite(p.BatteryFraction) &&
        p.BatteryFraction >= 0.0 && p.BatteryFraction <= 1.0 &&
        Finite(p.ClutchEngagement) &&
        p.ClutchEngagement >= 0.0 &&
        p.ClutchEngagement <= 1.0 &&
        p.EngagedGear >= -1 && p.EngagedGear <= 12;
}
inline bool ValidRaceContext(RaceContext r) noexcept
{
    return Finite(r.ClockSeconds) && r.ClockSeconds >= 0.0 &&
        Finite(r.SessionSeconds) && r.SessionSeconds >= 0.0 &&
        Finite(r.FrameDeltaSeconds) &&
        r.FrameDeltaSeconds >= 0.0 &&
        r.FrameDeltaSeconds <= MaximumFrameTimeSeconds &&
        Finite(r.StartLightSeconds) &&
        r.RequiredLaps >= 1 && r.RequiredLaps <= 500 &&
        r.CompletedLaps >= 0 &&
        r.CompletedLaps <= r.RequiredLaps &&
        r.RequiredGateCount >= 2 && r.RequiredGateCount <= 256;
}
inline SnapshotFault ValidateVehicleFrame(
    PhysicalVehicleFrame f, bool requireAllWheels = false) noexcept
{
    if (!f.PhysicsQualified ||
        !f.ChassisOnWorldStatic ||
        !f.Race.EngineGameModeReady)
        return SnapshotFault::Uninitialized;
    if (!PhysicalActorIdentityValid(f.Identity))
        return SnapshotFault::MissingSession;
    if (f.Race.Phase != RacePhase::Green &&
        f.Race.Phase != RacePhase::Caution)
        return SnapshotFault::WrongPhase;
    if (!Finite(f.TimeSeconds) || f.TimeSeconds < 0.0 ||
        !ValidRaceContext(f.Race))
        return SnapshotFault::StaleTime;
    if (!ValidChassisGeometry(f.Chassis))
        return SnapshotFault::InvalidPose;
    if (!ValidDriverControls(f.Controls))
        return SnapshotFault::InvalidControls;
    if (!ValidPowertrainObservation(f.Powertrain))
        return SnapshotFault::EnergyDiscontinuity;
    if (!ValidTrackObservation(f.Track))
        return SnapshotFault::UnsupportedGeometry;
    for (WheelObservation wheel : f.Wheels)
    {
        if (!ValidWheelObservation(wheel))
            return SnapshotFault::InvalidContact;
        if (requireAllWheels &&
            (!wheel.ContactVerified || !wheel.PhysicalContact ||
             wheel.NormalLoadN <= 1.0))
            return SnapshotFault::InvalidContact;
    }
    return SnapshotFault::None;
}
inline SnapshotFault ValidateFleetFrame(
    const FleetFrame& f, bool requireSix = true) noexcept
{
    if (!f.EngineRosterVerified || !f.OriginalMapVerified ||
        !Finite(f.TimeSeconds)) return SnapshotFault::Uninitialized;
    if (f.Count <= 0 || f.Count > PhysicalFleetCapacity ||
        (requireSix && f.Count != PhysicalFleetCapacity))
        return SnapshotFault::MissingSession;
    for (int i = 0; i < f.Count; ++i)
    {
        const PhysicalVehicleFrame& a =
            f.Members[static_cast<std::size_t>(i)];
        if (a.Identity.Session != f.Session ||
            a.Identity.SessionRevision != f.Revision ||
            a.Identity.Slot != i)
            return SnapshotFault::MissingSession;
        if (ValidateVehicleFrame(a) != SnapshotFault::None)
            return SnapshotFault::InvalidPose;
        if (std::abs(a.TimeSeconds - f.TimeSeconds) > 0.10)
            return SnapshotFault::StaleTime;
        for (int j = 0; j < i; ++j)
        {
            const PhysicalVehicleFrame& b =
                f.Members[static_cast<std::size_t>(j)];
            if (a.Identity.Actor == b.Identity.Actor ||
                a.Identity.Frame != b.Identity.Frame)
                return SnapshotFault::DuplicateActor;
        }
    }
    return SnapshotFault::None;
}
inline BoundedCommand SafelyRestrict(
    BoundedCommand first, BoundedCommand second) noexcept
{
    BoundedCommand out{};
    if (!first.Valid || !second.Valid) return out;
    out.ThrottleCeiling = std::min(
        first.ThrottleCeiling, second.ThrottleCeiling);
    out.BrakeFloor = std::max(
        first.BrakeFloor, second.BrakeFloor);
    out.SteeringCeiling = std::min(
        first.SteeringCeiling, second.SteeringCeiling);
    out.SpeedCeilingCmS = std::min(
        first.SpeedCeilingCmS, second.SpeedCeilingCmS);
    out.RequireStop = first.RequireStop || second.RequireStop;
    out.Valid = Finite(out.ThrottleCeiling) &&
        Finite(out.BrakeFloor) && Finite(out.SteeringCeiling) &&
        out.ThrottleCeiling >= 0.0 &&
        out.ThrottleCeiling <= 1.0 &&
        out.BrakeFloor >= 0.0 && out.BrakeFloor <= 1.0 &&
        out.SteeringCeiling >= 0.0 &&
        out.SteeringCeiling <= 1.0;
    out.RestrictionOnly = true;
    if (out.RequireStop)
    {
        out.ThrottleCeiling = 0.0;
        out.BrakeFloor = 1.0;
    }
    return out;
}
} // namespace latebrake::racecraft
