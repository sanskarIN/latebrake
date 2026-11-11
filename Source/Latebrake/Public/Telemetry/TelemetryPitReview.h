#pragma once

#include "Telemetry/TelemetryFuelStint.h"
#include "Telemetry/TelemetryTireStint.h"
#include "Telemetry/TelemetryWeatherTrend.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class PitReviewReason : std::uint8_t
{
    None,
    FuelReserve,
    TireWear,
    TireTemperature,
    WeatherChange,
    Damage,
    ScheduledService,
    InsufficientEvidence
};

struct PitSessionEvidence final
{
    FuelStintReview Fuel{};
    TireStintReview Tire{};
    WeatherTrendReview Weather{};
    double DamageFraction = 0.0;
    double EngineTemperatureC = 90.0;
    double TrackLengthCm = 0.0;
    double MeanLapSeconds = 0.0;
    double PitLaneTransitSeconds = 0.0;
    double StationaryServiceSeconds = 0.0;
    double ExpectedFuelRefillSeconds = 0.0;
    double ExpectedTireChangeSeconds = 0.0;
    double ExpectedRepairSeconds = 0.0;
    double RemainingRaceSeconds = 0.0;
    int RemainingLaps = 0;
    int RacePosition = 0;
    int TotalDrivers = 0;
    bool EngineQualified = false;
    bool PitLaneExists = false;
    bool ServiceAllowed = false;
    bool ReliableClock = false;
};

struct PitReviewSettings final
{
    double MaximumPitTransitSeconds = 240.0;
    double MinimumPitTransitSeconds = 3.0;
    double MinimumFuelReserveLiters = 3.0;
    double CriticalTireWearFraction = 0.85;
    double CriticalTireTemperatureC = 155.0;
    double CriticalDamageFraction = 0.55;
    double SevereDamageFraction = 0.80;
    double MinimumRemainingRaceSeconds = 20.0;
    double MaximumRemainingRaceSeconds = 86400.0;
    double MaximumServiceSeconds = 300.0;
    double WetnessSwitchThreshold = 0.45;
    double PredictiveLapVarianceAllowance = 0.12;
    double MaximumRecommendedStopLossSeconds = 180.0;
    bool RequireDirectFuelSensors = true;
    bool RequireDirectTireSensors = true;
    bool NoOfficialPitCommand = true;
};

inline bool ValidPitReviewSettings(
    PitReviewSettings s) noexcept
{
    return Finite(s.MaximumPitTransitSeconds) &&
        Finite(s.MinimumPitTransitSeconds) &&
        s.MaximumPitTransitSeconds >
            s.MinimumPitTransitSeconds &&
        s.MinimumPitTransitSeconds > 0.0 &&
        Finite(s.MinimumFuelReserveLiters) &&
        s.MinimumFuelReserveLiters >= 0.0 &&
        Finite(s.CriticalTireWearFraction) &&
        s.CriticalTireWearFraction > 0.0 &&
        s.CriticalTireWearFraction <= 1.0 &&
        Finite(s.CriticalTireTemperatureC) &&
        s.CriticalTireTemperatureC > 0.0 &&
        Finite(s.CriticalDamageFraction) &&
        s.CriticalDamageFraction > 0.0 &&
        Finite(s.SevereDamageFraction) &&
        s.SevereDamageFraction >
            s.CriticalDamageFraction &&
        Finite(s.MinimumRemainingRaceSeconds) &&
        s.MinimumRemainingRaceSeconds > 0.0 &&
        Finite(s.MaximumRemainingRaceSeconds) &&
        s.MaximumRemainingRaceSeconds >
            s.MinimumRemainingRaceSeconds &&
        Finite(s.MaximumServiceSeconds) &&
        s.MaximumServiceSeconds > 0.0 &&
        Finite(s.WetnessSwitchThreshold) &&
        s.WetnessSwitchThreshold >= 0.0 &&
        s.WetnessSwitchThreshold <= 1.0 &&
        Finite(s.PredictiveLapVarianceAllowance) &&
        s.PredictiveLapVarianceAllowance >= 0.0 &&
        Finite(s.MaximumRecommendedStopLossSeconds) &&
        s.MaximumRecommendedStopLossSeconds > 0.0 &&
        s.NoOfficialPitCommand;
}

