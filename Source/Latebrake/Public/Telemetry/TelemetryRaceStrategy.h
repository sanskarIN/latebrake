#pragma once

#include "Telemetry/TelemetryFuelStrategy.h"
#include "Telemetry/TelemetryPitStrategyReview.h"
#include "Telemetry/TelemetryVehicleHealth.h"
#include "Telemetry/TelemetryReliability.h"
#include "Telemetry/TelemetryOpponents.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class StrategyKind : std::uint8_t
{
    Unavailable,
    HoldCurrentPace,
    ConserveFuel,
    ProtectTires,
    CoolEngine,
    CoolBrakes,
    PitSoonForFuel,
    PitSoonForTires,
    PitSoonForRepair,
    PrepareForCaution,
    ReviewEndOfRace
};

enum class StrategyRisk : std::uint8_t
{
    Unknown,
    Low,
    Moderate,
    Elevated,
    High,
    Unacceptable
};

enum class StrategyEvidence : std::uint8_t
{
    Missing,
    Measured,
    Estimated,
    Partial,
    Contradictory
};

struct RaceStrategySettings final
{
    double MaximumTimeHorizonSeconds = 3600.0;
    double MinimumValidLapSeconds = 4.0;
    double MaximumValidLapSeconds = 7200.0;
    double DefaultPitLossSeconds = 23.0;
    double FuelReserveFraction = 0.03;
    double TireCriticalWear = 0.88;
    double TireRiskWear = 0.68;
    double EngineCriticalC = 140.0;
    double BrakeCriticalC = 850.0;
    double MaximumRemainingLaps = 128.0;
    double AssumedCautionProbability = 0.0;
    double MaximumTrafficTimeLossSecondsPerLap = 8.0;
    double ConservativeFuelBurnMargin = 0.12;
    double TireWearPacePenaltyPerLapSeconds = 2.0;
    double DamagePacePenaltySeconds = 5.0;
    double MinimumAssessmentConfidence = 0.7;
    double MinimumDecisionHysteresisSeconds = 10.0;
    double MaximumPredictionErrorFraction = 0.35;
    int MaximumCandidatePlans = 8;
    bool RequireQualifiedFuelEstimate = true;
    bool NeverTreatAdviceAsRaceControl = true;
};

inline bool ValidRaceStrategySettings(
    const RaceStrategySettings& s) noexcept
{
    return Finite(s.MaximumTimeHorizonSeconds) &&
           s.MaximumTimeHorizonSeconds > 0.0 &&
           Finite(s.MinimumValidLapSeconds) &&
           s.MinimumValidLapSeconds > 0.0 &&
           Finite(s.MaximumValidLapSeconds) &&
           s.MaximumValidLapSeconds > s.MinimumValidLapSeconds &&
           Finite(s.DefaultPitLossSeconds) &&
           s.DefaultPitLossSeconds >= 0.0 &&
           Finite(s.FuelReserveFraction) &&
           s.FuelReserveFraction >= 0.0 &&
           s.FuelReserveFraction < 1.0 &&
           Finite(s.TireCriticalWear) &&
           s.TireCriticalWear > s.TireRiskWear &&
           s.TireCriticalWear <= 1.0 &&
           Finite(s.TireRiskWear) &&
           s.TireRiskWear >= 0.0 &&
           Finite(s.EngineCriticalC) &&
           s.EngineCriticalC > 0.0 &&
           Finite(s.BrakeCriticalC) &&
           s.BrakeCriticalC > 0.0 &&
           Finite(s.MaximumRemainingLaps) &&
           s.MaximumRemainingLaps > 0.0 &&
           Finite(s.AssumedCautionProbability) &&
           s.AssumedCautionProbability >= 0.0 &&
           s.AssumedCautionProbability <= 1.0 &&
           Finite(s.MaximumTrafficTimeLossSecondsPerLap) &&
           s.MaximumTrafficTimeLossSecondsPerLap >= 0.0 &&
           Finite(s.ConservativeFuelBurnMargin) &&
           s.ConservativeFuelBurnMargin >= 0.0 &&
           Finite(s.TireWearPacePenaltyPerLapSeconds) &&
           s.TireWearPacePenaltyPerLapSeconds >= 0.0 &&
           Finite(s.DamagePacePenaltySeconds) &&
           s.DamagePacePenaltySeconds >= 0.0 &&
           Finite(s.MinimumAssessmentConfidence) &&
           s.MinimumAssessmentConfidence >= 0.0 &&
           s.MinimumAssessmentConfidence <= 1.0 &&
           Finite(s.MinimumDecisionHysteresisSeconds) &&
           s.MinimumDecisionHysteresisSeconds >= 0.0 &&
           Finite(s.MaximumPredictionErrorFraction) &&
           s.MaximumPredictionErrorFraction >= 0.0 &&
           s.MaximumCandidatePlans >= 1 &&
           s.MaximumCandidatePlans <= 8;
}

