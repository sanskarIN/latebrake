#pragma once

#include "Telemetry/TelemetryVehicleBalance.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class SuspensionEventKind : std::uint8_t
{
    None,
    Compression,
    Extension,
    Bottoming,
    Topping,
    ReboundOscillation,
    ContactLoss,
    LoadImbalance
};

struct SuspensionReviewSettings final
{
    double MinimumCompressionCm = -10.0;
    double MaximumCompressionCm = 26.0;
    double BottomingFraction = 0.94;
    double ToppingFraction = 0.08;
    double MinimumWheelLoadN = 25.0;
    double MaximumWheelLoadN = 250000.0;
    double ExcessiveLeftRightLoadDifference = 0.52;
    double PitchWarningRadians = 0.22;
    double RollWarningRadians = 0.27;
    double MaximumCompressionVelocityCmS = 250.0;
    double MaximumCompressionAccelerationCmS2 = 5000.0;
    double MaximumSampleSeconds = 0.10;
    double OscillationFrequencyThresholdHz = 1.4;
    bool RequireCompleteContact = true;
};

inline bool ValidSuspensionReviewSettings(
    SuspensionReviewSettings s) noexcept
{
    return Finite(s.MinimumCompressionCm) &&
        Finite(s.MaximumCompressionCm) &&
        s.MaximumCompressionCm >
            s.MinimumCompressionCm &&
        Finite(s.BottomingFraction) &&
        s.BottomingFraction > 0.0 &&
        s.BottomingFraction <= 1.0 &&
        Finite(s.ToppingFraction) &&
        s.ToppingFraction >= 0.0 &&
        s.ToppingFraction < s.BottomingFraction &&
        Finite(s.MinimumWheelLoadN) &&
        s.MinimumWheelLoadN >= 0.0 &&
        Finite(s.MaximumWheelLoadN) &&
        s.MaximumWheelLoadN > s.MinimumWheelLoadN &&
        Finite(s.ExcessiveLeftRightLoadDifference) &&
        s.ExcessiveLeftRightLoadDifference > 0.0 &&
        Finite(s.PitchWarningRadians) &&
        s.PitchWarningRadians > 0.0 &&
        Finite(s.RollWarningRadians) &&
        s.RollWarningRadians > 0.0 &&
        Finite(s.MaximumCompressionVelocityCmS) &&
        s.MaximumCompressionVelocityCmS > 0.0 &&
        Finite(s.MaximumCompressionAccelerationCmS2) &&
        s.MaximumCompressionAccelerationCmS2 > 0.0 &&
        Finite(s.MaximumSampleSeconds) &&
        s.MaximumSampleSeconds > 0.0 &&
        Finite(s.OscillationFrequencyThresholdHz) &&
        s.OscillationFrequencyThresholdHz > 0.0;
}

struct SuspensionSample final
{
    std::array<double, 4> CompressionCm{};
    std::array<double, 4> NormalLoadNewtons{};
    std::array<bool, 4> Contact{};
    double TimeSeconds = 0.0;
    double SpeedCmS = 0.0;
    double RollRadians = 0.0;
    double PitchRadians = 0.0;
    double LateralAccelerationG = 0.0;
    double LongitudinalAccelerationG = 0.0;
    double RoadBumpHeightCm = 0.0;
    double RideHeightCm = 0.0;
    bool FourWheelSuspensionAvailable = false;
    bool OriginalEngineObserved = false;
};

struct SuspensionWheelReview final
{
    double CompressionCm = 0.0;
    double CompressionFraction = 0.0;
    double CompressionVelocityCmS = 0.0;
    double CompressionAccelerationCmS2 = 0.0;
    double NormalLoadNewtons = 0.0;
    double LoadFraction = 0.0;
    double LongestBottomingSeconds = 0.0;
    double BottomingSeconds = 0.0;
    double ToppingSeconds = 0.0;
    double ContactLostSeconds = 0.0;
    double MaximumCompressionCm = 0.0;
    double MinimumCompressionCm = 0.0;
    double MeanCompressionCm = 0.0;
    int Oscillations = 0;
    int BottomingEvents = 0;
    int ContactLossEvents = 0;
    bool Bottoming = false;
    bool Topping = false;
    bool Supported = false;
    bool Valid = false;
};

