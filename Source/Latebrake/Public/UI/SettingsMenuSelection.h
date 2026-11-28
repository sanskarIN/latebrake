#pragma once

#include "UI/HudTelemetryPriority.h"
#include "UI/MenuNavigation.h"
#include "Vehicles/CameraMotionPreference.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// Settings has its own cursor. Browse never mutates the chosen value;
// Select changes one explicitly named setting. No garage or career
// selection, engine input, or progression authority is held here.
enum class SettingsControl : std::uint8_t
{
    SpeedUnits,
    HudDetail,
    CircuitLighting,
    CameraMotion,
    Count
};

inline constexpr int SettingsControlCount=static_cast<int>(SettingsControl::Count);
inline constexpr std::array<SettingsControl,SettingsControlCount> SettingsControls{{
    SettingsControl::SpeedUnits,SettingsControl::HudDetail,SettingsControl::CircuitLighting,
    SettingsControl::CameraMotion
}};

constexpr bool ValidSettingsControl(SettingsControl control) noexcept
{
    return static_cast<unsigned>(control)<static_cast<unsigned>(SettingsControl::Count);
}

constexpr SettingsControl NextSettingsControl(SettingsControl control) noexcept
{
    return ValidSettingsControl(control)
        ?SettingsControls[(static_cast<unsigned>(control)+1u)%SettingsControls.size()]
        :SettingsControl::SpeedUnits;
}

struct SettingsMenuAccess final
{
    bool MenuOpen=false;
    MenuPage Page=MenuPage::Closed;
    bool LocalPlayer=false;
    bool FocusSuspended=false;
    bool ManualPauseSuspended=false;
    bool ProfileReadOnly=false;
    bool ActualLightsReady=false;
    bool ActualCameraReady=false;

    constexpr bool OwnsInput() const noexcept
    {
        return MenuOpen && Page==MenuPage::Settings && LocalPlayer && !FocusSuspended;
    }
    constexpr bool Allows(SettingsControl control) const noexcept
    {
        if(!OwnsInput())return false;
        switch(control)
        {
        case SettingsControl::SpeedUnits:return !ProfileReadOnly;
        case SettingsControl::HudDetail:return true;
        case SettingsControl::CircuitLighting:return !ManualPauseSuspended && ActualLightsReady;
        case SettingsControl::CameraMotion:return ActualCameraReady;
        default:return false;
        }
    }
    constexpr bool operator==(const SettingsMenuAccess&) const noexcept=default;
};

struct SettingsMenuSnapshot final
{
    SettingsMenuAccess Access{};
    SettingsControl Selected=SettingsControl::SpeedUnits;
    bool MilesPerHour=false;
    HudTelemetryMode HudMode=HudTelemetryMode::Balanced;
    bool Dusk=false;
    std::uint64_t Revision=0;
    CameraMotionMode CameraMotion=CameraMotionMode::Dynamic;
    constexpr bool operator==(const SettingsMenuSnapshot&) const noexcept=default;
};

struct SettingsControlDescription final
{
    const char* Title="";
    const char* ShortTitle="";
    const char* Value="";
    const char* Status="";
    const char* ShortStatus="";
    bool Enabled=false;
    bool Selected=false;
    bool Saved=false;
};

constexpr SettingsControlDescription DescribeSettingsControl(
    const SettingsMenuSnapshot& snapshot,SettingsControl control) noexcept
{
    if(!ValidSettingsControl(control) || !ValidHudTelemetryMode(snapshot.HudMode) ||
        !ValidCameraMotionMode(snapshot.CameraMotion))
        return {};
    const bool enabled=snapshot.Access.Allows(control);
    const bool selected=snapshot.Selected==control;
    const bool owns=snapshot.Access.OwnsInput();
    const char* value="";
    const char* title="";
    const char* shortTitle="";
    const char* status="";
    const char* shortStatus="";
    bool saved=false;
    switch(control)
    {
    case SettingsControl::SpeedUnits:
        title="SPEED DISPLAY";
        shortTitle="UNITS";
        value=snapshot.MilesPerHour?"MPH":"KM/H";
        saved=true;
        status=enabled?"SAVED TO PROFILE":!owns?"INPUT UNAVAILABLE":"PROFILE READ ONLY";
        shortStatus=enabled?"SAVED":!owns?"LOCKED":"READ ONLY";
        break;
    case SettingsControl::HudDetail:
        title="HUD DETAIL";
        shortTitle="HUD";
        value=HudTelemetryModeName(snapshot.HudMode);
        status=enabled?"CURRENT SESSION ONLY":"INPUT UNAVAILABLE";
        shortStatus=enabled?"SESSION":"LOCKED";
        break;
    case SettingsControl::CircuitLighting:
        title="CIRCUIT LIGHTING";
        shortTitle="LIGHTING";
        value=snapshot.Dusk?"DUSK":"DAY";
        status=enabled?"SESSION VISUAL PREVIEW":!owns?"INPUT UNAVAILABLE":"PREVIEW UNAVAILABLE";
        shortStatus=enabled?"VISUAL":!owns?"LOCKED":"UNAVAILABLE";
        break;
    case SettingsControl::CameraMotion:
        title="CAMERA EFFECTS";
        shortTitle="CAMERA";
        value=CameraMotionModeName(snapshot.CameraMotion);
        status=enabled?"CURRENT SESSION ONLY":!owns?"INPUT UNAVAILABLE":"CAMERA UNAVAILABLE";
        shortStatus=enabled?"SESSION":!owns?"LOCKED":"UNAVAILABLE";
        break;
    default:return {};
    }
    return {title,shortTitle,value,status,shortStatus,enabled,selected,saved};
}

class SettingsMenuSelection final
{
public:
    SettingsControl Current() const noexcept {return Selected;}
    std::uint64_t Revision() const noexcept {return Version;}

    // A browse press belongs to Settings even if all settings except
    // telemetry are disabled. Do not fall through into career browsing.
    bool Browse(SettingsMenuAccess access) noexcept
    {
        if(!access.OwnsInput())return false;
        Selected=NextSettingsControl(Selected);
        Changed();
        return true;
    }

    bool Select(SettingsMenuAccess access,SettingsControl control) noexcept
    {
        if(!access.OwnsInput() || !ValidSettingsControl(control))return false;
        Selected=control;
        Changed();
        return true;
    }

    void Changed() noexcept
    {
        ++Version;
        if(Version==0)++Version;
    }

    void Reset() noexcept
    {
        Selected=SettingsControl::SpeedUnits;
        Changed();
    }

private:
    SettingsControl Selected=SettingsControl::SpeedUnits;
    std::uint64_t Version=1;
};
} // namespace latebrake