struct PitScenario final
{
    double EstimatedPitLossSeconds = 0.0;
    double FuelFillSeconds = 0.0;
    double TireChangeSeconds = 0.0;
    double RepairSeconds = 0.0;
    double ExpectedPostServiceLapSeconds = 0.0;
    double ExpectedLapTimeGainSeconds = 0.0;
    double ExpectedRemainingRaceBenefitSeconds = 0.0;
    double ExpectedFuelAtFinishLiters = 0.0;
    double ExpectedTireWearAtFinish = 0.0;
    double RemainingLapsEstimated = 0.0;
    double RiskPenaltySeconds = 0.0;
    double NetTimeBenefitSeconds = 0.0;
    double EvidenceConfidence = 0.0;
    bool Refuel = false;
    bool ChangeTires = false;
    bool Repair = false;
    bool WeatherAppropriateTireChange = false;
    bool HasEnoughEvidence = false;
    bool FasterThanStayOutEstimate = false;
    bool Valid = false;
};

struct PitReviewResult final
{
    PitReviewReason PrimaryReason = PitReviewReason::None;
    PitScenario StayOut{};
    PitScenario FuelOnly{};
    PitScenario TiresOnly{};
    PitScenario FullService{};
    PitScenario Selected{};
    double FuelRiskScore = 0.0;
    double TireRiskScore = 0.0;
    double DamageRiskScore = 0.0;
    double WeatherRiskScore = 0.0;
    double EstimatedTrackTimeAtRiskSeconds = 0.0;
    double RemainingRaceFraction = 0.0;
    double WorstCasePitLossSeconds = 0.0;
    double BestCaseNetGainSeconds = 0.0;
    double Confidence = 0.0;
    int EvaluatedScenarios = 0;
    bool RecommendService = false;
    bool UrgentSafetyReview = false;
    bool RealPitInfrastructureVerified = false;
    bool IssuedPitCommand = false;
    bool AdvisoryOnly = true;
    bool Valid = false;
};

inline bool ValidPitEvidence(
    PitSessionEvidence evidence,
    PitReviewSettings settings) noexcept
{
    return evidence.EngineQualified &&
        evidence.ReliableClock &&
        Finite(evidence.DamageFraction) &&
        Finite(evidence.EngineTemperatureC) &&
        Finite(evidence.TrackLengthCm) &&
        Finite(evidence.MeanLapSeconds) &&
        Finite(evidence.PitLaneTransitSeconds) &&
        Finite(evidence.StationaryServiceSeconds) &&
        Finite(evidence.ExpectedFuelRefillSeconds) &&
        Finite(evidence.ExpectedTireChangeSeconds) &&
        Finite(evidence.ExpectedRepairSeconds) &&
        Finite(evidence.RemainingRaceSeconds) &&
        evidence.DamageFraction >= 0.0 &&
        evidence.DamageFraction <= 1.0 &&
        evidence.EngineTemperatureC >= -100.0 &&
        evidence.EngineTemperatureC <= 1000.0 &&
        evidence.TrackLengthCm >= 100.0 &&
        evidence.MeanLapSeconds >= 4.0 &&
        evidence.MeanLapSeconds <= 7200.0 &&
        evidence.PitLaneTransitSeconds >=
            settings.MinimumPitTransitSeconds &&
        evidence.PitLaneTransitSeconds <=
            settings.MaximumPitTransitSeconds &&
        evidence.StationaryServiceSeconds >= 0.0 &&
        evidence.StationaryServiceSeconds <=
            settings.MaximumServiceSeconds &&
        evidence.ExpectedFuelRefillSeconds >= 0.0 &&
        evidence.ExpectedTireChangeSeconds >= 0.0 &&
        evidence.ExpectedRepairSeconds >= 0.0 &&
        evidence.RemainingRaceSeconds >= 0.0 &&
        evidence.RemainingRaceSeconds <=
            settings.MaximumRemainingRaceSeconds &&
        evidence.RemainingLaps >= 0 &&
        evidence.RemainingLaps <= MaximumTelemetryLaps &&
        evidence.RacePosition >= 0 &&
        evidence.TotalDrivers >= 0 &&
        evidence.TotalDrivers <= MaximumTelemetryDrivers;
}

