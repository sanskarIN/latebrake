#include "UI/HudDetailTextFlow.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
void Check(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "HUD detail text flow FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct TestWrap final
{
    latebrake::HudDetailWrapResult Result;
    std::vector<std::u16string> Lines;
};

TestWrap Wrap(std::u16string_view source, double width,
              int capacity = 128)
{
    TestWrap wrapped;
    const auto unit = [&](int index) -> std::uint32_t
    {
        return static_cast<std::uint32_t>(source[
            static_cast<std::size_t>(index)]);
    };
    const auto measure = [&](int begin, int count) -> double
    {
        double pixels = 0;
        for (int index = begin; index < begin + count; ++index)
        {
            const auto value = unit(index);
            pixels += value == u'W' ? 3.0 :
                value == u'i' ? 0.5 : 1.0;
        }
        return pixels;
    };
    wrapped.Result = latebrake::WrapHudDetailText(
        static_cast<int>(source.size()), width,
        unit, measure,
        [&](int begin, int count)
        {
            Check(begin >= 0 && count > 0 &&
                  begin + count <= static_cast<int>(source.size()),
                  "emitted slices remain inside the text");
            Check(measure(begin, count) <= width + 0.00001,
                  "every visible row is truly font-width bounded");
            Check(latebrake::IsHudDetailSafeBoundary(
                0, begin, static_cast<int>(source.size()), unit),
                "start must be Unicode-safe");
            Check(latebrake::IsHudDetailSafeBoundary(
                0, begin + count, static_cast<int>(source.size()), unit),
                "end must be Unicode-safe");
            wrapped.Lines.emplace_back(source.substr(
                static_cast<std::size_t>(begin),
                static_cast<std::size_t>(count)));
        }, capacity);
    Check(wrapped.Result.Lines == static_cast<int>(wrapped.Lines.size()),
          "reported row count matches emitted text");
    return wrapped;
}
} // namespace