struct SuspensionReview final
{
    std::array<SuspensionWheelReview, 4> Wheels{};
    double FrontMeanCompressionCm = 0.0;
    double RearMeanCompressionCm = 0.0;
    double LeftMeanCompressionCm = 0.0;
    double RightMeanCompressionCm = 0.0;
    double FrontLoadFraction = 0.0;
    double RearLoadFraction = 0.0;
    double LateralLoadTransferFraction = 0.0;
    double LongitudinalLoadTransferFraction = 0.0;
    double MeanRideHeightCm = 0.0;
    double MaximumRollRadians = 0.0;
    double MaximumPitchRadians = 0.0;
    double MaximumBumpHeightCm = 0.0;
    double TotalObservedSeconds = 0.0;
    double TotalBottomingSeconds = 0.0;
    double TotalContactLossSeconds = 0.0;
    double MeanCompressionRangeCm = 0.0;
    int BottomingWheelCount = 0;
    int ContactLossWheelCount = 0;
    int QualifiedFrames = 0;
    int RejectedFrames = 0;
    bool PoorDampingSuspected = false;
    bool RideHeightRisk = false;
    bool EngineQualified = false;
    bool AdvisoryOnly = true;
    bool Valid = false;
};

inline bool ValidSuspensionSample(
    SuspensionSample s,
    SuspensionReviewSettings settings) noexcept
{
    if (!s.OriginalEngineObserved ||
        !s.FourWheelSuspensionAvailable ||
        !Finite(s.TimeSeconds) ||
        !Finite(s.SpeedCmS) ||
        !Finite(s.RollRadians) ||
        !Finite(s.PitchRadians) ||
        !Finite(s.LateralAccelerationG) ||
        !Finite(s.LongitudinalAccelerationG) ||
        !Finite(s.RoadBumpHeightCm) ||
        !Finite(s.RideHeightCm) ||
        s.TimeSeconds < 0.0 ||
        s.SpeedCmS < 0.0 ||
        s.SpeedCmS > MaximumPlanarSpeedCmS ||
        std::abs(s.RollRadians) > Pi ||
        std::abs(s.PitchRadians) > Pi)
    {
        return false;
    }
    for (std::size_t i = 0; i < 4; ++i)
    {
        if (!Finite(s.CompressionCm[i]) ||
            !Finite(s.NormalLoadNewtons[i]) ||
            s.CompressionCm[i] <
                settings.MinimumCompressionCm - 30.0 ||
            s.CompressionCm[i] >
                settings.MaximumCompressionCm + 30.0 ||
            s.NormalLoadNewtons[i] < 0.0 ||
            s.NormalLoadNewtons[i] >
                settings.MaximumWheelLoadN)
        {
            return false;
        }
    }
    return true;
}

class SuspensionEventMonitor final
{
public:
    void Reset() noexcept
    {
        Settings = {};
        Previous = {};
        HasPrevious = false;
        Last = {};
        MaxCompression.fill(
            -std::numeric_limits<double>::infinity());
        MinCompression.fill(
            std::numeric_limits<double>::infinity());
        SumCompression.fill(0.0);
        PreviousVelocity.fill(0.0);
        PreviousVelocitySign.fill(0);
        Oscillations.fill(0);
        BottomEvents.fill(0);
        ContactEvents.fill(0);
        BottomSeconds.fill(0.0);
        TopSeconds.fill(0.0);
        ContactSeconds.fill(0.0);
        LastBottoming.fill(false);
        LastContact.fill(false);
        TotalSeconds = 0.0;
        TotalRideHeight = 0.0;
        MaximumRoll = 0.0;
        MaximumPitch = 0.0;
        MaximumBump = 0.0;
        Frames = 0;
        Rejected = 0;
    }

    bool Configure(
        SuspensionReviewSettings settings) noexcept
    {
        if (!ValidSuspensionReviewSettings(settings))
        {
            return false;
        }
        Reset();
        Settings = settings;
        return true;
    }

