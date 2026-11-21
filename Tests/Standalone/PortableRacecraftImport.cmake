# Source-only C++20 models from owner's earlier draft #346.
# Each fixture is built independently; no Unreal runtime is invoked here.
set(LATEBRAKE_PORTABLE_IMPORT_TESTS
    AerodynamicWakeTraffic
    BrakePressureCoherence
    CircuitStartGrid
    CornerGripForecast
    DrivingInputsAndTraffic
    ExperimentalFleetStartPolicy
    FleetRoadUnionCoverage
    GridRivalLedger
    LiveSixCarStartingGrid
    PhysicalRaceOperationsEvidence
    PhysicalRideAndContact
    PhysicalSafetyControlFusion
    PhysicalSurfaceGripAtlas
    PlayerFleetRecoveryClearance
    RaceEngineerTelemetry
    RacecraftExtendedSystems
    RacecraftFoundations
    RivalFleetRiskSupervisor
    RivalFleetTraffic
    RivalRecoveryGrid
    RivalRecoveryOccupancy
    SixCarAggregateSafety
    TelemetryAdvancedAnalytics
    TelemetryExtendedSubsystem
)
foreach(_lb IN LISTS LATEBRAKE_PORTABLE_IMPORT_TESTS)
    set(_target "latebrake_import_${_lb}")
    add_executable(${_target} "${_lb}Tests.cpp")
    target_include_directories(${_target} PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/../../Source/Latebrake/Public")
    target_compile_features(${_target} PRIVATE cxx_std_20)
    if(MSVC)
        target_compile_options(${_target} PRIVATE /W4 /WX)
    else()
        target_compile_options(${_target} PRIVATE
            -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME "SourceRacecraft${_lb}" COMMAND ${_target})
endforeach()
unset(_lb)
unset(_target)
