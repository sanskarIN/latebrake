#pragma once
#include "Racecraft/ChassisLoadTransfer.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
struct SuspensionParameters final
{
    double SpringRateNPerCm = 95.0;
    double BumpDamperNPerCmS = 1.4;
    double ReboundDamperNPerCmS = 2.2;
    double MaximumCompressionCm = 26.0;
    double MaximumExtensionCm = 14.0;
    double StaticCompressionCm = 12.0;
    double BumpStopStartCm = 21.0;
    double BumpStopStiffnessNPerCm2 = 35.0;
    double ReboundStopStartCm = -11.0;
    double ReboundStopStiffnessNPerCm2 = 25.0;
    double MaxDampingForceN = 7000.0;
    double MaximumSpringForceN = 18000.0;
    double MaximumPerFrameTravelCm = 5.0;
    double MinimumPhysicalContactLoadN = 30.0;
    double AntiRollBarStiffnessNPerCm = 180.0;
    double RollCenterHeightCm = 16.0;
    double MotionRatio = 0.95;
    double DamperFadeStartC = 120.0;
    double DamperFadeCriticalC = 210.0;
    double DamperHeatTransferCoefficient = 0.0002;
    double DamperCoolingTimeConstantSeconds = 30.0;
};
struct SuspensionWheelState final
{
    double CompressionCm = 0.0;
    double CompressionVelocityCmS = 0.0;
    double SpringForceN = 0.0;
    double DamperForceN = 0.0;
    double BumpStopForceN = 0.0;
    double ReboundStopForceN = 0.0;
    double AntiRollBarForceN = 0.0;
    double NetVerticalForceN = 0.0;
    double DamperTemperatureC = 35.0;
    double DamperFadeFraction = 0.0;
    double AbsorbedEnergyJ = 0.0;
    double ExtensionCm = 0.0;
    bool Bottoming = false;
    bool Topping = false;
    bool Contact = false;
    bool Qualified = false;
};
struct SuspensionFrameInput final
{
    std::array<double, 4> CompressionCm{};
    std::array<double, 4> NormalLoadN{};
    std::array<double, 4> RoadElevationCm{};
    std::array<bool, 4> ContactVerified{};
    double RollRadians = 0.0;
    double PitchRadians = 0.0;
    double AmbientTemperatureC = 25.0;
    double DeltaSeconds = 0.0;
    bool BodyPhysicsReady = false;
    bool EngineWheelSensorsReady = false;
};
struct SuspensionForceFrame final
{
    std::array<SuspensionWheelState, 4> Wheels{};
    double FrontAxleForceN = 0.0;
    double RearAxleForceN = 0.0;
    double TotalVerticalForceN = 0.0;
    double NetRollMomentNm = 0.0;
    double NetPitchMomentNm = 0.0;
    double DissipatedEnergyJ = 0.0;
    double MeanDamperFade = 0.0;
    double MeanRideCompressionCm = 0.0;
    double TotalAntiRollForceN = 0.0;
    int BottomingWheels = 0;
    int ToppingWheels = 0;
    int ValidContactWheels = 0;
    bool PhysicalContactQualified = false;
    bool SourcePhysicsOnly = true;
    bool Valid = false;
};
inline bool ValidSuspensionParameters(
    SuspensionParameters s) noexcept
{
    return Finite(s.SpringRateNPerCm) &&
        s.SpringRateNPerCm > 0.0 &&
        Finite(s.BumpDamperNPerCmS) &&
        s.BumpDamperNPerCmS >= 0.0 &&
        Finite(s.ReboundDamperNPerCmS) &&
        s.ReboundDamperNPerCmS >= 0.0 &&
        Finite(s.MaximumCompressionCm) &&
        s.MaximumCompressionCm > 0.0 &&
        Finite(s.MaximumExtensionCm) &&
        s.MaximumExtensionCm > 0.0 &&
        Finite(s.StaticCompressionCm) &&
        s.StaticCompressionCm >= 0.0 &&
        s.StaticCompressionCm < s.MaximumCompressionCm &&
        Finite(s.BumpStopStartCm) &&
        s.BumpStopStartCm > s.StaticCompressionCm &&
        s.BumpStopStartCm <= s.MaximumCompressionCm &&
        Finite(s.BumpStopStiffnessNPerCm2) &&
        s.BumpStopStiffnessNPerCm2 >= 0.0 &&
        Finite(s.ReboundStopStartCm) &&
        s.ReboundStopStartCm < 0.0 &&
        Finite(s.ReboundStopStiffnessNPerCm2) &&
        s.ReboundStopStiffnessNPerCm2 >= 0.0 &&
        Finite(s.MaxDampingForceN) &&
        s.MaxDampingForceN > 0.0 &&
        Finite(s.MaximumSpringForceN) &&
        s.MaximumSpringForceN > 0.0 &&
        Finite(s.MaximumPerFrameTravelCm) &&
        s.MaximumPerFrameTravelCm > 0.0 &&
        Finite(s.MinimumPhysicalContactLoadN) &&
        s.MinimumPhysicalContactLoadN >= 0.0 &&
        Finite(s.AntiRollBarStiffnessNPerCm) &&
        s.AntiRollBarStiffnessNPerCm >= 0.0 &&
        Finite(s.MotionRatio) && s.MotionRatio > 0.0 &&
        Finite(s.DamperFadeStartC) &&
        Finite(s.DamperFadeCriticalC) &&
        s.DamperFadeCriticalC > s.DamperFadeStartC &&
        Finite(s.DamperHeatTransferCoefficient) &&
        s.DamperHeatTransferCoefficient >= 0.0 &&
        Finite(s.DamperCoolingTimeConstantSeconds) &&
        s.DamperCoolingTimeConstantSeconds > 0.0;
}
inline bool ValidSuspensionFrameInput(
    const SuspensionFrameInput& s,
    const SuspensionParameters& settings) noexcept
{
    if (!s.BodyPhysicsReady || !s.EngineWheelSensorsReady ||
        !ValidFrameDelta(s.DeltaSeconds) ||
        !Finite(s.RollRadians) ||
        !Finite(s.PitchRadians) ||
        !Finite(s.AmbientTemperatureC) ||
        std::abs(s.RollRadians) > Pi * 0.6 ||
        std::abs(s.PitchRadians) > Pi * 0.6)
        return false;
    for (std::size_t i = 0; i < 4; ++i)
    {
        if (!Finite(s.CompressionCm[i]) ||
            !Finite(s.NormalLoadN[i]) ||
            !Finite(s.RoadElevationCm[i]) ||
            s.CompressionCm[i] <
                -settings.MaximumExtensionCm ||
            s.CompressionCm[i] >
                settings.MaximumCompressionCm ||
            s.NormalLoadN[i] < 0.0 ||
            s.NormalLoadN[i] > 300000.0)
            return false;
    }
    return true;
}
inline SuspensionWheelState ComputeSuspensionWheel(
    double compressionCm,
    double velocityCmS,
    double contactLoadN,
    bool verifiedContact,
    double leftRightBarForceN,
    double previousDamperC,
    double ambientC, double dt,
    SuspensionParameters p) noexcept
{
    SuspensionWheelState out;
    out.CompressionCm = compressionCm;
    out.CompressionVelocityCmS = velocityCmS;
    out.ExtensionCm = std::max(
        0.0, -compressionCm);
    out.Contact = verifiedContact &&
        contactLoadN >= p.MinimumPhysicalContactLoadN;
    out.Bottoming =
        compressionCm >= p.BumpStopStartCm;
    out.Topping =
        compressionCm <= p.ReboundStopStartCm;
    const double springTravel =
        compressionCm - p.StaticCompressionCm;
    out.SpringForceN = std::clamp(
        p.SpringRateNPerCm * springTravel *
            p.MotionRatio * p.MotionRatio,
        -p.MaximumSpringForceN,
        p.MaximumSpringForceN);
    const double damperRate = velocityCmS >= 0.0 ?
        p.BumpDamperNPerCmS : p.ReboundDamperNPerCmS;
    out.DamperFadeFraction = Smoothstep(
        p.DamperFadeStartC,
        p.DamperFadeCriticalC,
        previousDamperC);
    out.DamperForceN = std::clamp(
        damperRate * velocityCmS *
        (1.0 - 0.65 * out.DamperFadeFraction),
        -p.MaxDampingForceN, p.MaxDampingForceN);
    const double bumpDepth =
        std::max(0.0,
            compressionCm - p.BumpStopStartCm);
    const double topDepth =
        std::max(0.0,
            p.ReboundStopStartCm - compressionCm);
    out.BumpStopForceN =
        p.BumpStopStiffnessNPerCm2 * Square(bumpDepth);
    out.ReboundStopForceN =
        -p.ReboundStopStiffnessNPerCm2 * Square(topDepth);
    out.AntiRollBarForceN =
        leftRightBarForceN;
    out.NetVerticalForceN =
        out.Contact ?
        out.SpringForceN +
            out.DamperForceN +
            out.BumpStopForceN +
            out.ReboundStopForceN +
            out.AntiRollBarForceN : 0.0;
    out.AbsorbedEnergyJ =
        std::abs(out.DamperForceN *
                 velocityCmS /
                 CentimetersPerMeter) * dt;
    const double sourceC =
        previousDamperC + out.AbsorbedEnergyJ *
        p.DamperHeatTransferCoefficient;
    out.DamperTemperatureC = ExponentialResponse(
        sourceC, ambientC,
        p.DamperCoolingTimeConstantSeconds, dt);
    out.Qualified = Finite(out.NetVerticalForceN) &&
        Finite(out.DamperTemperatureC);
    return out;
}
class FourCornerSuspension final
{
public:
    bool Configure(SuspensionParameters front,
                   SuspensionParameters rear) noexcept
    {
        if (!ValidSuspensionParameters(front) ||
            !ValidSuspensionParameters(rear))
            return false;
        Front = front;
        Rear = rear;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last = {};
        LastCompression.fill(0.0);
        DamperHeat.fill(30.0);
        HasHistory = false;
        Frames = 0;
        Rejections = 0;
        BottomEvents = 0;
        ContactLostFrames = 0;
        AbsorbedEnergyJ = 0.0;
    }
    SuspensionForceFrame Step(
        SuspensionFrameInput input,
        double wheelbaseCm, double frontTrackCm,
        double rearTrackCm) noexcept
    {
        SuspensionForceFrame out;
        if (!ValidSuspensionFrameInput(input, Front) ||
            !ValidSuspensionFrameInput(input, Rear) ||
            !Finite(wheelbaseCm) || wheelbaseCm <= 0.0 ||
            !Finite(frontTrackCm) || frontTrackCm <= 0.0 ||
            !Finite(rearTrackCm) || rearTrackCm <= 0.0)
        {
            ++Rejections;
            return out;
        }
        for (std::size_t i = 0; i < 4; ++i)
        {
            const SuspensionParameters& settings =
                i < 2 ? Front : Rear;
            const double previous =
                HasHistory ?
                LastCompression[i] : input.CompressionCm[i];
            const double delta = input.CompressionCm[i] -
                previous;
            if (HasHistory &&
                std::abs(delta) >
                    settings.MaximumPerFrameTravelCm)
            {
                ++Rejections;
                HasHistory = false;
                return {};
            }
            const std::size_t partner = i % 2 == 0 ?
                i + 1 : i - 1;
            const double bar = settings.AntiRollBarStiffnessNPerCm *
                (input.CompressionCm[i] -
                 input.CompressionCm[partner]) *
                0.5;
            const double velocity = HasHistory ?
                delta / input.DeltaSeconds : 0.0;
            out.Wheels[i] = ComputeSuspensionWheel(
                input.CompressionCm[i], velocity,
                input.NormalLoadN[i], input.ContactVerified[i],
                bar, DamperHeat[i],
                input.AmbientTemperatureC,
                input.DeltaSeconds, settings);
            if (!out.Wheels[i].Qualified)
            {
                ++Rejections;
                return {};
            }
            LastCompression[i] =
                input.CompressionCm[i];
            DamperHeat[i] =
                out.Wheels[i].DamperTemperatureC;
            out.TotalVerticalForceN +=
                out.Wheels[i].NetVerticalForceN;
            out.TotalAntiRollForceN +=
                std::abs(bar);
            out.DissipatedEnergyJ +=
                out.Wheels[i].AbsorbedEnergyJ;
            out.MeanDamperFade +=
                out.Wheels[i].DamperFadeFraction;
            out.MeanRideCompressionCm +=
                input.CompressionCm[i];
            out.BottomingWheels +=
                out.Wheels[i].Bottoming ? 1 : 0;
            out.ToppingWheels +=
                out.Wheels[i].Topping ? 1 : 0;
            out.ValidContactWheels +=
                out.Wheels[i].Contact ? 1 : 0;
            if (i < 2)
                out.FrontAxleForceN +=
                    out.Wheels[i].NetVerticalForceN;
            else
                out.RearAxleForceN +=
                    out.Wheels[i].NetVerticalForceN;
        }
        out.MeanDamperFade *= 0.25;
        out.MeanRideCompressionCm *= 0.25;
        out.NetRollMomentNm =
            ((out.Wheels[0].NetVerticalForceN -
              out.Wheels[1].NetVerticalForceN) *
                frontTrackCm +
             (out.Wheels[2].NetVerticalForceN -
              out.Wheels[3].NetVerticalForceN) *
                rearTrackCm) / (2.0 * CentimetersPerMeter);
        out.NetPitchMomentNm =
            (out.FrontAxleForceN -
             out.RearAxleForceN) * wheelbaseCm /
            (2.0 * CentimetersPerMeter);
        out.PhysicalContactQualified =
            out.ValidContactWheels >= 2;
        out.SourcePhysicsOnly = true;
        out.Valid = Finite(out.TotalVerticalForceN) &&
            Finite(out.NetRollMomentNm);
        HasHistory = true;
        Last = out;
        AbsorbedEnergyJ += out.DissipatedEnergyJ;
        ++Frames;
        BottomEvents += static_cast<std::uint64_t>(
            out.BottomingWheels);
        ContactLostFrames +=
            out.ValidContactWheels < 4 ? 1U : 0U;
        return out;
    }
    SuspensionForceFrame Snapshot() const noexcept
    {
        return Last;
    }
    std::uint64_t QualifiedFrames() const noexcept { return Frames; }
    std::uint64_t RejectedFrames() const noexcept
    {
        return Rejections;
    }
    std::uint64_t ContactLossFrames() const noexcept
    {
        return ContactLostFrames;
    }
    double TotalAbsorbedEnergyJ() const noexcept
    {
        return AbsorbedEnergyJ;
    }
private:
    SuspensionParameters Front{};
    SuspensionParameters Rear{};
    SuspensionForceFrame Last{};
    std::array<double, 4> LastCompression{};
    std::array<double, 4> DamperHeat{};
    std::uint64_t Frames = 0;
    std::uint64_t Rejections = 0;
    std::uint64_t BottomEvents = 0;
    std::uint64_t ContactLostFrames = 0;
    double AbsorbedEnergyJ = 0.0;
    bool HasHistory = false;
};
} // namespace latebrake::racecraft
