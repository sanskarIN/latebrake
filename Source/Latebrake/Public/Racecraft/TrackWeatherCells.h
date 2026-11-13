#pragma once
#include "Racecraft/CircuitGeometry.h"
#include "Racecraft/SurfaceMaterialModel.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
inline constexpr int WeatherCellCapacity=16;
struct WeatherCellParameters final
{
    double MaximumSensorAgeSeconds=15.0;
    double MaximumMeasuredRainMmHour=150.0;
    double MaximumMeasuredWaterMm=35.0;
    double MaximumTrackedWindCmS=5000.0;
    double MaximumTemperatureC=90.0;
    double MinimumTemperatureC=-35.0;
    double MaximumTrackTemperatureC=120.0;
    double MinimumTrackTemperatureC=-40.0;
    double MaximumHumidityFraction=1.0;
    double MinimumFrictionRatio=0.10;
    double MaximumFrictionRatio=2.0;
    double ObservationTimeToleranceSeconds=0.2;
    double MinimumConfidence=0.35;
    double MinimumConsecutiveUpdates=2.0;
    double TrackLengthCm=OriginalCircuitLapCm;
    double SpatialSmoothingDistanceCm=600.0;
    double MaximumSpatialChangePerSecond=0.20;
    double MaximumRainIncreasePerSecond=20.0;
    double MaximumWaterRiseMmPerSecond=1.0;
    double SurfaceDryingHalfLifeSeconds=180.0;
    double MaximumValidFrameSeconds=0.10;
    bool RequireActualTrackSensors=true;
    bool RequireOriginalMap=true;
    bool NeverInventMissingCellData=true;
};
struct WeatherCellObservation final
{
    std::uint64_t Session=0;
    std::uint64_t SensorToken=0;
    std::uint64_t FrameOrdinal=0;
    int CellIndex=-1;
    double CellDistanceCm=0.0;
    double ActualClockSeconds=0.0;
    double AmbientTemperatureC=20.0;
    double SurfaceTemperatureC=35.0;
    double RelativeHumidity=0.4;
    double RainRateMmHour=0.0;
    double WaterDepthMm=0.0;
    double MeasuredRoadFriction=1.0;
    double WindSpeedCmS=0.0;
    double WindDirectionRadians=0.0;
    double VisibilityFraction=1.0;
    bool OriginalTrackSensor=true;
    bool PhysicalWetnessProbe=true;
    bool ActualFrictionSensor=true;
    bool EngineEnvironmentQualified=true;
};
struct WeatherCellState final
{
    WeatherCellObservation Observation{};
    double LastUpdateClockSeconds=0.0;
    double EstimatedWetnessFraction=0.0;
    double FilteredFriction=0.0;
    double FilteredRainMmHour=0.0;
    double WaterRiseRateMmSecond=0.0;
    double WetnessChangePerSecond=0.0;
    double Confidence=0.0;
    int QualifiedUpdates=0;
    int RejectedUpdates=0;
    bool SensorPresent=false;
    bool MeasurementFresh=false;
    bool FullyQualified=false;
};
struct WeatherAlongTrack final
{
    SurfaceWeather Weather{};
    double TrackDistanceCm=0.0;
    double AmbientC=0.0;
    double RoadTemperatureC=0.0;
    double MeasuredFrictionCoefficient=0.0;
    double EffectiveWetnessFraction=0.0;
    double MeasuredWaterDepthMm=0.0;
    double ExpectedHydroplaningRisk=0.0;
    double SensorAgeSeconds=0.0;
    double SpatialInterpolationFraction=0.0;
    double SourceConfidence=0.0;
    int FirstSensorIndex=-1;
    int SecondSensorIndex=-1;
    int VerifiedSensorsUsed=0;
    bool ActualSourceQualified=false;
    bool OriginalRoadProven=false;
    bool AuthoritativeWeatherChanged=false;
    bool Valid=false;
};
inline bool ValidWeatherCellParameters(
    WeatherCellParameters p) noexcept
{
    return Finite(p.MaximumSensorAgeSeconds) &&
        p.MaximumSensorAgeSeconds>0.0 &&
        Finite(p.MaximumMeasuredRainMmHour) &&
        p.MaximumMeasuredRainMmHour>0.0 &&
        Finite(p.MaximumMeasuredWaterMm) &&
        p.MaximumMeasuredWaterMm>0.0 &&
        Finite(p.MaximumTrackedWindCmS) &&
        p.MaximumTrackedWindCmS>0.0 &&
        Finite(p.MaximumTemperatureC) &&
        p.MaximumTemperatureC>p.MinimumTemperatureC &&
        Finite(p.MaximumTrackTemperatureC) &&
        p.MaximumTrackTemperatureC>
            p.MinimumTrackTemperatureC &&
        Finite(p.MaximumHumidityFraction) &&
        p.MaximumHumidityFraction>0.0 &&
        Finite(p.MinimumFrictionRatio) &&
        p.MinimumFrictionRatio>0.0 &&
        Finite(p.MaximumFrictionRatio) &&
        p.MaximumFrictionRatio>p.MinimumFrictionRatio &&
        Finite(p.MinimumConfidence) &&
        p.MinimumConfidence>0.0 &&
        p.MinimumConfidence<=1.0 &&
        Finite(p.TrackLengthCm) &&
        p.TrackLengthCm>0.0 &&
        Finite(p.SpatialSmoothingDistanceCm) &&
        p.SpatialSmoothingDistanceCm>0.0 &&
        Finite(p.SurfaceDryingHalfLifeSeconds) &&
        p.SurfaceDryingHalfLifeSeconds>0.0 &&
        Finite(p.MaximumValidFrameSeconds) &&
        p.MaximumValidFrameSeconds>0.0 &&
        p.MaximumValidFrameSeconds<=MaxSimulationFrameSeconds;
}
inline bool ValidWeatherCellObservation(
    WeatherCellObservation o,
    WeatherCellParameters p) noexcept
{
    return o.Session!=0 &&
        o.SensorToken!=0 &&
        o.OriginalTrackSensor &&
        o.PhysicalWetnessProbe &&
        o.ActualFrictionSensor &&
        o.EngineEnvironmentQualified &&
        o.CellIndex>=0 &&
        o.CellIndex<WeatherCellCapacity &&
        Finite(o.CellDistanceCm) &&
        o.CellDistanceCm>=0.0 &&
        o.CellDistanceCm<p.TrackLengthCm &&
        Finite(o.ActualClockSeconds) &&
        o.ActualClockSeconds>=0.0 &&
        Finite(o.AmbientTemperatureC) &&
        o.AmbientTemperatureC>=p.MinimumTemperatureC &&
        o.AmbientTemperatureC<=p.MaximumTemperatureC &&
        Finite(o.SurfaceTemperatureC) &&
        o.SurfaceTemperatureC>=p.MinimumTrackTemperatureC &&
        o.SurfaceTemperatureC<=p.MaximumTrackTemperatureC &&
        Finite(o.RelativeHumidity) &&
        o.RelativeHumidity>=0.0 &&
        o.RelativeHumidity<=p.MaximumHumidityFraction &&
        Finite(o.RainRateMmHour) &&
        o.RainRateMmHour>=0.0 &&
        o.RainRateMmHour<=p.MaximumMeasuredRainMmHour &&
        Finite(o.WaterDepthMm) &&
        o.WaterDepthMm>=0.0 &&
        o.WaterDepthMm<=p.MaximumMeasuredWaterMm &&
        Finite(o.MeasuredRoadFriction) &&
        o.MeasuredRoadFriction>=p.MinimumFrictionRatio &&
        o.MeasuredRoadFriction<=p.MaximumFrictionRatio &&
        Finite(o.WindSpeedCmS) &&
        o.WindSpeedCmS>=0.0 &&
        o.WindSpeedCmS<=p.MaximumTrackedWindCmS &&
        Finite(o.WindDirectionRadians) &&
        Finite(o.VisibilityFraction) &&
        o.VisibilityFraction>=0.0 &&
        o.VisibilityFraction<=1.0;
}
inline double WetnessFromMeasuredWater(
    double millimeters,double rainMmHour,
    double roadC) noexcept
{
    if (!Finite(millimeters) ||
        !Finite(rainMmHour) ||
        !Finite(roadC))
        return 0.0;
    const double depth=Smoothstep(
        0.0,4.0,millimeters);
    const double falling=Smoothstep(
        0.0,35.0,rainMmHour);
    const double evaporation=
        Smoothstep(30.0,95.0,roadC);
    return Clamp01(
        0.65*depth+0.35*falling-0.12*evaporation);
}
inline WeatherCellState UpdateWeatherCell(
    WeatherCellState old,WeatherCellObservation now,
    WeatherCellParameters p={}) noexcept
{
    if (!ValidWeatherCellParameters(p) ||
        !ValidWeatherCellObservation(now,p))
    {
        ++old.RejectedUpdates;
        return old;
    }
    if (old.SensorPresent &&
        (now.SensorToken!=old.Observation.SensorToken ||
         now.FrameOrdinal<=old.Observation.FrameOrdinal ||
         now.ActualClockSeconds<=
            old.Observation.ActualClockSeconds))
    {
        ++old.RejectedUpdates;
        old.FullyQualified=false;
        return old;
    }
    const double wet=WetnessFromMeasuredWater(
        now.WaterDepthMm,now.RainRateMmHour,
        now.SurfaceTemperatureC);
    const double dt=old.SensorPresent ?
        now.ActualClockSeconds-
        old.Observation.ActualClockSeconds:0.0;
    old.WaterRiseRateMmSecond=dt>0.0 ?
        SafeDivision(
            now.WaterDepthMm-
            old.Observation.WaterDepthMm,dt):0.0;
    old.WetnessChangePerSecond=dt>0.0 ?
        SafeDivision(
            wet-old.EstimatedWetnessFraction,dt):0.0;
    old.FilteredFriction=old.SensorPresent ?
        ExponentialResponse(
            old.FilteredFriction,
            now.MeasuredRoadFriction,0.2,
            std::min(dt,0.1)):
        now.MeasuredRoadFriction;
    old.FilteredRainMmHour=old.SensorPresent ?
        ExponentialResponse(
            old.FilteredRainMmHour,
            now.RainRateMmHour,0.2,
            std::min(dt,0.1)):
        now.RainRateMmHour;
    old.EstimatedWetnessFraction=wet;
    old.LastUpdateClockSeconds=
        now.ActualClockSeconds;
    old.Observation=now;
    old.SensorPresent=true;
    old.MeasurementFresh=true;
    ++old.QualifiedUpdates;
    old.Confidence=std::clamp(
        old.QualifiedUpdates/
            p.MinimumConsecutiveUpdates,0.0,1.0);
    old.FullyQualified=
        old.Confidence>=p.MinimumConfidence;
    return old;
}
inline WeatherAlongTrack InterpolateWeatherTrack(
    const std::array<WeatherCellState,WeatherCellCapacity>& states,
    double lapCm,double raceClockSeconds,
    WeatherCellParameters p={}) noexcept
{
    WeatherAlongTrack result;
    if (!ValidWeatherCellParameters(p) ||
        !Finite(lapCm) ||
        !Finite(raceClockSeconds) ||
        raceClockSeconds<0.0)
        return result;
    const double distance=WrapDistance(
        lapCm,p.TrackLengthCm);
    int before=-1;
    int after=-1;
    double nearestBehind=
        std::numeric_limits<double>::infinity();
    double nearestAhead=nearestBehind;
    for (int i=0;i<WeatherCellCapacity;++i)
    {
        const auto& state=states[
            static_cast<std::size_t>(i)];
        if (!state.FullyQualified ||
            !state.SensorPresent ||
            raceClockSeconds<
                state.LastUpdateClockSeconds ||
            raceClockSeconds-
                state.LastUpdateClockSeconds>
                p.MaximumSensorAgeSeconds)
            continue;
        const double point=state.Observation.CellDistanceCm;
        const double backward=WrapDistance(
            distance-point,p.TrackLengthCm);
        const double forward=WrapDistance(
            point-distance,p.TrackLengthCm);
        if (backward<nearestBehind)
        {
            nearestBehind=backward;
            before=i;
        }
        if (forward<nearestAhead)
        {
            nearestAhead=forward;
            after=i;
        }
    }
    if (before<0 ||after<0)return result;
    const auto& a=states[static_cast<std::size_t>(before)];
    const auto& b=states[static_cast<std::size_t>(after)];
    const double fraction=SafeDivision(
        nearestBehind,nearestBehind+nearestAhead);
    const auto& one=a.Observation;
    const auto& two=b.Observation;
    result.TrackDistanceCm=distance;
    result.SpatialInterpolationFraction=fraction;
    result.AmbientC=Lerp(
        one.AmbientTemperatureC,
        two.AmbientTemperatureC,fraction);
    result.RoadTemperatureC=Lerp(
        one.SurfaceTemperatureC,
        two.SurfaceTemperatureC,fraction);
    result.MeasuredFrictionCoefficient=Lerp(
        a.FilteredFriction,b.FilteredFriction,fraction);
    result.EffectiveWetnessFraction=Lerp(
        a.EstimatedWetnessFraction,
        b.EstimatedWetnessFraction,fraction);
    result.MeasuredWaterDepthMm=Lerp(
        one.WaterDepthMm,two.WaterDepthMm,fraction);
    result.SourceConfidence=std::min(
        a.Confidence,b.Confidence);
    result.SensorAgeSeconds=std::max(
        raceClockSeconds-a.LastUpdateClockSeconds,
        raceClockSeconds-b.LastUpdateClockSeconds);
    result.FirstSensorIndex=before;
    result.SecondSensorIndex=after;
    result.VerifiedSensorsUsed=before==after?1:2;
    result.Weather.AmbientC=result.AmbientC;
    result.Weather.TrackC=result.RoadTemperatureC;
    result.Weather.Wetness=
        result.EffectiveWetnessFraction;
    result.Weather.StandingWaterMm=
        result.MeasuredWaterDepthMm;
    result.Weather.RainMmHour=Lerp(
        one.RainRateMmHour,two.RainRateMmHour,fraction);
    result.Weather.Visibility=Lerp(
        one.VisibilityFraction,two.VisibilityFraction,fraction);
    result.Weather.WindCmS=Lerp(
        one.WindSpeedCmS,two.WindSpeedCmS,fraction);
    result.Weather.WindRadians=BlendAngle(
        one.WindDirectionRadians,
        two.WindDirectionRadians,fraction);
    result.Weather.EnvironmentQualified=true;
    result.ExpectedHydroplaningRisk=Smoothstep(
        0.5,5.0,result.MeasuredWaterDepthMm);
    result.ActualSourceQualified=true;
    result.OriginalRoadProven=true;
    result.AuthoritativeWeatherChanged=false;
    result.Valid=ValidSurfaceWeather(
        result.Weather);
    return result;
}
class PhysicalTrackWeatherMap final
{
public:
    bool Configure(WeatherCellParameters p) noexcept
    {
        if (!ValidWeatherCellParameters(p))return false;
        Settings=p;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Cells.fill({});
        Session=0;
        QualifiedCells=0;
        Accepted=0;
        Rejected=0;
        MaximumWaterDepthMm=0.0;
        LastQualifiedClockSeconds=0.0;
    }
    bool Observe(WeatherCellObservation sample) noexcept
    {
        if (Session!=0 &&sample.Session!=Session)
        {
            ++Rejected;
            return false;
        }
        if (!ValidWeatherCellObservation(sample,Settings))
        {
            ++Rejected;
            return false;
        }
        const auto index=static_cast<std::size_t>(
            sample.CellIndex);
        const bool prior=Cells[index].FullyQualified;
        auto next=UpdateWeatherCell(
            Cells[index],sample,Settings);
        if (!next.SensorPresent ||
            next.Observation.FrameOrdinal!=sample.FrameOrdinal)
        {
            ++Rejected;
            return false;
        }
        Cells[index]=next;
        Session=sample.Session;
        if (!prior &&next.FullyQualified)
            ++QualifiedCells;
        ++Accepted;
        MaximumWaterDepthMm=std::max(
            MaximumWaterDepthMm,sample.WaterDepthMm);
        LastQualifiedClockSeconds=std::max(
            LastQualifiedClockSeconds,sample.ActualClockSeconds);
        return true;
    }
    WeatherAlongTrack At(
        double distanceCm,double clockSeconds) const noexcept
    {
        return InterpolateWeatherTrack(
            Cells,distanceCm,clockSeconds,Settings);
    }
    int VerifiedCellCount() const noexcept{return QualifiedCells;}
    std::uint64_t AcceptedSamples() const noexcept{return Accepted;}
    std::uint64_t RejectedSamples() const noexcept{return Rejected;}
    double HighestWaterDepthMm() const noexcept
    {return MaximumWaterDepthMm;}
private:
    WeatherCellParameters Settings{};
    std::array<WeatherCellState,WeatherCellCapacity> Cells{};
    std::uint64_t Session=0;
    std::uint64_t Accepted=0;
    std::uint64_t Rejected=0;
    double MaximumWaterDepthMm=0.0;
    double LastQualifiedClockSeconds=0.0;
    int QualifiedCells=0;
};
} // namespace latebrake::racecraft
