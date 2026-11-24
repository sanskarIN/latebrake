#pragma once

#include "Telemetry/TelemetryReplayBuffer.h"
#include "Telemetry/TelemetryEventJournal.h"
#include "Telemetry/TelemetrySectorAnalysis.h"
#include "Telemetry/TelemetryStintPerformance.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

namespace latebrake::engineering
{
enum class ExportFormat : std::uint8_t
{
    CsvReplay,
    CsvEvents,
    CsvSectors,
    CsvLaps,
    JsonSummary
};

enum class ExportFailure : std::uint8_t
{
    None,
    InvalidDestination,
    InsufficientCapacity,
    InvalidNumber,
    InvalidIdentity,
    UnsupportedFormat,
    MissingQualifiedData,
    MalformedText,
    NotStarted,
    AlreadyFinished,
    TooManyRows
};

struct ExportSettings final
{
    int MaximumRows = 4096;
    int MaximumBytes = 262144;
    int DecimalPlaces = 6;
    bool RequireNativeSource = true;
    bool IncludeSessionIds = false;
    bool IncludeRawPosition = false;
    bool IncludeControlInputs = true;
    bool UseMetricUnits = true;
    bool IncludeSampleQuality = true;
    bool RejectIncompleteReplaySegments = true;
};

inline bool ValidExportSettings(
    const ExportSettings& s) noexcept
{
    return s.MaximumRows > 0 &&
           s.MaximumRows <= MaximumReplaySamples &&
           s.MaximumBytes >= 128 &&
           s.MaximumBytes <= 1048576 &&
           s.DecimalPlaces >= 0 &&
           s.DecimalPlaces <= 6;
}

struct ExportResult final
{
    ExportFailure Failure = ExportFailure::None;
    ExportFormat Format = ExportFormat::CsvReplay;
    int BytesWritten = 0;
    int RowsWritten = 0;
    int RowsRejected = 0;
    int Capacity = 0;
    std::uint64_t SessionToken = 0;
    std::uint64_t ActorToken = 0;
    bool HadNativeObservations = false;
    bool RedactedIdentity = true;
    bool Finalized = false;
    bool Qualified = false;
};

class BoundedTextWriter final
{
public:
    bool Begin(char* destination, int capacity) noexcept
    {
        Buffer = destination;
        Capacity = capacity;
        Used = 0;
        Good = Buffer != nullptr && Capacity > 1;
        if (Good)
        {
            Buffer[0] = '\0';
        }
        return Good;
    }

    bool Append(char value) noexcept
    {
        if (!Good || Used + 1 >= Capacity)
        {
            Good = false;
            return false;
        }
        Buffer[Used++] = value;
        Buffer[Used] = '\0';
        return true;
    }

    bool Append(std::string_view text) noexcept
    {
        if (!Good ||
            text.size() > static_cast<std::size_t>(
                Capacity - Used - 1))
        {
            Good = false;
            return false;
        }
        for (char ch : text)
        {
            Buffer[Used++] = ch;
        }
        Buffer[Used] = '\0';
        return true;
    }

    bool AppendUnsigned(std::uint64_t value) noexcept
    {
        char digits[24]{};
        int n = 0;
        do
        {
            digits[n++] = static_cast<char>(
                '0' + value % 10U);
            value /= 10U;
        }
        while (value != 0 && n < static_cast<int>(sizeof(digits)));
        while (n > 0)
        {
            if (!Append(digits[--n]))
            {
                return false;
            }
        }
        return true;
    }

    bool AppendSigned(std::int64_t value) noexcept
    {
        if (value < 0 && !Append('-'))
        {
            return false;
        }
        const std::uint64_t magnitude = value < 0 ?
            static_cast<std::uint64_t>(-(value + 1)) + 1U :
            static_cast<std::uint64_t>(value);
        return AppendUnsigned(magnitude);
    }

