#pragma once

#include "Garage/GarageAppearance.h"
#include "Garage/GarageTuningKey.h"
#include "Racing/QuickRaceModePicker.h"
#include "UI/MenuNavigation.h"

#include <cstdint>

namespace latebrake
{
// Temporary practice tuning is never represented as a GarageState mutation.
// The saved garage remains the sole authority for ownership and rewards.
enum class PracticeTrialAction : std::uint8_t
{
    Toggle, NextCar, ChooseCar, NextEngine, NextGrip, Reset
};

struct PracticeTrialAccess final
{
    bool MenuOpen=false;
    MenuPage Page=MenuPage::Closed;
    bool LocalDriver=false;
    bool Suspended=false;
    QuickPracticePreset Preset=QuickPracticePreset::None;

    constexpr bool AllowsEdit() const noexcept
    {
        return MenuOpen && Page==MenuPage::QuickRace && LocalDriver &&
            !Suspended && IsQuickPractice(Preset);
    }
};

inline bool ValidPracticeTrialKey(GarageTuningKey key) noexcept
{
    return GarageState::IsVehicle(key.Selected) &&
        key.Engine>=0 && key.Engine<=3 && key.Grip>=0 && key.Grip<=3;
}

struct PracticeTrialReadout final
{
    bool Enabled=false;
    GarageTuningKey Key{};
    bool Owned=false;
    VehiclePerformance Performance{};
};

inline VehiclePerformance PracticeTrialPerformance(GarageTuningKey key) noexcept
{
    if(!ValidPracticeTrialKey(key))return {};
    // Match GarageState::Performance through its own implementation.
    // This private, synthetic snapshot never leaves the function.
    GarageState model;
    model.Load({0,KnownVehicleMask,key.Selected,key.Engine,key.Grip});
    return model.Performance();
}

class PracticeTrialSetup final
{
public:
    PracticeTrialReadout Pending(GarageSnapshot saved) const noexcept
    {
        return Describe(PendingEnabled,PendingKey,saved);
    }

    PracticeTrialReadout Active(QuickPracticePreset preset,
        GarageSnapshot saved) const noexcept
    {
        return Describe(IsQuickPractice(preset) && ActiveEnabled,ActiveKey,saved);
    }

    bool Edit(PracticeTrialAccess access,PracticeTrialAction action,
        GarageSnapshot saved,int vehicle=-1) noexcept
    {
        if(!access.AllowsEdit())return false;
        if(action==PracticeTrialAction::Toggle)
        {
            if(PendingEnabled)
            {
                PendingEnabled=false;
                PendingKey={};
            }
            else
            {
                PendingKey=SanitizedSavedKey(saved);
                PendingEnabled=true;
            }
            return true;
        }
        if(!PendingEnabled || !ValidPracticeTrialKey(PendingKey))return false;
        auto next=PendingKey;
        switch(action)
        {
        case PracticeTrialAction::NextCar:
            next.Selected=(next.Selected+1)%VehicleCount;
            break;
        case PracticeTrialAction::ChooseCar:
            if(!GarageState::IsVehicle(vehicle))return false;
            next.Selected=vehicle;
            break;
        case PracticeTrialAction::NextEngine:
            next.Engine=(next.Engine+1)%4;
            break;
        case PracticeTrialAction::NextGrip:
            next.Grip=(next.Grip+1)%4;
            break;
        case PracticeTrialAction::Reset:
            next=SanitizedSavedKey(saved);
            break;
        default:return false;
        }
        if(next==PendingKey)return false;
        PendingKey=next;
        return true;
    }

    // Only an accepted practice launch can copy pending settings to the
    // active race. Restarting keeps this same snapshot.
    bool BeginPractice(PracticeTrialAccess access) noexcept
    {
        if(!access.AllowsEdit())return false;
        ActiveEnabled=PendingEnabled && ValidPracticeTrialKey(PendingKey);
        ActiveKey=ActiveEnabled?PendingKey:GarageTuningKey{};
        return true;
    }

    void EndPractice() noexcept
    {
        ActiveEnabled=false;
        ActiveKey={};
    }

    // Pending preferences remain session-local and can be reused for a
    // later deliberate launch. Neither can authorize profile operations.
private:
    static GarageTuningKey SanitizedSavedKey(GarageSnapshot saved) noexcept
    {
        GarageState copy;
        copy.Load(saved);
        return MakeGarageTuningKey(copy.Snapshot());
    }

    static PracticeTrialReadout Describe(bool enabled,GarageTuningKey key,
        GarageSnapshot saved) noexcept
    {
        GarageState copy;
        copy.Load(saved);
        enabled=enabled && ValidPracticeTrialKey(key);
        if(!enabled)key=MakeGarageTuningKey(copy.Snapshot());
        return {enabled,key,copy.Owns(key.Selected),PracticeTrialPerformance(key)};
    }

    bool PendingEnabled=false;
    GarageTuningKey PendingKey{};
    bool ActiveEnabled=false;
    GarageTuningKey ActiveKey{};
};

inline GarageLook PracticeTrialLook(PracticeTrialReadout trial,
    const GarageAppearanceState& saved) noexcept
{
    // An unowned trial car is always presented in its factory appearance.
    // Saved owned-car styling is read only, not an editable trial asset.
    return trial.Enabled && !trial.Owned?FactoryGarageLook(trial.Key.Selected):
        saved.Get(trial.Key.Selected);
}
} // namespace latebrake
