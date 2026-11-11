#pragma once

#include "Telemetry/TelemetryLapAnalysis.h"
#include "Telemetry/TelemetryFilters.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class FuelForecastConfidence : std::uint8_t
{
    Missing,
    Preliminary,
    Sampled,
    Qualified,
    Inconsistent
};

enum class FuelStrategyAction : std::uint8_t
{
    NoEstimate,
    Continue,
    Conserve,
    ReassessAtNextLap,
    PitAtNextSafeOpportunity,
    InsufficientFuelToFinish
};

struct FuelAnalysisSettings final
{
    double MinimumFuelChangeFraction = 0.000005;
    double MaximumFuelChangePerSecond = 0.08;
    double MaximumFuelIncreaseWithoutService = 0.0001;
    double MinimumDistanceForFuelRateCm = 300.0;
    double MinimumLapConsumptionFraction = 0.001;
    double MaximumLapConsumptionFraction = 0.45;
    double MinimumObservationSeconds = 2.0;
    double MinimumReserveFraction = 0.025;
    double SafetyMarginFraction = 0.10;
    double ConservativeBurnMultiplier = 1.12;
    double CautionBurnMultiplier = 0.72;
    double MaximumSampleGapSeconds = 0.1;
    double MaximumEstimatedRemainingLaps = 100.0;
    int MinimumQualifiedLaps = 2;
    int RollingLapWindow = 12;
    bool RequireNativeFuelFraction = true;
    bool RejectUnexpectedRefuel = true;
};

inline bool ValidFuelSettings(
    const FuelAnalysisSettings& s) noexcept
{
    return Finite(s.MinimumFuelChangeFraction) &&
           s.MinimumFuelChangeFraction > 0.0 &&
           Finite(s.MaximumFuelChangePerSecond) &&
           s.MaximumFuelChangePerSecond >
               s.MinimumFuelChangeFraction &&
           Finite(s.MaximumFuelIncreaseWithoutService) &&
           s.MaximumFuelIncreaseWithoutService >= 0.0 &&
           Finite(s.MinimumDistanceForFuelRateCm) &&
           s.MinimumDistanceForFuelRateCm > 0.0 &&
           Finite(s.MinimumLapConsumptionFraction) &&
           s.MinimumLapConsumptionFraction > 0.0 &&
           Finite(s.MaximumLapConsumptionFraction) &&
           s.MaximumLapConsumptionFraction >
               s.MinimumLapConsumptionFraction &&
           s.MaximumLapConsumptionFraction <= 1.0 &&
           Finite(s.MinimumObservationSeconds) &&
           s.MinimumObservationSeconds > 0.0 &&
           Finite(s.MinimumReserveFraction) &&
           s.MinimumReserveFraction >= 0.0 &&
           Finite(s.SafetyMarginFraction) &&
           s.SafetyMarginFraction >= 0.0 &&
           Finite(s.ConservativeBurnMultiplier) &&
           s.ConservativeBurnMultiplier >= 1.0 &&
           Finite(s.CautionBurnMultiplier) &&
           s.CautionBurnMultiplier > 0.0 &&
           s.CautionBurnMultiplier <= 1.0 &&
           Finite(s.MaximumSampleGapSeconds) &&
           s.MaximumSampleGapSeconds > 0.0 &&
           s.MaximumSampleGapSeconds <= 1.0 &&
           Finite(s.MaximumEstimatedRemainingLaps) &&
           s.MaximumEstimatedRemainingLaps > 1.0 &&
           s.MinimumQualifiedLaps >= 1 &&
           s.RollingLapWindow >= s.MinimumQualifiedLaps &&
           s.RollingLapWindow <= 32;
}

struct FuelSample final
{
    double MonotonicSeconds = 0.0;
    double RaceClockSeconds = 0.0;
    double FuelFraction = 0.0;
    double BurnSincePrevious = 0.0;
    double ObservedDistanceCm = 0.0;
    double ObservedSeconds = 0.0;
    double BurnPerSecond = 0.0;
    double BurnPerKilometer = 0.0;
    double FuelPerLap = 0.0;
    double SmoothedFuelRatePerSecond = 0.0;
    double EstimatedLapsRemaining = 0.0;
    double CurrentLapConsumedFraction = 0.0;
    int LapIndex = -1;
    bool InPitLane = false;
    bool ServicePossible = false;
    bool FuelIncreaseObserved = false;
    bool Valid = false;
};

