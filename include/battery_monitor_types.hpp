// components/battery_monitor/include/battery_monitor_types.hpp
#pragma once

#include <cstdint>

namespace battery_monitor {

/**
 * @brief Supported battery chemistry types for percentage calculation and state classification.
 */
enum class BatteryChemistry : uint8_t {
    LI_ION_18650 = 0, ///< Standard 1S Li-Po / 18650 Li-Ion (Min: 3300mV, Max: 4200mV)
    LIFEPO4_1S = 1,   ///< 1S LiFePO4 battery (Min: 2800mV, Max: 3650mV)
    CUSTOM = 2        ///< User-defined voltage range via custom_min_mv and custom_max_mv
};

/**
 * @brief Represents battery health and operational state based on voltage thresholds.
 */
enum class BatteryState : uint8_t {
    UNKNOWN = 0,  ///< Uninitialized or unmeasured battery state
    CRITICAL = 1, ///< Voltage below safe operation limit; immediate sleep required
    LOW = 2,      ///< Voltage low; non-essential tasks should be reduced
    NORMAL = 3,   ///< Battery operating within optimal voltage range
    FULL = 4      ///< Battery fully charged or powered via external source
};

/**
 * @brief Configuration for battery hardware voltage divider and chemistry.
 */
struct BatteryMonitorConfig
{
    uint32_t divider_top_ohms = 240000;              ///< Resistor value connected to Battery positive terminal (ohms)
    uint32_t divider_bottom_ohms = 240000;           ///< Resistor value connected to Ground (ohms)
    BatteryChemistry chemistry = BatteryChemistry::LI_ION_18650; ///< Chemistry curve for % and state
    uint16_t custom_min_mv = 3300;                   ///< 0% voltage threshold for CUSTOM chemistry (mV)
    uint16_t custom_max_mv = 4200;                   ///< 100% voltage threshold for CUSTOM chemistry (mV)
    uint16_t critical_threshold_mv = 0;              ///< Override for CRITICAL state threshold (0 = auto per chemistry)
    uint16_t low_threshold_mv = 0;                   ///< Override for LOW state threshold (0 = auto per chemistry)
    uint16_t full_threshold_mv = 0;                  ///< Override for FULL state threshold (0 = auto per chemistry)
};

/**
 * @brief Configuration for battery ADC reader sampling and calibration.
 */
struct BatteryAdcConfig
{
    int gpio_num = 3;                ///< GPIO Pin number connected to the battery divider output
    uint8_t sample_count = 16;       ///< Number of samples to average for a single reading
    uint32_t sample_delay_us = 1000; ///< Delay between samples in microseconds
    bool enable_calibration = true;  ///< Flag to enable/disable ADC calibration
};

/**
 * @brief Result of a battery measurement.
 */
struct BatteryReading
{
    uint16_t voltage_mv = 0;                    ///< Estimated battery voltage in millivolts
    uint16_t adc_mv = 0;                        ///< Measured ADC pin voltage in millivolts
    uint8_t percent = 0;                        ///< Calculated battery percentage (0 to 100%)
    BatteryState state = BatteryState::UNKNOWN; ///< Battery operational state classification
};

} // namespace battery_monitor