int main()
{
    using namespace latebrake;

    const auto narrative = Wrap(
        u"GARAGE SIX CARS EACH HAS A LONG DESCRIPTION", 12.0);
    Check(narrative.Result.Complete && narrative.Result.Lines >= 4,
          "long garage details become multiple readable rows");
    std::u16string actual;
    for (const auto& row : narrative.Lines)
    {
        for (const char16_t unit : row)
        {
            if (unit != u' ') actual.push_back(unit);
        }
    }
    std::u16string expected;
    for (const char16_t unit :
         std::u16string(u"GARAGE SIX CARS EACH HAS A LONG DESCRIPTION"))
    {
        if (unit != u' ') expected.push_back(unit);
    }
    Check(actual == expected,
          "wrapping preserves every nonseparator character");

    const auto solid = Wrap(u"ABCDEFGHIJKL", 4.0);
    Check(solid.Result.Complete && solid.Result.Lines == 3 &&
          solid.Lines[0] == u"ABCD" &&
          solid.Lines[1] == u"EFGH" &&
          solid.Lines[2] == u"IJKL",
          "unbreakable words hard-wrap without losing glyphs");

    const auto proportional = Wrap(u"WWWWiiiiiiii", 6.0);
    Check(proportional.Result.Complete &&
          proportional.Lines[0] == u"WW" &&
          proportional.Lines.size() >= 2,
          "proportional font metrics are used instead of character counts");

    const auto multilingual = Wrap(u"A\U0001F680B", 2.0);
    Check(multilingual.Result.Complete &&
          multilingual.Lines.size() == 3 &&
          multilingual.Lines[0] == u"A" &&
          multilingual.Lines[1] == u"\U0001F680" &&
          multilingual.Lines[2] == u"B",
          "UTF-16 surrogate pairs are never cut in half");
    Check(!Wrap(u"\U0001F680", 1.0).Result.Complete,
          "a too-wide emoji fails closed instead of drawing half a pair");

    const auto accent = Wrap(u"e\u0301x", 2.0);
    Check(accent.Result.Complete && accent.Lines.size() == 2 &&
          accent.Lines[0] == u"e\u0301" &&
          accent.Lines[1] == u"x",
          "combining accents stay attached to their base");
    Check(!Wrap(u"e\u0301x", 1.0).Result.Complete,
          "no orphaned combining marks when a cluster cannot fit");

    const auto hindi = Wrap(u"\u0915\u093E\u0930", 2.0);
    Check(hindi.Result.Complete && hindi.Lines.size() == 2 &&
          hindi.Lines[0] == u"\u0915\u093E" &&
          hindi.Lines[1] == u"\u0930",
          "common Devanagari vowel signs remain with their base");
    Check(!Wrap(u"\u0915\u093E", 1.0).Result.Complete,
          "narrow lanes do not emit isolated Hindi base letters");

    // A Hindi conjunct written without an explicit ZWJ must remain
    // one shaping cluster: KA + VIRAMA + SSA, followed by a plain X.
    const auto conjunct = Wrap(u"\u0915\u094D\u0937X", 3.0);
    Check(conjunct.Result.Complete && conjunct.Lines.size() == 2 &&
          conjunct.Lines[0] == u"\u0915\u094D\u0937" &&
          conjunct.Lines[1] == u"X",
          "Devanagari virama conjunct remains intact");
    Check(!Wrap(u"\u0915\u094D\u0937", 2.0).Result.Complete,
          "too-narrow lanes cannot break a shaped Hindi conjunct");

    // Prevent glyphs from breaking across common Indic vowel-sign and
    // virama-consonant clusters, beyond Devanagari/Hindi alone.
    const std::u16string_view conjuncts[] = {
        u"\u0995\u09CD\u09B7X", // Bengali
        u"\u0A95\u0ACD\u0AB7X", // Gujarati
        u"\u0B95\u0BCD\u0BB7X", // Tamil
        u"\u0C15\u0C4D\u0C37X", // Telugu
        u"\u0C95\u0CCD\u0CB7X", // Kannada
        u"\u0D15\u0D4D\u0D37X"  // Malayalam
    };
    for (const auto conjunctText : conjuncts)
    {
        const auto visible = Wrap(conjunctText, 3.0);
        Check(visible.Result.Complete && visible.Lines.size() == 2 &&
              visible.Lines[0] == conjunctText.substr(0, 3) &&
              visible.Lines[1] == u"X",
              "Indic script conjuncts remain single shaped clusters");
        const auto narrow = Wrap(conjunctText, 2.0);
        Check(!narrow.Result.Complete && narrow.Result.Lines == 0,
              "too-narrow lanes never emit half an Indic conjunct");
    }
    for (const auto vowelText : {
        std::u16string_view(u"\u0995\u09BEZ"), // Bengali matra
        std::u16string_view(u"\u0B95\u0BBEZ"), // Tamil matra
        std::u16string_view(u"\u0D15\u0D3EZ")  // Malayalam matra
    })
    {
        Check(!Wrap(vowelText, 1.0).Result.Complete,
              "combining vowel sign cannot be emitted independently");
        const auto whole = Wrap(vowelText, 2.0);
        Check(whole.Result.Complete && whole.Lines.size() == 2 &&
              whole.Lines[0] == vowelText.substr(0, 2),
              "base and Indic vowel sign must stay together");
    }

    const auto joined = Wrap(u"\U0001F468\u200D\U0001F4BBZ", 5.0);
    Check(joined.Result.Complete && joined.Lines.size() == 2 &&
          joined.Lines[0] == u"\U0001F468\u200D\U0001F4BB",
          "ZWJ-connected emoji stay together");
    Check(!Wrap(u"\U0001F468\u200D\U0001F4BBZ", 4.0).Result.Complete,
          "too-wide joined emoji fail closed");
    const auto flag = Wrap(u"\U0001F1EE\U0001F1F3Z", 4.0);
    Check(flag.Result.Complete && flag.Lines.size() == 2 &&
          flag.Lines[0] == u"\U0001F1EE\U0001F1F3",
          "regional-indicator flags stay paired");
    Check(!Wrap(u"\U0001F1EE\U0001F1F3Z", 2.0).Result.Complete,
          "an incomplete flag may not render");

    const auto paragraphs = Wrap(u"\r\n  FIRST  \n SECOND ", 12.0);
    Check(paragraphs.Result.Complete &&
          paragraphs.Lines.size() == 2 &&
          paragraphs.Lines[0] == u"FIRST" &&
          paragraphs.Lines[1] == u"SECOND",
          "CRLF, whitespace and trailing spaces never create phantom rows");
    const auto exactBudget = Wrap(u"ONE\nTWO\n \r\n", 12.0, 2);
    Check(exactBudget.Result.Complete && exactBudget.Result.Lines == 2 &&
          exactBudget.Lines[0] == u"ONE" &&
          exactBudget.Lines[1] == u"TWO",
          "only trailing line separators cannot trigger a false omission");
    const auto extraAfterBudget = Wrap(u"ONE\nTWO\nTHREE", 12.0, 2);
    Check(!extraAfterBudget.Result.Complete &&
          extraAfterBudget.Result.Lines == 2,
          "real text past the final visible row remains incomplete");

    const auto limited = Wrap(u"ONE TWO THREE FOUR FIVE", 4.0, 2);
    Check(!limited.Result.Complete && limited.Result.Lines == 2 &&
          limited.Result.Consumed <
              static_cast<int>(std::u16string_view(
                  u"ONE TWO THREE FOUR FIVE").size()),
          "row limits report truncation without drawing offscreen");
    Check(Wrap(u"", 12.0).Result.Complete,
          "empty strings are a completed zero-row wrap");
    Check(!Wrap(u"HELLO", 0.0).Result.Complete &&
          !Wrap(u"HELLO", std::numeric_limits<double>::quiet_NaN())
              .Result.Complete,
          "invalid geometry fails closed");

    // A shaping engine can measure the longer candidate as fitting but
    // its shorter whole-word form as wider (different ligature context).
    // The emitted row must be measured AFTER selecting the word boundary.
    const std::u16string changedAfterWordBreak(u"ABCD EFGHIJ");
    bool emittedBadWord = false;
    const auto unsafeWord = WrapHudDetailText(
        static_cast<int>(changedAfterWordBreak.size()), 8.0,
        [&](int i) {
            return static_cast<std::uint32_t>(changedAfterWordBreak[
                static_cast<std::size_t>(i)]);
        },
        [](int begin, int count) -> double {
            return begin == 0 && count == 4 ? 20.0 :
                static_cast<double>(count);
        },
        [&](int, int) { emittedBadWord = true; });
    Check(!unsafeWord.Complete && unsafeWord.Lines == 0 &&
          unsafeWord.Consumed == 0 && !emittedBadWord,
          "word-boundary reflow must remeasure actual emitted glyphs");

    // The same property holds for the trailing spaces of an otherwise
    // measured-as-fitting paragraph. Trimming may change shaping width.
    const std::u16string trimmedSource(u"AB  ");
    bool emittedBadTrim = false;
    const auto unsafeTrim = WrapHudDetailText(
        static_cast<int>(trimmedSource.size()), 4.0,
        [&](int i) {
            return static_cast<std::uint32_t>(trimmedSource[
                static_cast<std::size_t>(i)]);
        },
        [](int, int count) -> double {
            return count == 2 ? 9.0 : static_cast<double>(count);
        },
        [&](int, int) { emittedBadTrim = true; });
    Check(!unsafeTrim.Complete && unsafeTrim.Lines == 0 &&
          unsafeTrim.Consumed == 0 && !emittedBadTrim,
          "trailing-space trim must not bypass final measured text width");

    const std::u16string badMeasurement(u"HELLO");
    const auto invalid = WrapHudDetailText(
        5, 10.0, [&](int i) { return badMeasurement[
            static_cast<std::size_t>(i)]; },
        [](int, int) { return std::numeric_limits<double>::infinity(); },
        [](int, int) {});
    Check(!invalid.Complete && invalid.Lines == 0,
          "nonfinite scaled font measurements are rejected");

    std::cout << "HUD detail measured wrapping and Unicode tests passed.\n";
    return EXIT_SUCCESS;
}