    bool Observe(SuspensionSample sample) noexcept
    {
        if (!ValidSuspensionSample(sample, Settings))
        {
            ++Rejected;
            return false;
        }
        double dt = 0.0;
        if (HasPrevious)
        {
            dt = sample.TimeSeconds -
                Previous.TimeSeconds;
            if (dt <= 0.0 ||
                dt > Settings.MaximumSampleSeconds)
            {
                ++Rejected;
                Previous = sample;
                return false;
            }
        }
        const double fullTravel =
            Settings.MaximumCompressionCm -
            Settings.MinimumCompressionCm;
        double totalNormalLoad = 0.0;
        for (double load : sample.NormalLoadNewtons)
        {
            totalNormalLoad += load;
        }
        SuspensionReview result = Last;
        for (std::size_t i = 0; i < 4; ++i)
        {
            SuspensionWheelReview& w =
                result.Wheels[i];
            const double compression =
                sample.CompressionCm[i];
            const double normalized = Clamp01(
                SafeRatio(
                    compression -
                    Settings.MinimumCompressionCm,
                    fullTravel));
            const bool bottom =
                normalized >= Settings.BottomingFraction;
            const bool top =
                normalized <= Settings.ToppingFraction;
            const bool contact =
                sample.Contact[i] &&
                sample.NormalLoadNewtons[i] >=
                    Settings.MinimumWheelLoadN;
            double velocity = 0.0;
            double acceleration = 0.0;
            if (HasPrevious)
            {
                velocity =
                    (compression -
                     Previous.CompressionCm[i]) / dt;
                acceleration =
                    (velocity - PreviousVelocity[i]) / dt;
                if (std::abs(velocity) >
                        Settings.MaximumCompressionVelocityCmS ||
                    std::abs(acceleration) >
                        Settings.MaximumCompressionAccelerationCmS2)
                {
                    ++Rejected;
                    Previous = sample;
                    return false;
                }
                const int sign = velocity > 1.0 ?
                    1 : velocity < -1.0 ? -1 : 0;
                if (sign != 0 &&
                    PreviousVelocitySign[i] != 0 &&
                    sign != PreviousVelocitySign[i])
                {
                    ++Oscillations[i];
                }
                if (sign != 0)
                {
                    PreviousVelocitySign[i] = sign;
                }
            }
            PreviousVelocity[i] = velocity;
            if (bottom && !LastBottoming[i])
            {
                ++BottomEvents[i];
            }
            if (!contact && LastContact[i])
            {
                ++ContactEvents[i];
            }
            LastBottoming[i] = bottom;
            LastContact[i] = contact;
            if (bottom)
            {
                BottomSeconds[i] += dt;
            }
            if (top)
            {
                TopSeconds[i] += dt;
            }
            if (!contact)
            {
                ContactSeconds[i] += dt;
            }
            MaxCompression[i] =
                std::max(MaxCompression[i], compression);
            MinCompression[i] =
                std::min(MinCompression[i], compression);
            SumCompression[i] += compression;
            w.CompressionCm = compression;
            w.CompressionFraction = normalized;
            w.CompressionVelocityCmS = velocity;
            w.CompressionAccelerationCmS2 = acceleration;
            w.NormalLoadNewtons =
                sample.NormalLoadNewtons[i];
            w.LoadFraction = SafeRatio(
                w.NormalLoadNewtons, totalNormalLoad);
            w.Bottoming = bottom;
            w.Topping = top;
            w.Supported = contact;
            w.BottomingSeconds = BottomSeconds[i];
            w.ToppingSeconds = TopSeconds[i];
            w.ContactLostSeconds = ContactSeconds[i];
            w.MaximumCompressionCm = MaxCompression[i];
            w.MinimumCompressionCm = MinCompression[i];
            w.MeanCompressionCm = SafeRatio(
                SumCompression[i], Frames + 1);
            w.Oscillations = Oscillations[i];
            w.BottomingEvents = BottomEvents[i];
            w.ContactLossEvents = ContactEvents[i];
            w.Valid = true;
        }
        TotalSeconds += dt;
        TotalRideHeight += sample.RideHeightCm;
        MaximumRoll = std::max(
            MaximumRoll, std::abs(sample.RollRadians));
        MaximumPitch = std::max(
            MaximumPitch, std::abs(sample.PitchRadians));
        MaximumBump = std::max(
            MaximumBump, std::abs(sample.RoadBumpHeightCm));
        ++Frames;
        result.FrontMeanCompressionCm = 0.5 * (
            result.Wheels[0].MeanCompressionCm +
            result.Wheels[1].MeanCompressionCm);
        result.RearMeanCompressionCm = 0.5 * (
            result.Wheels[2].MeanCompressionCm +
            result.Wheels[3].MeanCompressionCm);
        result.LeftMeanCompressionCm = 0.5 * (
            result.Wheels[0].MeanCompressionCm +
            result.Wheels[2].MeanCompressionCm);
        result.RightMeanCompressionCm = 0.5 * (
            result.Wheels[1].MeanCompressionCm +
            result.Wheels[3].MeanCompressionCm);
        result.FrontLoadFraction =
            result.Wheels[0].LoadFraction +
            result.Wheels[1].LoadFraction;
        result.RearLoadFraction =
            result.Wheels[2].LoadFraction +
            result.Wheels[3].LoadFraction;
        result.LateralLoadTransferFraction =
            result.Wheels[0].LoadFraction +
            result.Wheels[2].LoadFraction -
            result.Wheels[1].LoadFraction -
            result.Wheels[3].LoadFraction;
        result.LongitudinalLoadTransferFraction =
            result.FrontLoadFraction -
            result.RearLoadFraction;
        result.MeanRideHeightCm =
            TotalRideHeight / Frames;
        result.MaximumRollRadians = MaximumRoll;
        result.MaximumPitchRadians = MaximumPitch;
        result.MaximumBumpHeightCm = MaximumBump;
        result.TotalObservedSeconds = TotalSeconds;
        result.TotalBottomingSeconds = 0.0;
        result.TotalContactLossSeconds = 0.0;
        result.BottomingWheelCount = 0;
        result.ContactLossWheelCount = 0;
        result.MeanCompressionRangeCm = 0.0;
        result.PoorDampingSuspected = false;
        for (std::size_t i = 0; i < 4; ++i)
        {
            result.TotalBottomingSeconds +=
                BottomSeconds[i];
            result.TotalContactLossSeconds +=
                ContactSeconds[i];
            result.BottomingWheelCount +=
                result.Wheels[i].Bottoming ? 1 : 0;
            result.ContactLossWheelCount +=
                !result.Wheels[i].Supported ? 1 : 0;
            result.MeanCompressionRangeCm +=
                MaxCompression[i] - MinCompression[i];
            if (TotalSeconds > 1.0 &&
                Oscillations[i] / TotalSeconds >=
                    Settings.OscillationFrequencyThresholdHz)
            {
                result.PoorDampingSuspected = true;
            }
        }
        result.MeanCompressionRangeCm *= 0.25;
        result.RideHeightRisk =
            result.BottomingWheelCount > 0 ||
            MaximumRoll >= Settings.RollWarningRadians ||
            MaximumPitch >= Settings.PitchWarningRadians;
        result.QualifiedFrames = Frames;
        result.RejectedFrames = Rejected;
        result.EngineQualified =
            sample.OriginalEngineObserved;
        result.AdvisoryOnly = true;
        result.Valid = !Settings.RequireCompleteContact ||
            result.ContactLossWheelCount == 0;
        Last = result;
        Previous = sample;
        HasPrevious = true;
        return true;
    }

