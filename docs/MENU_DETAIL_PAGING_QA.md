# Navigable menu details — v0.5.69 (SOURCE development)

LateBrake's earlier v0.5.68 HUD constrained Results, Career and
Garage menu rows to stay above the physical Android steering pad.
However, surplus rows were deliberately **hidden** without a way to
inspect them. This milestone adds actual, bounded page-by-page access
to all generated menu details.

## New controls and behavior

| Context | Control | Behavior |
| --- | --- | --- |
| Open menu, keyboard | **Up / Down arrows** | Previous / next **details page** |
| Open menu, keyboard | Page Up / Page Down | Previous / next **menu category** (unchanged) |
| Open menu, keyboard | Enter | Select the current item (unchanged) |
| Open menu, keyboard | Tab | Close menu (unchanged) |
| Open menu, Android | **UP / DOWN** upper-right pads | Previous / next **details page** |
| Open menu, Android | NEXT / SELECT / BROWSE / CLOSE | Existing menu navigation and career actions, unchanged |
| Driving (menu closed) | RESTART / RECOVER pads | Existing race actions, unchanged |

The HUD keeps the title and control instructions pinned at the top.
All other generated page lines are collected in order and displayed
in a bounded `MenuDetailWindow`, with `DETAIL PAGE 1/3 | UP/DOWN`
in the existing reserved overflow-notice baseline. Opening another
menu category or closing/reopening a menu resets to the first detail
page. A viewport resize, or fewer generated results, clamps the
previous offset to the new valid range. Invalid or negative list
sizes and zero-capacity viewports fail closed.

A SAVE FAILED banner retains its independent reserved baseline and
is never hidden by paging. Text width remains constrained by actual
Unreal `GetTextSize` measurement and the physical Android touch
action column. No racing clock, gate, car controls, garage transactions,
save data, career rewards or page-category navigation logic is changed.

## Source verification

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release --parallel
ctest --test-dir Tests/Standalone/build -C Release -R '^(MenuNavigation|HudMenuVerticalPlan|MenuDetailPager)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'MenuDetailPaginationBindingTests.py' -v
```

`MenuDetailPagerTests.cpp` exercises multi-page Results, partial final
page, previous/next boundaries, changing content size, live viewport
resize, page-category transitions, close/reopen reset, zero capacity
and bounded one-thousand-row policies.

The Unreal source-binding checks cover the real HUD's full detail
collection, menu rendering, fresh first-press touch routing, keyboard
bindings that execute during a paused world, and preservation of
separate SAVE FAILED and PageUp/PageDown behavior.

**This does not test UE 5.8 UHT/Editor compilation, in-engine draw
order, Android device tap delivery, visual DPI scaling or installed
Windows/Android binaries.** Before calling it game-ready, build the
actual authorized engine targets, cook the authored original circuit,
and test every menu with physical touch/keyboard/gamepad at small
portrait/landscape sizes, interrupt/resume boundaries, and a real
save-failure condition. Capture screenshots/logs/hashes for that exact
build. This code is not a stable or playable public game release.
