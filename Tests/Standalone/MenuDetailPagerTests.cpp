#include "UI/MenuDetailPager.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Check(bool yes, const char* message)
{
    if (!yes)
    {
        std::cerr << "MenuDetailPager FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    MenuDetailPager pager;
    Check(!pager.Next() && !pager.Previous() &&
          pager.Window().PageCount == 0, "closed menu cannot paginate");
    pager.Synchronize(MenuPage::Results, 11, 3);
    auto w = pager.Window();
    Check(w.Begin == 0 && w.End == 3 && w.PageCount == 4 &&
          w.PageNumber == 1 && !w.HasPrevious && w.HasNext,
          "initial Results details use first 3 visible rows");
    Check(pager.Next(), "second detail page exists");
    w = pager.Window();
    Check(w.Begin == 3 && w.End == 6 && w.PageNumber == 2 &&
          w.HasPrevious && w.HasNext, "second page shows disjoint details");
    Check(pager.Next() && pager.Next() && !pager.Next(),
          "last page reached once, cannot scroll beyond");
    w = pager.Window();
    Check(w.Begin == 9 && w.End == 11 && w.PageCount == 4 &&
          w.PageNumber == 4 && w.HasPrevious && !w.HasNext,
          "final partial page exposes all remaining Results rows");
    Check(pager.Previous() && pager.Window().Begin == 6,
          "previous action returns to adjacent detail page");

    pager.Synchronize(MenuPage::Results, 11, 5);
    Check(pager.Window().Begin == 5 && pager.Window().End == 10,
          "live resize clamps current page without losing page identity");
    pager.Synchronize(MenuPage::Results, 11, 2);
    Check(pager.Window().Begin <= 5 && pager.Window().End > 5,
          "narrowing viewport retains the formerly first visible row");
    pager.Synchronize(MenuPage::Results, 2, 5);
    Check(pager.Window().Begin == 0 && pager.Window().PageCount == 1 &&
          !pager.Next(), "shrinking content clamps to valid first page");

    pager.Synchronize(MenuPage::Garage, 6, 2);
    Check(pager.Window().Begin == 0 && pager.Window().PageNumber == 1,
          "changing page never inherits a previous scroll offset");
    pager.Next();
    pager.Synchronize(MenuPage::Closed, 6, 2);
    Check(pager.Window().PageCount == 0 && !pager.Next(),
          "closing a menu fails closed and clears scroll");
    pager.Synchronize(MenuPage::Garage, 6, 2);
    Check(pager.Window().Begin == 0,
          "reopen always starts at first detail page");
    pager.Synchronize(MenuPage::Results, 4, 0);
    Check(pager.Window().PageCount == 0 && !pager.Next(),
          "zero visible rows cannot claim a page");
    pager.Synchronize(MenuPage::Results, 4, -1);
    Check(pager.Window().PageCount == 0 && !pager.Next(),
          "negative visible rows fail closed");
    pager.Synchronize(MenuPage::Results, -3, 4);
    Check(pager.Window().PageCount == 0 && !pager.Next(),
          "negative content count fails closed");
    pager.Synchronize(MenuPage::Results, 1000000, 1);
    Check(pager.Window().Total == 1024 &&
          pager.Window().PageCount == 1024, "large lists remain bounded");
    for (int i = 0; i < 1024; ++i) pager.Next();
    Check(pager.Window().Begin == 1023 && !pager.Next(),
          "no overflow when scrolling thousands of details");
    pager.Reset();
    Check(pager.GetPage() == MenuPage::Closed && !pager.Previous(),
          "explicit reset is idempotent");
    std::cout << "Menu detail pagination tests passed.\n";
    return EXIT_SUCCESS;
}
