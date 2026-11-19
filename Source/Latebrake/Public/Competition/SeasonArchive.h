#pragma once

#include "ChampionshipSeason.h"
#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::competition {

// Bounded, versioned analytics-only season snapshot. The caller owns actual
// filesystem access; this deliberately cannot mutate Unreal SaveGame state.
inline constexpr std::size_t MaximumSeasonArchiveBytes = 1024;
inline constexpr std::uint32_t SeasonArchiveMagic = 0x4353424cU; // LBSC
inline constexpr std::uint16_t SeasonArchiveVersion = 1;
inline constexpr std::size_t SeasonArchiveHeaderBytes = 28;
inline constexpr std::size_t SeasonArchiveEntryBytes = 152;
inline constexpr std::size_t SeasonArchiveChecksumBytes = 4;

struct SeasonArchive final {
    std::array<std::uint8_t, MaximumSeasonArchiveBytes> bytes{};
    std::size_t length = 0;
    bool valid = false;
};

struct ImportedSeasonRanking final {
    SeasonRanking ranking{};
    std::uint64_t seasonIdentity = 0;
    bool valid = false;
};

inline std::uint32_t SeasonArchiveChecksum(
    const std::uint8_t* source, std::size_t length) noexcept {
    std::uint32_t checksum = 0xffffffffU;
    for (std::size_t i = 0; i < length; ++i) {
        checksum ^= static_cast<std::uint32_t>(source[i]);
        for (int bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask =
                0U - (checksum & 1U);
            checksum = (checksum >> 1U) ^ (0xedb88320U & mask);
        }
    }
    return ~checksum;
}

class SeasonArchiveWriter final {
public:
    explicit SeasonArchiveWriter(SeasonArchive& out) noexcept
        : destination_(out) {}

    bool Put8(std::uint8_t value) noexcept {
        if (!ok_ || destination_.length >= destination_.bytes.size()) {
            ok_ = false;
            return false;
        }
        destination_.bytes[destination_.length++] = value;
        return true;
    }

    bool Put16(std::uint16_t value) noexcept {
        for (int i = 0; i < 2; ++i) {
            if (!Put8(static_cast<std::uint8_t>(value & 0xffU))) {
                return false;
            }
            value = static_cast<std::uint16_t>(value >> 8U);
        }
        return true;
    }

    bool Put32(std::uint32_t value) noexcept {
        for (int i = 0; i < 4; ++i) {
            if (!Put8(static_cast<std::uint8_t>(value & 0xffU))) {
                return false;
            }
            value >>= 8U;
        }
        return true;
    }

    bool Put64(std::uint64_t value) noexcept {
        for (int i = 0; i < 8; ++i) {
            if (!Put8(static_cast<std::uint8_t>(value & 0xffU))) {
                return false;
            }
            value >>= 8U;
        }
        return true;
    }

    bool PutNonnegative(int value) noexcept {
        return value >= 0 && Put32(static_cast<std::uint32_t>(value));
    }

    bool Good() const noexcept { return ok_; }

private:
    SeasonArchive& destination_;
    bool ok_ = true;
};

class SeasonArchiveReader final {
public:
    SeasonArchiveReader(const std::uint8_t* source,
                        std::size_t length) noexcept
        : source_(source), length_(length) {}

    std::uint8_t Get8() noexcept {
        if (!ok_ || source_ == nullptr || cursor_ >= length_) {
            ok_ = false;
            return 0;
        }
        return source_[cursor_++];
    }

    std::uint16_t Get16() noexcept {
        std::uint16_t output = 0;
        for (int i = 0; i < 2; ++i) {
            const auto byte = Get8();
            output |= static_cast<std::uint16_t>(
                static_cast<std::uint16_t>(byte) << (8 * i));
        }
        return output;
    }

    std::uint32_t Get32() noexcept {
        std::uint32_t output = 0;
        for (int i = 0; i < 4; ++i) {
            const auto byte = Get8();
            output |= static_cast<std::uint32_t>(byte) << (8 * i);
        }
        return output;
    }

    std::uint64_t Get64() noexcept {
        std::uint64_t output = 0;
        for (int i = 0; i < 8; ++i) {
            const auto byte = Get8();
            output |= static_cast<std::uint64_t>(byte) << (8 * i);
        }
        return output;
    }

    int GetNonnegative() noexcept {
        const std::uint32_t source = Get32();
        if (!ok_ || source > static_cast<std::uint32_t>(INT_MAX)) {
            ok_ = false;
            return 0;
        }
        return static_cast<int>(source);
    }

    bool Good() const noexcept { return ok_; }
    std::size_t Cursor() const noexcept { return cursor_; }

private:
    const std::uint8_t* source_ = nullptr;
    std::size_t length_ = 0;
    std::size_t cursor_ = 0;
    bool ok_ = true;
};

