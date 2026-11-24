#include "UI/HudDetailTextFlow.h"
#include "UI/HudInstrumentDriveBadge.h"
#include "UI/HudMenuMouseControls.h"
#include "UI/NitroHudReadout.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace
{
void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "HUD viewport integration FAIL: "
                  << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct MeasuredRows final
{
    latebrake::HudDetailWrapResult Result;
    std::vector<std::u16string> Rows;
};

MeasuredRows Wrap(std::u16string_view source, double width, int cap = 32)
{
    MeasuredRows rows;
    const auto unit = [&](int index) -> std::uint32_t
    {
        return static_cast<std::uint32_t>(
            source[static_cast<std::size_t>(index)]);
    };
    const auto measure = [&](int begin, int count) -> double
    {
        double pixels = 0.0;
        for (int index = begin; index < begin + count; ++index)
        {
            const auto cp = unit(index);
            pixels += cp == u'W' ? 3.0 : cp == u'i' ? 0.5 : 1.0;
        }
        return pixels;
    };
    rows.Result = latebrake::WrapHudDetailText(
        static_cast<int>(source.size()), width, unit, measure,
        [&](int begin, int count)
        {
            Require(count > 0 && begin >= 0 &&
                    begin + count <= static_cast<int>(source.size()),
                    "wrapped row refers to valid source");
            Require(measure(begin, count) <= width,
                    "final shaped line fits measured HUD lane");
            Require(latebrake::IsHudDetailSafeBoundary(
                    0, begin, static_cast<int>(source.size()), unit) &&
                    latebrake::IsHudDetailSafeBoundary(
                    0, begin + count, static_cast<int>(source.size()), unit),
                    "Unicode graphemes are not split across rows");
            rows.Rows.emplace_back(source.substr(
                static_cast<std::size_t>(begin),
                static_cast<std::size_t>(count)));
        }, cap);
    return rows;
}
} // namespace

