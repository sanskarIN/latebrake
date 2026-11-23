#include "UI/GarageInputFocusPolicy.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Verify(bool check,const char* name)
{
    if (!check)
    {
        std::cerr << "Garage input focus: " << name << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    // A key can be pressed in any visible page; without a context gate,
    // a duplicated gamepad binding might reset throttle or steering in
    // the middle of an actual race.
    constexpr MenuPage AllPages[] = {
        MenuPage::Closed, MenuPage::Main, MenuPage::QuickRace,
        MenuPage::Career, MenuPage::Championships, MenuPage::Challenges,
        MenuPage::Garage, MenuPage::Results, MenuPage::Pause,
        MenuPage::Settings, MenuPage::Profile
    };
    for (MenuPage page : AllPages)
    {
        Verify(!CanConsumeGarageShortcut(false,page),
            "closed/hidden menu may never consume driving shortcuts");
        const bool accepted = CanConsumeGarageShortcut(true,page);
        Verify(accepted == (page == MenuPage::Garage),
            "only open visible Garage can consume gamepad/keyboard routes");
        Verify(!CanConsumeGarageShortcut(true,page,true),
            "focus loss/manual pause forbids even a visible Garage shortcut");
        Verify(CanUseGarageVehicleHotkey(false,page),
            "legacy E car-select remains available with no menu");
        Verify(CanUseGarageVehicleHotkey(true,page) ==
                (page == MenuPage::Garage),
            "only visible Garage may select a car with the menu open");
        Verify(!CanUseGarageVehicleHotkey(false,page,true),
            "standalone race pause/focus loss forbids hidden E selection");
        Verify(CanUseGarageVehicleHotkey(true,page,true) ==
                (page == MenuPage::Garage),
            "visible Garage remains eligible even when engine world paused");
        Verify(!CanUseGarageVehicleHotkey(true,page,true,true) &&
               !CanUseGarageVehicleHotkey(true,page,false,true) &&
               !CanUseGarageVehicleHotkey(false,page,false,true),
            "an explicit suspension supersedes every menu and engine pause state");
        // Paid offers NEVER exist behind a closed menu. E intentionally
        // retains its unpaused quick-race owned-car cycling behavior.
        Verify(!CanUseGaragePurchaseHotkey(false,page) &&
               !CanUseGaragePurchaseHotkey(false,page,true),
            "Q/U/I cannot stage or commit purchases during live driving");
        Verify(CanUseGaragePurchaseHotkey(true,page) ==
               (page == MenuPage::Garage),
            "Q/U/I require visible Garage, including during menu pause");
        Verify(!CanUseGaragePurchaseHotkey(true,page,true),
            "manual suspension or focus loss revokes even open Garage spending");
    }
    MenuNavigation navigator;
    Verify(!navigator.IsOpen(), "new navigation starts closed");
    Verify(!CanConsumeGarageShortcut(navigator.IsOpen(),
            navigator.Current()), "no consumption in racing default");
    navigator.Open(MenuPage::Garage);
    Verify(CanConsumeGarageShortcut(navigator.IsOpen(),
            navigator.Current()), "open Garage accepts a garage action");
    Verify(CanUseGaragePurchaseHotkey(navigator.IsOpen(),
            navigator.Current()), "visible Garage grants paid shortcut focus");
    navigator.Next();
    Verify(!CanConsumeGarageShortcut(navigator.IsOpen(),
            navigator.Current()), "page change immediately rejects action");
    Verify(!CanUseGaragePurchaseHotkey(navigator.IsOpen(),
            navigator.Current()), "leaving Garage cancels purchase hotkey focus");
    navigator.Open(MenuPage::Garage);
    navigator.Close();
    Verify(!CanConsumeGarageShortcut(navigator.IsOpen(),
            navigator.Current()), "return to race immediately restores control");
    for (int i=0;i<300000;++i)
    {
        navigator.Open((i%4==0)?MenuPage::Garage:
            (i%4==1)?MenuPage::QuickRace:
            (i%4==2)?MenuPage::Results:MenuPage::Pause);
        Verify(CanConsumeGarageShortcut(navigator.IsOpen(),
            navigator.Current())==(i%4==0),
            "300k menu changes never leak garage actions to driving");
        Verify(CanUseGaragePurchaseHotkey(navigator.IsOpen(),
            navigator.Current())==(i%4==0),
            "300k transitions require open Garage for purchases");
        Verify(!CanConsumeGarageShortcut(navigator.IsOpen(),
            navigator.Current(),true) &&
            !CanUseGarageVehicleHotkey(navigator.IsOpen(),
            navigator.Current(),true,true),
            "300k suspension transitions forbid garage input and spending");
        navigator.Close();
        Verify(!CanConsumeGarageShortcut(navigator.IsOpen(),
            navigator.Current()), "close always revokes shortcut authority");
        Verify(!CanUseGaragePurchaseHotkey(navigator.IsOpen(),
            navigator.Current()), "menu close always revokes purchase focus");
        Verify(CanUseGarageVehicleHotkey(navigator.IsOpen(),
            navigator.Current()),
            "closed menu restores legacy quick-race selection");
        Verify(!CanUseGarageVehicleHotkey(navigator.IsOpen(),
            navigator.Current(),true),
            "standalone race pause revokes E while menu is closed");
    }
    std::cout << "Garage input focus is menu/page gated.\n";
    return EXIT_SUCCESS;
}