inline bool ValidArchiveSummary(
    const DriverSeasonSummary& s, int completedRounds,
    int scheduledRounds) noexcept {
    if (s.actor == 0 || completedRounds < 0 ||
        scheduledRounds < completedRounds ||
        scheduledRounds > static_cast<int>(MaximumSeasonRounds) ||
        s.points < 0 || s.points > completedRounds * 26 ||
        s.roundsEntered != completedRounds ||
        s.classifiedFinishes < 0 ||
        s.classifiedFinishes > completedRounds ||
        s.fastestLapBonuses < 0 ||
        s.fastestLapBonuses > s.classifiedFinishes ||
        s.wins < 0 || s.wins > s.classifiedFinishes ||
        s.podiums < s.wins ||
        s.podiums > s.classifiedFinishes) {
        return false;
    }
    int positionTotal = 0;
    int awardedTotal = 0;
    int podiumTotal = 0;
    for (std::size_t pos = 0; pos < MaximumSeasonDrivers; ++pos) {
        const int count = s.positionCounts[pos];
        if (count < 0 || count > completedRounds) return false;
        positionTotal += count;
        if (pos < 3) podiumTotal += count;
    }
    for (int round = 0; round < scheduledRounds; ++round) {
        const int points = s.pointsPerRound[static_cast<std::size_t>(round)];
        if (points < 0 || points > 26 ||
            (round >= completedRounds && points != 0)) {
            return false;
        }
        awardedTotal += points;
    }
    for (int round = scheduledRounds;
         round < static_cast<int>(MaximumSeasonRounds); ++round) {
        if (s.pointsPerRound[static_cast<std::size_t>(round)] != 0) {
            return false;
        }
    }
    return positionTotal == s.classifiedFinishes &&
           podiumTotal == s.podiums &&
           s.positionCounts[0] == s.wins &&
           awardedTotal == s.points;
}

inline bool ValidArchiveRanking(const SeasonRanking& ranking) noexcept {
    if (!ranking.valid ||
        ranking.count < 2 ||
        ranking.count > MaximumSeasonDrivers ||
        ranking.completedRounds < 0 ||
        ranking.remainingRounds < 0 ||
        ranking.completedRounds + ranking.remainingRounds >
            static_cast<int>(MaximumSeasonRounds)) {
        return false;
    }
    const int totalRounds =
        ranking.completedRounds + ranking.remainingRounds;
    std::array<int, MaximumSeasonDrivers> overallPositions{};
    int totalFastestBonuses = 0;
    for (std::size_t i = 0; i < ranking.count; ++i) {
        const auto& driver = ranking.sorted[i];
        if (!ValidArchiveSummary(driver, ranking.completedRounds,
                                 totalRounds)) {
            return false;
        }
        totalFastestBonuses += driver.fastestLapBonuses;
        for (std::size_t p = 0; p < overallPositions.size(); ++p) {
            overallPositions[p] += driver.positionCounts[p];
        }
        for (std::size_t j = 0; j < i; ++j) {
            const auto& earlier = ranking.sorted[j];
            if (driver.actor == earlier.actor) return false;
            // Archive contents must retain the canonical championship
            // order; untrusted bytes cannot rename an unsorted leaderboard
            // as an official-looking sorted standings report.
            if (driver.points > earlier.points) return false;
            if (driver.points == earlier.points &&
                driver.wins > earlier.wins) return false;
            if (driver.points == earlier.points &&
                driver.wins == earlier.wins &&
                driver.podiums > earlier.podiums) return false;
            if (driver.points == earlier.points &&
                driver.wins == earlier.wins &&
                driver.podiums == earlier.podiums) {
                for (std::size_t p = 0; p < MaximumSeasonDrivers; ++p) {
                    if (driver.positionCounts[p] !=
                        earlier.positionCounts[p]) {
                        if (driver.positionCounts[p] >
                            earlier.positionCounts[p]) return false;
                        break;
                    }
                }
            }
        }
    }
    if (totalFastestBonuses > ranking.completedRounds) return false;
    for (int positionTotal : overallPositions) {
        if (positionTotal > ranking.completedRounds) return false;
    }
    // Only one verified car can receive a given finishing position in
    // each race. A fabricated points archive must not exceed the physical
    // six-car event's maximum combined awards for one round.
    for (int r = 0; r < ranking.completedRounds; ++r) {
        int totalAwarded = 0;
        for (std::size_t driver = 0; driver < ranking.count; ++driver) {
            totalAwarded += ranking.sorted[driver].pointsPerRound[
                static_cast<std::size_t>(r)];
        }
        if (totalAwarded > 89) return false;
    }
    return true;
}

