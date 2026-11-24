#pragma once

#include "Career/CareerProgress.h"
#include "Garage/GarageAppearance.h"

namespace latebrake
{
// Pure policy for the Unreal Saved/LoadGameToSlot serialization boundary.
// This guards an older client that was already running when another build
// changed its local profile. Unknown/unreadable data is never overwritten
// simply because the older client can construct a fresh SaveGame object.
enum class ProfileSaveWriteState
{
    NewProfile, CompatibleProfile, UnreadableProfile,
    CorruptSchema, NewerCareer, NewerAppearance
};

constexpr ProfileSaveWriteState AssessProfileSaveWrite(
    bool slotExists, bool deserialized,
    int careerVersion, int appearanceVersion) noexcept
{
    if (!slotExists) return ProfileSaveWriteState::NewProfile;
    if (!deserialized) return ProfileSaveWriteState::UnreadableProfile;
    if (careerVersion < 0 || appearanceVersion < 0)
        return ProfileSaveWriteState::CorruptSchema;
    if (careerVersion > CareerSaveSchema)
        return ProfileSaveWriteState::NewerCareer;
    if (appearanceVersion >
        static_cast<int>(GarageAppearanceState::SaveVersion))
        return ProfileSaveWriteState::NewerAppearance;
    return ProfileSaveWriteState::CompatibleProfile;
}

constexpr bool CanWriteProfileSave(ProfileSaveWriteState state) noexcept
{
    return state == ProfileSaveWriteState::NewProfile ||
           state == ProfileSaveWriteState::CompatibleProfile;
}
static_assert(CanWriteProfileSave(AssessProfileSaveWrite(false,false,0,0)));
static_assert(CanWriteProfileSave(AssessProfileSaveWrite(true,true,0,0)));
static_assert(!CanWriteProfileSave(AssessProfileSaveWrite(true,false,0,0)));
static_assert(!CanWriteProfileSave(AssessProfileSaveWrite(
    true,true,CareerSaveSchema+1,0)));
static_assert(!CanWriteProfileSave(AssessProfileSaveWrite(
    true,true,CareerSaveSchema,
    static_cast<int>(GarageAppearanceState::SaveVersion)+1)));
} // namespace latebrake