struct StrategyInputs final
{
    FuelRaceProjection Fuel{};
    FuelStintSummary FuelStint{};
    VehicleHealthSnapshot Health{};
    FleetTelemetrySnapshot Traffic{};
    ReliabilitySnapshot Reliability{};
    TelemetryLap LatestLap{};
    double ReferenceRaceLapSeconds = 0.0;
    double CurrentTireWear = 0.0;
    double CurrentDamageFraction = 0.0;
    double EstimatedPitTimeLossSeconds = 0.0;
    double MonotonicSeconds = 0.0;
    double CautionProbability = 0.0;
    double PredictedTrafficLossSecondsPerLap = 0.0;
    double RemainingLaps = 0.0;
    int CompletedLaps = 0;
    int ScheduledLaps = 0;
    bool RaceActive = false;
    bool NativeObserved = false;
    bool PhysicalPitServiceAvailable = false;
    bool CurrentCaution = false;
    std::uint64_t SessionToken = 0;
};

struct StrategyCandidate final
{
    StrategyKind Kind = StrategyKind::Unavailable;
    StrategyRisk Risk = StrategyRisk::Unknown;
    StrategyEvidence Evidence = StrategyEvidence::Missing;
    double EstimatedFinishSeconds = 0.0;
    double EstimatedFuelAtFinish = 0.0;
    double TimeCostSeconds = 0.0;
    double FuelCostFraction = 0.0;
    double TireWearAtFinish = 0.0;
    double Confidence = 0.0;
    double EstimatedTimeGainSeconds = 0.0;
    double EstimatedTimeLossSeconds = 0.0;
    double PitTimeLossSeconds = 0.0;
    double MechanicalRiskFraction = 0.0;
    double FuelReserveShortfallFraction = 0.0;
    double OpportunityScore = 0.0;
    double RiskPenalty = 0.0;
    int StopsAssumed = 0;
    bool RequiresService = false;
    bool FuelFeasible = false;
    bool TireFeasible = false;
    bool CanFinish = false;
    bool Qualified = false;
};

struct StrategyAssessment final
{
    std::array<StrategyCandidate, 8> Candidates{};
    StrategyKind Preferred = StrategyKind::Unavailable;
    StrategyRisk HighestRisk = StrategyRisk::Unknown;
    StrategyEvidence Evidence = StrategyEvidence::Missing;
    double AssessedAtSeconds = 0.0;
    double RemainingRaceSeconds = 0.0;
    double MinimumFeasibleFinishSeconds = 0.0;
    double EstimatedAvoidableLossSeconds = 0.0;
    double Confidence = 0.0;
    double ProjectedFinishFuel = 0.0;
    double ConservativeFuelReserve = 0.0;
    double CurrentWearFraction = 0.0;
    double CurrentDamageFraction = 0.0;
    double BestScore = 0.0;
    int CandidateCount = 0;
    int PreferredCandidateIndex = -1;
    int FeasibleCandidates = 0;
    int HighRiskCandidates = 0;
    bool NeedFuelReview = false;
    bool NeedTireReview = false;
    bool NeedMechanicalInspection = false;
    bool UnderCaution = false;
    bool ForecastQualified = false;
    bool AdvisoryOnly = true;
};

inline double StrategyEstimatedLapTime(
    double referenceLap,
    double wear,
    double damage,
    double trafficLoss,
    const RaceStrategySettings& settings) noexcept
{
    if (!Finite(referenceLap) ||
        referenceLap < settings.MinimumValidLapSeconds ||
        referenceLap > settings.MaximumValidLapSeconds ||
        !Finite(wear) || !Finite(damage) ||
        !Finite(trafficLoss))
    {
        return 0.0;
    }
    const double wearPenalty =
        std::max(0.0, wear - 0.4) *
        settings.TireWearPacePenaltyPerLapSeconds;
    const double damagePenalty =
        Clamp01(damage) * settings.DamagePacePenaltySeconds;
    const double limitedTraffic =
        std::clamp(trafficLoss, 0.0,
                   settings.MaximumTrafficTimeLossSecondsPerLap);
    return referenceLap + wearPenalty +
        damagePenalty + limitedTraffic;
}

