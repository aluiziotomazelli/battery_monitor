# BatteryMonitor Component API

This document provides the reference for the BatteryMonitor component API.

---

## Core Types

### `enum class BatteryChemistry`
Supported battery chemistry profiles for percentage calculation and state classification.

| Value | Raw Type | Default Voltage Range | Description |
|-------|----------|-----------------------|-------------|
| `LI_ION_18650` | `uint8_t` | 3300 mV - 4200 mV | Standard 1S Li-Po / 18650 Li-Ion cell. |
| `LIFEPO4_1S` | `uint8_t` | 2800 mV - 3650 mV | 1S LiFePO4 battery cell. |
| `CUSTOM` | `uint8_t` | Custom | Custom user-defined voltage range (`custom_min_mv` to `custom_max_mv`). |

### `enum class BatteryState`
Represents operational battery health and capacity classification.

| Value | Raw Type | Description |
|-------|----------|-------------|
| `UNKNOWN` | `uint8_t` | Uninitialized or unmeasured battery state. |
| `CRITICAL` | `uint8_t` | Voltage below safe operation limit; immediate sleep/shutdown required. |
| `LOW` | `uint8_t` | Low battery level; non-essential tasks should be reduced or disabled. |
| `NORMAL` | `uint8_t` | Battery operating within optimal voltage range. |
| `FULL` | `uint8_t` | Battery fully charged or connected to external power source. |

### `struct BatteryMonitorConfig`
Configuration parameters for the voltage divider, battery chemistry, and operational state thresholds.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `divider_top_ohms` | `uint32_t` | 240000 | Resistor value connected to Battery positive terminal (ohms). |
| `divider_bottom_ohms` | `uint32_t` | 240000 | Resistor value connected to Ground (ohms). |
| `chemistry` | `BatteryChemistry` | `LI_ION_18650` | Battery chemistry profile used for % and state calculation. |
| `custom_min_mv` | `uint16_t` | 3300 | 0% voltage threshold for `CUSTOM` chemistry (mV). |
| `custom_max_mv` | `uint16_t` | 4200 | 100% voltage threshold for `CUSTOM` chemistry (mV). |
| `critical_threshold_mv` | `uint16_t` | 0 | Override threshold for `CRITICAL` state (0 = default per chemistry). |
| `low_threshold_mv` | `uint16_t` | 0 | Override threshold for `LOW` state (0 = default per chemistry). |
| `full_threshold_mv` | `uint16_t` | 0 | Override threshold for `FULL` state (0 = default per chemistry). |

> [!IMPORTANT]
> **Configuration Validation during `init()`**:
> The config parameters are validated when calling `init()`. If validation fails, `init()` returns `ESP_ERR_INVALID_ARG`.
> The rule is:
> 1. `divider_bottom_ohms` cannot be `0`.

### `struct BatteryAdcConfig`
Configuration parameters for battery ADC hardware channel and sampling.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `gpio_num` | `int` | 3 | GPIO Pin number connected to the battery divider output. |
| `sample_count` | `uint8_t` | 16 | Number of samples to average for a single reading. |
| `sample_delay_us` | `uint32_t` | 1000 | Delay between consecutive samples in microseconds. |
| `enable_calibration` | `bool` | `true` | Enable/disable ESP32 eFuse calibration scheme for ADC readings. |

### `struct BatteryReading`
Container for the results of a battery measurement.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `voltage_mv` | `uint16_t` | 0 | Compensated battery voltage in millivolts. |
| `adc_mv` | `uint16_t` | 0 | Measured ADC pin voltage in millivolts. |
| `percent` | `uint8_t` | 0 | Calculated battery percentage (0 to 100%). |
| `state` | `BatteryState` | `UNKNOWN` | Battery operational state classification. |

---

## Component Interfaces

### `class IBatteryMonitor`
Abstract interface providing high-level battery voltage, percentage, and state monitoring.

#### `esp_err_t init()`
Initializes the battery monitor and underlying ADC reader.
* **Returns**: `ESP_OK` on success, or hardware initialization error code.

#### `esp_err_t deinit()`
Deinitializes the battery monitor and cleans up ADC resources.
* **Returns**: `ESP_OK` on success, or hardware deinitialization error code.

#### `esp_err_t read(BatteryReading& out)`
Takes a new battery reading, calculates compensated voltage, calculates percentage (0-100%), and classifies battery state.
* **Parameters**: `out` - Reference to a structure where the reading results will be stored.
* **Returns**: `ESP_OK` on success, `ESP_ERR_INVALID_STATE` if not initialized, or a reading error.

#### `bool is_initialized()`
Checks if the battery monitor is initialized.
* **Returns**: `true` if initialized, `false` otherwise.

#### `static uint8_t calculate_percent(uint16_t voltage_mv, const BatteryMonitorConfig& config)`
Calculates battery percentage (0-100%) for a given voltage based on configured chemistry.

#### `static BatteryState classify_state(uint16_t voltage_mv, const BatteryMonitorConfig& config)`
Classifies battery operational state based on voltage and thresholds.

---

## `class IAdcBatteryReader`
Abstract interface for the low-level ADC driver reader.

#### `esp_err_t init()`
Sets up the ADC peripheral unit, configures the oneshot channel with 12 dB attenuation, and initializes the calibration scheme if enabled.
* **Returns**: `ESP_OK` on success, or ADC driver error code.

#### `esp_err_t deinit()`
Cleans up oneshot ADC unit and calibration configuration.
* **Returns**: `ESP_OK` on success, or ADC driver error code.

#### `esp_err_t read_adc_mv(uint16_t& out_mv)`
Performs multiple ADC readings, averages the raw values, and translates the raw average to voltage in millivolts (using calibration if available).
* **Parameters**: `out_mv` - Reference to store the calculated millivolt reading.
* **Returns**: `ESP_OK` on success, `ESP_ERR_INVALID_STATE` if not initialized, or read error.

#### `bool is_initialized()`
Checks if the ADC reader driver is initialized.
* **Returns**: `true` if initialized, `false` otherwise.
