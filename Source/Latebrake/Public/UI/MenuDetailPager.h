#pragma once

#include "UI/MenuNavigation.h"

#include <algorithm>

namespace latebrake
{
// Presentation-only, finite menu-detail pagination. The page's TITLE
// always remains in a fixed header; detail rows are individually
// inspectable without crossing Android's physical steering pad.
// Gameplay navigation, rewards, clock and touch ownership never depend
// on these offsets.
struct MenuDetailWindow final
{
    int Begin = 0;
    int End = 0;
    int Total = 0;
    int PageNumber = 0;
    int PageCount = 0;
    bool HasPrevious = false;
    bool HasNext = false;
};

class MenuDetailPager final
{
public:
    void Reset() noexcept
    {
        CurrentPage = MenuPage::Closed;
        SelectedPage = 0;
        TotalRows = 0;
        RowsPerPage = 0;
    }

    void Synchronize(MenuPage page, int total, int capacity) noexcept
    {
        // A new menu page or closed menu can never inherit a hidden
        // detail offset from the previous Results/Career page.
        if (page == MenuPage::Closed)
        {
            Reset();
            return;
        }
        const bool changedPage = CurrentPage != page;
        const int previousFirstRow = SelectedPage * RowsPerPage;
        if (changedPage)
        {
            SelectedPage = 0;
            CurrentPage = page;
        }
        TotalRows = std::clamp(total, 0, 1024);
        RowsPerPage = std::clamp(capacity, 0, 100);
        // Keep the first previously visible detail in view after resize
        // rather than retaining an arbitrary old page ordinal (which
        // could jump over information when available rows increase).
        if (!changedPage && RowsPerPage > 0)
        {
            SelectedPage = previousFirstRow / RowsPerPage;
        }
        const int count = PageCount();
        SelectedPage = count > 0
            ? std::min(SelectedPage, count - 1) : 0;
    }

    bool Next() noexcept
    {
        if (RowsPerPage <= 0 || SelectedPage + 1 >= PageCount())
        {
            return false;
        }
        ++SelectedPage;
        return true;
    }

    bool Previous() noexcept
    {
        if (RowsPerPage <= 0 || SelectedPage <= 0)
        {
            return false;
        }
        --SelectedPage;
        return true;
    }

    MenuDetailWindow Window() const noexcept
    {
        if (RowsPerPage <= 0 || TotalRows <= 0 ||
            CurrentPage == MenuPage::Closed)
        {
            return {};
        }
        const int begin = SelectedPage * RowsPerPage;
        return {
            begin,
            std::min(TotalRows, begin + RowsPerPage),
            TotalRows,
            SelectedPage + 1,
            PageCount(),
            SelectedPage > 0,
            begin + RowsPerPage < TotalRows
        };
    }

    MenuPage GetPage() const noexcept { return CurrentPage; }

private:
    int PageCount() const noexcept
    {
        return RowsPerPage > 0 && TotalRows > 0
            ? 1 + (TotalRows - 1) / RowsPerPage : 0;
    }
    MenuPage CurrentPage = MenuPage::Closed;
    int SelectedPage = 0;
    int TotalRows = 0;
    int RowsPerPage = 0;
};
} // namespace latebrake