struct FuelStintSummary final
{
    double StartedFuelFraction = 0.0;
    double LatestFuelFraction = 0.0;
    double TotalConsumedFraction = 0.0;
    double TotalRefueledFraction = 0.0;
    double TotalObservedDistanceCm = 0.0;
    double TotalObservedSeconds = 0.0;
    double ConsumedWhileRacing = 0.0;
    double ConsumedUnderCaution = 0.0;
    double MeanFuelFractionPerLap = 0.0;
    double MedianFuelFractionPerLap = 0.0;
    double WorstLapFuelConsumption = 0.0;
    double BestLapFuelConsumption = 0.0;
    double EstimatedLapsRemaining = 0.0;
    double ConservativeLapsRemaining = 0.0;
    double EstimatedDistanceRemainingCm = 0.0;
    double FuelRatePerKilometer = 0.0;
    std::uint64_t QualifiedFrames = 0;
    std::uint64_t RejectedFrames = 0;
    std::uint64_t UnexpectedIncreases = 0;
    std::uint64_t VerifiedRefuelEvents = 0;
    int CompletedFuelLaps = 0;
    int StintOrdinal = 0;
    int CurrentLapIndex = -1;
    FuelForecastConfidence Confidence =
        FuelForecastConfidence::Missing;
    bool SensorContinuity = false;
    bool Qualified = false;
};

