#include "Racing/GhostDisplayPreference.h"

#include <cstdlib>
#include <iostream>

namespace {
void Check(bool condition, const char* description)
{
    if (!condition)
    {
        std::cerr << "GhostDisplayPreference FAIL: " << description << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}
int main()
{
    latebrake::GhostDisplayPreference preference;
    Check(preference.IsEnabled(), "ghost defaults enabled for local racing");
    Check(!preference.CanRender(false), "no imaginary replay without valid lap");
    Check(preference.CanRender(true), "valid replay may render by default");
    Check(!preference.Toggle(false) && preference.IsEnabled(),
          "invalid or menu-owned input must not change visibility");
    Check(preference.Toggle(true) && !preference.IsEnabled(),
          "accepted human press disables rendering");
    Check(!preference.CanRender(true) && !preference.CanRender(false),
          "hiding must suppress even otherwise eligible ghost");
    Check(!preference.Toggle(false) && !preference.IsEnabled(),
          "a blocked repeat cannot secretly re-enable ghost");
    Check(preference.Toggle(true) && preference.IsEnabled() &&
          preference.CanRender(true), "second valid press restores ghost");
    for (int i = 0; i < 1000; ++i)
    {
        const bool expected = i % 2 == 1;
        Check(preference.Toggle(true) &&
              preference.IsEnabled() == expected,
              "repeated explicit toggles are deterministic");
    }
    Check(preference.IsEnabled(), "visibility returns to enabled");
    std::cout << "Run-local ghost visibility policy passed.\n";
    return EXIT_SUCCESS;
}