    bool AppendFixed(double value, int decimalPlaces = 4) noexcept
    {
        if (!Finite(value) ||
            decimalPlaces < 0 || decimalPlaces > 6 ||
            std::abs(value) > 1.0e12)
        {
            Good = false;
            return false;
        }
        constexpr std::array<std::uint64_t, 7> powers{{
            1U, 10U, 100U, 1000U, 10000U, 100000U, 1000000U
        }};
        const std::uint64_t scale =
            powers[static_cast<std::size_t>(decimalPlaces)];
        const double scaled =
            std::round(std::abs(value) *
                       static_cast<double>(scale));
        if (!Finite(scaled) ||
            scaled >= static_cast<double>(
                std::numeric_limits<std::uint64_t>::max()))
        {
            Good = false;
            return false;
        }
        if (std::signbit(value) && scaled > 0.0 && !Append('-'))
        {
            return false;
        }
        const std::uint64_t integral =
            static_cast<std::uint64_t>(scaled);
        if (!AppendUnsigned(integral / scale))
        {
            return false;
        }
        if (decimalPlaces == 0)
        {
            return true;
        }
        if (!Append('.'))
        {
            return false;
        }
        std::uint64_t fraction = integral % scale;
        std::uint64_t divider = scale / 10U;
        for (int i = 0; i < decimalPlaces; ++i)
        {
            const std::uint64_t digit =
                divider > 0 ? fraction / divider : 0;
            if (!Append(static_cast<char>('0' + digit)))
            {
                return false;
            }
            if (divider > 0)
            {
                fraction %= divider;
                divider /= 10U;
            }
        }
        return true;
    }

    bool AppendBoolean(bool value) noexcept
    {
        return Append(value ? "true" : "false");
    }

    bool AppendCsvEscaped(std::string_view value) noexcept
    {
        bool quote = false;
        for (char c : value)
        {
            if (c == ',' || c == '\n' || c == '\r' ||
                c == '"')
            {
                quote = true;
            }
            if (static_cast<unsigned char>(c) < 32U &&
                c != '\n' && c != '\r' && c != '\t')
            {
                Good = false;
                return false;
            }
        }
        if (quote && !Append('"'))
        {
            return false;
        }
        for (char c : value)
        {
            if (c == '"' && !Append('"'))
            {
                return false;
            }
            if (!Append(c))
            {
                return false;
            }
        }
        return !quote || Append('"');
    }

    bool AppendJsonQuoted(std::string_view text) noexcept
    {
        if (!Append('"'))
        {
            return false;
        }
        for (char ch : text)
        {
            const unsigned char code =
                static_cast<unsigned char>(ch);
            if (code < 32U)
            {
                if (ch == '\n')
                {
                    if (!Append("\\n")) return false;
                }
                else if (ch == '\t')
                {
                    if (!Append("\\t")) return false;
                }
                else if (ch == '\r')
                {
                    if (!Append("\\r")) return false;
                }
                else
                {
                    Good = false;
                    return false;
                }
            }
            else if (ch == '"' || ch == '\\')
            {
                if (!Append('\\') || !Append(ch))
                {
                    return false;
                }
            }
            else if (!Append(ch))
            {
                return false;
            }
        }
        return Append('"');
    }

    bool Restore(int used) noexcept
    {
        if (!Buffer || used < 0 || used >= Capacity)
        {
            return false;
        }
        Used = used;
        Buffer[Used] = '\0';
        Good = true;
        return true;
    }

    int Size() const noexcept { return Used; }
    int Limit() const noexcept { return Capacity; }
    bool Ready() const noexcept { return Good; }
    const char* Data() const noexcept { return Buffer; }

private:
    char* Buffer = nullptr;
    int Capacity = 0;
    int Used = 0;
    bool Good = false;
};

class TelemetryExportSession final
{
public:
    void Reset() noexcept
    {
        Writer = {};
        Settings = {};
        Result = {};
        Started = false;
        Finished = false;
    }

    bool Begin(
        char* destination, int capacity,
        ExportFormat format,
        ExportSettings settings = {}) noexcept
    {
        if (!ValidExportSettings(settings) ||
            capacity > settings.MaximumBytes ||
            capacity < 128 ||
            !Writer.Begin(destination, capacity))
        {
            Result.Failure = ExportFailure::InvalidDestination;
            return false;
        }
        Settings = settings;
        Result = {};
        Result.Format = format;
        Result.Capacity = capacity;
        Result.RedactedIdentity = !settings.IncludeSessionIds;
        Started = true;
        Finished = false;
        bool header = false;
        switch (format)
        {
        case ExportFormat::CsvReplay:
            header = Writer.Append(
                "time_s,lap,sector,distance_cm,speed_kmh,throttle,brake,steer,slip_rad,on_road,native\n");
            break;
        case ExportFormat::CsvEvents:
            header = Writer.Append(
                "time_s,kind,channel,severity,slot,related_slot,magnitude,duration_s,native\n");
            break;
        case ExportFormat::CsvSectors:
            header = Writer.Append(
                "lap,sector,seconds,distance_cm,mean_kmh,brake_seconds,slip_seconds,offtrack_seconds,qualified\n");
            break;
        case ExportFormat::CsvLaps:
            header = Writer.Append(
                "lap,seconds,distance_cm,mean_kmh,top_kmh,brake_seconds,offtrack_seconds,slip_seconds,quality\n");
            break;
        case ExportFormat::JsonSummary:
            header = Writer.Append(
                "{\"schema\":1,\"producer\":\"latebrake-source-only\",\"native_gameplay_verified\":false,\"rows\":[");
            break;
        }
        if (!header)
        {
            Result.Failure = ExportFailure::InsufficientCapacity;
        }
        return header;
    }