class FuelStintMonitor final
{
public:
    bool Configure(FuelAnalysisSettings settings) noexcept
    {
        if (!ValidFuelSettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        Last = {};
        Summary = {};
        PreviousFuel = 0.0;
        PreviousPosition = {};
        PreviousClock = 0.0;
        PreviousLap = -1;
        LapStartFuel = 0.0;
        Session = 0;
        Actor = 0;
        HasPrevious = false;
        LapBurns.Reset();
        BurnRateFilter.Reset();
    }

    void Suspend() noexcept
    {
        HasPrevious = false;
        Summary.SensorContinuity = false;
        BurnRateFilter.Reset();
    }

    bool Observe(
        const TelemetryFrame& frame,
        const TelemetryFrameDecision& gate,
        const TelemetryKinematics& motion) noexcept
    {
        if (!gate.Accepted || !motion.Valid ||
            !Finite(gate.ValidatedDeltaSeconds) ||
            gate.ValidatedDeltaSeconds <= 0.0 ||
            gate.ValidatedDeltaSeconds >
                Settings.MaximumSampleGapSeconds ||
            !Finite(frame.Dynamics.FuelFraction) ||
            frame.Dynamics.FuelFraction < 0.0 ||
            frame.Dynamics.FuelFraction > 1.0 ||
            (Settings.RequireNativeFuelFraction &&
             (!frame.Race.EnginePhysicsReady ||
              !IsEngineObservation(frame.Identity.Trust))))
        {
            Summary.RejectedFrames =
                SaturatingIncrement(Summary.RejectedFrames);
            Suspend();
            return false;
        }
        if (Session != 0 &&
            (Session != frame.Identity.SessionToken ||
             Actor != frame.Identity.ActorToken))
        {
            Reset();
        }
        Session = frame.Identity.SessionToken;
        Actor = frame.Identity.ActorToken;
        const double fuel = frame.Dynamics.FuelFraction;
        const double dt = gate.ValidatedDeltaSeconds;
        const Vector2 position = motion.PositionCm;
        const bool hasEarlier = HasPrevious;
        if (hasEarlier &&
            (frame.MonotonicSeconds <= PreviousClock ||
             frame.Race.LapIndex < PreviousLap ||
             frame.Race.LapIndex > PreviousLap + 1))
        {
            Summary.RejectedFrames =
                SaturatingIncrement(Summary.RejectedFrames);
            Suspend();
            return false;
        }
        FuelSample sample;
        sample.MonotonicSeconds = frame.MonotonicSeconds;
        sample.RaceClockSeconds = frame.RaceClockSeconds;
        sample.FuelFraction = fuel;
        sample.LapIndex = frame.Race.LapIndex;
        sample.InPitLane = frame.Environment.InPitLane;
        sample.ServicePossible = frame.Environment.InPitLane &&
            motion.SpeedCmS <= 100.0;
        sample.ObservedSeconds = dt;
        sample.ObservedDistanceCm = hasEarlier ?
            Distance(position, PreviousPosition) : 0.0;
        if (!hasEarlier)
        {
            Summary.StartedFuelFraction = fuel;
            LapStartFuel = fuel;
            PreviousLap = frame.Race.LapIndex;
        }
        if (hasEarlier)
        {
            const double delta = PreviousFuel - fuel;
            if (delta < -Settings.MaximumFuelIncreaseWithoutService)
            {
                sample.FuelIncreaseObserved = true;
                if (!sample.ServicePossible &&
                    Settings.RejectUnexpectedRefuel)
                {
                    Summary.UnexpectedIncreases =
                        SaturatingIncrement(
                            Summary.UnexpectedIncreases);
                    Summary.RejectedFrames =
                        SaturatingIncrement(Summary.RejectedFrames);
                    Suspend();
                    return false;
                }
                Summary.TotalRefueledFraction += -delta;
                Summary.VerifiedRefuelEvents =
                    SaturatingIncrement(Summary.VerifiedRefuelEvents);
                Summary.StintOrdinal += 1;
                LapBurns.Reset();
                LapStartFuel = fuel;
            }
            else if (delta >= Settings.MinimumFuelChangeFraction)
            {
                if (delta / dt >
                    Settings.MaximumFuelChangePerSecond)
                {
                    Summary.RejectedFrames =
                        SaturatingIncrement(Summary.RejectedFrames);
                    Suspend();
                    return false;
                }
                sample.BurnSincePrevious = delta;
                Summary.TotalConsumedFraction += delta;
                if (frame.Race.Phase == TelemetryPhase::Caution)
                {
                    Summary.ConsumedUnderCaution += delta;
                }
                else
                {
                    Summary.ConsumedWhileRacing += delta;
                }
            }
            if (frame.Race.LapIndex == PreviousLap + 1 &&
                frame.Race.VerifiedForwardGate)
            {
                const double lapConsumption =
                    LapStartFuel - PreviousFuel;
                if (lapConsumption >=
                        Settings.MinimumLapConsumptionFraction &&
                    lapConsumption <=
                        Settings.MaximumLapConsumptionFraction &&
                    LapBurns.Push(lapConsumption))
                {
                    ++Summary.CompletedFuelLaps;
                    Summary.MeanFuelFractionPerLap = LapBurns.Mean();
                    Summary.MedianFuelFractionPerLap =
                        LapBurns.Percentile(0.5);
                    Summary.WorstLapFuelConsumption =
                        LapBurns.Maximum();
                    Summary.BestLapFuelConsumption =
                        LapBurns.Minimum();
                }
                LapStartFuel = fuel;
            }
        }
        Summary.LatestFuelFraction = fuel;
        Summary.TotalObservedDistanceCm += sample.ObservedDistanceCm;
        Summary.TotalObservedSeconds += dt;
        Summary.FuelRatePerKilometer =
            Summary.TotalObservedDistanceCm >=
                    Settings.MinimumDistanceForFuelRateCm ?
                SafeRatio(Summary.TotalConsumedFraction,
                          Summary.TotalObservedDistanceCm /
                              CmPerKilometer) : 0.0;
        sample.BurnPerSecond =
            SafeRatio(sample.BurnSincePrevious, dt);
        sample.BurnPerKilometer =
            sample.ObservedDistanceCm > Tiny ?
                SafeRatio(sample.BurnSincePrevious,
                    sample.ObservedDistanceCm / CmPerKilometer) : 0.0;
        sample.SmoothedFuelRatePerSecond =
            BurnRateFilter.Observe(
                sample.BurnPerSecond, dt, 1.0);
        sample.CurrentLapConsumedFraction =
            std::max(0.0, LapStartFuel - fuel);
        sample.FuelPerLap = Summary.MeanFuelFractionPerLap;
        if (Summary.MeanFuelFractionPerLap >
            Settings.MinimumLapConsumptionFraction)
        {
            const double usable = std::max(
                0.0, fuel - Settings.MinimumReserveFraction);
            sample.EstimatedLapsRemaining = std::clamp(
                usable / Summary.MeanFuelFractionPerLap,
                0.0, Settings.MaximumEstimatedRemainingLaps);
            Summary.EstimatedLapsRemaining =
                sample.EstimatedLapsRemaining;
            const double conservativeBurn = std::max(
                Summary.WorstLapFuelConsumption,
                Summary.MeanFuelFractionPerLap *
                    Settings.ConservativeBurnMultiplier);
            Summary.ConservativeLapsRemaining =
                std::clamp(
                    usable / std::max(conservativeBurn, Tiny),
                    0.0, Settings.MaximumEstimatedRemainingLaps);
            Summary.EstimatedDistanceRemainingCm =
                SafeRatio(usable, Summary.FuelRatePerKilometer) *
                CmPerKilometer;
        }
        Summary.CurrentLapIndex = frame.Race.LapIndex;
        Summary.SensorContinuity = true;
        Summary.QualifiedFrames =
            SaturatingIncrement(Summary.QualifiedFrames);
        Summary.Confidence =
            Summary.CompletedFuelLaps >= Settings.MinimumQualifiedLaps ?
                FuelForecastConfidence::Qualified :
            Summary.CompletedFuelLaps > 0 ?
                FuelForecastConfidence::Sampled :
            Summary.TotalObservedSeconds >=
                Settings.MinimumObservationSeconds ?
                FuelForecastConfidence::Preliminary :
                FuelForecastConfidence::Missing;
        Summary.Qualified = Summary.Confidence ==
            FuelForecastConfidence::Qualified;
        sample.Valid = true;
        Last = sample;
        PreviousFuel = fuel;
        PreviousPosition = position;
        PreviousClock = frame.MonotonicSeconds;
        PreviousLap = frame.Race.LapIndex;
        HasPrevious = true;
        return true;
    }

    FuelSample Snapshot() const noexcept { return Last; }
    FuelStintSummary History() const noexcept { return Summary; }
    bool ForecastQualified() const noexcept
    {
        return Summary.Qualified &&
            Summary.MeanFuelFractionPerLap > 0.0;
    }
    double LapsRemainingConservative() const noexcept
    {
        return Summary.ConservativeLapsRemaining;
    }

private:
    FuelAnalysisSettings Settings{};
    FuelSample Last{};
    FuelStintSummary Summary{};
    RollingWindow<32> LapBurns;
    ExponentialFilter BurnRateFilter;
    Vector2 PreviousPosition{};
    double PreviousFuel = 0.0;
    double PreviousClock = 0.0;
    double LapStartFuel = 0.0;
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    int PreviousLap = -1;
    bool HasPrevious = false;
};

struct FuelRaceProjection final
{
    FuelStrategyAction Action = FuelStrategyAction::NoEstimate;
    FuelForecastConfidence Confidence =
        FuelForecastConfidence::Missing;
    double CurrentFuelFraction = 0.0;
    double RemainingLaps = 0.0;
    double AverageBurnPerLap = 0.0;
    double ConservativeBurnPerLap = 0.0;
    double RequiredFuelFraction = 0.0;
    double ProjectedFinishFuelFraction = 0.0;
    double ReserveShortfallFraction = 0.0;
    double FuelSavingRequiredPerLap = 0.0;
    double FuelToAddFraction = 0.0;
    double PredictedRacingLapsOnFuel = 0.0;
    double PredictedCautionLapsOnFuel = 0.0;
    double ConfidenceMarginLaps = 0.0;
    bool FinishPossible = false;
    bool FuelSavingLikelyNeeded = false;
    bool StopRecommendedForReview = false;
    bool Qualified = false;
};

inline FuelRaceProjection EvaluateFuelRaceStrategy(
    FuelStintSummary stint,
    int currentLap, int scheduledLaps,
    FuelAnalysisSettings settings = {},
    double expectedCautionFraction = 0.0) noexcept
{
    FuelRaceProjection out;
    if (!ValidFuelSettings(settings) ||
        !Finite(expectedCautionFraction) ||
        expectedCautionFraction < 0.0 ||
        expectedCautionFraction > 1.0 ||
        scheduledLaps < 1 ||
        scheduledLaps > MaximumTelemetryLaps ||
        currentLap < 0 || currentLap > scheduledLaps ||
        stint.Confidence == FuelForecastConfidence::Missing ||
        !Finite(stint.LatestFuelFraction) ||
        stint.LatestFuelFraction < 0.0 ||
        stint.LatestFuelFraction > 1.0 ||
        stint.MeanFuelFractionPerLap <= 0.0)
    {
        return out;
    }
    out.Confidence = stint.Confidence;
    out.CurrentFuelFraction = stint.LatestFuelFraction;
    out.RemainingLaps =
        static_cast<double>(scheduledLaps - currentLap);
    out.AverageBurnPerLap = stint.MeanFuelFractionPerLap;
    out.ConservativeBurnPerLap = std::max(
        stint.WorstLapFuelConsumption,
        stint.MeanFuelFractionPerLap *
            settings.ConservativeBurnMultiplier);
    const double projectedBurn =
        out.ConservativeBurnPerLap *
        (1.0 - expectedCautionFraction +
         expectedCautionFraction * settings.CautionBurnMultiplier);
    out.RequiredFuelFraction =
        out.RemainingLaps * projectedBurn +
        settings.MinimumReserveFraction;
    out.ProjectedFinishFuelFraction =
        stint.LatestFuelFraction - out.RemainingLaps * projectedBurn;
    out.ReserveShortfallFraction =
        std::max(0.0,
            out.RequiredFuelFraction - stint.LatestFuelFraction);
    out.FuelSavingRequiredPerLap =
        SafeRatio(out.ReserveShortfallFraction,
                  std::max(1.0, out.RemainingLaps));
    out.FuelToAddFraction =
        std::clamp(out.ReserveShortfallFraction, 0.0, 1.0);
    out.PredictedRacingLapsOnFuel =
        SafeRatio(std::max(0.0,
            stint.LatestFuelFraction -
            settings.MinimumReserveFraction),
            out.ConservativeBurnPerLap);
    out.PredictedCautionLapsOnFuel =
        SafeRatio(std::max(0.0,
            stint.LatestFuelFraction -
            settings.MinimumReserveFraction),
            out.ConservativeBurnPerLap *
                settings.CautionBurnMultiplier);
    out.ConfidenceMarginLaps =
        out.PredictedRacingLapsOnFuel -
        out.RemainingLaps;
    out.FinishPossible =
        out.ReserveShortfallFraction <= 0.0;
    out.FuelSavingLikelyNeeded =
        out.ReserveShortfallFraction > 0.0;
    out.StopRecommendedForReview =
        out.ReserveShortfallFraction >
            out.AverageBurnPerLap * 0.5;
    if (out.FinishPossible)
    {
        out.Action = FuelStrategyAction::Continue;
    }
    else if (out.FuelSavingRequiredPerLap <=
             out.AverageBurnPerLap * 0.1)
    {
        out.Action = FuelStrategyAction::Conserve;
    }
    else if (out.StopRecommendedForReview)
    {
        out.Action =
            FuelStrategyAction::PitAtNextSafeOpportunity;
    }
    else
    {
        out.Action = FuelStrategyAction::ReassessAtNextLap;
    }
    if (out.ReserveShortfallFraction >
        out.AverageBurnPerLap *
        std::max(1.0, out.RemainingLaps) * 0.5)
    {
        out.Action = FuelStrategyAction::InsufficientFuelToFinish;
    }
    out.Qualified = stint.Qualified;
    return out;
}
} // namespace latebrake::engineering
