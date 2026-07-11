# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.1] - 2026-07-11

### Changed
- Migrated ADC and Timer HAL abstractions to the `idf_hals` submodule.
- Replaced `BmHalTimer` with `idf_hals::SysRomHAL`.
- Pointed internal dependencies to `idf_hals` namespace.

## [2.0.0] - 2026-07-10

### Removed
- Domain-specific logic (`BatteryState` enum, state classification, linear percentage calculation).
- Configurable thresholds (`empty_mv`, `full_mv`, `low_mv`, `critical_mv`) from `BatteryMonitorConfig`.
- `percent` and `state` fields from `BatteryReading`.
- Threshold validation logic and related config validation tests.

### Changed
- Simplified `BatteryMonitor` to act strictly as a voltage reader (performing only divider ratio compensation).
- Updated unit tests and documentation to match the simplified API.

## [1.0.0] - 2026-07-09

### Added
- Initial release of BatteryMonitor component for ESP-IDF.
- Abstract interface for battery monitoring (`IBatteryMonitor`) and low-level ADC driver reading (`IAdcBatteryReader`).
- HAL abstraction wrappers for ESP-IDF oneshot ADC (`HalAdcOneshot`) and curve-fitting calibration (`HalAdcCalibration`).
- Timer HAL wrapper (`BmHalTimer`) prefixed to avoid target naming conflicts.
- Multi-sample averaging logic with custom sample delay and sample counts to eliminate noise.
- Linear battery percentage mapping and voltage divider scaling formulas.
- Battery state classifications (`UNKNOWN`, `CRITICAL`, `LOW`, `NORMAL`, `FULL`) based on configurable millivolt thresholds.
- Conditional CMake compilation targeting `linux` (host) to compile mock environments without hardware constraints.
- Host-based unit tests using Google Test and Google Mock frameworks.
- Production integration into `WaterTankApp` and `main.cpp` for XIAO-ESP32-C3.
- Complete documentation including API Reference (`API.md`) and Architectural Decisions (`README.md`).

[2.0.0]: https://github.com/aluiziotomazelli/battery-monitor/releases/tag/v2.0.0
[1.0.0]: https://github.com/aluiziotomazelli/battery-monitor/releases/tag/v1.0.0
