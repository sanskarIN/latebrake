#include "UI/MenuNavigation.h"
#include <cstdlib>
#include <iostream>
#include <string>

static void Check(bool ok, const char* what)
{
    if (!ok) { std::cerr << "Menu FAIL: " << what << '\n'; std::exit(1); }
}
int main()
{
    using namespace latebrake;
    MenuNavigation menu;
    Check(!menu.IsOpen() && menu.Current() == MenuPage::Closed, "starts closed");
    menu.Next();
    Check(!menu.IsOpen(), "disabled navigation cannot open itself");
    menu.Toggle();
    Check(menu.Current() == MenuPage::Main, "toggle opens main");
    menu.Next();
    Check(menu.Current() == MenuPage::QuickRace, "next page");
    menu.Previous();
    Check(menu.Current() == MenuPage::Main, "previous page");
    menu.Previous();
    Check(menu.Current() == MenuPage::Profile, "previous wraps");
    Check(std::string(menu.Title()) == "Profile Summary", "title matches menu data");
    menu.Next();
    Check(menu.Current() == MenuPage::Main, "next wraps");
    menu.Open(MenuPage::Settings);
    Check(menu.Current() == MenuPage::Settings, "settings directly accessible");
    menu.Close();
    Check(!menu.IsOpen(), "close neutral");
    menu.Open(static_cast<MenuPage>(254));
    Check(menu.Current() == MenuPage::Main, "invalid page fails closed to main");
    menu.Toggle();
    Check(!menu.IsOpen(), "toggle closes");
    std::cout << "Menu navigation passed.\n";
    return 0;
}
