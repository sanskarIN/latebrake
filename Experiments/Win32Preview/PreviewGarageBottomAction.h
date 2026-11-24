#pragma once
#include "PreviewGarageSelection.h"

// Real button partition in the 960x640 Win32 2D showroom.
// A locked preview can only request the existing two-step unlock flow.
// An owned UNEQUIPPED preview has two nonoverlapping targets:
// EQUIP ONLY (does not race), or EQUIP + RACE (requires a durable save).
// All actions are invalid outside the actual bottom dock hitbox.
namespace latebrake_preview
{
enum class PreviewGarageBottomAction
{
    None, EquipOnly, Race, Unlock
};
constexpr PreviewGarageBottomAction PreviewGarageButtonAction(
    int x,int y,int viewed,int equipped,unsigned int ownedMask) noexcept
{
    if(x<477||x>=870||y<520||y>=554 ||
       !ValidPreviewCar(viewed)||!ValidPreviewCar(equipped))
        return PreviewGarageBottomAction::None;
    if(!OwnsPreviewCar(ownedMask,viewed))
        return PreviewGarageBottomAction::Unlock;
    if(viewed==equipped)return PreviewGarageBottomAction::Race;
    if(x<662)return PreviewGarageBottomAction::EquipOnly;
    if(x>=672)return PreviewGarageBottomAction::Race;
    return PreviewGarageBottomAction::None;
}
static_assert(PreviewGarageButtonAction(
    500,534,5,0,1u|(1u<<5))==PreviewGarageBottomAction::EquipOnly);
static_assert(PreviewGarageButtonAction(
    666,534,5,0,1u|(1u<<5))==PreviewGarageBottomAction::None);
static_assert(PreviewGarageButtonAction(
    730,534,5,0,1u|(1u<<5))==PreviewGarageBottomAction::Race);
static_assert(PreviewGarageButtonAction(
    500,534,5,0,1u)==PreviewGarageBottomAction::Unlock);
} // namespace latebrake_preview
