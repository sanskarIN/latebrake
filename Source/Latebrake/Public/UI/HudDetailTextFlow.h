#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace latebrake
{
// Read-only layout spans: caller owns text, typography and rendering.
// Actual font measurements, not an assumed fixed-width alphabet, decide
// the line breaks. The result never grants gameplay or input authority.
struct HudDetailWrapResult final
{
    int Lines = 0;
    int Consumed = 0;
    bool Complete = false;
};

inline bool IsHudDetailSpace(std::uint32_t codeUnit) noexcept
{
    return codeUnit == 0x20u || codeUnit == 0x09u;
}

inline bool IsHudDetailExtend(std::uint32_t scalar) noexcept
{
    return (scalar >= 0x0300u && scalar <= 0x036Fu) ||
        (scalar >= 0x1AB0u && scalar <= 0x1AFFu) ||
        (scalar >= 0x1DC0u && scalar <= 0x1DFFu) ||
        (scalar >= 0x20D0u && scalar <= 0x20FFu) ||
        (scalar >= 0xFE20u && scalar <= 0xFE2Fu) ||
        (scalar >= 0xFE00u && scalar <= 0xFE0Fu) ||
        (scalar >= 0xE0100u && scalar <= 0xE01EFu) ||
        (scalar >= 0x1F3FBu && scalar <= 0x1F3FFu) ||
        // Common Devanagari vowel signs, virama and accent marks.
        (scalar >= 0x0900u && scalar <= 0x0903u) ||
        (scalar >= 0x093Au && scalar <= 0x094Fu) ||
        (scalar >= 0x0951u && scalar <= 0x0957u) ||
        (scalar >= 0x0962u && scalar <= 0x0963u) ||
        // Selected combining marks, vowel signs and viramas from major
        // Indic scripts. This remains a conservative subset of UAX #29;
        // these ranges prevent partial menu glyphs without inventing a
        // complete locale-specific shaping or line-breaking engine.
        (scalar >= 0x0981u && scalar <= 0x0983u) ||
        scalar == 0x09BCu ||
        (scalar >= 0x09BEu && scalar <= 0x09CDu) ||
        scalar == 0x09D7u ||
        (scalar >= 0x09E2u && scalar <= 0x09E3u) ||
        (scalar >= 0x0A01u && scalar <= 0x0A03u) ||
        scalar == 0x0A3Cu ||
        (scalar >= 0x0A3Eu && scalar <= 0x0A4Du) ||
        (scalar >= 0x0A70u && scalar <= 0x0A71u) ||
        scalar == 0x0A75u ||
        (scalar >= 0x0A81u && scalar <= 0x0A83u) ||
        scalar == 0x0ABCu ||
        (scalar >= 0x0ABEu && scalar <= 0x0ACDu) ||
        (scalar >= 0x0AE2u && scalar <= 0x0AE3u) ||
        (scalar >= 0x0B01u && scalar <= 0x0B03u) ||
        scalar == 0x0B3Cu ||
        (scalar >= 0x0B3Eu && scalar <= 0x0B4Du) ||
        (scalar >= 0x0B56u && scalar <= 0x0B57u) ||
        (scalar >= 0x0B62u && scalar <= 0x0B63u) ||
        scalar == 0x0B82u ||
        (scalar >= 0x0BBEu && scalar <= 0x0BCDu) ||
        scalar == 0x0BD7u ||
        (scalar >= 0x0C00u && scalar <= 0x0C04u) ||
        scalar == 0x0C3Cu ||
        (scalar >= 0x0C3Eu && scalar <= 0x0C4Du) ||
        (scalar >= 0x0C55u && scalar <= 0x0C56u) ||
        (scalar >= 0x0C62u && scalar <= 0x0C63u) ||
        (scalar >= 0x0C81u && scalar <= 0x0C83u) ||
        scalar == 0x0CBCu ||
        (scalar >= 0x0CBEu && scalar <= 0x0CCDu) ||
        (scalar >= 0x0CD5u && scalar <= 0x0CD6u) ||
        (scalar >= 0x0CE2u && scalar <= 0x0CE3u) ||
        (scalar >= 0x0D00u && scalar <= 0x0D03u) ||
        (scalar >= 0x0D3Bu && scalar <= 0x0D4Du) ||
        scalar == 0x0D57u ||
        (scalar >= 0x0D62u && scalar <= 0x0D63u) ||
        scalar == 0x200Cu; // ZWNJ stays with the preceding cluster.
}

inline bool IsHudDetailIndicVirama(std::uint32_t scalar) noexcept
{
    switch (scalar)
    {
    case 0x094Du: // Devanagari
    case 0x09CDu: // Bengali
    case 0x0A4Du: // Gurmukhi
    case 0x0ACDu: // Gujarati
    case 0x0B4Du: // Odia
    case 0x0BCDu: // Tamil
    case 0x0C4Du: // Telugu
    case 0x0CCDu: // Kannada
    case 0x0D4Du: // Malayalam
        return true;
    default:
        return false;
    }
}

inline bool IsHudDetailIndicConsonant(std::uint32_t scalar) noexcept
{
    return (scalar >= 0x0915u && scalar <= 0x0939u) ||
        (scalar >= 0x0958u && scalar <= 0x095Fu) ||
        (scalar >= 0x0995u && scalar <= 0x09B9u) ||
        (scalar >= 0x0A15u && scalar <= 0x0A39u) ||
        (scalar >= 0x0A95u && scalar <= 0x0AB9u) ||
        (scalar >= 0x0B15u && scalar <= 0x0B39u) ||
        (scalar >= 0x0B95u && scalar <= 0x0BB9u) ||
        (scalar >= 0x0C15u && scalar <= 0x0C39u) ||
        (scalar >= 0x0C95u && scalar <= 0x0CB9u) ||
        (scalar >= 0x0D15u && scalar <= 0x0D39u);
}

template <typename CodeUnit>
std::uint32_t HudDetailScalarAt(
    int index, int length, CodeUnit codeUnit) noexcept
{
    if (index < 0 || index >= length)
    {
        return 0;
    }
    const auto first = static_cast<std::uint32_t>(codeUnit(index));
    if (first >= 0xD800u && first <= 0xDBFFu &&
        index + 1 < length)
    {
        const auto second =
            static_cast<std::uint32_t>(codeUnit(index + 1));
        if (second >= 0xDC00u && second <= 0xDFFFu)
        {
            return 0x10000u + ((first - 0xD800u) << 10u) +
                (second - 0xDC00u);
        }
    }
    return first;
}

template <typename CodeUnit>
std::uint32_t HudDetailScalarBefore(
    int index, CodeUnit codeUnit) noexcept
{
    if (index <= 0)
    {
        return 0;
    }
    const auto last = static_cast<std::uint32_t>(codeUnit(index - 1));
    if (last >= 0xDC00u && last <= 0xDFFFu && index >= 2)
    {
        const auto first =
            static_cast<std::uint32_t>(codeUnit(index - 2));
        if (first >= 0xD800u && first <= 0xDBFFu)
        {
            return 0x10000u + ((first - 0xD800u) << 10u) +
                (last - 0xDC00u);
        }
    }
    return last;
}

// A conservative subset of Unicode extended-grapheme boundaries.
// In particular, never cut a UTF-16 surrogate pair, a combining accent,
// common Indic matras and virama conjuncts, emoji modifiers, joined emoji or flag pair.
// This is not a full UAX #29 implementation; unsupported clusters can
// still require full shaping-engine line breaking in a future UMG UI.
template <typename CodeUnit>
bool IsHudDetailSafeBoundary(
    int begin, int cut, int end, CodeUnit codeUnit) noexcept
{
    if (cut <= begin || cut >= end)
    {
        return true;
    }
    const std::uint32_t previous =
        HudDetailScalarBefore(cut, codeUnit);
    const std::uint32_t next =
        HudDetailScalarAt(cut, end, codeUnit);
    if ((previous >= 0xD800u && previous <= 0xDBFFu &&
         next >= 0xDC00u && next <= 0xDFFFu) ||
        IsHudDetailExtend(next) ||
        // Devanagari consonant conjuncts (e.g. KA + VIRAMA + SSA)
        // must not lose their shaping connection at a hard line break.
        (IsHudDetailIndicVirama(previous) &&
         IsHudDetailIndicConsonant(next)) ||
        previous == 0x200Du || next == 0x200Du ||
        (previous == 0x000Du && next == 0x000Au))
    {
        return false;
    }
    // Regional-indicator emoji group into pairs, not arbitrary runs.
    if (previous >= 0x1F1E6u && previous <= 0x1F1FFu &&
        next >= 0x1F1E6u && next <= 0x1F1FFu)
    {
        int preceding = 0;
        for (int at = cut; at > begin;)
        {
            const auto scalar = HudDetailScalarBefore(at, codeUnit);
            if (scalar < 0x1F1E6u || scalar > 0x1F1FFu)
            {
                break;
            }
            ++preceding;
            at -= scalar > 0xFFFFu ? 2 : 1;
        }
        return preceding % 2 == 0;
    }
    return true;
}

// emit(start, count) receives bounded nonempty slices in display order.
// measure(start, count) must return the ACTUAL scaled font width in pixels.
// Hard-wrap only if no whole-word break fits. Do not render a glyph when
// even its complete cluster cannot fit in the supplied text lane.
template <typename CodeUnit, typename Measure, typename Emit>
HudDetailWrapResult WrapHudDetailText(
    int length, double maxWidth, CodeUnit codeUnit,
    Measure measure, Emit emit, int maxLines = 128) noexcept
{
    HudDetailWrapResult result;
    if (length < 0 || length > 16384 || maxLines <= 0 ||
        maxLines > 1024 || !std::isfinite(maxWidth) || maxWidth <= 0.0)
    {
        return result;
    }

    int position = 0;
    while (position < length && result.Lines < maxLines)
    {
        const auto code = static_cast<std::uint32_t>(codeUnit(position));
        if (code == 0x0Au || code == 0x0Du)
        {
            ++position;
            if (code == 0x0Du && position < length &&
                static_cast<std::uint32_t>(codeUnit(position)) == 0x0Au)
            {
                ++position;
            }
            continue;
        }
        if (IsHudDetailSpace(code))
        {
            ++position;
            continue;
        }
        int paragraphEnd = position;
        while (paragraphEnd < length)
        {
            const auto next =
                static_cast<std::uint32_t>(codeUnit(paragraphEnd));
            if (next == 0x0Au || next == 0x0Du)
            {
                break;
            }
            ++paragraphEnd;
        }

        int cut = paragraphEnd;
        const double entireWidth =
            measure(position, paragraphEnd - position);
        if (!std::isfinite(entireWidth) || entireWidth < 0.0)
        {
            result.Consumed = position;
            return result;
        }
        if (entireWidth > maxWidth)
        {
            int low = position;
            int high = paragraphEnd;
            while (low < high)
            {
                const int mid = low + (high - low + 1) / 2;
                const double width = measure(position, mid - position);
                if (std::isfinite(width) && width >= 0.0 &&
                    width <= maxWidth)
                {
                    low = mid;
                }
                else
                {
                    high = mid - 1;
                }
            }
            cut = low;
            while (cut > position &&
                   !IsHudDetailSafeBoundary(
                       position, cut, paragraphEnd, codeUnit))
            {
                --cut;
            }
            if (cut <= position)
            {
                result.Consumed = position;
                return result;
            }
            // A text engine can have slightly nonmonotone kerning widths.
            // Recheck the exact final candidate rather than assuming
            // every lower binary-search candidate was also a valid fit.
            const double finalWidth = measure(position, cut - position);
            if (!std::isfinite(finalWidth) || finalWidth < 0.0 ||
                finalWidth > maxWidth)
            {
                result.Consumed = position;
                return result;
            }
        }
        int nextPosition = cut;
        if (cut < paragraphEnd)
        {
            for (int at = cut; at > position; --at)
            {
                if (IsHudDetailSpace(
                    static_cast<std::uint32_t>(codeUnit(at - 1))))
                {
                    if (at - 1 > position)
                    {
                        cut = at - 1;
                        nextPosition = at;
                    }
                    break;
                }
            }
        }
        while (cut > position && IsHudDetailSpace(
            static_cast<std::uint32_t>(codeUnit(cut - 1))))
        {
            --cut;
        }
        if (cut > position)
        {
            // The final emitted slice can differ from the binary-search
            // candidate after preferring a word break or removing trailing
            // whitespace. Proportional shaping/ligatures need not have
            // monotonic substring widths, so measure the ACTUAL slice.
            // Never paint outside the lane if shaping changed its width.
            const double emittedWidth = measure(position, cut - position);
            if (!std::isfinite(emittedWidth) || emittedWidth < 0.0 ||
                emittedWidth > maxWidth)
            {
                result.Consumed = position;
                return result;
            }
            emit(position, cut - position);
            ++result.Lines;
        }
        position = nextPosition;
    }
    // Reaching the visible row budget is not truncation if the only
    // remaining input is formatting whitespace. Consume that tail so
    // an exact-fit paragraph does not display a false omission notice.
    // Nonspace content still fails closed as incomplete.
    while (position < length)
    {
        const std::uint32_t code =
            static_cast<std::uint32_t>(codeUnit(position));
        if (code != 0x0Au && code != 0x0Du &&
            !IsHudDetailSpace(code))
        {
            break;
        }
        ++position;
    }
    result.Consumed = position;
    result.Complete = position == length;
    return result;
}
} // namespace latebrake
