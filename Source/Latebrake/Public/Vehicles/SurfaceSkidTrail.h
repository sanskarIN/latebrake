#pragma once

#include "Vehicles/SkidTrailGeometry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake
{
struct SurfaceSkidContact final
{
    bool Supported = false;
    bool PavedRoad = false;
    std::uint64_t SurfaceKey = 0;
    SkidVector Position{};
    SkidVector Normal{};
    double ForwardCmPerSecond = 0.0;
    double LateralCmPerSecond = 0.0;
    double NormalLoadNewtons = 0.0;
    bool ModeledSlipAvailable = false;
    double ModeledBrakingSlip = 0.0;
};

struct SurfaceSkidFrame final
{
    double DeltaSeconds = 0.0;
    std::uint64_t TeleportRevision = 0;
    bool DrivingAllowed = false;
    std::array<SurfaceSkidContact, 4> Contacts{};
};

struct SkidStripBatch final
{
    bool Valid = false;
    unsigned Count = 0;
    std::array<SkidStrip, 16> Strips{};
};

// Bounded, distance-spaced visual trail sampler. No force, grip, heat,
// contact, race clock or saved result can be produced by this class.
class SurfaceSkidTrail final
{
public:
    static constexpr double MinimumSpacingCm = 22.0;
    static constexpr double MaximumPieceCm = 180.0;
    static constexpr double MaximumJumpCm = MaximumPieceCm * 4.0;

    void BreakContinuity() noexcept
    {
        Anchors = {};
        HasRevision = false;
    }

    SkidStripBatch Advance(const SurfaceSkidFrame& frame) noexcept
    {
        if (!frame.DrivingAllowed || !std::isfinite(frame.DeltaSeconds) ||
            frame.DeltaSeconds <= 0.0 || frame.DeltaSeconds > 0.10)
        {
            BreakContinuity();
            return {};
        }
        if (!HasRevision || Revision != frame.TeleportRevision)
        {
            Anchors = {};
            Revision = frame.TeleportRevision;
            HasRevision = true;
        }
        SkidStripBatch out;
        out.Valid = true;
        for (unsigned wheel = 0; wheel < 4; ++wheel)
        {
            const auto& contact = frame.Contacts[wheel];
            auto& anchor = Anchors[wheel];
            if (!CanStamp(contact))
            {
                anchor = {};
                continue;
            }
            const double intensity = Intensity(contact);
            if (intensity < 0.10)
            {
                anchor = {};
                continue;
            }
            if (!anchor.Valid || anchor.SurfaceKey != contact.SurfaceKey)
            {
                anchor = {true, contact.SurfaceKey, contact.Position, contact.Normal, 0.0};
                continue;
            }
            anchor.Seconds += frame.DeltaSeconds;
            const auto displacement = contact.Position - anchor.Position;
            const double distance = SkidLength(displacement);
            if (!std::isfinite(distance) || distance > MaximumJumpCm || anchor.Seconds > 0.20 ||
                SkidDot(anchor.Normal, contact.Normal) < 0.98)
            {
                anchor = {true, contact.SurfaceKey, contact.Position, contact.Normal, 0.0};
                continue;
            }
            if (distance < MinimumSpacingCm) continue;
            const auto normalSum = anchor.Normal + contact.Normal;
            const auto meanNormal = normalSum * (1.0 / SkidLength(normalSum));
            if (std::abs(SkidDot(displacement, meanNormal)) > 4.0)
            {
                // Resampling must not dilute a step-height discontinuity
                // into several apparently small, floating pieces.
                anchor = {true, contact.SurfaceKey, contact.Position, contact.Normal, 0.0};
                continue;
            }
            const unsigned pieces = static_cast<unsigned>(std::ceil(distance / MaximumPieceCm));
            if (pieces == 0 || pieces > 4 || out.Count + pieces > out.Strips.size())
            {
                anchor = {};
                continue;
            }
            // Prepare the entire wheel's batch before accepting any piece.
            // One invalid surface fit never leaves half a bridge rendered.
            std::array<SkidStrip, 4> proposed{};
            bool good = true;
            for (unsigned piece = 0; piece < pieces; ++piece)
            {
                const double a = static_cast<double>(piece) / pieces;
                const double b = static_cast<double>(piece + 1u) / pieces;
                auto firstNormal = anchor.Normal * (1.0-a) + contact.Normal * a;
                auto lastNormal = anchor.Normal * (1.0-b) + contact.Normal * b;
                firstNormal = firstNormal * (1.0 / SkidLength(firstNormal));
                lastNormal = lastNormal * (1.0 / SkidLength(lastNormal));
                proposed[piece] = MakeSkidStrip(anchor.Position + displacement * a,
                    anchor.Position + displacement * b, firstNormal, lastNormal, intensity);
                good = good && proposed[piece].Valid;
            }
            if (good)
                for (unsigned piece = 0; piece < pieces; ++piece)
                    out.Strips[out.Count++] = proposed[piece];
            anchor = {true, contact.SurfaceKey, contact.Position, contact.Normal, 0.0};
        }
        return out;
    }

    static double Intensity(const SurfaceSkidContact& c) noexcept
    {
        if (!std::isfinite(c.ForwardCmPerSecond) || !std::isfinite(c.LateralCmPerSecond) ||
            (c.ModeledSlipAvailable && !std::isfinite(c.ModeledBrakingSlip))) return 0.0;
        const double lateralRatio = std::abs(c.LateralCmPerSecond) /
            std::max(350.0, std::abs(c.ForwardCmPerSecond));
        const double lateral = std::clamp((lateralRatio - 0.10) / 0.38, 0.0, 1.0);
        const double braking = c.ModeledSlipAvailable
            ? std::clamp((std::abs(c.ModeledBrakingSlip) - 0.10) / 0.40, 0.0, 1.0) : 0.0;
        return std::max(lateral, braking);
    }

private:
    struct Anchor final
    {
        bool Valid = false;
        std::uint64_t SurfaceKey = 0;
        SkidVector Position{};
        SkidVector Normal{};
        double Seconds = 0.0;
    };

    static bool CanStamp(const SurfaceSkidContact& c) noexcept
    {
        return c.Supported && c.PavedRoad && c.SurfaceKey != 0 &&
            IsFiniteSkidVector(c.Position, 1.0e8) && IsSkidUnitNormal(c.Normal) &&
            std::isfinite(c.ForwardCmPerSecond) && std::abs(c.ForwardCmPerSecond) <= 15000.0 &&
            std::isfinite(c.LateralCmPerSecond) && std::abs(c.LateralCmPerSecond) <= 15000.0 &&
            std::hypot(c.ForwardCmPerSecond, c.LateralCmPerSecond) >= 450.0 &&
            std::isfinite(c.NormalLoadNewtons) && c.NormalLoadNewtons >= 30.0 &&
            c.NormalLoadNewtons <= 1000000.0 &&
            (!c.ModeledSlipAvailable || (std::isfinite(c.ModeledBrakingSlip) &&
                std::abs(c.ModeledBrakingSlip) <= 4.0));
    }

    std::array<Anchor, 4> Anchors{};
    std::uint64_t Revision = 0;
    bool HasRevision = false;
};
} // namespace latebrake
