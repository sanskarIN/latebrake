#pragma once

#include "Telemetry/TelemetryMath.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
inline constexpr int MaximumTelemetryDrivers = 6;
inline constexpr int MaximumTelemetrySectors = 32;
inline constexpr int MaximumTelemetryLaps = 128;
inline constexpr int MaximumTelemetryEvents = 256;
inline constexpr int MaximumTelemetryFrames = 4096;
inline constexpr int MaximumTelemetryTrackNodes = 128;
inline constexpr double MaximumTelemetrySeconds = 604800.0;
inline constexpr double MaximumFrameDeltaSeconds = 0.10;
inline constexpr double MaximumPlanarSpeedCmS = 16000.0;
inline constexpr double MaximumWorldMagnitudeCm = 20000000.0;

enum class TelemetryTrust : std::uint8_t
{
    Missing,
    Synthetic,
    SourceOnly,
    EngineObserved
};

enum class TelemetryVehicleClass : std::uint8_t
{
    Unknown,
    Road,
    Sprint,
    Touring,
    Rally,
    Prototype
};

enum class TelemetrySurface : std::uint8_t
{
    Unknown,
    Asphalt,
    Kerb,
    Gravel,
    Grass,
    Dirt,
    Water
};

enum class TelemetryPhase : std::uint8_t
{
    PreGrid,
    Countdown,
    Racing,
    Caution,
    Paused,
    Finished,
    Aborted
};

enum class TelemetryEventKind : std::uint8_t
{
    Unknown,
    LapStart,
    SectorSplit,
    LapComplete,
    BrakeApplication,
    BrakeRelease,
    TractionLimit,
    WheelSlip,
    CornerEntry,
    Apex,
    CornerExit,
    Overtake,
    PositionLost,
    OffTrack,
    WallContact,
    CarContact,
    Recovery,
    RaceInterrupted,
    SensorDiscontinuity,
    ObservationRejected,
    RiskBrake,
    FuelLow,
    TireCritical,
    EngineOverheat
};

enum class TelemetryQuality : std::uint8_t
{
    Missing,
    Invalid,
    Suspect,
    Qualified
};

enum class TelemetryReject : std::uint8_t
{
    None,
    MissingPhysics,
    WrongSession,
    WrongDriver,
    InvalidClock,
    ClockReversed,
    InvalidPosition,
    InvalidVelocity,
    InvalidAttitude,
    InvalidControls,
    InvalidDynamics,
    ImpossibleJump,
    InvalidPhase,
    InvalidWheelReadout,
    InvalidDamage,
    InvalidEnvironment,
    StaleObservation,
    CapacityExceeded
};

struct TelemetryControls final
{
    double Throttle = 0.0;
    double Brake = 0.0;
    double Steering = 0.0;
    double Handbrake = 0.0;
    double Clutch = 0.0;
    int Gear = 0;
    bool ShiftUp = false;
    bool ShiftDown = false;
    bool Boost = false;
    bool Autonomous = false;
};

struct TelemetryWheels final
{
    std::array<double, 4> AngularSpeedRadS{};
    std::array<double, 4> NormalLoadNewtons{};
    std::array<double, 4> SlipRatio{};
    std::array<double, 4> SlipAngleRadians{};
    std::array<double, 4> TireTemperatureCelsius{};
    std::array<double, 4> TireWearFraction{};
    std::array<bool, 4> Contact{};
};

struct TelemetryDynamics final
{
    Vector3 WorldPositionCm{};
    Vector3 WorldVelocityCmS{};
    Vector3 WorldAccelerationCmS2{};
    Vector3 AngularVelocityRadS{};
    double YawRadians = 0.0;
    double PitchRadians = 0.0;
    double RollRadians = 0.0;
    double EngineRpm = 0.0;
    double MotorTorqueNm = 0.0;
    double BrakeTemperatureCelsius = 0.0;
    double EngineTemperatureCelsius = 0.0;
    double FuelFraction = 1.0;
    double ChassisDamageFraction = 0.0;
    double GripScale = 1.0;
    double RoadSlopeRadians = 0.0;
    double SuspensionCompressionCm = 0.0;
    double RideHeightCm = 0.0;
};

struct TelemetryEnvironment final
{
    TelemetrySurface Surface = TelemetrySurface::Unknown;
    double AirTemperatureCelsius = 20.0;
    double TrackTemperatureCelsius = 25.0;
    double AmbientWetness = 0.0;
    double WindSpeedCmS = 0.0;
    double WindDirectionRadians = 0.0;
    double VisibilityFraction = 1.0;
    double RoadFrictionEstimate = 1.0;
    bool OnPhysicalPavement = false;
    bool InPitLane = false;
    bool CautionActive = false;
};