inline SeasonArchive EncodeSeasonRanking(
    std::uint64_t seasonIdentity,
    const SeasonRanking& ranking) noexcept {
    SeasonArchive encoded{};
    if (seasonIdentity == 0 || !ValidArchiveRanking(ranking)) {
        return encoded;
    }
    SeasonArchiveWriter writer(encoded);
    writer.Put32(SeasonArchiveMagic);
    writer.Put16(SeasonArchiveVersion);
    writer.Put16(0);
    writer.Put64(seasonIdentity);
    writer.Put32(static_cast<std::uint32_t>(
        ranking.completedRounds + ranking.remainingRounds));
    writer.Put32(static_cast<std::uint32_t>(ranking.completedRounds));
    writer.Put32(static_cast<std::uint32_t>(ranking.count));
    if (!writer.Good() ||
        encoded.length != SeasonArchiveHeaderBytes) {
        return SeasonArchive{};
    }
    for (std::size_t i = 0; i < ranking.count; ++i) {
        const auto& s = ranking.sorted[i];
        writer.Put64(s.actor);
        writer.PutNonnegative(s.points);
        writer.PutNonnegative(s.roundsEntered);
        writer.PutNonnegative(s.classifiedFinishes);
        writer.PutNonnegative(s.fastestLapBonuses);
        writer.PutNonnegative(s.wins);
        writer.PutNonnegative(s.podiums);
        for (int count : s.positionCounts) {
            writer.PutNonnegative(count);
        }
        for (int points : s.pointsPerRound) {
            writer.PutNonnegative(points);
        }
        if (!writer.Good()) return SeasonArchive{};
    }
    const std::size_t bodyBytes = SeasonArchiveHeaderBytes +
        ranking.count * SeasonArchiveEntryBytes;
    if (encoded.length != bodyBytes) return SeasonArchive{};
    const auto crc = SeasonArchiveChecksum(
        encoded.bytes.data(), encoded.length);
    writer.Put32(crc);
    if (!writer.Good() ||
        encoded.length != bodyBytes + SeasonArchiveChecksumBytes) {
        return SeasonArchive{};
    }
    encoded.valid = true;
    return encoded;
}

inline ImportedSeasonRanking DecodeSeasonRanking(
    const std::uint8_t* bytes, std::size_t length,
    std::uint64_t expectedSeasonIdentity) noexcept {
    ImportedSeasonRanking decoded{};
    if (bytes == nullptr || expectedSeasonIdentity == 0 ||
        length < SeasonArchiveHeaderBytes + SeasonArchiveChecksumBytes ||
        length > MaximumSeasonArchiveBytes) {
        return decoded;
    }
    // Verify the trailing CRC before trusting any declared sizes, offsets,
    // identities or individual result fields.
    const std::size_t bodyLength = length - SeasonArchiveChecksumBytes;
    SeasonArchiveReader checksumReader(bytes + bodyLength,
                                       SeasonArchiveChecksumBytes);
    const auto storedChecksum = checksumReader.Get32();
    if (!checksumReader.Good() ||
        SeasonArchiveChecksum(bytes, bodyLength) != storedChecksum) {
        return decoded;
    }
    SeasonArchiveReader reader(bytes, bodyLength);
    const auto magic = reader.Get32();
    const auto version = reader.Get16();
    const auto reserved = reader.Get16();
    const auto session = reader.Get64();
    const auto scheduled = reader.Get32();
    const auto completed = reader.Get32();
    const auto count = reader.Get32();
    if (!reader.Good() || magic != SeasonArchiveMagic ||
        version != SeasonArchiveVersion || reserved != 0 ||
        session != expectedSeasonIdentity ||
        scheduled > MaximumSeasonRounds || completed > scheduled ||
        count < 2 || count > MaximumSeasonDrivers ||
        bodyLength != SeasonArchiveHeaderBytes +
            static_cast<std::size_t>(count) * SeasonArchiveEntryBytes) {
        return decoded;
    }
    SeasonRanking ranking{};
    ranking.count = static_cast<std::size_t>(count);
    ranking.completedRounds = static_cast<int>(completed);
    ranking.remainingRounds = static_cast<int>(scheduled - completed);
    for (std::size_t i = 0; i < ranking.count; ++i) {
        auto& s = ranking.sorted[i];
        s.actor = reader.Get64();
        s.points = reader.GetNonnegative();
        s.roundsEntered = reader.GetNonnegative();
        s.classifiedFinishes = reader.GetNonnegative();
        s.fastestLapBonuses = reader.GetNonnegative();
        s.wins = reader.GetNonnegative();
        s.podiums = reader.GetNonnegative();
        for (int& item : s.positionCounts) {
            item = reader.GetNonnegative();
        }
        for (int& item : s.pointsPerRound) {
            item = reader.GetNonnegative();
        }
        if (!reader.Good()) return ImportedSeasonRanking{};
    }
    if (reader.Cursor() != bodyLength) return decoded;
    ranking.valid = true;
    if (!ValidArchiveRanking(ranking)) return decoded;
    decoded.seasonIdentity = session;
    decoded.ranking = ranking;
    decoded.valid = true;
    return decoded;
}

} // namespace latebrake::competition
