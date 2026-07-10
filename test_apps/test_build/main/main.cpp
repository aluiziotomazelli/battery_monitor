#include "esp_log.h"
#include "battery_monitor.hpp"
#include "adc_battery_reader.hpp"
#include "hal_adc_oneshot.hpp"
#include "hal_adc_calibration.hpp"
#include "bm_hal_timer.hpp"

extern "C" void app_main(void)
{
    ESP_LOGI("main", "BatteryMonitor - Build validation app");

    // 1. Declare the tools (stateless HALs)
    static battery_monitor::HalAdcOneshot oneshot_hal;
    static battery_monitor::HalAdcCalibration cali_hal;
    static battery_monitor::BmHalTimer timer_hal;

    // 2. Configure configurations
    battery_monitor::BatteryAdcConfig adc_cfg = {
        .gpio_num = 3,
        .sample_count = 16,
        .sample_delay_us = 1000,
        .enable_calibration = true
    };

    battery_monitor::BatteryMonitorConfig monitor_cfg = {
        .divider_top_ohms = 240000,
        .divider_bottom_ohms = 240000,
        .empty_mv = 3000,
        .full_mv = 4200,
        .low_mv = 3400,
        .critical_mv = 3200
    };

    // 3. Instantiate component classes (Injecting dependencies)
    static battery_monitor::AdcBatteryReader adc_reader(oneshot_hal, cali_hal, timer_hal, adc_cfg);
    static battery_monitor::BatteryMonitor monitor(adc_reader, monitor_cfg);

    // 4. Initialize and read
    if (monitor.init() == ESP_OK) {
        battery_monitor::BatteryReading reading;
        if (monitor.read(reading) == ESP_OK) {
            ESP_LOGI("main", "Battery voltage: %d mV (%d%%), state: %d",
                     reading.voltage_mv, reading.percent, static_cast<int>(reading.state));
        }
        monitor.deinit();
    }
}