    SuspensionReview Snapshot() const noexcept
    {
        return Last;
    }

    int QualifiedFrames() const noexcept
    {
        return Frames;
    }

    int RejectedFrames() const noexcept
    {
        return Rejected;
    }

private:
    SuspensionReviewSettings Settings{};
    SuspensionSample Previous{};
    SuspensionReview Last{};
    std::array<double, 4> MaxCompression{};
    std::array<double, 4> MinCompression{};
    std::array<double, 4> SumCompression{};
    std::array<double, 4> PreviousVelocity{};
    std::array<int, 4> PreviousVelocitySign{};
    std::array<int, 4> Oscillations{};
    std::array<int, 4> BottomEvents{};
    std::array<int, 4> ContactEvents{};
    std::array<double, 4> BottomSeconds{};
    std::array<double, 4> TopSeconds{};
    std::array<double, 4> ContactSeconds{};
    std::array<bool, 4> LastBottoming{};
    std::array<bool, 4> LastContact{};
    double TotalSeconds = 0.0;
    double TotalRideHeight = 0.0;
    double MaximumRoll = 0.0;
    double MaximumPitch = 0.0;
    double MaximumBump = 0.0;
    int Frames = 0;
    int Rejected = 0;
    bool HasPrevious = false;
};
} // namespace latebrake::engineering