inline PitScenario AssessPitScenario(
    const PitSessionEvidence& e,
    const PitReviewSettings& settings,
    bool refuel, bool changeTires,
    bool repair) noexcept
{
    PitScenario result;
    if (!ValidPitEvidence(e, settings))
    {
        return result;
    }
    result.Refuel = refuel;
    result.ChangeTires = changeTires;
    result.Repair = repair;
    result.RemainingLapsEstimated =
        static_cast<double>(e.RemainingLaps);
    if (refuel &&
        (!e.Fuel.PhysicalSensorTrusted ||
         !e.Fuel.HasConsumptionEstimate))
    {
        return result;
    }
    if (changeTires &&
        (!e.Tire.Valid ||
         !e.Tire.FourContactSensorsQualified))
    {
        return result;
    }
    if (refuel)
    {
        result.FuelFillSeconds =
            e.ExpectedFuelRefillSeconds;
    }
    if (changeTires)
    {
        result.TireChangeSeconds =
            e.ExpectedTireChangeSeconds;
    }
    if (repair)
    {
        result.RepairSeconds =
            e.ExpectedRepairSeconds;
    }
    result.EstimatedPitLossSeconds =
        (refuel || changeTires || repair) ?
            e.PitLaneTransitSeconds +
            std::max({
                result.FuelFillSeconds,
                result.TireChangeSeconds,
                result.RepairSeconds
            }) + e.StationaryServiceSeconds : 0.0;
    const double fuelShortfallLiters =
        std::max(0.0,
            e.Fuel.AverageLitersPerLap *
                static_cast<double>(e.RemainingLaps) -
            e.Fuel.UsableLiters);
    const double tireRisk =
        e.Tire.HighestWear >
            settings.CriticalTireWearFraction ?
        Clamp01((e.Tire.HighestWear -
                 settings.CriticalTireWearFraction) /
                std::max(0.01,
                    1.0 - settings.CriticalTireWearFraction)) :
        0.0;
    const double damageRisk = Clamp01(
        (e.DamageFraction -
         settings.CriticalDamageFraction) /
        std::max(0.01,
            1.0 - settings.CriticalDamageFraction));
    result.ExpectedLapTimeGainSeconds =
        (changeTires ? tireRisk * e.MeanLapSeconds * 0.08 : 0.0) +
        (repair ? damageRisk * e.MeanLapSeconds * 0.06 : 0.0);
    result.ExpectedRemainingRaceBenefitSeconds =
        result.ExpectedLapTimeGainSeconds *
        static_cast<double>(e.RemainingLaps);
    result.RiskPenaltySeconds =
        (!refuel && fuelShortfallLiters > 0.0) ?
            e.RemainingRaceSeconds * 0.5 : 0.0;
    result.RiskPenaltySeconds +=
        (!changeTires && tireRisk > 0.75) ?
            e.RemainingRaceSeconds * 0.08 : 0.0;
    result.RiskPenaltySeconds +=
        (!repair && damageRisk > 0.70) ?
            e.RemainingRaceSeconds * 0.10 : 0.0;
    result.NetTimeBenefitSeconds =
        result.ExpectedRemainingRaceBenefitSeconds +
        result.RiskPenaltySeconds -
        result.EstimatedPitLossSeconds;
    result.ExpectedPostServiceLapSeconds =
        std::max(1.0, e.MeanLapSeconds -
                      result.ExpectedLapTimeGainSeconds);
    result.ExpectedFuelAtFinishLiters =
        e.Fuel.RemainingLiters -
        e.Fuel.AverageLitersPerLap *
            static_cast<double>(e.RemainingLaps);
    if (refuel)
    {
        result.ExpectedFuelAtFinishLiters =
            std::max(0.0, e.Fuel.RemainingLiters);
    }
    result.ExpectedTireWearAtFinish =
        changeTires ? 0.0 :
        e.Tire.HighestWear + *std::max_element(
            e.Tire.WearPerLap.begin(), e.Tire.WearPerLap.end()) *
            static_cast<double>(e.RemainingLaps);
    result.WeatherAppropriateTireChange =
        changeTires && e.Weather.Valid &&
        e.Weather.CurrentWetness >=
            settings.WetnessSwitchThreshold;
    result.EvidenceConfidence =
        (e.Fuel.PhysicalSensorTrusted ? 0.25 : 0.0) +
        (e.Tire.Valid ? 0.25 : 0.0) +
        (e.Weather.OriginalWetnessMeasured ? 0.20 : 0.0) +
        (e.ReliableClock ? 0.15 : 0.0) +
        (e.PitLaneExists ? 0.15 : 0.0);
    result.HasEnoughEvidence =
        result.EvidenceConfidence >= 0.50;
    result.FasterThanStayOutEstimate =
        result.NetTimeBenefitSeconds > 0.0;
    result.Valid = Finite(result.NetTimeBenefitSeconds) &&
        Finite(result.EstimatedPitLossSeconds) &&
        result.HasEnoughEvidence;
    return result;
}