int main()
{
    using namespace latebrake;
    const auto wrapped = Wrap(
        u"GARAGE CAR ONE WIDE WHEELS \u0915\u094D\u0937 \n"
        u"CAR TWO LONG SUSPENSION COMPARISON \n"
        u"RACE RESULTS WITH \U0001F680 AND PIT DATA", 15.0);
    Require(wrapped.Result.Complete &&
            wrapped.Rows.size() > 6 &&
            wrapped.Result.Lines == static_cast<int>(wrapped.Rows.size()),
            "multi-language details are wrapped before real paging");
    const auto exact = Wrap(u"ONE\nTWO\r\n  ", 8.0, 2);
    Require(exact.Result.Complete && exact.Result.Lines == 2,
            "row budget with trailing formatting is not false overflow");
    const auto extra = Wrap(u"ONE\nTWO\nTHREE", 8.0, 2);
    Require(!extra.Result.Complete && extra.Result.Lines == 2,
            "genuinely truncated menu detail remains incomplete");

    const double max = std::numeric_limits<double>::max();
    Require(!DesktopMenuMouseRect{max, 20.0, max, 40.0}.Contains(max, 30.0) &&
            !DesktopMenuMouseRect{20.0, max, 40.0, max}.Contains(30.0, max),
            "overflowing far-edge rectangle never accepts a click");
    Require(!DesktopMenuMouseRect{max, 20.0, 1.0, 40.0}.Contains(max, 30.0) &&
            !DesktopMenuMouseRect{20.0, max, 40.0, 1.0}.Contains(30.0, max),
            "precision-collapsed rectangle is inert");
    Require(DesktopMenuMouseRect{10.0, 20.0, 30.0, 40.0}.Contains(10.0, 20.0) &&
            !DesktopMenuMouseRect{10.0, 20.0, 30.0, 40.0}.Contains(40.0, 30.0),
            "normal rectangle retains half-open click edges");
    const auto tall = CalculateHudMenuVerticalPlan(
        1280.0, 1e12, CalculateHudLayout(1280.0, 720.0), false);
    Require(tall.Valid && tall.MaxDetailsRows == 90,
            "large representable geometry clamps before integer conversion");
    Require(ChooseHudTextPrefix(20, 4.0, 20.0, 3.0,
        [](int n) { return n == 0 ? 10.0 : 3.0; }) == -1,
        "measured ellipsis mismatch cannot leak outside text width");

    const struct { int Width; int Height; } dimensions[] = {
        {900, 600}, {1024, 768}, {1280, 720},
        {1920, 1080}, {2560, 1440}, {3440, 1440}
    };
    int activeBadges = 0;
    for (const auto& dimension : dimensions)
    {
        const double width = static_cast<double>(dimension.Width);
        const double height = static_cast<double>(dimension.Height);
        const auto layout = CalculateHudLayout(width, height);
        const auto menu = CalculateDesktopMenuMouseLayout(
            width, height, layout, false, true);
        const auto wheel = CalculateDesktopMenuWheelZone(
            width, height, layout, false, true);
        const auto vertical = CalculateHudMenuVerticalPlan(
            width, height, layout, false);
        Require(menu.Valid && wheel.Width > 0.0 && wheel.Height > 0.0 &&
                vertical.Valid && vertical.MaxDetailsRows > 0,
                "all supported viewports have bounded navigation geometry");
        Require(wheel.X + wheel.Width <= menu.Left &&
                wheel.Y + wheel.Height < height,
                "wheel lane never intersects buttons or footer");

        MenuDetailPager pager;
        pager.Synchronize(MenuPage::Garage,
            static_cast<int>(wrapped.Rows.size()), 2);
        auto available = AvailableDesktopMenuDetails(MenuPage::Garage, pager);
        Require(!available.DetailsUp && available.DetailsDown,
                "first wrapped menu page enables only forward navigation");
        const auto up = menu.Buttons[2].Bounds;
        const auto down = menu.Buttons[3].Bounds;
        Require(HitTestDesktopMenuMouse(menu,
                    up.X + up.Width / 2, up.Y + up.Height / 2,
                    available) == DesktopMenuMouseAction::None &&
                HitTestDesktopMenuMouse(menu,
                    down.X + down.Width / 2, down.Y + down.Height / 2,
                    available) == DesktopMenuMouseAction::DetailsDown,
                "pointer dispatch matches first-page arrow availability");
        Require(pager.Next(), "wrapped detail rows have a second page");
        available = AvailableDesktopMenuDetails(MenuPage::Garage, pager);
        Require(available.DetailsUp &&
                AdmitDesktopMenuWheel(wheel,
                    wheel.X + wheel.Width / 2,
                    wheel.Y + wheel.Height / 2, -1, available),
                "middle-page wheel and button navigation share authority");
        for (int i = 0; i < 128; ++i) pager.Next();
        available = AvailableDesktopMenuDetails(MenuPage::Garage, pager);
        Require(available.DetailsUp && !available.DetailsDown,
                "last page rejects extra down navigation");
        pager.Synchronize(MenuPage::Results, 1, 2);
        available = AvailableDesktopMenuDetails(MenuPage::Garage, pager);
        Require(!available.DetailsUp && !available.DetailsDown,
                "page change instantly invalidates stale menu actions");

        const auto hidden = CalculateHudInstrumentCluster(
            width, height, layout, false, true, false,
            100.0, 0.5, 0.2, 0.1);
        Require(!hidden.Visible,
                "open menu always suppresses live driving instruments");
        const auto cluster = CalculateHudInstrumentCluster(
            width, height, layout, false, false, false,
            100.0, 0.5, 0.2, 0.1);
        if (cluster.Visible)
        {
            const double s = cluster.Scale;
            const auto badge = CalculateHudInstrumentDriveBadge(
                cluster.Panel, s, 92.0 * s, 22.0 * s, 15.0 * s,
                112.0 * s, 15.0 * s);
            Require(badge.Visible &&
                    badge.X - 5.0 * s >= cluster.Panel.X &&
                    badge.Right + 5.0 * s <
                        cluster.Panel.X + cluster.Panel.Width,
                    "instrument badge and background stay inside HUD panel");
            ++activeBadges;
        }
    }
    Require(activeBadges == 6,
            "every full-size desktop renders a safe drive badge");
    Require(!CalculateHudMenuVerticalPlan(
            1280.0, 1e20, CalculateHudLayout(1280.0, 720.0),
            false).Valid,
            "precision-collapsed notice baselines fail closed");
    Require(!CalculateDesktopMenuMouseLayout(
            1280.0, 720.0, CalculateHudLayout(1280.0, 720.0),
            true, true).Valid,
            "desktop dock never steals Android touch input");

    ArcadeNitroBoost nitro;
    Require(ReadNitroHud(nitro, 0.0, true).Phase ==
                NitroHudPhase::BuildSpeed &&
            ReadNitroHud(nitro, 0.0, false).Phase ==
                NitroHudPhase::Unavailable,
            "actual nitro readout does not imply unauthorized boost");
    std::cout << "Integrated HUD viewport safety passed.\n";
    return EXIT_SUCCESS;
}