inline StrategyCandidate BuildStrategyCandidate(
    StrategyKind kind, const StrategyInputs& input,
    const RaceStrategySettings& settings) noexcept
{
    StrategyCandidate candidate;
    candidate.Kind = kind;
    if (!input.RaceActive ||
        !Finite(input.RemainingLaps) ||
        !Finite(input.CurrentTireWear) ||
        !Finite(input.CurrentDamageFraction) ||
        !Finite(input.ReferenceRaceLapSeconds) ||
        input.RemainingLaps < 0.0 ||
        input.RemainingLaps > settings.MaximumRemainingLaps)
    {
        return candidate;
    }
    const double predictedLap = StrategyEstimatedLapTime(
        input.ReferenceRaceLapSeconds,
        input.CurrentTireWear,
        input.CurrentDamageFraction,
        input.PredictedTrafficLossSecondsPerLap,
        settings);
    if (predictedLap <= 0.0)
    {
        return candidate;
    }
    const double remaining = input.RemainingLaps;
    double expectedFuelBurn =
        input.FuelStint.MeanFuelFractionPerLap *
        (1.0 + settings.ConservativeFuelBurnMargin);
    const double caution = std::clamp(
        input.CautionProbability, 0.0, 1.0);
    expectedFuelBurn *= 1.0 - 0.25 * caution;
    const double existingFuel = input.FuelStint.LatestFuelFraction;
    const double existingWear = Clamp01(input.CurrentTireWear);
    const double damage = Clamp01(input.CurrentDamageFraction);
    double strategyTime = predictedLap * remaining;
    double timePenalty = 0.0;
    double burnMultiplier = 1.0;
    double wearMultiplier = 1.0;
    bool needsPit = false;
    bool tiresReset = false;
    bool fuelReset = false;
    bool repairs = false;
    switch (kind)
    {
    case StrategyKind::HoldCurrentPace:
        break;
    case StrategyKind::ConserveFuel:
        burnMultiplier = 0.82;
        timePenalty = 0.9 * remaining;
        break;
    case StrategyKind::ProtectTires:
        wearMultiplier = 0.73;
        timePenalty = 0.6 * remaining;
        break;
    case StrategyKind::CoolEngine:
        timePenalty = 1.0 * remaining;
        break;
    case StrategyKind::CoolBrakes:
        timePenalty = 0.7 * remaining;
        break;
    case StrategyKind::PitSoonForFuel:
        needsPit = true;
        fuelReset = true;
        break;
    case StrategyKind::PitSoonForTires:
        needsPit = true;
        tiresReset = true;
        break;
    case StrategyKind::PitSoonForRepair:
        needsPit = true;
        repairs = true;
        break;
    case StrategyKind::PrepareForCaution:
        timePenalty = 0.3 * remaining;
        break;
    case StrategyKind::ReviewEndOfRace:
        timePenalty = 0.0;
        break;
    case StrategyKind::Unavailable:
        return candidate;
    }
    candidate.RequiresService = needsPit;
    candidate.StopsAssumed = needsPit ? 1 : 0;
    if (needsPit)
    {
        candidate.PitTimeLossSeconds =
            input.EstimatedPitTimeLossSeconds > 0.0 ?
                input.EstimatedPitTimeLossSeconds :
                settings.DefaultPitLossSeconds;
        strategyTime += candidate.PitTimeLossSeconds;
    }
    strategyTime += timePenalty;
    const double startingFuel =
        fuelReset ? 1.0 : existingFuel;
    candidate.FuelCostFraction =
        remaining * expectedFuelBurn * burnMultiplier;
    candidate.EstimatedFuelAtFinish =
        startingFuel - candidate.FuelCostFraction;
    candidate.FuelReserveShortfallFraction =
        std::max(0.0,
            settings.FuelReserveFraction -
            candidate.EstimatedFuelAtFinish);
    candidate.TireWearAtFinish = tiresReset ?
        std::clamp(remaining * 0.015 * wearMultiplier, 0.0, 1.0) :
        std::clamp(existingWear +
            remaining * 0.015 * wearMultiplier, 0.0, 1.0);
    const double endDamage = repairs ? 0.0 : damage;
    candidate.MechanicalRiskFraction = std::max({
        candidate.TireWearAtFinish,
        endDamage,
        input.Health.EngineTemperatureC >
            settings.EngineCriticalC ? 0.9 : 0.0,
        input.Health.BrakeTemperatureC >
            settings.BrakeCriticalC ? 0.9 : 0.0
    });
    candidate.FuelFeasible =
        candidate.EstimatedFuelAtFinish >=
            settings.FuelReserveFraction;
    candidate.TireFeasible =
        candidate.TireWearAtFinish < settings.TireCriticalWear;
    candidate.CanFinish =
        candidate.FuelFeasible && candidate.TireFeasible &&
        candidate.MechanicalRiskFraction < 0.99 &&
        (!needsPit || input.PhysicalPitServiceAvailable);
    candidate.Risk =
        !candidate.CanFinish ? StrategyRisk::Unacceptable :
        candidate.MechanicalRiskFraction > 0.85 ?
            StrategyRisk::High :
        candidate.MechanicalRiskFraction > 0.70 ?
            StrategyRisk::Elevated :
        candidate.MechanicalRiskFraction > 0.50 ?
            StrategyRisk::Moderate : StrategyRisk::Low;
    candidate.Confidence = std::min({
        input.Reliability.OverallConfidence,
        input.Health.CombinedConfidence,
        input.FuelStint.Qualified ? 1.0 : 0.5
    });
    candidate.Evidence = candidate.Confidence >=
        settings.MinimumAssessmentConfidence ?
        StrategyEvidence::Measured : StrategyEvidence::Partial;
    candidate.EstimatedFinishSeconds = strategyTime;
    candidate.TimeCostSeconds = timePenalty +
        candidate.PitTimeLossSeconds;
    candidate.RiskPenalty = !candidate.CanFinish ? 1.0e6 :
        100.0 * candidate.MechanicalRiskFraction +
        500.0 * candidate.FuelReserveShortfallFraction;
    candidate.OpportunityScore = -strategyTime -
        candidate.RiskPenalty;
    candidate.Qualified = candidate.Confidence >=
        settings.MinimumAssessmentConfidence &&
        (!settings.RequireQualifiedFuelEstimate ||
         input.FuelStint.Qualified);
    return candidate;
}