inline PitReviewResult EvaluatePitReview(
    PitSessionEvidence evidence,
    PitReviewSettings settings = {}) noexcept
{
    PitReviewResult result;
    if (!ValidPitReviewSettings(settings) ||
        !ValidPitEvidence(evidence, settings))
    {
        result.PrimaryReason =
            PitReviewReason::InsufficientEvidence;
        return result;
    }
    result.StayOut = AssessPitScenario(
        evidence, settings, false, false, false);
    result.FuelOnly = AssessPitScenario(
        evidence, settings, true, false, false);
    result.TiresOnly = AssessPitScenario(
        evidence, settings, false, true, false);
    result.FullService = AssessPitScenario(
        evidence, settings, true, true, true);
    result.EvaluatedScenarios =
        static_cast<int>(result.StayOut.Valid) +
        static_cast<int>(result.FuelOnly.Valid) +
        static_cast<int>(result.TiresOnly.Valid) +
        static_cast<int>(result.FullService.Valid);
    result.FuelRiskScore = Clamp01(
        SafeRatio(
            evidence.Fuel.AverageLitersPerLap *
                evidence.RemainingLaps -
            evidence.Fuel.UsableLiters,
            std::max(1.0, evidence.Fuel.RemainingLiters)));
    result.TireRiskScore = Clamp01(
        SafeRatio(
            evidence.Tire.HighestWear -
            settings.CriticalTireWearFraction,
            1.0 - settings.CriticalTireWearFraction));
    result.DamageRiskScore = Clamp01(
        SafeRatio(
            evidence.DamageFraction -
            settings.CriticalDamageFraction,
            1.0 - settings.CriticalDamageFraction));
    result.WeatherRiskScore = evidence.Weather.Valid ?
        Clamp01(std::abs(evidence.Weather.CurrentWetness -
                         settings.WetnessSwitchThreshold) *
                0.5) : 0.0;
    result.EstimatedTrackTimeAtRiskSeconds =
        evidence.RemainingRaceSeconds *
        std::max({result.FuelRiskScore,
                  result.TireRiskScore,
                  result.DamageRiskScore});
    result.RemainingRaceFraction = Clamp01(
        SafeRatio(evidence.RemainingRaceSeconds,
                  settings.MaximumRemainingRaceSeconds));
    const std::array<PitScenario, 4> candidates{{
        result.StayOut, result.FuelOnly,
        result.TiresOnly, result.FullService
    }};
    result.Selected = result.StayOut;
    result.BestCaseNetGainSeconds = 0.0;
    for (const PitScenario& candidate : candidates)
    {
        if (!candidate.Valid)
        {
            continue;
        }
        result.WorstCasePitLossSeconds =
            std::max(result.WorstCasePitLossSeconds,
                     candidate.EstimatedPitLossSeconds);
        if (!result.Selected.Valid ||
            candidate.NetTimeBenefitSeconds >
                result.Selected.NetTimeBenefitSeconds)
        {
            result.Selected = candidate;
        }
        result.BestCaseNetGainSeconds =
            std::max(result.BestCaseNetGainSeconds,
                     candidate.NetTimeBenefitSeconds);
    }
    result.Confidence = result.Selected.EvidenceConfidence;
    result.RecommendService = result.Selected.Valid &&
        (result.Selected.Refuel ||
         result.Selected.ChangeTires ||
         result.Selected.Repair) &&
        evidence.ServiceAllowed &&
        evidence.PitLaneExists &&
        evidence.RemainingRaceSeconds >=
            settings.MinimumRemainingRaceSeconds &&
        result.Selected.NetTimeBenefitSeconds > 0.0;
    result.UrgentSafetyReview =
        evidence.DamageFraction >=
            settings.SevereDamageFraction ||
        evidence.Tire.HighestWear >= 0.95 ||
        evidence.Fuel.CriticalFuel;
    if (result.FuelRiskScore >= 0.5)
    {
        result.PrimaryReason =
            PitReviewReason::FuelReserve;
    }
    else if (result.TireRiskScore >= 0.5)
    {
        result.PrimaryReason =
            PitReviewReason::TireWear;
    }
    else if (result.DamageRiskScore >= 0.5)
    {
        result.PrimaryReason =
            PitReviewReason::Damage;
    }
    else if (result.WeatherRiskScore >= 0.3)
    {
        result.PrimaryReason =
            PitReviewReason::WeatherChange;
    }
    result.RealPitInfrastructureVerified =
        evidence.PitLaneExists;
    result.IssuedPitCommand = false;
    result.AdvisoryOnly = true;
    result.Valid = result.Selected.Valid;
    return result;
}
} // namespace latebrake::engineering
