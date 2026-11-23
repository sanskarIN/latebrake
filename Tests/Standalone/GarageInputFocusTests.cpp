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
    }
    MenuNavigation navigator;
    Verify(!navigator.IsOpen(), "new navigation starts closed");
    Verify(!CanConsumeGarageShortcut(navigator.IsOpen(),
            navigator.Current()), "no consumption in racing default");
    navigator.Open(MenuPage::Garage);
    Verify(CanConsumeGarageShortcut(navigator.IsOpen(),
            navigator.Current()), "open Garage accepts a garage action");
    navigator.Next();
    Verify(!CanConsumeGarageShortcut(navigator.IsOpen(),
            navigator.Current()), "page change immediately rejects action");
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
        navigator.Close();
        Verify(!CanConsumeGarageShortcut(navigator.IsOpen(),
            navigator.Current()), "close always revokes shortcut authority");
    }
    std::cout << "Garage input focus is menu/page gated.\n";
    return EXIT_SUCCESS;
}