    bool AddReplay(ReplaySample sample) noexcept
    {
        if (!AllowRow(ExportFormat::CsvReplay) ||
            !sample.Valid ||
            (Settings.RequireNativeSource &&
             !sample.NativeObserved))
        {
            ++Result.RowsRejected;
            return false;
        }
        const int restore = Writer.Size();
        const bool ok =
            Writer.AppendFixed(sample.MonotonicSeconds, 4) &&
            Writer.Append(',') &&
            Writer.AppendSigned(sample.LapIndex) &&
            Writer.Append(',') &&
            Writer.AppendSigned(sample.SectorIndex) &&
            Writer.Append(',') &&
            Writer.AppendFixed(sample.TrackDistanceCm, 2) &&
            Writer.Append(',') &&
            Writer.AppendFixed(
                sample.SpeedCmS * CmPerSecondToKmh, 3) &&
            Writer.Append(',') &&
            Writer.AppendFixed(sample.Throttle, 4) &&
            Writer.Append(',') &&
            Writer.AppendFixed(sample.Brake, 4) &&
            Writer.Append(',') &&
            Writer.AppendFixed(sample.Steering, 4) &&
            Writer.Append(',') &&
            Writer.AppendFixed(sample.SlipRadians, 5) &&
            Writer.Append(',') &&
            Writer.AppendBoolean(sample.OnPavement) &&
            Writer.Append(',') &&
            Writer.AppendBoolean(sample.NativeObserved) &&
            Writer.Append('\n');
        return CommitRow(ok, restore, sample.NativeObserved);
    }

    bool AddEvent(EventRecord event) noexcept
    {
        if (!AllowRow(ExportFormat::CsvEvents) ||
            !event.Accepted ||
            (Settings.RequireNativeSource &&
             !event.EngineObserved))
        {
            ++Result.RowsRejected;
            return false;
        }
        const int restore = Writer.Size();
        const bool ok =
            Writer.AppendFixed(event.Event.MonotonicSeconds, 4) &&
            Writer.Append(',') &&
            Writer.AppendSigned(
                static_cast<int>(event.Event.Kind)) &&
            Writer.Append(',') &&
            Writer.AppendSigned(static_cast<int>(event.Channel)) &&
            Writer.Append(',') &&
            Writer.AppendSigned(static_cast<int>(event.Severity)) &&
            Writer.Append(',') &&
            Writer.AppendSigned(event.Event.DriverSlot) &&
            Writer.Append(',') &&
            Writer.AppendSigned(event.Event.RelatedSlot) &&
            Writer.Append(',') &&
            Writer.AppendFixed(event.Event.Magnitude, 4) &&
            Writer.Append(',') &&
            Writer.AppendFixed(event.Event.DurationSeconds, 4) &&
            Writer.Append(',') &&
            Writer.AppendBoolean(event.EngineObserved) &&
            Writer.Append('\n');
        return CommitRow(ok, restore, event.EngineObserved);
    }

    bool AddSector(SectorPassage sector) noexcept
    {
        if (!AllowRow(ExportFormat::CsvSectors) ||
            !sector.Qualified)
        {
            ++Result.RowsRejected;
            return false;
        }
        const int restore = Writer.Size();
        const bool ok =
            Writer.AppendSigned(sector.LapIndex) &&
            Writer.Append(',') &&
            Writer.AppendSigned(sector.SectorIndex) &&
            Writer.Append(',') &&
            Writer.AppendFixed(sector.DurationSeconds, 4) &&
            Writer.Append(',') &&
            Writer.AppendFixed(sector.DrivenDistanceCm, 2) &&
            Writer.Append(',') &&
            Writer.AppendFixed(
                sector.MeanSpeedCmS * CmPerSecondToKmh, 3) &&
            Writer.Append(',') &&
            Writer.AppendFixed(sector.BrakingSeconds, 3) &&
            Writer.Append(',') &&
            Writer.AppendFixed(sector.SlipSeconds, 3) &&
            Writer.Append(',') &&
            Writer.AppendFixed(sector.OffTrackSeconds, 3) &&
            Writer.Append(',') &&
            Writer.AppendBoolean(sector.VerifiedForwardGate) &&
            Writer.Append('\n');
        return CommitRow(ok, restore, sector.VerifiedForwardGate);
    }

