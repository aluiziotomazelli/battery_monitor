// components/battery_monitor/include/battery_monitor_types.hpp
#pragma once

#include <cstdint>

namespace battery_monitor {

/**
 * @brief Configuration for battery hardware voltage divider.
 */
struct BatteryMonitorConfig
{
    uint32_t divider_top_ohms = 240000;    ///< Resistor value connected to Battery positive terminal (ohms)
    uint32_t divider_bottom_ohms = 240000; ///< Resistor value connected to Ground (ohms)
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
};

} // namespace battery_monitor
