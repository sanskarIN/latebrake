#pragma once

#include "Garage/GarageAppearance.h"

namespace latebrake
{
// Direct selection is an optimistic UI request, not ownership authority.
// The caller supplies the exact displayed vehicle and look so an old
// pointer event cannot overwrite a newer selection or another car.
struct GarageDirectAppearanceRequest final
{
    int VehicleId = -1;
    GarageLook ExpectedLook{};
    GarageLookLayer Layer = GarageLookLayer::Paint;
    int Choice = -1;
};

enum class GarageDirectAppearanceStatus
{
    Invalid, Stale, Unchanged, Ready
};

struct GarageDirectAppearanceDecision final
{
    GarageDirectAppearanceStatus Status = GarageDirectAppearanceStatus::Invalid;
    GarageLook Result{};
};

constexpr int GarageLookChoice(GarageLook look, GarageLookLayer layer) noexcept
{
    switch (layer)
    {
    case GarageLookLayer::Paint: return look.Paint;
    case GarageLookLayer::Stripe: return look.Stripe;
    case GarageLookLayer::Wheels: return look.Wheels;
    default: return -1;
    }
}

constexpr GarageDirectAppearanceDecision PreviewGarageDirectAppearance(
    int viewedId, GarageLook current, GarageDirectAppearanceRequest request) noexcept
{
    if (!GarageState::IsVehicle(viewedId) ||
        !GarageState::IsVehicle(request.VehicleId) ||
        !ValidGarageLook(current) || !ValidGarageLook(request.ExpectedLook) ||
        request.Choice < 0 || request.Choice >= GarageLookChoiceCount(request.Layer))
        return {};
    if (viewedId != request.VehicleId || !SameGarageLook(current, request.ExpectedLook))
        return {GarageDirectAppearanceStatus::Stale, current};
    if (GarageLookChoice(current, request.Layer) == request.Choice)
        return {GarageDirectAppearanceStatus::Unchanged, current};

    auto next = current;
    switch (request.Layer)
    {
    case GarageLookLayer::Paint: next.Paint=static_cast<std::uint8_t>(request.Choice); break;
    case GarageLookLayer::Stripe: next.Stripe=static_cast<std::uint8_t>(request.Choice); break;
    case GarageLookLayer::Wheels: next.Wheels=static_cast<std::uint8_t>(request.Choice); break;
    default: return {};
    }
    return {GarageDirectAppearanceStatus::Ready, next};
}
} // namespace latebrake
