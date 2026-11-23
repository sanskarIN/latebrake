#include "PreviewGarageStyle.h"

#include <cstdlib>
#include <iostream>
#include <random>

namespace
{
void Check(bool ok, const char* why)
{
    if (!ok)
    {
        std::cerr << "Preview garage appearance failure: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void TestCatalogAndOriginalIDs()
{
    using namespace latebrake_preview;
    static_assert(PreviewCarCount == 12);
    static_assert(PreviewCarsPerPage == 6);
    static_assert(PreviewPaintCount == 8);
    static_assert(PreviewAppearanceOptionCount(0) == 8);
    static_assert(PreviewAppearanceOptionCount(1) == 3);
    static_assert(PreviewAppearanceOptionCount(2) == 3);
    static_assert(!ValidPreviewCar(-1) && !ValidPreviewCar(12));
    for (int i = 0; i < PreviewCarCount; ++i)
    {
        auto style = FactoryPreviewAppearance(i);
        Check(ValidPreviewAppearance(style), "factory look is valid");
        Check(PreviewPaintColor(style) == PreviewPaintPalette[PreviewFactoryPaint[i]],
              "factory colors map exactly to immutable ID");
        Check(i / PreviewCarsPerPage <= 1, "exactly two six-car pages");
        Check(PreviewAppearanceValue(style, 1) == 0u &&
              PreviewAppearanceValue(style, 2) == 0u,
              "new styles default to simple clean wheels and paint");
    }
    Check(FactoryPreviewAppearance(-1).Paint == PreviewFactoryPaint[0] &&
          FactoryPreviewAppearance(123).Paint == PreviewFactoryPaint[0],
          "unknown IDs fall back to starter without OOB read");
}

void TestChoicesCannotEscapeBounds()
{
    using namespace latebrake_preview;
    for (int id = 0; id < PreviewCarCount; ++id)
    {
        PreviewAppearance a = FactoryPreviewAppearance(id);
        for (int type = 0; type < 3; ++type)
        {
            const unsigned int count = PreviewAppearanceOptionCount(type);
            for (unsigned int choice = 0; choice < count; ++choice)
            {
                Check(ChangePreviewAppearance(a, type, choice),
                      "all supported appearance choices accepted");
                Check(PreviewAppearanceValue(a, type) == choice,
                      "exact selected style stored");
                Check(ValidPreviewAppearance(a),
                      "other per-car style choices remain valid");
            }
            const auto before = a;
            Check(!ChangePreviewAppearance(a, type, count),
                  "out-of-range choice rejected");
            Check(a.Paint == before.Paint && a.Vinyl == before.Vinyl &&
                  a.Wheels == before.Wheels, "invalid choice cannot alter profile");
        }
        const auto before = a;
        Check(!ChangePreviewAppearance(a, -1, 0) &&
              !ChangePreviewAppearance(a, 3, 0),
              "unknown customization category rejected");
        Check(a.Paint == before.Paint && a.Vinyl == before.Vinyl &&
              a.Wheels == before.Wheels, "unknown category leaves settings inert");
    }
    PreviewAppearance invalid{999u, 0u, 0u};
    Check(!ValidPreviewAppearance(invalid) &&
          !ChangePreviewAppearance(invalid, 0, 1),
          "corrupt input cannot be modified into a valid save by accident");
}

void TestIntegrityAndPerCarIsolation()
{
    using namespace latebrake_preview;
    PreviewAppearance looks[PreviewCarCount]{};
    for (int i = 0; i < PreviewCarCount; ++i)
        looks[i] = FactoryPreviewAppearance(i);
    Check(ValidatePreviewAppearanceArray(looks), "all defaults are valid");
    constexpr unsigned int magic = 0x3147424Cu, version = 1u;
    const unsigned int baseline = PreviewAppearanceChecksum(magic, version, looks);
    Check(PreviewAppearanceChecksum(magic, version, looks) == baseline,
          "same persisted state has stable deterministic checksum");
    Check(PreviewAppearanceChecksum(magic, version + 1u, looks) != baseline,
          "different save version is not silently accepted");
    for (int id = 0; id < PreviewCarCount; ++id)
    {
        auto saved = looks[id];
        for (int category = 0; category < 3; ++category)
        {
            const unsigned int next = (PreviewAppearanceValue(saved, category) + 1u) %
                                      PreviewAppearanceOptionCount(category);
            Check(ChangePreviewAppearance(looks[id], category, next),
                  "single vehicle customization accepted");
            Check(PreviewAppearanceChecksum(magic, version, looks) != baseline,
                  "any one changed field invalidates old checksum");
            for (int other = 0; other < PreviewCarCount; ++other)
                if (id != other)
                    Check(looks[other].Paint == FactoryPreviewAppearance(other).Paint &&
                          looks[other].Vinyl == 0 && looks[other].Wheels == 0,
                          "changing one vehicle never changes another");
            looks[id] = saved;
        }
    }
    Check(PreviewAppearanceChecksum(magic, version, looks) == baseline,
          "restoring original colors restores exact checksum");
    looks[11].Wheels = 999u;
    Check(!ValidatePreviewAppearanceArray(looks),
          "one invalid style must reject the entire persisted record");
}

void TestFutureSchemaDowngradeGuard()
{
    using namespace latebrake_preview;
    static_assert(PreviewLookFileMagic == 0x3147424Cu);
    static_assert(PreviewLookFileVersion == 1u);
    static_assert(IsNewerPreviewAppearanceSchema(PreviewLookFileMagic,2u));
    static_assert(!IsNewerPreviewAppearanceSchema(PreviewLookFileMagic,1u));
    Check(!IsNewerPreviewAppearanceSchema(PreviewLookFileMagic,0u),
          "legacy/invalid appearance version is not an asserted future format");
    Check(!IsNewerPreviewAppearanceSchema(PreviewLookFileMagic,1u),
          "same schema may be overwritten after validation");
    for (unsigned int future=2u;future<100000u;++future)
        Check(IsNewerPreviewAppearanceSchema(PreviewLookFileMagic,future),
              "future version cannot be downgraded by old trial");
    Check(IsNewerPreviewAppearanceSchema(PreviewLookFileMagic,0xffffffffu),
          "max unsigned future version is still protected");
    Check(!IsNewerPreviewAppearanceSchema(0u,0xffffffffu) &&
          !IsNewerPreviewAppearanceSchema(PreviewLookFileMagic ^ 1u,2u),
          "unrelated magic is not misinterpreted as a future schema");
}

void TestDeterministicStress()
{
    using namespace latebrake_preview;
    std::mt19937 rng{0xBBAACC11u};
    PreviewAppearance looks[PreviewCarCount]{};
    for (int i = 0; i < PreviewCarCount; ++i)
        looks[i] = FactoryPreviewAppearance(i);
    for (int i = 0; i < 250000; ++i)
    {
        const int id = static_cast<int>(rng() % PreviewCarCount);
        const int category = static_cast<int>(rng() % 3u);
        const unsigned int options = PreviewAppearanceOptionCount(category);
        const unsigned int option = rng() % options;
        const unsigned int old = PreviewAppearanceChecksum(0x3147424Cu, 1u, looks);
        const auto before = looks[id];
        Check(ChangePreviewAppearance(looks[id], category, option),
              "250k randomized per-car changes stay inside finite palette");
        Check(ValidatePreviewAppearanceArray(looks),
              "every changed array remains valid");
        const unsigned int now = PreviewAppearanceChecksum(0x3147424Cu, 1u, looks);
        if (PreviewAppearanceValue(before, category) != option)
            Check(now != old, "a real choice change modifies serialized checksum");
        else
            Check(now == old, "no-op selection preserves serialized checksum");
    }
}
} // namespace

int main()
{
    TestCatalogAndOriginalIDs();
    TestChoicesCannotEscapeBounds();
    TestIntegrityAndPerCarIsolation();
    TestFutureSchemaDowngradeGuard();
    TestDeterministicStress();
    std::cout << "12-car 2D showroom and persisted livery policy passed.\n";
    return EXIT_SUCCESS;
}
