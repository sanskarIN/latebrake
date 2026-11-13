#pragma once
#include "Racecraft/PhysicalSixCarRaceTrace.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace latebrake::racecraft
{
// A replay archive is a presentation artifact; bytes imported from disk
// are NEVER authenticated Chaos physics or authoritative race telemetry.
inline constexpr std::uint32_t PhysicalReplayMagic=0x5042524cU;
inline constexpr std::uint16_t PhysicalReplayVersion=1;
inline constexpr std::size_t PhysicalReplayMaxBytes=128*1024;
inline constexpr double PhysicalReplayPositionScale=16.0;
inline constexpr double PhysicalReplayVelocityScale=16.0;
inline constexpr double PhysicalReplayAngleScale=1000000.0;
inline constexpr double PhysicalReplayConfidenceScale=10000.0;
inline constexpr double PhysicalReplayClockScale=1000000.0;
enum class PhysicalReplayArchiveFault : std::uint8_t
{
    None, Empty, InvalidSource, InvalidMagic, InvalidVersion,
    UnsupportedHeader, TooLong, Truncated, ChecksumMismatch,
    InvalidIdentity, NonFiniteValue, OutOfRangeValue,
    TemporalDiscontinuity, TrailingBytes
};
struct PhysicalReplayActor final
{
    std::uint64_t Actor=0;
    Vec2 PositionCm{};
    Vec2 VelocityCmS{};
    double HeadingRadians=0.0;
    double TrackDistanceCm=0.0;
    double TrackLateralOffsetCm=0.0;
    double HalfLengthCm=0.0;
    double HalfWidthCm=0.0;
    double SourceConfidence=0.0;
};
struct PhysicalReplayRecord final
{
    std::array<PhysicalReplayActor,PhysicalFleetCapacity> Actors{};
    std::uint64_t SourceFrame=0;
    double SourceClockSeconds=0.0;
    double OriginalThrottle=0.0;
    double OriginalBrake=0.0;
    double OriginalSteering=0.0;
    bool EncodedWithoutEngineActuation=true;
    bool Valid=false;
};
struct PhysicalReplayReadOnlyView final
{
    std::array<PhysicalReplayActor,PhysicalFleetCapacity> Actors{};
    double RequestedSeconds=0.0;
    std::uint64_t EarlierFrame=0;
    std::uint64_t LaterFrame=0;
    double Fraction=0.0;
    bool ImportIsUntrusted=true;
    bool MayDriveChaosBody=false;
    bool MayCreditRaceResults=false;
    bool Valid=false;
};
class PhysicalReplayByteWriter final
{
public:
    bool WriteU8(std::uint8_t v)
    {
        if(Bytes.size()>=PhysicalReplayMaxBytes)return false;
        Bytes.push_back(v);
        return true;
    }
    bool WriteU16(std::uint16_t v)
    {
        return WriteU8(static_cast<std::uint8_t>(v)) &&
            WriteU8(static_cast<std::uint8_t>(v>>8));
    }
    bool WriteU32(std::uint32_t v)
    {
        return WriteU16(static_cast<std::uint16_t>(v)) &&
            WriteU16(static_cast<std::uint16_t>(v>>16));
    }
    bool WriteU64(std::uint64_t v)
    {
        return WriteU32(static_cast<std::uint32_t>(v)) &&
            WriteU32(static_cast<std::uint32_t>(v>>32));
    }
    bool WriteI32(std::int32_t v)
    {
        return WriteU32(std::bit_cast<std::uint32_t>(v));
    }
    bool WriteFixed(double value,double scale)
    {
        if (!Finite(value) || !Finite(scale) || scale<=0.0)
            return false;
        const double fixed=std::round(value*scale);
        if(!Finite(fixed) ||
           fixed<static_cast<double>(
               std::numeric_limits<std::int32_t>::min()) ||
           fixed>static_cast<double>(
               std::numeric_limits<std::int32_t>::max()))
            return false;
        return WriteI32(static_cast<std::int32_t>(fixed));
    }
    bool WriteClock(double seconds)
    {
        if(!Finite(seconds) || seconds<0.0 ||
           seconds>static_cast<double>(
               std::numeric_limits<std::uint64_t>::max())/
                   PhysicalReplayClockScale)
            return false;
        return WriteU64(static_cast<std::uint64_t>(
            std::round(seconds*PhysicalReplayClockScale)));
    }
    std::span<const std::uint8_t> Data() const noexcept
    {return Bytes;}
    const std::vector<std::uint8_t>& Copy() const noexcept
    {return Bytes;}
    void Clear() noexcept {Bytes.clear();}
private:
    std::vector<std::uint8_t> Bytes{};
};
class PhysicalReplayByteReader final
{
public:
    explicit PhysicalReplayByteReader(
        std::span<const std::uint8_t> bytes) noexcept:Bytes(bytes){}
    bool ReadU8(std::uint8_t& out) noexcept
    {
        if(Cursor>=Bytes.size())return false;
        out=Bytes[Cursor++];
        return true;
    }
    bool ReadU16(std::uint16_t& out) noexcept
    {
        std::uint8_t a=0,b=0;
        if(!ReadU8(a) || !ReadU8(b))return false;
        out=static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(a) |
            static_cast<std::uint16_t>(b)<<8);
        return true;
    }
    bool ReadU32(std::uint32_t& out) noexcept
    {
        std::uint16_t a=0,b=0;
        if(!ReadU16(a) || !ReadU16(b))return false;
        out=static_cast<std::uint32_t>(a) |
            static_cast<std::uint32_t>(b)<<16;
        return true;
    }
    bool ReadU64(std::uint64_t& out) noexcept
    {
        std::uint32_t a=0,b=0;
        if(!ReadU32(a) || !ReadU32(b))return false;
        out=static_cast<std::uint64_t>(a) |
            static_cast<std::uint64_t>(b)<<32;
        return true;
    }
    bool ReadI32(std::int32_t& out) noexcept
    {
        std::uint32_t data=0;
        if(!ReadU32(data))return false;
        out=std::bit_cast<std::int32_t>(data);
        return true;
    }
    bool ReadFixed(double& out,double scale) noexcept
    {
        std::int32_t value=0;
        if(!Finite(scale) || scale<=0.0 || !ReadI32(value))
            return false;
        out=static_cast<double>(value)/scale;
        return Finite(out);
    }
    bool ReadClock(double& out) noexcept
    {
        std::uint64_t encoded=0;
        if(!ReadU64(encoded))return false;
        out=static_cast<double>(encoded)/PhysicalReplayClockScale;
        return Finite(out);
    }
    std::size_t Remaining() const noexcept
    {return Bytes.size()-Cursor;}
    std::size_t Offset() const noexcept{return Cursor;}
private:
    std::span<const std::uint8_t> Bytes;
    std::size_t Cursor=0;
};
inline std::uint32_t PhysicalReplayChecksum(
    std::span<const std::uint8_t> bytes) noexcept
{
    std::uint32_t crc=0xffffffffU;
    for(const std::uint8_t byte:bytes)
    {
        crc^=byte;
        for(int bit=0;bit<8;++bit)
            crc=(crc>>1)^((crc&1U)?0xedb88320U:0U);
    }
    return ~crc;
}
inline bool ValidArchivedPhysicalReplayActor(
    const PhysicalReplayActor& actor) noexcept
{
    return actor.Actor!=0 && Finite(actor.PositionCm) &&
        Finite(actor.VelocityCmS) &&
        Finite(actor.HeadingRadians) &&
        std::abs(actor.HeadingRadians)<=Pi &&
        Finite(actor.TrackDistanceCm) &&
        Finite(actor.TrackLateralOffsetCm) &&
        Finite(actor.HalfLengthCm) &&
        actor.HalfLengthCm>=50.0 &&
        actor.HalfLengthCm<=600.0 &&
        Finite(actor.HalfWidthCm) &&
        actor.HalfWidthCm>=30.0 &&
        actor.HalfWidthCm<=300.0 &&
        Finite(actor.SourceConfidence) &&
        actor.SourceConfidence>=0.8 &&
        actor.SourceConfidence<=1.0 &&
        Length(actor.VelocityCmS)<=MaximumPhysicalSpeedCmS;
}
inline bool EncodePhysicalReplayActor(
    PhysicalReplayByteWriter& writer,
    const PhysicalTraceActor& actor)
{
    if(!actor.OriginalCollisionBody ||
       !PhysicalActorIdentityValid(actor.Identity))
        return false;
    return writer.WriteU64(actor.Identity.Actor) &&
        writer.WriteFixed(actor.Pose.Position.X,
            PhysicalReplayPositionScale) &&
        writer.WriteFixed(actor.Pose.Position.Y,
            PhysicalReplayPositionScale) &&
        writer.WriteFixed(actor.VelocityCmS.X,
            PhysicalReplayVelocityScale) &&
        writer.WriteFixed(actor.VelocityCmS.Y,
            PhysicalReplayVelocityScale) &&
        writer.WriteFixed(WrappedAngle(
            actor.Pose.HeadingRadians),PhysicalReplayAngleScale) &&
        writer.WriteFixed(actor.TrackDistanceCm,
            PhysicalReplayPositionScale) &&
        writer.WriteFixed(actor.TrackLateralOffsetCm,
            PhysicalReplayPositionScale) &&
        writer.WriteFixed(actor.HalfLengthCm,
            PhysicalReplayPositionScale) &&
        writer.WriteFixed(actor.HalfWidthCm,
            PhysicalReplayPositionScale) &&
        writer.WriteFixed(actor.SensorConfidence,
            PhysicalReplayConfidenceScale);
}
inline bool DecodePhysicalReplayActor(
    PhysicalReplayByteReader& reader,
    PhysicalReplayActor& actor) noexcept
{
    if(!reader.ReadU64(actor.Actor) ||
       !reader.ReadFixed(actor.PositionCm.X,
           PhysicalReplayPositionScale) ||
       !reader.ReadFixed(actor.PositionCm.Y,
           PhysicalReplayPositionScale) ||
       !reader.ReadFixed(actor.VelocityCmS.X,
           PhysicalReplayVelocityScale) ||
       !reader.ReadFixed(actor.VelocityCmS.Y,
           PhysicalReplayVelocityScale) ||
       !reader.ReadFixed(actor.HeadingRadians,
           PhysicalReplayAngleScale) ||
       !reader.ReadFixed(actor.TrackDistanceCm,
           PhysicalReplayPositionScale) ||
       !reader.ReadFixed(actor.TrackLateralOffsetCm,
           PhysicalReplayPositionScale) ||
       !reader.ReadFixed(actor.HalfLengthCm,
           PhysicalReplayPositionScale) ||
       !reader.ReadFixed(actor.HalfWidthCm,
           PhysicalReplayPositionScale) ||
       !reader.ReadFixed(actor.SourceConfidence,
           PhysicalReplayConfidenceScale))
        return false;
    return ValidArchivedPhysicalReplayActor(actor);
}
struct PhysicalReplayArchiveBytes final
{
    std::vector<std::uint8_t> Bytes{};
    PhysicalReplayArchiveFault Fault=PhysicalReplayArchiveFault::Empty;
    std::uint32_t Checksum=0;
    std::uint16_t Version=0;
    std::size_t RecordedFrames=0;
    bool Valid=false;
};
inline PhysicalReplayArchiveBytes ExportPhysicalReplayArchive(
    const PhysicalSixCarRaceTrace& trace)
{
    PhysicalReplayArchiveBytes out;
    if(trace.Size()<2 || trace.Size()>PhysicalTraceCapacity ||
       !trace.ReviewInterval().Valid)
    {
        out.Fault=PhysicalReplayArchiveFault::InvalidSource;
        return out;
    }
    const auto* first=trace.OldestFirst(0);
    if(!first || !first->Valid)return out;
    PhysicalReplayByteWriter writer;
    bool ok=writer.WriteU32(PhysicalReplayMagic) &&
        writer.WriteU16(PhysicalReplayVersion) &&
        writer.WriteU16(PhysicalFleetCapacity) &&
        writer.WriteU32(static_cast<std::uint32_t>(trace.Size())) &&
        writer.WriteU64(first->Identity.Session) &&
        writer.WriteU32(first->Identity.SessionRevision) &&
        writer.WriteU8(static_cast<std::uint8_t>(
            first->Identity.Slot));
    for(const auto actor:first->ActorRoster)
        ok=ok && writer.WriteU64(actor);
    for(std::size_t i=0;i<trace.Size() && ok;++i)
    {
        const auto* frame=trace.OldestFirst(i);
        if(!frame || !frame->Valid ||
           !PhysicalTraceSameRoster(*first,*frame))
        {
            ok=false;
            break;
        }
        ok=writer.WriteU64(frame->Identity.Frame) &&
            writer.WriteClock(frame->PhysicsClockSeconds) &&
            writer.WriteFixed(frame->OriginalThrottle,10000.0) &&
            writer.WriteFixed(frame->OriginalBrake,10000.0) &&
            writer.WriteFixed(frame->OriginalSteering,10000.0);
        for(const auto& actor:frame->Actors)
            ok=ok && EncodePhysicalReplayActor(writer,actor);
    }
    if(!ok || writer.Data().size()>PhysicalReplayMaxBytes-4)
    {
        out.Fault=PhysicalReplayArchiveFault::TooLong;
        return out;
    }
    out.Checksum=PhysicalReplayChecksum(writer.Data());
    if(!writer.WriteU32(out.Checksum))
    {
        out.Fault=PhysicalReplayArchiveFault::TooLong;
        return out;
    }
    out.Bytes=writer.Copy();
    out.Version=PhysicalReplayVersion;
    out.RecordedFrames=trace.Size();
    out.Fault=PhysicalReplayArchiveFault::None;
    out.Valid=true;
    return out;
}
class PhysicalReplayArchiveReader final
{
public:
    PhysicalReplayArchiveFault Import(
        std::span<const std::uint8_t> bytes) noexcept
    {
        Clear();
        if(bytes.empty())
            return Fail(PhysicalReplayArchiveFault::Empty);
        if(bytes.size()>PhysicalReplayMaxBytes)
            return Fail(PhysicalReplayArchiveFault::TooLong);
        if(bytes.size()<4+2+2+4+8+4+1+6*8+4)
            return Fail(PhysicalReplayArchiveFault::Truncated);
        PhysicalReplayByteReader payload(bytes.subspan(
            0,bytes.size()-4));
        PhysicalReplayByteReader tail(bytes.last(4));
        std::uint32_t storedChecksum=0;
        if(!tail.ReadU32(storedChecksum))
            return Fail(PhysicalReplayArchiveFault::Truncated);
        if(PhysicalReplayChecksum(bytes.first(bytes.size()-4))!=
           storedChecksum)
            return Fail(PhysicalReplayArchiveFault::ChecksumMismatch);
        std::uint32_t magic=0,count=0,revision=0;
        std::uint16_t version=0,fleet=0;
        std::uint64_t session=0;
        std::uint8_t selfSlot=255;
        if(!payload.ReadU32(magic))
            return Fail(PhysicalReplayArchiveFault::Truncated);
        if(magic!=PhysicalReplayMagic)
            return Fail(PhysicalReplayArchiveFault::InvalidMagic);
        if(!payload.ReadU16(version))
            return Fail(PhysicalReplayArchiveFault::Truncated);
        if(version!=PhysicalReplayVersion)
            return Fail(PhysicalReplayArchiveFault::InvalidVersion);
        if(!payload.ReadU16(fleet) ||
           !payload.ReadU32(count) ||
           !payload.ReadU64(session) ||
           !payload.ReadU32(revision) ||
           !payload.ReadU8(selfSlot))
            return Fail(PhysicalReplayArchiveFault::Truncated);
        if(fleet!=PhysicalFleetCapacity ||
           count<2 || count>PhysicalTraceCapacity ||
           session==0 || selfSlot>=PhysicalFleetCapacity)
            return Fail(PhysicalReplayArchiveFault::UnsupportedHeader);
        std::array<std::uint64_t,PhysicalFleetCapacity> roster{};
        for(auto& actor:roster)
            if(!payload.ReadU64(actor) || actor==0)
                return Fail(PhysicalReplayArchiveFault::InvalidIdentity);
        for(int i=0;i<PhysicalFleetCapacity;++i)
            for(int j=i+1;j<PhysicalFleetCapacity;++j)
                if(roster[static_cast<std::size_t>(i)]==
                   roster[static_cast<std::size_t>(j)])
                    return Fail(PhysicalReplayArchiveFault::InvalidIdentity);
        std::uint64_t previousFrame=0;
        double previousClock=0.0;
        for(std::uint32_t i=0;i<count;++i)
        {
            PhysicalReplayRecord record;
            if(!payload.ReadU64(record.SourceFrame) ||
               !payload.ReadClock(record.SourceClockSeconds) ||
               !payload.ReadFixed(record.OriginalThrottle,10000.0) ||
               !payload.ReadFixed(record.OriginalBrake,10000.0) ||
               !payload.ReadFixed(record.OriginalSteering,10000.0))
                return Fail(PhysicalReplayArchiveFault::Truncated);
            if(record.SourceFrame==0 ||
               record.OriginalThrottle<0.0 ||
               record.OriginalThrottle>1.0 ||
               record.OriginalBrake<0.0 ||
               record.OriginalBrake>1.0 ||
               std::abs(record.OriginalSteering)>1.0)
                return Fail(PhysicalReplayArchiveFault::OutOfRangeValue);
            for(std::size_t slot=0;slot<record.Actors.size();++slot)
            {
                if(!DecodePhysicalReplayActor(payload,
                       record.Actors[slot]))
                    return Fail(PhysicalReplayArchiveFault::Truncated);
                if(record.Actors[slot].Actor!=roster[slot])
                    return Fail(PhysicalReplayArchiveFault::InvalidIdentity);
            }
            if(i>0)
            {
                const double dt=record.SourceClockSeconds-
                    previousClock;
                if(record.SourceFrame!=previousFrame+1 ||
                   dt<=0.0 || dt>MaximumFrameTimeSeconds+0.00001)
                    return Fail(
                        PhysicalReplayArchiveFault::TemporalDiscontinuity);
                for(std::size_t slot=0;slot<record.Actors.size();++slot)
                {
                    const auto& old=Records[i-1].Actors[slot];
                    const auto& newer=record.Actors[slot];
                    if(Distance(old.PositionCm,newer.PositionCm)>
                        MaximumPhysicalSpeedCmS*dt+200.0)
                        return Fail(
                            PhysicalReplayArchiveFault::TemporalDiscontinuity);
                }
            }
            record.Valid=true;
            Records[i]=record;
            previousFrame=record.SourceFrame;
            previousClock=record.SourceClockSeconds;
            ++Count;
        }
        if(payload.Remaining()!=0)
            return Fail(PhysicalReplayArchiveFault::TrailingBytes);
        Session=session;
        Revision=revision;
        SelfSlot=selfSlot;
        Roster=roster;
        ImportedChecksum=storedChecksum;
        Fault=PhysicalReplayArchiveFault::None;
        return Fault;
    }
    void Clear() noexcept
    {
        Count=0;
        Session=0;
        Revision=0;
        SelfSlot=255;
        Roster.fill(0);
        ImportedChecksum=0;
        Fault=PhysicalReplayArchiveFault::Empty;
    }
    PhysicalReplayReadOnlyView Preview(double clockSeconds) const noexcept
    {
        PhysicalReplayReadOnlyView out;
        if(Fault!=PhysicalReplayArchiveFault::None ||
           Count<2 || !Finite(clockSeconds))
            return out;
        for(std::size_t i=1;i<Count;++i)
        {
            const auto& a=Records[i-1];
            const auto& b=Records[i];
            if(clockSeconds<a.SourceClockSeconds ||
               clockSeconds>b.SourceClockSeconds)
                continue;
            const double dt=b.SourceClockSeconds-
                a.SourceClockSeconds;
            if(dt<=0.0 || dt>MaximumFrameTimeSeconds)
                return out;
            const double fraction=(clockSeconds-
                a.SourceClockSeconds)/dt;
            for(std::size_t slot=0;slot<out.Actors.size();++slot)
            {
                const auto& from=a.Actors[slot];
                const auto& to=b.Actors[slot];
                auto actor=from;
                actor.PositionCm=Lerp(from.PositionCm,
                    to.PositionCm,fraction);
                actor.VelocityCmS=Lerp(from.VelocityCmS,
                    to.VelocityCmS,fraction);
                actor.HeadingRadians=BlendAngle(
                    from.HeadingRadians,to.HeadingRadians,fraction);
                actor.TrackDistanceCm=Lerp(
                    from.TrackDistanceCm,to.TrackDistanceCm,fraction);
                actor.TrackLateralOffsetCm=Lerp(
                    from.TrackLateralOffsetCm,
                    to.TrackLateralOffsetCm,fraction);
                actor.SourceConfidence=std::min(
                    from.SourceConfidence,to.SourceConfidence);
                out.Actors[slot]=actor;
            }
            out.RequestedSeconds=clockSeconds;
            out.EarlierFrame=a.SourceFrame;
            out.LaterFrame=b.SourceFrame;
            out.Fraction=fraction;
            out.Valid=true;
            return out;
        }
        return out;
    }
    std::size_t Size() const noexcept{return Count;}
    std::uint64_t SourceSession() const noexcept{return Session;}
    std::uint32_t SourceRevision() const noexcept{return Revision;}
    std::uint8_t SourceSlot() const noexcept{return SelfSlot;}
    std::uint32_t Checksum() const noexcept{return ImportedChecksum;}
    PhysicalReplayArchiveFault LastFault() const noexcept{return Fault;}
    const PhysicalReplayRecord* Get(std::size_t index) const noexcept
    {return index<Count?&Records[index]:nullptr;}
private:
    PhysicalReplayArchiveFault Fail(
        PhysicalReplayArchiveFault fault) noexcept
    {
        Clear();
        Fault=fault;
        return Fault;
    }
    std::array<PhysicalReplayRecord,PhysicalTraceCapacity> Records{};
    std::array<std::uint64_t,PhysicalFleetCapacity> Roster{};
    std::uint64_t Session=0;
    std::uint32_t Revision=0;
    std::uint8_t SelfSlot=255;
    std::uint32_t ImportedChecksum=0;
    std::size_t Count=0;
    PhysicalReplayArchiveFault Fault=PhysicalReplayArchiveFault::Empty;
};
} // namespace latebrake::racecraft