struct TelemetryIdentity final
{
    std::uint64_t SessionToken = 0;
    std::uint64_t ActorToken = 0;
    std::uint64_t FrameOrdinal = 0;
    int DriverSlot = -1;
    int SessionRevision = 0;
    TelemetryTrust Trust = TelemetryTrust::Missing;
    TelemetryVehicleClass VehicleClass = TelemetryVehicleClass::Unknown;
};

struct TelemetryRaceStatus final
{
    TelemetryPhase Phase = TelemetryPhase::PreGrid;
    int LapIndex = 0;
    int NextGateIndex = 0;
    int CreditedGateOrdinal = 0;
    int PositionOrdinal = 0;
    int SectorIndex = 0;
    double CurrentLapSeconds = 0.0;
    double LastLapSeconds = 0.0;
    double RemainingDistanceCm = 0.0;
    bool VerifiedForwardGate = false;
    bool VerifiedRecovery = false;
    bool EnginePhysicsReady = false;
    bool LocalRaceClockRunning = false;
    bool RaceResultOfficial = false;
};

struct TelemetryFrame final
{
    TelemetryIdentity Identity{};
    TelemetryDynamics Dynamics{};
    TelemetryWheels Wheels{};
    TelemetryControls Controls{};
    TelemetryEnvironment Environment{};
    TelemetryRaceStatus Race{};
    double MonotonicSeconds = 0.0;
    double DeltaSeconds = 0.0;
    double RaceClockSeconds = 0.0;
    double SpeedometerKmH = 0.0;
    double InputLatencySeconds = 0.0;
    std::uint32_t SourceFlags = 0;
};

struct TelemetryEvent final
{
    TelemetryEventKind Kind = TelemetryEventKind::Unknown;
    TelemetryQuality Quality = TelemetryQuality::Missing;
    double MonotonicSeconds = 0.0;
    double RaceClockSeconds = 0.0;
    double TrackDistanceCm = 0.0;
    double Magnitude = 0.0;
    double DurationSeconds = 0.0;
    int DriverSlot = -1;
    int SectorIndex = -1;
    int LapIndex = -1;
    int RelatedSlot = -1;
    std::uint64_t ActorToken = 0;
    std::uint64_t RelatedToken = 0;
    std::uint32_t Flags = 0;
};

struct TelemetryFrameDecision final
{
    TelemetryQuality Quality = TelemetryQuality::Missing;
    TelemetryReject Reason = TelemetryReject::MissingPhysics;
    bool Accepted = false;
    bool StartNewSegment = false;
    bool ResetFilters = false;
    double ValidatedDeltaSeconds = 0.0;
    double DistanceSincePreviousCm = 0.0;
    double YawDeltaRadians = 0.0;
};

struct TelemetryKinematics final
{
    Vector2 PositionCm{};
    Vector2 VelocityCmS{};
    Vector2 AccelerationCmS2{};
    Vector2 SmoothedVelocityCmS{};
    Vector2 SmoothedAccelerationCmS2{};
    double SpeedCmS = 0.0;
    double SpeedKmh = 0.0;
    double ForwardSpeedCmS = 0.0;
    double LateralSpeedCmS = 0.0;
    double LongitudinalAccelerationCmS2 = 0.0;
    double LateralAccelerationCmS2 = 0.0;
    double YawRadians = 0.0;
    double YawRateRadS = 0.0;
    double SlipAngleRadians = 0.0;
    double SlipAngleDegrees = 0.0;
    double EstimatedCurvature = 0.0;
    double JerkCmS3 = 0.0;
    double DistanceCm = 0.0;
    bool Valid = false;
};

struct TelemetryZone final
{
    double StartCm = 0.0;
    double EndCm = 0.0;
    double CenterCm = 0.0;
    double HalfWidthCm = 0.0;
    double PeakCurvature = 0.0;
    double TargetSpeedCmS = 0.0;
    int TrackSegment = -1;
    int ZoneId = -1;
    bool Valid = false;
};

struct TelemetrySector final
{
    int Index = -1;
    double StartCm = 0.0;
    double EndCm = 0.0;
    double LastSeconds = 0.0;
    double BestSeconds = 0.0;
    double WorstSeconds = 0.0;
    double MeanSeconds = 0.0;
    int TimedCount = 0;
    bool Completed = false;
};

