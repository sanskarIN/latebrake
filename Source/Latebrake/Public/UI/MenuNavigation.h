#pragma once

#include <array>
#include <cstdint>

namespace latebrake
{
enum class MenuPage : std::uint8_t
{
    Closed = 0, Main, QuickRace, Career, Championships, Challenges,
    Garage, Results, Pause, Settings, Profile
};
inline constexpr std::array<const char*, 11> MenuPageNames{{
    "Closed", "Main Menu", "Quick Race", "Career", "Championships",
    "Challenges", "Garage", "Race Results", "Pause Menu",
    "Settings", "Profile Summary"
}};
class MenuNavigation final
{
public:
    bool IsOpen() const noexcept { return Page != MenuPage::Closed; }
    MenuPage Current() const noexcept { return Page; }
    const char* Title() const noexcept
    {
        return MenuPageNames[static_cast<std::size_t>(Page)];
    }
    void Open(MenuPage page = MenuPage::Main) noexcept
    {
        const int id = static_cast<int>(page);
        Page = id <= 0 || id >= static_cast<int>(MenuPageNames.size())
            ? MenuPage::Main : page;
    }
    void Close() noexcept { Page = MenuPage::Closed; }
    void Toggle() noexcept
    {
        if (IsOpen()) Close();
        else Open();
    }
    void Next() noexcept
    {
        if (!IsOpen()) return;
        const int id = static_cast<int>(Page);
        Page = static_cast<MenuPage>(id >= 10 ? 1 : id + 1);
    }
    void Previous() noexcept
    {
        if (!IsOpen()) return;
        const int id = static_cast<int>(Page);
        Page = static_cast<MenuPage>(id <= 1 ? 10 : id - 1);
    }
private:
    MenuPage Page = MenuPage::Closed;
};
} // namespace latebrake
