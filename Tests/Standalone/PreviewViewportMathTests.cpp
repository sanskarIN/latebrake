#include "PreviewViewportMath.h"

#include <array>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <random>

namespace
{
void Check(bool condition, const char* why)
{
    if (!condition)
    {
        std::cerr << "PreviewViewportMath failure: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void TestMapping(int clientW, int clientH)
{
    using namespace latebrake_preview;
    const auto v = FitPreviewViewport(clientW, clientH);
    Check(v.x >= 0 && v.y >= 0 &&
          v.w >= 1 && v.h >= 1 &&
          static_cast<long long>(v.x) + v.w <= clientW &&
          static_cast<long long>(v.y) + v.h <= clientH,
          "positive client dimensions fit entirely inside the viewport");
    int x = -51;
    int y = -97;
    Check(TranslatePreviewMouse(v, v.x, v.y, x, y) &&
          x == 0 && y == 0,
          "top-left canvas edge maps exactly to zero");
    const int right = static_cast<int>(
        static_cast<long long>(v.x) + v.w - 1);
    const int bottom = static_cast<int>(
        static_cast<long long>(v.y) + v.h - 1);
    Check(TranslatePreviewMouse(v, right, bottom, x, y) &&
          x >= 0 && x < PreviewCanvasWidth &&
          y >= 0 && y < PreviewCanvasHeight,
          "last visible client pixel is always in the canvas");
    x = -51;
    y = -97;
    const long long beyondRight =
        static_cast<long long>(v.x) + v.w;
    const long long beyondBottom =
        static_cast<long long>(v.y) + v.h;
    if (beyondRight <= INT_MAX)
    {
        Check(!TranslatePreviewMouse(
                  v, static_cast<int>(beyondRight), v.y, x, y) &&
              x == -51 && y == -97,
              "right black boundary never registers a menu input");
    }
    if (beyondBottom <= INT_MAX)
    {
        Check(!TranslatePreviewMouse(
                  v, v.x, static_cast<int>(beyondBottom), x, y) &&
              x == -51 && y == -97,
              "bottom black boundary never registers a menu input");
    }
    if (v.x > 0)
    {
        Check(!TranslatePreviewMouse(v, v.x - 1, v.y, x, y) &&
              x == -51 && y == -97,
              "left pillarbox never activates racing UI");
    }
    if (v.y > 0)
    {
        Check(!TranslatePreviewMouse(v, v.x, v.y - 1, x, y) &&
              x == -51 && y == -97,
              "top letterbox never activates racing UI");
    }

    // Monotonicity and independently calculated floor coordinates: a
    // large monitor/DPI change can never wrap input into another button.
    int priorX = -1;
    for (int n = 0; n <= 128; ++n)
    {
        const long long px = static_cast<long long>(v.x) +
            (static_cast<long long>(v.w - 1) * n) / 128;
        Check(TranslatePreviewMouse(v, static_cast<int>(px), v.y,
                                    x, y) && y == 0,
              "valid horizontal canvas mapping");
        Check(x >= priorX && x < PreviewCanvasWidth,
              "mouse coordinate remains monotonic within client");
        const int expected = static_cast<int>(
            (px - v.x) * PreviewCanvasWidth / v.w);
        Check(x == expected, "floor mapping respects exact native pixels");
        priorX = x;
    }
}

void TestBadDimensions()
{
    using namespace latebrake_preview;
    for (const std::array<int, 2> size : {
        std::array<int, 2>{0, 100},
        {100, 0}, {-1, 100}, {100, -1},
        {INT_MIN, INT_MAX}, {INT_MAX, INT_MIN}})
    {
        const auto vp = FitPreviewViewport(size[0], size[1]);
        Check(vp.w == 0 && vp.h == 0,
              "invalid client size cannot create a hit-test surface");
        int x = 23;
        int y = 42;
        Check(!TranslatePreviewMouse(vp, 0, 0, x, y) &&
              x == 23 && y == 42,
              "invalid sizes never overwrite last valid input");
    }

    const std::array<CanvasViewport, 5> invalid = {{
        {0, 0, 0, 640}, {0, 0, 960, -1}, {-1, 0, 1, 1},
        {0, -1, 1, 1}, {0, 0, -1, 1}
    }};
    for (const auto& v : invalid)
    {
        int x = 3;
        int y = 4;
        Check(!TranslatePreviewMouse(v, 0, 0, x, y) &&
              x == 3 && y == 4,
              "corrupt viewport fails closed without coordinate writes");
    }

    // Previously x + width overflowed a 32-bit signed int, so a
    // valid pixel adjacent to INT_MAX could be misclassified as a
    // black bar, and a bad pixel could appear to click a menu entry.
    const CanvasViewport atMaximum{INT_MAX, INT_MAX, 1, 1};
    int x = -1;
    int y = -1;
    Check(TranslatePreviewMouse(atMaximum, INT_MAX, INT_MAX, x, y) &&
          x == 0 && y == 0,
          "valid maximum native client position does not overflow");
    Check(!TranslatePreviewMouse(atMaximum, INT_MIN, INT_MIN, x, y) &&
          x == 0 && y == 0,
          "extreme negative coordinates remain rejected");
}

void TestTypicalAndLargeDisplays()
{
    using namespace latebrake_preview;
    const auto desktop = FitPreviewViewport(1920, 1080);
    Check(desktop.x == 150 && desktop.y == 0 &&
          desktop.w == 1620 && desktop.h == 1080,
          "1080p display has 150px side bars");
    int x = -1, y = -1;
    Check(TranslatePreviewMouse(desktop, 960, 540, x, y) &&
          x == PreviewCanvasWidth / 2 &&
          y == PreviewCanvasHeight / 2,
          "center pixel is mapped precisely on 1080p");
    const auto square = FitPreviewViewport(800, 800);
    Check(square.w == 800 && square.h == 533 && square.y == 133,
          "square window creates symmetric top and bottom bars");

    for (const std::array<int, 2> size : {
        std::array<int, 2>{1, 1}, {1, INT_MAX},
        {INT_MAX, 1}, {INT_MAX, INT_MAX},
        {7680, 4320}, {1024, 768}, {2560, 1440},
        {1280, 720}, {800, 600}, {2, 2}, {960, 640}})
    {
        TestMapping(size[0], size[1]);
    }
}

void TestDeterministicStress()
{
    using namespace latebrake_preview;
    // Fixed seed and no nondeterministic callbacks: same 100k cases on
    // Windows, Ubuntu and macOS under -Werror /WX.
    std::mt19937 engine{0x17ad23u};
    std::uniform_int_distribution<int> dim(1, INT_MAX);
    std::uniform_int_distribution<int> divisor(1, 4);
    for (int i = 0; i < 100000; ++i)
    {
        const int w = dim(engine) / divisor(engine);
        const int h = dim(engine) / divisor(engine);
        const auto v = FitPreviewViewport(w, h);
        Check(v.w >= 1 && v.h >= 1 &&
              static_cast<long long>(v.x) + v.w <= w &&
              static_cast<long long>(v.y) + v.h <= h,
              "100k viewport fits are exact and nonoverflowing");

        int x = -4;
        int y = -5;
        const int innerX = static_cast<int>(
            static_cast<long long>(v.x) +
            static_cast<long long>(v.w - 1) * (i % 997) / 997);
        const int innerY = static_cast<int>(
            static_cast<long long>(v.y) +
            static_cast<long long>(v.h - 1) * (i % 991) / 991);
        Check(TranslatePreviewMouse(v, innerX, innerY, x, y) &&
              x >= 0 && x < PreviewCanvasWidth &&
              y >= 0 && y < PreviewCanvasHeight,
              "100k valid pointer mappings remain strictly bounded");
    }
}
} // namespace

int main()
{
    TestTypicalAndLargeDisplays();
    TestBadDimensions();
    TestDeterministicStress();
    std::cout << "2D Win64 trial viewport and mouse QA passed.\n";
    return EXIT_SUCCESS;
}