struct TelemetryLap final
{
    int LapIndex = -1;
    double StartedAtSeconds = 0.0;
    double FinishedAtSeconds = 0.0;
    double DurationSeconds = 0.0;
    double DistanceCm = 0.0;
    double TopSpeedKmh = 0.0;
    double MeanSpeedKmh = 0.0;
    double TotalBrakingSeconds = 0.0;
    double TotalCoastingSeconds = 0.0;
    double TotalOffTrackSeconds = 0.0;
    double TotalHighSlipSeconds = 0.0;
    double TotalThrottleSeconds = 0.0;
    double TotalWheelspinSeconds = 0.0;
    double TotalTractionLimitedSeconds = 0.0;
    double TotalBrakeTemperatureLoad = 0.0;
    int BrakeApplications = 0;
    int CornerCount = 0;
    int HighSlipEvents = 0;
    int ContactCount = 0;
    int RecoveryCount = 0;
    int SectorCount = 0;
    TelemetryQuality Quality = TelemetryQuality::Missing;
    std::array<double, MaximumTelemetrySectors> SectorTimes{};
};

struct TelemetryWindow final
{
    double StartedSeconds = 0.0;
    double EndedSeconds = 0.0;
    double ElapsedSeconds = 0.0;
    double DistanceCm = 0.0;
    double MeanSpeedCmS = 0.0;
    double MaximumSpeedCmS = 0.0;
    double MaximumAccelerationG = 0.0;
    double MaximumBrakingG = 0.0;
    double MaximumLateralG = 0.0;
    std::uint64_t Frames = 0;
    bool Qualified = false;
};

struct TelemetryCounters final
{
    std::uint64_t Observed = 0;
    std::uint64_t Accepted = 0;
    std::uint64_t Rejected = 0;
    std::uint64_t EpochResets = 0;
    std::uint64_t ClockResets = 0;
    std::uint64_t SensorFaults = 0;
    std::uint64_t Discontinuities = 0;
    std::uint64_t InactiveFrames = 0;
    std::uint64_t CapacityEvents = 0;
    std::array<std::uint64_t, 18> RejectReasons{};
};

inline constexpr std::size_t TelemetryRejectIndex(
    TelemetryReject reason) noexcept
{
    return static_cast<std::size_t>(reason);
}

inline bool IsRaceMoving(TelemetryPhase phase) noexcept
{
    return phase == TelemetryPhase::Racing ||
           phase == TelemetryPhase::Caution;
}

inline bool IsRaceInactive(TelemetryPhase phase) noexcept
{
    return phase == TelemetryPhase::PreGrid ||
           phase == TelemetryPhase::Countdown ||
           phase == TelemetryPhase::Paused ||
           phase == TelemetryPhase::Finished ||
           phase == TelemetryPhase::Aborted;
}

inline bool IsEngineObservation(TelemetryTrust trust) noexcept
{
    return trust == TelemetryTrust::EngineObserved;
}

inline Vector2 PlanarPosition(const TelemetryFrame& frame) noexcept
{
    return {frame.Dynamics.WorldPositionCm.X,
            frame.Dynamics.WorldPositionCm.Y};
}

inline Vector2 PlanarVelocity(const TelemetryFrame& frame) noexcept
{
    return {frame.Dynamics.WorldVelocityCmS.X,
            frame.Dynamics.WorldVelocityCmS.Y};
}

inline Vector2 PlanarAcceleration(const TelemetryFrame& frame) noexcept
{
    return {frame.Dynamics.WorldAccelerationCmS2.X,
            frame.Dynamics.WorldAccelerationCmS2.Y};
}

inline double PlanarSpeed(const TelemetryFrame& frame) noexcept
{
    return Length(PlanarVelocity(frame));
}

inline double WheelMeanTemperature(const TelemetryWheels& wheels) noexcept
{
    double total = 0.0;
    for (double temperature : wheels.TireTemperatureCelsius)
    {
        total += temperature;
    }
    return total / 4.0;
}

inline double WheelContactFraction(const TelemetryWheels& wheels) noexcept
{
    int contacts = 0;
    for (bool inContact : wheels.Contact)
    {
        contacts += inContact ? 1 : 0;
    }
    return static_cast<double>(contacts) / 4.0;
}

inline double HighestWheelSlip(const TelemetryWheels& wheels) noexcept
{
    double maximum = 0.0;
    for (double ratio : wheels.SlipRatio)
    {
        maximum = std::max(maximum, std::abs(ratio));
    }
    return maximum;
}

inline double MeanWheelWear(const TelemetryWheels& wheels) noexcept
{
    double sum = 0.0;
    for (double wear : wheels.TireWearFraction)
    {
        sum += wear;
    }
    return sum / 4.0;
}
} // namespace latebrake::engineering
