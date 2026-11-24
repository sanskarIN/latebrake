#include "UI/HudDetailTextFlow.h"
#include "UI/HudInstrumentDriveBadge.h"
#include "UI/NitroHudReadout.h"
#include "UI/MenuDetailPager.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
void Require(bool ok, const char* why)
{
    if (!ok)
    {
        std::cerr << "Combined desktop HUD FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    // A menu owns the viewport in an actual garage/results UI; the drive
    // instruments must be absent rather than competing with menu details.
    for (int width : {900, 1280, 1920})
    {
        const HudLayout layout = CalculateHudLayout(width, 720);
        const auto menuGauge = CalculateHudInstrumentCluster(
            width, 720, layout, false, true, false,
            120.0, 0.5, 0.3, 0.0);
        Require(!menuGauge.Visible, "open garage hides the race instruments");

        // All detail rows are bounded to real measured presentation space
        // and then paged independently of the garage input controls.
        const std::u16string source =
            u"GARAGE CAR ONE SPECIFICATIONS AND SUSPENSION. "
            u"CAR TWO GEAR RATIO SETUP AND TYRE HEALTH. "
            u"CHAMPIONSHIP LONG SESSION REVIEW.";
        std::vector<std::u16string> rows;
        const auto unit = [&](int i) -> std::uint32_t
        {
            return static_cast<std::uint32_t>(
                source[static_cast<std::size_t>(i)]);
        };
        const auto result = WrapHudDetailText(
            static_cast<int>(source.size()), 22.0, unit,
            [](int, int count) -> double
            {
                return static_cast<double>(count);
            },
            [&](int start, int count)
            {
                rows.emplace_back(source.substr(
                    static_cast<std::size_t>(start),
                    static_cast<std::size_t>(count)));
            }, 32);
        Require(result.Complete && rows.size() >= 4,
            "multiple garage details survive measured wrapping");
        MenuDetailPager pager;
        pager.Synchronize(MenuPage::Garage,
            static_cast<int>(rows.size()), 3);
        const auto first = pager.Window();
        Require(first.Total == static_cast<int>(rows.size()) &&
                first.HasNext && !first.HasPrevious,
            "wrapped rows are actually eligible for paging");
        Require(pager.Next() && pager.Window().HasPrevious,
            "later garage detail rows can be reached");

        const auto gauges = CalculateHudInstrumentCluster(
            width, 720, layout, false, false, false,
            120.0, 0.5, 0.3, 0.0);
        Require(gauges.Visible, "instruments reappear once menu closes");
        const double s = gauges.Scale;
        const auto gear = CalculateHudInstrumentDriveBadge(
            gauges.Panel, s, 92.0 * s, 22.0 * s, 15.0 * s,
            114.0 * s, 15.0 * s);
        Require(gear.Visible &&
                gear.X >= gauges.Panel.X &&
                gear.Right < gauges.Panel.X + gauges.Panel.Width,
            "measured gear badge shares panel without leaving its lane");
    }

    // The same real arcade NitroBoost component exposes status snapshots;
    // the HUD must never drive its authoritative heat/recharge clock.
    ArcadeNitroBoost nitro;
    auto slow = ReadNitroHud(nitro, 0.0, true);
    Require(slow.Valid && slow.Phase == NitroHudPhase::BuildSpeed,
        "initial stored nitro does not advertise a false ready status");
    nitro.Press(true);
    const auto drive = nitro.Step(0.1, true, false, 1.0, 800.0);
    Require(drive.Active, "real gameplay ability activates independently");
    const auto observed = ReadNitroHud(nitro, 28.8, true);
    Require(observed.Valid && observed.Phase == NitroHudPhase::Active,
        "instrument status follows the active authoritative ability");
    const double before = nitro.ChargeFraction();
    const double beforeCooldown = nitro.CooldownRemainingSeconds();
    for (int i = 0; i < 100; ++i)
    {
        Require(ReadNitroHud(nitro, 28.8, true).Phase ==
                    NitroHudPhase::Active,
            "repeated HUD reads remain observational");
    }
    Require(nitro.ChargeFraction() == before &&
            nitro.CooldownRemainingSeconds() == beforeCooldown,
        "combined HUD cannot advance, recharge, or activate nitro");
    Require(ReadNitroHud(nitro, 28.8, false).Phase ==
                NitroHudPhase::Unavailable,
        "guarded race masks nitro status independently of its charge");

    std::cout << "Combined HUD instruments, nitro and menu pager passed.\n";
    return EXIT_SUCCESS;
}
