#pragma once
#include "Racecraft/EngineTorqueModel.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake::racecraft
{
inline constexpr int GearboxMaximumForwardGears = 8;
struct GearboxRatios final
{
    std::array<double, GearboxMaximumForwardGears> Forward{{
        3.26, 2.10, 1.53, 1.18, 0.96, 0.80, 0.69, 0.60
    }};
    double Reverse = 3.10;
    double FinalDrive = 3.70;
    int ForwardCount = 6;
};
struct GearboxParameters final
{
    GearboxRatios Ratios{};
    double ClutchMaximumTorqueNm = 900.0;
    double ClutchEngagePerSecond = 5.0;
    double ClutchReleasePerSecond = 9.0;
    double ShiftInitiationSeconds = 0.04;
    double ShiftDisengageSeconds = 0.08;
    double ShiftEngageSeconds = 0.11;
    double MinimumShiftSeconds = 0.12;
    double MaximumShiftSeconds = 1.0;
    double ShiftCooldownSeconds = 0.16;
    double AutomaticUpshiftRpm = 7300.0;
    double AutomaticDownshiftRpm = 2100.0;
    double ReverseMaximumSpeedCmS = 180.0;
    double ManualShiftMaximumSpeedCmS = MaximumPhysicalSpeedCmS;
    double ShiftClutchTemperatureGainCPerJ = 0.00005;
    double ClutchOverheatStartC = 165.0;
    double ClutchOverheatCriticalC = 240.0;
    double GearboxEfficiency = 0.94;
    double DrivelineRotationalLossNm = 8.0;
    double AntiStallThrottleFraction = 0.15;
    double LaunchClutchSlipLimit = 0.62;
    bool Automatic = true;
    bool AllowReverse = true;
    bool AllowSkipShift = false;
};
struct GearboxObservation final
{
    double EngineRpm = 0.0;
    double RequestedThrottle = 0.0;
    double RequestedClutch = 0.0;
    double MeasuredVehicleSpeedCmS = 0.0;
    double EstimatedDrivenWheelRpm = 0.0;
    double EngineTorqueNm = 0.0;
    double MeasuredClutchTemperatureC = 70.0;
    double DeltaSeconds = 0.0;
    int RequestedGear = 0;
    int ObservedGear = 0;
    bool EngineRunning = false;
    bool PhysicalGearReadoutReady = false;
    bool EngineQualified = false;
};
enum class GearboxPhase : std::uint8_t
{
    Neutral, Engaged, Release, Disengaged, Selecting,
    Engaging, Cooldown, Fault
};
struct GearboxState final
{
    GearboxPhase Phase = GearboxPhase::Neutral;
    int CurrentGear = 0;
    int TargetGear = 0;
    double ShiftTimerSeconds = 0.0;
    double CooldownRemainingSeconds = 0.0;
    double ClutchEngagement = 0.0;
    double ClutchTemperatureC = 70.0;
    double LastDrivelineTorqueNm = 0.0;
    double AccumulatedSlipEnergyJ = 0.0;
    int Upshifts = 0;
    int Downshifts = 0;
    int RejectedShifts = 0;
    bool ManualShiftPending = false;
};
struct GearboxOutput final
{
    double WheelTorqueNm = 0.0;
    double EffectiveRatio = 0.0;
    double ClutchEngagement = 0.0;
    double SlipPowerWatts = 0.0;
    double ClutchHeatJ = 0.0;
    double DrivelineLossNm = 0.0;
    double ShiftProgressFraction = 0.0;
    double PredictedWheelRpm = 0.0;
    double EngineRpmToWheelRpm = 0.0;
    double GearboxEfficiency = 0.0;
    int CommandedGear = 0;
    int ActualMeasuredGear = 0;
    GearboxPhase Phase = GearboxPhase::Neutral;
    bool ShiftInProgress = false;
    bool Overheated = false;
    bool UnsafeReverseRequested = false;
    bool ActualGearAuthority = false;
    bool Valid = false;
};
inline bool ValidGearboxParameters(GearboxParameters s) noexcept
{
    if (s.Ratios.ForwardCount < 1 ||
        s.Ratios.ForwardCount > GearboxMaximumForwardGears ||
        !Finite(s.Ratios.Reverse) || s.Ratios.Reverse <= 0.0 ||
        !Finite(s.Ratios.FinalDrive) || s.Ratios.FinalDrive <= 0.0 ||
        !Finite(s.ClutchMaximumTorqueNm) ||
        s.ClutchMaximumTorqueNm <= 0.0 ||
        !Finite(s.ClutchEngagePerSecond) ||
        s.ClutchEngagePerSecond <= 0.0 ||
        !Finite(s.ClutchReleasePerSecond) ||
        s.ClutchReleasePerSecond <= 0.0 ||
        !Finite(s.ShiftInitiationSeconds) ||
        s.ShiftInitiationSeconds <= 0.0 ||
        !Finite(s.ShiftDisengageSeconds) ||
        s.ShiftDisengageSeconds <= 0.0 ||
        !Finite(s.ShiftEngageSeconds) ||
        s.ShiftEngageSeconds <= 0.0 ||
        !Finite(s.MinimumShiftSeconds) ||
        s.MinimumShiftSeconds <= 0.0 ||
        !Finite(s.MaximumShiftSeconds) ||
        s.MaximumShiftSeconds <= s.MinimumShiftSeconds ||
        !Finite(s.ShiftCooldownSeconds) ||
        s.ShiftCooldownSeconds < 0.0 ||
        !Finite(s.AutomaticUpshiftRpm) ||
        !Finite(s.AutomaticDownshiftRpm) ||
        s.AutomaticUpshiftRpm <= s.AutomaticDownshiftRpm ||
        !Finite(s.ReverseMaximumSpeedCmS) ||
        s.ReverseMaximumSpeedCmS <= 0.0 ||
        !Finite(s.GearboxEfficiency) ||
        s.GearboxEfficiency <= 0.0 ||
        s.GearboxEfficiency > 1.0 ||
        !Finite(s.DrivelineRotationalLossNm) ||
        s.DrivelineRotationalLossNm < 0.0)
        return false;
    for (int i = 0; i < s.Ratios.ForwardCount; ++i)
    {
        if (!Finite(s.Ratios.Forward[static_cast<std::size_t>(i)]) ||
            s.Ratios.Forward[static_cast<std::size_t>(i)] <= 0.0)
            return false;
    }
    return true;
}
inline bool ValidGearboxObservation(
    GearboxObservation s,
    GearboxParameters settings) noexcept
{
    return s.EngineQualified &&
        s.PhysicalGearReadoutReady &&
        s.EngineRunning &&
        ValidFrameDelta(s.DeltaSeconds) &&
        Finite(s.EngineRpm) &&
        s.EngineRpm >= 0.0 &&
        s.EngineRpm <= 30000.0 &&
        Finite(s.RequestedThrottle) &&
        s.RequestedThrottle >= 0.0 &&
        s.RequestedThrottle <= 1.0 &&
        Finite(s.RequestedClutch) &&
        s.RequestedClutch >= 0.0 &&
        s.RequestedClutch <= 1.0 &&
        Finite(s.MeasuredVehicleSpeedCmS) &&
        std::abs(s.MeasuredVehicleSpeedCmS) <=
            MaximumPhysicalSpeedCmS &&
        Finite(s.EstimatedDrivenWheelRpm) &&
        std::abs(s.EstimatedDrivenWheelRpm) <= 5000.0 &&
        Finite(s.EngineTorqueNm) &&
        std::abs(s.EngineTorqueNm) <= 20000.0 &&
        Finite(s.MeasuredClutchTemperatureC) &&
        s.MeasuredClutchTemperatureC >= -80.0 &&
        s.MeasuredClutchTemperatureC <= 900.0 &&
        s.RequestedGear >= -1 &&
        s.RequestedGear <= settings.Ratios.ForwardCount &&
        s.ObservedGear >= -1 &&
        s.ObservedGear <= settings.Ratios.ForwardCount;
}
inline double GearRatio(
    const GearboxRatios& ratios, int gear) noexcept
{
    if (gear < -1 || gear > ratios.ForwardCount)
        return 0.0;
    if (gear == 0) return 0.0;
    if (gear == -1) return -ratios.Reverse *
        ratios.FinalDrive;
    return ratios.Forward[static_cast<std::size_t>(gear - 1)] *
        ratios.FinalDrive;
}
inline int ChooseAutomaticGear(
    GearboxObservation frame,
    GearboxParameters settings,
    int currentGear) noexcept
{
    if (!settings.Automatic ||
        currentGear < 0 ||
        currentGear > settings.Ratios.ForwardCount)
        return currentGear;
    if (currentGear == 0) return 1;
    if (frame.EngineRpm >= settings.AutomaticUpshiftRpm &&
        currentGear < settings.Ratios.ForwardCount)
        return currentGear + 1;
    if (frame.EngineRpm <= settings.AutomaticDownshiftRpm &&
        currentGear > 1)
        return currentGear - 1;
    return currentGear;
}
inline double CalculateClutchSlipPower(
    double engineRpm, double wheelRpm,
    double ratio, double transmittedTorqueNm) noexcept
{
    if (!Finite(engineRpm) || !Finite(wheelRpm) ||
        !Finite(ratio) || !Finite(transmittedTorqueNm))
        return 0.0;
    const double radPerSecond =
        (engineRpm - wheelRpm * std::abs(ratio)) *
        TwoPi / 60.0;
    return std::abs(radPerSecond * transmittedTorqueNm);
}
class GearboxController final
{
public:
    bool Configure(GearboxParameters settings) noexcept
    {
        if (!ValidGearboxParameters(settings)) return false;
        Settings = settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        State = {};
        Last = {};
        Accepted = 0;
        Rejected = 0;
        ReverseVetoes = 0;
        TotalShiftSeconds = 0.0;
        MaximumSlipPowerWatts = 0.0;
    }
    GearboxOutput Step(GearboxObservation frame) noexcept
    {
        GearboxOutput output;
        if (!ValidGearboxObservation(frame, Settings))
        {
            ++Rejected;
            return output;
        }
        if (Accepted == 0)
        {
            State.CurrentGear = frame.ObservedGear;
            State.TargetGear = frame.ObservedGear;
            State.ClutchEngagement =
                frame.ObservedGear == 0 ? 0.0 : 1.0;
            State.Phase = frame.ObservedGear == 0 ?
                GearboxPhase::Neutral : GearboxPhase::Engaged;
        }
        State.ClutchTemperatureC =
            frame.MeasuredClutchTemperatureC;
        const int desired = Settings.Automatic ?
            ChooseAutomaticGear(
                frame, Settings, State.CurrentGear) :
            frame.RequestedGear;
        output.UnsafeReverseRequested =
            desired == -1 &&
            (std::abs(frame.MeasuredVehicleSpeedCmS) >
                Settings.ReverseMaximumSpeedCmS ||
             !Settings.AllowReverse);
        if (output.UnsafeReverseRequested)
        {
            ++ReverseVetoes;
            ++State.RejectedShifts;
        }
        const bool shouldShift =
            desired != State.CurrentGear &&
            !output.UnsafeReverseRequested &&
            (Settings.AllowSkipShift ||
             std::abs(desired - State.CurrentGear) <= 1) &&
            State.CooldownRemainingSeconds <= 0.0;
        if (shouldShift &&
            (State.Phase == GearboxPhase::Engaged ||
             State.Phase == GearboxPhase::Neutral))
        {
            State.TargetGear = desired;
            State.Phase = GearboxPhase::Release;
            State.ShiftTimerSeconds = 0.0;
        }
        const double dt = frame.DeltaSeconds;
        State.CooldownRemainingSeconds = std::max(
            0.0, State.CooldownRemainingSeconds - dt);
        if (State.Phase == GearboxPhase::Release ||
            State.Phase == GearboxPhase::Disengaged ||
            State.Phase == GearboxPhase::Selecting ||
            State.Phase == GearboxPhase::Engaging)
        {
            State.ShiftTimerSeconds += dt;
            TotalShiftSeconds += dt;
            if (State.Phase == GearboxPhase::Release)
            {
                State.ClutchEngagement = std::max(
                    0.0, State.ClutchEngagement -
                    Settings.ClutchReleasePerSecond * dt);
                if (State.ShiftTimerSeconds >=
                    Settings.ShiftInitiationSeconds)
                    State.Phase = GearboxPhase::Disengaged;
            }
            if (State.Phase == GearboxPhase::Disengaged &&
                State.ShiftTimerSeconds >=
                Settings.ShiftDisengageSeconds)
                State.Phase = GearboxPhase::Selecting;
            if (State.Phase == GearboxPhase::Selecting)
            {
                if (frame.ObservedGear == State.TargetGear)
                {
                    State.CurrentGear = frame.ObservedGear;
                    State.Phase = GearboxPhase::Engaging;
                }
            }
            if (State.Phase == GearboxPhase::Engaging)
            {
                State.ClutchEngagement = std::min(
                    1.0, State.ClutchEngagement +
                    Settings.ClutchEngagePerSecond * dt);
                if (State.ClutchEngagement >= 0.99 &&
                    State.ShiftTimerSeconds >=
                        Settings.MinimumShiftSeconds)
                {
                    State.CurrentGear = State.TargetGear;
                    State.Phase = GearboxPhase::Cooldown;
                    State.CooldownRemainingSeconds =
                        Settings.ShiftCooldownSeconds;
                    if (desired > 0)
                        ++State.Upshifts;
                    else
                        ++State.Downshifts;
                }
            }
            if (State.ShiftTimerSeconds >
                Settings.MaximumShiftSeconds)
            {
                State.Phase = GearboxPhase::Fault;
                State.ClutchEngagement = 0.0;
                ++State.RejectedShifts;
            }
        }
        else if (State.Phase == GearboxPhase::Cooldown &&
                 State.CooldownRemainingSeconds <= 0.0)
            State.Phase = GearboxPhase::Engaged;
        const double ratio = GearRatio(
            Settings.Ratios, State.CurrentGear);
        const double clutchDemand =
            std::clamp(State.ClutchEngagement *
                (1.0 - frame.RequestedClutch), 0.0, 1.0);
        const double transmitted = std::clamp(
            frame.EngineTorqueNm * clutchDemand,
            -Settings.ClutchMaximumTorqueNm,
            Settings.ClutchMaximumTorqueNm);
        output.ClutchHeatJ = CalculateClutchSlipPower(
            frame.EngineRpm,
            frame.EstimatedDrivenWheelRpm,
            ratio, transmitted) * dt;
        State.AccumulatedSlipEnergyJ +=
            output.ClutchHeatJ;
        output.SlipPowerWatts =
            SafeDivision(output.ClutchHeatJ, dt);
        MaximumSlipPowerWatts = std::max(
            MaximumSlipPowerWatts, output.SlipPowerWatts);
        const double fade = Smoothstep(
            Settings.ClutchOverheatStartC,
            Settings.ClutchOverheatCriticalC,
            frame.MeasuredClutchTemperatureC);
        output.DrivelineLossNm =
            Settings.DrivelineRotationalLossNm * Sign(ratio);
        output.WheelTorqueNm =
            ratio * transmitted * Settings.GearboxEfficiency *
            (1.0 - 0.65 * fade);
        output.EffectiveRatio = ratio;
        output.ClutchEngagement = clutchDemand;
        output.PredictedWheelRpm =
            SafeDivision(frame.EngineRpm, ratio);
        output.EngineRpmToWheelRpm =
            std::abs(ratio);
        output.GearboxEfficiency =
            Settings.GearboxEfficiency;
        output.ShiftProgressFraction = Clamp01(
            SafeDivision(State.ShiftTimerSeconds,
                         Settings.MinimumShiftSeconds));
        output.CommandedGear = State.TargetGear;
        output.ActualMeasuredGear = frame.ObservedGear;
        output.Phase = State.Phase;
        output.ShiftInProgress =
            State.Phase == GearboxPhase::Release ||
            State.Phase == GearboxPhase::Disengaged ||
            State.Phase == GearboxPhase::Selecting ||
            State.Phase == GearboxPhase::Engaging;
        output.Overheated = fade >= 0.95;
        output.ActualGearAuthority = false;
        output.Valid = Finite(output.WheelTorqueNm) &&
            Finite(output.SlipPowerWatts) &&
            Finite(output.PredictedWheelRpm);
        ++Accepted;
        Last = output;
        return output;
    }
    GearboxOutput Snapshot() const noexcept { return Last; }
    GearboxState StateSnapshot() const noexcept { return State; }
    std::uint64_t AcceptedFrames() const noexcept { return Accepted; }
    std::uint64_t RejectedFrames() const noexcept { return Rejected; }
    std::uint64_t ReverseSafetyVetoes() const noexcept
    {
        return ReverseVetoes;
    }
    double GearChangeSeconds() const noexcept
    {
        return TotalShiftSeconds;
    }
private:
    GearboxParameters Settings{};
    GearboxState State{};
    GearboxOutput Last{};
    std::uint64_t Accepted = 0;
    std::uint64_t Rejected = 0;
    std::uint64_t ReverseVetoes = 0;
    double TotalShiftSeconds = 0.0;
    double MaximumSlipPowerWatts = 0.0;
};
} // namespace latebrake::racecraft