    bool AddLap(TelemetryLap lap) noexcept
    {
        if (!AllowRow(ExportFormat::CsvLaps) ||
            lap.Quality != TelemetryQuality::Qualified ||
            !Finite(lap.DurationSeconds) ||
            lap.DurationSeconds <= 0.0)
        {
            ++Result.RowsRejected;
            return false;
        }
        const int restore = Writer.Size();
        const bool ok =
            Writer.AppendSigned(lap.LapIndex) &&
            Writer.Append(',') &&
            Writer.AppendFixed(lap.DurationSeconds, 4) &&
            Writer.Append(',') &&
            Writer.AppendFixed(lap.DistanceCm, 2) &&
            Writer.Append(',') &&
            Writer.AppendFixed(lap.MeanSpeedKmh, 3) &&
            Writer.Append(',') &&
            Writer.AppendFixed(lap.TopSpeedKmh, 3) &&
            Writer.Append(',') &&
            Writer.AppendFixed(lap.TotalBrakingSeconds, 3) &&
            Writer.Append(',') &&
            Writer.AppendFixed(lap.TotalOffTrackSeconds, 3) &&
            Writer.Append(',') &&
            Writer.AppendFixed(lap.TotalHighSlipSeconds, 3) &&
            Writer.Append(',') &&
            Writer.AppendSigned(static_cast<int>(lap.Quality)) &&
            Writer.Append('\n');
        return CommitRow(ok, restore, true);
    }

    bool AddJsonMetric(
        std::string_view key, double value,
        bool nativeObserved) noexcept
    {
        if (!AllowRow(ExportFormat::JsonSummary) ||
            !Finite(value) || key.empty() || key.size() > 64U ||
            (Settings.RequireNativeSource && !nativeObserved))
        {
            ++Result.RowsRejected;
            return false;
        }
        const int restore = Writer.Size();
        const bool ok =
            (Result.RowsWritten == 0 || Writer.Append(',')) &&
            Writer.Append("{\"metric\":") &&
            Writer.AppendJsonQuoted(key) &&
            Writer.Append(",\"value\":") &&
            Writer.AppendFixed(value, Settings.DecimalPlaces) &&
            Writer.Append(",\"native\":") &&
            Writer.AppendBoolean(nativeObserved) &&
            Writer.Append('}');
        return CommitRow(ok, restore, nativeObserved);
    }

    ExportResult Finish() noexcept
    {
        if (!Started || Finished)
        {
            Result.Failure = Started ?
                ExportFailure::AlreadyFinished :
                ExportFailure::NotStarted;
            return Result;
        }
        if (Result.Format == ExportFormat::JsonSummary &&
            !Writer.Append("]}"))
        {
            Result.Failure = ExportFailure::InsufficientCapacity;
        }
        Finished = true;
        Result.BytesWritten = Writer.Size();
        Result.Finalized = Writer.Ready();
        Result.Qualified = Result.Finalized &&
            Result.RowsWritten > 0 &&
            Result.HadNativeObservations;
        return Result;
    }

    const char* Text() const noexcept { return Writer.Data(); }
    int SizeBytes() const noexcept { return Writer.Size(); }
    ExportResult Status() const noexcept { return Result; }

private:
    bool AllowRow(ExportFormat format) noexcept
    {
        if (!Started || Finished ||
            Result.Format != format ||
            !Writer.Ready())
        {
            Result.Failure = !Started ?
                ExportFailure::NotStarted :
                Finished ? ExportFailure::AlreadyFinished :
                ExportFailure::UnsupportedFormat;
            return false;
        }
        if (Result.RowsWritten >= Settings.MaximumRows)
        {
            Result.Failure = ExportFailure::TooManyRows;
            return false;
        }
        return true;
    }

    bool CommitRow(
        bool ok, int restore,
        bool native) noexcept
    {
        if (!ok)
        {
            Writer.Restore(restore);
            Result.Failure = ExportFailure::InsufficientCapacity;
            ++Result.RowsRejected;
            return false;
        }
        ++Result.RowsWritten;
        Result.BytesWritten = Writer.Size();
        Result.HadNativeObservations |= native;
        return true;
    }

    BoundedTextWriter Writer;
    ExportSettings Settings{};
    ExportResult Result{};
    bool Started = false;
    bool Finished = false;
};
} // namespace latebrake::engineering
