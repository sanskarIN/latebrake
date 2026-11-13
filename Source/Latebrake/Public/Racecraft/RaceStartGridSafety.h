#pragma once
#include "Racecraft/CircuitGeometry.h"
#include "Racecraft/RacecraftFrames.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake::racecraft
{
inline constexpr std::array<Vec2, PhysicalFleetCapacity>
    OriginalSixGridCenters = {{
        {-2550.0, -1800.0},
        {-2250.0, -1450.0},
        {-1850.0, -1800.0},
        {-1500.0, -1450.0},
        {-1100.0, -1800.0},
        {-750.0, -1450.0}
    }};
struct StartingGridSafetyParameters final
{
    double MaxPositionDisplacementCm = 100.0;
    double MaxHeadingDisplacementRadians = 0.20;
    double MaximumPlanarSpeedCmS = 120.0;
    double MaximumVerticalSpeedCmS = 55.0;
    double MaximumChassisHeightErrorCm = 45.0;
    double MinimumClearanceBetweenBodiesCm = 40.0;
    double MaximumPhysicalSizeXcm = 240.0;
    double MaximumPhysicalSizeYcm = 125.0;
    double MaximumArrivalTimeOffsetSeconds = 0.08;
    double MaximumCountdownTimeGapSeconds = 0.12;
    int MinimumSupportingWheelContacts = 3;
    bool RejectDifferentOriginalMap = true;
    bool RequireAllSixSlots = true;
    bool RequireVerifiedZContact = true;
    bool RequireExactActorIdentity = true;
};
enum class StartingGridFault : std::uint8_t
{
    None, MissingCar, DuplicateActor, WrongSlot,
    WrongSession, IncorrectPosition, IncorrectHeading,
    ExcessVelocity, ExcessVerticalMotion, MissingContact,
    BodyOverlapping, OffPavement, DifferentObservationTime,
    InvalidPhysics, InvalidChassisHeight
};
struct GridActorObservation final
{
    RaceIdentity Identity{};
    Vec3 WorldPositionCm{};
    Vec3 VelocityCmS{};
    double HeadingRadians = 0.0;
    double ChassisHalfLengthCm = 205.0;
    double ChassisHalfWidthCm = 91.0;
    double ChassisHalfHeightCm = 58.0;
    double ExpectedGroundTopCm = 75.0;
    double TimeSeconds = 0.0;
    std::array<bool, 4> PhysicalWheelContact{};
    int GroundTraceValidHits = 0;
    bool EngineBodySimulating = false;
    bool WorldStaticBelowBody = false;
    bool NoWallOverlap = false;
};
struct StartingGridReceipt final
{
    StartingGridFault Fault = StartingGridFault::MissingCar;
    int FaultSlot = -1;
    int OtherSlot = -1;
    int AcceptedCars = 0;
    int VerifiedContactWheels = 0;
    double MaximumPositionErrorCm = 0.0;
    double MaximumVerticalErrorCm = 0.0;
    double MaximumPlanarVelocityCmS = 0.0;
    double MinimumGapBetweenBodiesCm = 0.0;
    double MaximumSnapshotTimeDeltaSeconds = 0.0;
    std::array<std::uint64_t, PhysicalFleetCapacity> ActorRoster{};
    bool EveryOriginalPavementBody = false;
    bool EveryWorldStaticContactValid = false;
    bool AllSixCarsReal = false;
    bool SafeToBeginCountdown = false;
    bool AuthenticChaosStartGate = false;
};
inline bool ValidStartingGridParameters(
    StartingGridSafetyParameters s) noexcept
{
    return Finite(s.MaxPositionDisplacementCm) &&
        s.MaxPositionDisplacementCm > 0.0 &&
        Finite(s.MaxHeadingDisplacementRadians) &&
        s.MaxHeadingDisplacementRadians > 0.0 &&
        Finite(s.MaximumPlanarSpeedCmS) &&
        s.MaximumPlanarSpeedCmS > 0.0 &&
        Finite(s.MaximumVerticalSpeedCmS) &&
        s.MaximumVerticalSpeedCmS > 0.0 &&
        Finite(s.MaximumChassisHeightErrorCm) &&
        s.MaximumChassisHeightErrorCm > 0.0 &&
        Finite(s.MinimumClearanceBetweenBodiesCm) &&
        s.MinimumClearanceBetweenBodiesCm >= 0.0 &&
        Finite(s.MaximumPhysicalSizeXcm) &&
        s.MaximumPhysicalSizeXcm > 0.0 &&
        Finite(s.MaximumPhysicalSizeYcm) &&
        s.MaximumPhysicalSizeYcm > 0.0 &&
        Finite(s.MaximumArrivalTimeOffsetSeconds) &&
        s.MaximumArrivalTimeOffsetSeconds >= 0.0 &&
        s.MinimumSupportingWheelContacts >= 2 &&
        s.MinimumSupportingWheelContacts <= 4;
}
inline bool ValidGridActor(
    GridActorObservation a,
    StartingGridSafetyParameters settings) noexcept
{
    return PhysicalActorIdentityValid(a.Identity) &&
        a.EngineBodySimulating &&
        a.WorldStaticBelowBody &&
        a.NoWallOverlap &&
        Finite(a.WorldPositionCm) &&
        Finite(a.VelocityCmS) &&
        Finite(a.HeadingRadians) &&
        Finite(a.ChassisHalfLengthCm) &&
        Finite(a.ChassisHalfWidthCm) &&
        Finite(a.ChassisHalfHeightCm) &&
        Finite(a.ExpectedGroundTopCm) &&
        Finite(a.TimeSeconds) &&
        a.TimeSeconds >= 0.0 &&
        a.ChassisHalfLengthCm > 0.0 &&
        a.ChassisHalfLengthCm <= settings.MaximumPhysicalSizeXcm &&
        a.ChassisHalfWidthCm > 0.0 &&
        a.ChassisHalfWidthCm <= settings.MaximumPhysicalSizeYcm &&
        a.ChassisHalfHeightCm > 0.0 &&
        std::abs(a.VelocityCmS.Z) <=
            settings.MaximumVerticalSpeedCmS &&
        std::abs(a.WorldPositionCm.Z -
            (a.ExpectedGroundTopCm +
             a.ChassisHalfHeightCm)) <=
            settings.MaximumChassisHeightErrorCm &&
        a.GroundTraceValidHits >=
            settings.MinimumSupportingWheelContacts;
}
inline StartingGridReceipt ValidateOriginalSixCarGrid(
    const std::array<GridActorObservation, PhysicalFleetCapacity>& cars,
    std::uint64_t session,
    std::uint32_t revision,
    StartingGridSafetyParameters settings = {}) noexcept
{
    StartingGridReceipt out;
    out.MinimumGapBetweenBodiesCm =
        std::numeric_limits<double>::infinity();
    if (!ValidStartingGridParameters(settings) ||
        session == 0)
    {
        out.Fault = StartingGridFault::InvalidPhysics;
        return out;
    }
    const double beginning = cars[0].TimeSeconds;
    for (int i = 0; i < PhysicalFleetCapacity; ++i)
    {
        const GridActorObservation& car =
            cars[static_cast<std::size_t>(i)];
        out.FaultSlot = i;
        if (!ValidGridActor(car, settings))
        {
            out.Fault = StartingGridFault::InvalidPhysics;
            return out;
        }
        if (car.Identity.Session != session ||
            car.Identity.SessionRevision != revision)
        {
            out.Fault = StartingGridFault::WrongSession;
            return out;
        }
        if (car.Identity.Slot != i)
        {
            out.Fault = StartingGridFault::WrongSlot;
            return out;
        }
        for (int j = 0; j < i; ++j)
        {
            if (cars[static_cast<std::size_t>(j)].Identity.Actor ==
                car.Identity.Actor)
            {
                out.Fault = StartingGridFault::DuplicateActor;
                out.OtherSlot = j;
                return out;
            }
        }
        const Vec2 expected =
            OriginalSixGridCenters[static_cast<std::size_t>(i)];
        const Vec2 actual{
            car.WorldPositionCm.X,
            car.WorldPositionCm.Y
        };
        const double error = Distance(actual, expected);
        out.MaximumPositionErrorCm =
            std::max(out.MaximumPositionErrorCm, error);
        if (error > settings.MaxPositionDisplacementCm)
        {
            out.Fault = StartingGridFault::IncorrectPosition;
            return out;
        }
        const double errorHeading =
            std::abs(WrappedAngle(car.HeadingRadians));
        if (errorHeading >
            settings.MaxHeadingDisplacementRadians)
        {
            out.Fault = StartingGridFault::IncorrectHeading;
            return out;
        }
        const double speed = Length(Vec2{
            car.VelocityCmS.X,
            car.VelocityCmS.Y
        });
        out.MaximumPlanarVelocityCmS =
            std::max(out.MaximumPlanarVelocityCmS, speed);
        if (speed > settings.MaximumPlanarSpeedCmS)
        {
            out.Fault = StartingGridFault::ExcessVelocity;
            return out;
        }
        const double zError = std::abs(
            car.WorldPositionCm.Z -
            (car.ExpectedGroundTopCm +
             car.ChassisHalfHeightCm));
        out.MaximumVerticalErrorCm =
            std::max(out.MaximumVerticalErrorCm, zError);
        if (zError > settings.MaximumChassisHeightErrorCm)
        {
            out.Fault = StartingGridFault::InvalidChassisHeight;
            return out;
        }
        out.MaximumSnapshotTimeDeltaSeconds = std::max(
            out.MaximumSnapshotTimeDeltaSeconds,
            std::abs(car.TimeSeconds - beginning));
        if (out.MaximumSnapshotTimeDeltaSeconds >
            settings.MaximumArrivalTimeOffsetSeconds)
        {
            out.Fault = StartingGridFault::DifferentObservationTime;
            return out;
        }
        const Rect2 rect{actual,
            {car.ChassisHalfLengthCm,
             car.ChassisHalfWidthCm},
            car.HeadingRadians};
        if (!AnyOriginalSlabContains(rect))
        {
            out.Fault = StartingGridFault::OffPavement;
            return out;
        }
        for (int j = 0; j < i; ++j)
        {
            const GridActorObservation& prior =
                cars[static_cast<std::size_t>(j)];
            const Rect2 other{
                {prior.WorldPositionCm.X,
                 prior.WorldPositionCm.Y},
                {prior.ChassisHalfLengthCm,
                 prior.ChassisHalfWidthCm},
                prior.HeadingRadians
            };
            if (RectsOverlap(rect, other,
                settings.MinimumClearanceBetweenBodiesCm))
            {
                out.Fault = StartingGridFault::BodyOverlapping;
                out.OtherSlot = j;
                return out;
            }
            const double gap = Distance(
                rect.Center, other.Center) -
                (rect.HalfSize.X + other.HalfSize.X);
            out.MinimumGapBetweenBodiesCm = std::min(
                out.MinimumGapBetweenBodiesCm, gap);
        }
        out.ActorRoster[static_cast<std::size_t>(i)] =
            car.Identity.Actor;
        for (bool touching : car.PhysicalWheelContact)
            out.VerifiedContactWheels += touching ? 1 : 0;
        ++out.AcceptedCars;
    }
    out.Fault = StartingGridFault::None;
    out.FaultSlot = -1;
    out.AllSixCarsReal = true;
    out.EveryOriginalPavementBody = true;
    out.EveryWorldStaticContactValid = true;
    out.SafeToBeginCountdown = true;
    out.AuthenticChaosStartGate = false;
    return out;
}
class GridRosterGate final
{
public:
    void Reset() noexcept
    {
        Roster.fill(0);
        HasStableRoster = false;
        StableFrames = 0;
        Rejected = 0;
        Resets = 0;
        Last = {};
    }
    StartingGridReceipt Observe(
        const std::array<GridActorObservation, PhysicalFleetCapacity>& cars,
        std::uint64_t session,
        std::uint32_t revision) noexcept
    {
        Last = ValidateOriginalSixCarGrid(cars, session,
                                          revision, Settings);
        if (!Last.SafeToBeginCountdown)
        {
            ++Rejected;
            if (HasStableRoster) ++Resets;
            HasStableRoster = false;
            StableFrames = 0;
            return Last;
        }
        if (HasStableRoster && Last.ActorRoster != Roster)
        {
            ++Resets;
            StableFrames = 0;
        }
        Roster = Last.ActorRoster;
        HasStableRoster = true;
        ++StableFrames;
        return Last;
    }
    bool Configure(StartingGridSafetyParameters settings) noexcept
    {
        if (!ValidStartingGridParameters(settings)) return false;
        Settings = settings;
        Reset();
        return true;
    }
    bool ReadyForLights(int requiredStableFrames = 3) const noexcept
    {
        return HasStableRoster &&
            StableFrames >= requiredStableFrames;
    }
    StartingGridReceipt Snapshot() const noexcept { return Last; }
    std::uint64_t BadSnapshots() const noexcept { return Rejected; }
    std::uint64_t ReplacedRosters() const noexcept { return Resets; }
private:
    StartingGridSafetyParameters Settings{};
    std::array<std::uint64_t, PhysicalFleetCapacity> Roster{};
    StartingGridReceipt Last{};
    int StableFrames = 0;
    std::uint64_t Rejected = 0;
    std::uint64_t Resets = 0;
    bool HasStableRoster = false;
};
} // namespace latebrake::racecraft
