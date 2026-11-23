#pragma once

// Original native Win32/GDI 2D showroom policy; no external assets,
// no economy effects, no Unreal Engine assertions and no CRT dependency.
// IDs are append-only. The original six cars retain their indices.
namespace latebrake_preview
{
inline constexpr int PreviewCarCount = 12;
inline constexpr int PreviewCarsPerPage = 6;
inline constexpr int PreviewPaintCount = 8;
inline constexpr int PreviewVinylCount = 3;
inline constexpr int PreviewWheelCount = 3;
inline constexpr unsigned int PreviewPaintPalette[PreviewPaintCount] = {
    0xF65745u, 0xF0C54Du, 0x44D8CEu, 0xB67FFFu,
    0xF0F0F2u, 0xFD70B3u, 0x4388F5u, 0x4FE0A3u
};
inline constexpr unsigned int PreviewFactoryPaint[PreviewCarCount] = {
    0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 1u, 2u, 0u, 4u
};
struct PreviewAppearance final
{
    unsigned int Paint = 0;
    unsigned int Vinyl = 0;  // 0=clean, 1=center, 2=twin stripes
    unsigned int Wheels = 0; // 0=street, 1=sport, 2=track
};
static_assert(sizeof(PreviewAppearance) == 12,
    "Stable on-disk appearance requires exactly three 32-bit fields");

constexpr bool ValidPreviewCar(int id) noexcept
{
    return id >= 0 && id < PreviewCarCount;
}
constexpr PreviewAppearance FactoryPreviewAppearance(int id) noexcept
{
    return {PreviewFactoryPaint[ValidPreviewCar(id) ? id : 0], 0u, 0u};
}
constexpr bool ValidPreviewAppearance(PreviewAppearance a) noexcept
{
    return a.Paint < static_cast<unsigned>(PreviewPaintCount) &&
           a.Vinyl < static_cast<unsigned>(PreviewVinylCount) &&
           a.Wheels < static_cast<unsigned>(PreviewWheelCount);
}
constexpr unsigned int PreviewAppearanceValue(PreviewAppearance a,
                                               int category) noexcept
{
    return category == 0 ? a.Paint :
           category == 1 ? a.Vinyl : category == 2 ? a.Wheels : 0u;
}
constexpr unsigned int PreviewAppearanceOptionCount(int category) noexcept
{
    return category == 0 ? PreviewPaintCount :
           category == 1 ? PreviewVinylCount :
           category == 2 ? PreviewWheelCount : 0u;
}
constexpr bool ChangePreviewAppearance(PreviewAppearance& a,
                                       int category,
                                       unsigned int option) noexcept
{
    if (!ValidPreviewAppearance(a) ||
        option >= PreviewAppearanceOptionCount(category))
        return false;
    if (category == 0) a.Paint = option;
    else if (category == 1) a.Vinyl = option;
    else if (category == 2) a.Wheels = option;
    else return false;
    return true;
}
constexpr unsigned int PreviewPaintColor(PreviewAppearance a) noexcept
{
    return PreviewPaintPalette[
        a.Paint < static_cast<unsigned>(PreviewPaintCount) ? a.Paint : 0u];
}
// A deterministic FNV-1a word stream; no raw struct padding, float
// representation, endianness assumptions or external crypto library.
constexpr unsigned int PreviewAppearanceChecksum(
    unsigned int magic, unsigned int version,
    const PreviewAppearance (&all)[PreviewCarCount]) noexcept
{
    unsigned int h = 2166136261u;
    h = (h ^ magic) * 16777619u;
    h = (h ^ version) * 16777619u;
    for (int i = 0; i < PreviewCarCount; ++i)
    {
        h = (h ^ all[i].Paint) * 16777619u;
        h = (h ^ all[i].Vinyl) * 16777619u;
        h = (h ^ all[i].Wheels) * 16777619u;
    }
    return h;
}
constexpr bool ValidatePreviewAppearanceArray(
    const PreviewAppearance (&all)[PreviewCarCount]) noexcept
{
    for (int i = 0; i < PreviewCarCount; ++i)
        if (!ValidPreviewAppearance(all[i])) return false;
    return true;
}
} // namespace latebrake_preview