class RaceStrategyReview final
{
public:
    bool Configure(RaceStrategySettings settings) noexcept
    {
        if (!ValidRaceStrategySettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        Current = {};
        Previous = {};
        Session = 0;
        LastAssessedSeconds = 0.0;
        StablePreferredSeconds = 0.0;
        AcceptedReviews = 0;
        RejectedReviews = 0;
        HasPrevious = false;
    }

    bool Review(StrategyInputs input) noexcept
    {
        if (!ValidRaceStrategySettings(Settings) ||
            !Finite(input.MonotonicSeconds) ||
            input.MonotonicSeconds < 0.0 ||
            input.SessionToken == 0 ||
            !input.NativeObserved ||
            !input.RaceActive ||
            !Finite(input.RemainingLaps) ||
            input.RemainingLaps < 0.0 ||
            input.RemainingLaps > Settings.MaximumRemainingLaps)
        {
            RejectedReviews =
                SaturatingIncrement(RejectedReviews);
            return false;
        }
        if (Session != 0 &&
            Session != input.SessionToken)
        {
            Reset();
        }
        if (HasPrevious &&
            input.MonotonicSeconds < LastAssessedSeconds)
        {
            RejectedReviews =
                SaturatingIncrement(RejectedReviews);
            return false;
        }
        Session = input.SessionToken;
        StrategyAssessment assessment;
        assessment.AssessedAtSeconds = input.MonotonicSeconds;
        assessment.RemainingRaceSeconds =
            input.RemainingLaps * input.ReferenceRaceLapSeconds;
        assessment.CurrentWearFraction = input.CurrentTireWear;
        assessment.CurrentDamageFraction = input.CurrentDamageFraction;
        assessment.UnderCaution = input.CurrentCaution;
        assessment.NeedFuelReview = input.Fuel.Qualified &&
            input.Fuel.FuelSavingLikelyNeeded;
        assessment.NeedTireReview =
            input.CurrentTireWear >= Settings.TireRiskWear;
        assessment.NeedMechanicalInspection =
            input.Health.AnyCritical;
        assessment.Confidence = std::min({
            input.FuelStint.Qualified ? 1.0 : 0.5,
            input.Reliability.OverallConfidence,
            input.Health.CombinedConfidence
        });
        const std::array<StrategyKind, 8> options{{
            StrategyKind::HoldCurrentPace,
            StrategyKind::ConserveFuel,
            StrategyKind::ProtectTires,
            StrategyKind::CoolEngine,
            StrategyKind::CoolBrakes,
            StrategyKind::PitSoonForFuel,
            StrategyKind::PitSoonForTires,
            StrategyKind::PitSoonForRepair
        }};
        double bestScore =
            -std::numeric_limits<double>::infinity();
        for (StrategyKind kind : options)
        {
            if (assessment.CandidateCount >=
                Settings.MaximumCandidatePlans)
            {
                break;
            }
            StrategyCandidate candidate =
                BuildStrategyCandidate(kind, input, Settings);
            if (!candidate.Qualified)
            {
                continue;
            }
            const int index = assessment.CandidateCount++;
            assessment.Candidates[static_cast<std::size_t>(index)] =
                candidate;
            if (candidate.CanFinish)
            {
                ++assessment.FeasibleCandidates;
            }
            if (candidate.Risk == StrategyRisk::High ||
                candidate.Risk == StrategyRisk::Unacceptable)
            {
                ++assessment.HighRiskCandidates;
            }
            if (static_cast<int>(candidate.Risk) >
                static_cast<int>(assessment.HighestRisk))
            {
                assessment.HighestRisk = candidate.Risk;
            }
            if (candidate.OpportunityScore > bestScore)
            {
                bestScore = candidate.OpportunityScore;
                assessment.Preferred = kind;
                assessment.PreferredCandidateIndex = index;
                assessment.BestScore = bestScore;
                assessment.ProjectedFinishFuel =
                    candidate.EstimatedFuelAtFinish;
                assessment.MinimumFeasibleFinishSeconds =
                    candidate.EstimatedFinishSeconds;
            }
        }
        assessment.ForecastQualified =
            assessment.CandidateCount > 0 &&
            assessment.Confidence >=
                Settings.MinimumAssessmentConfidence;
        assessment.Evidence = assessment.ForecastQualified ?
            StrategyEvidence::Measured :
            StrategyEvidence::Partial;
        if (HasPrevious &&
            assessment.Preferred != Previous.Preferred)
        {
            StablePreferredSeconds = 0.0;
        }
        else if (HasPrevious)
        {
            StablePreferredSeconds +=
                input.MonotonicSeconds - LastAssessedSeconds;
        }
        if (HasPrevious &&
            StablePreferredSeconds <
                Settings.MinimumDecisionHysteresisSeconds &&
            Previous.ForecastQualified &&
            assessment.Preferred != Previous.Preferred)
        {
            assessment.Preferred = Previous.Preferred;
        }
        assessment.AdvisoryOnly = true;
        Previous = assessment;
        Current = assessment;
        LastAssessedSeconds = input.MonotonicSeconds;
        HasPrevious = true;
        AcceptedReviews = SaturatingIncrement(AcceptedReviews);
        return assessment.ForecastQualified;
    }

    StrategyAssessment Snapshot() const noexcept { return Current; }
    StrategyKind PreferredReviewAction() const noexcept
    {
        return Current.Preferred;
    }
    bool HasQualifiedAssessment() const noexcept
    {
        return Current.ForecastQualified;
    }
    std::uint64_t AcceptedAssessments() const noexcept
    {
        return AcceptedReviews;
    }
    std::uint64_t RejectedAssessments() const noexcept
    {
        return RejectedReviews;
    }

private:
    RaceStrategySettings Settings{};
    StrategyAssessment Current{};
    StrategyAssessment Previous{};
    std::uint64_t Session = 0;
    double LastAssessedSeconds = 0.0;
    double StablePreferredSeconds = 0.0;
    std::uint64_t AcceptedReviews = 0;
    std::uint64_t RejectedReviews = 0;
    bool HasPrevious = false;
};
} // namespace latebrake::engineering
