
#pragma once
#include "PreviewGarageStyle.h"
// No C runtime dependency: the actual Win64 GDI trial links /NODEFAULTLIB.
// Comparisons reject NaN/infinity and overlarge durations without CRT calls.

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
               Remaining>=0.0f && Remaining>0.0f &&
               Remaining<=OfferLifetimeSeconds;
    }
    void Advance(float dt) noexcept
    {
        if(!(dt>=0.0f && dt<=1000000.0f)){Reset();return;}
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
