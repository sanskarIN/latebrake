
#pragma once
#include "PreviewGarageStyle.h"
#include <cmath>

namespace latebrake_preview
{
enum class PreviewPurchaseChoice
{
    Rejected, Offer, Confirm
};

// Pure, per-session two-action purchase intent. The game still owns all
// credits, ownership, transactional SaveGame, and actual input focus gates.
struct PreviewGaragePurchaseConsent final
{
    static constexpr float OfferLifetimeSeconds=8.0f;
    int Target=-1;
    float Remaining=0.0f;

    void Reset() noexcept {Target=-1;Remaining=0.0f;}
    bool PendingFor(int id) const noexcept
    {
        return ValidPreviewCar(id) && Target==id &&
               std::isfinite(Remaining) && Remaining>0.0f &&
               Remaining<=OfferLifetimeSeconds;
    }
    void Advance(float dt) noexcept
    {
        if(!std::isfinite(dt)||dt<0.0f){Reset();return;}
        if(Target<0)return;
        if(dt>=Remaining){Reset();return;}
        Remaining-=dt;
    }
    PreviewPurchaseChoice Request(
        int viewed,unsigned int owned,int credits,int price) noexcept
    {
        if(!ValidPreviewCar(viewed) ||
           (owned & (1u<<static_cast<unsigned int>(viewed)))!=0u ||
           credits<0 || price<=0 || credits<price)
        {
            Reset();return PreviewPurchaseChoice::Rejected;
        }
        if(PendingFor(viewed))
        {
            Reset();return PreviewPurchaseChoice::Confirm;
        }
        Target=viewed;
        Remaining=OfferLifetimeSeconds;
        return PreviewPurchaseChoice::Offer;
    }
};
} // namespace latebrake_preview
