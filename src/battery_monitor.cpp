// components/battery_monitor/src/battery_monitor.cpp
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"

#include "battery_monitor.hpp"

static const char* TAG = "BatteryMonitor";

namespace battery_monitor {

BatteryMonitor::BatteryMonitor(IAdcBatteryReader& adc_reader, const BatteryMonitorConfig& config)
    : adc_reader_(adc_reader)
    , config_(config)
    , initialized_(false)
{
}

esp_err_t BatteryMonitor::init()
{
    if (initialized_) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = validate_config();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Configuration validation failed: %d", err);
        return err;
    }

    err = adc_reader_.init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC reader: %d", err);
        return err;
    }

    initialized_ = true;
    ESP_LOGI(TAG, "Initialized successfully");
    return ESP_OK;
}

esp_err_t BatteryMonitor::deinit()
{
    if (!initialized_) {
        ESP_LOGW(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = adc_reader_.deinit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to deinitialize ADC reader: %d", err);
        return err;
    }

    initialized_ = false;
    ESP_LOGI(TAG, "Deinitialized successfully");
    return ESP_OK;
}

esp_err_t BatteryMonitor::read(BatteryReading& out)
{
    if (!initialized_) {
        ESP_LOGE(TAG, "Cannot read: component not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    uint16_t adc_mv = 0;
    esp_err_t err = adc_reader_.read_adc_mv(adc_mv);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ADC voltage: %d", err);
        return err;
    }

    out.adc_mv = adc_mv;

    // Calculate battery voltage using voltage divider compensation
    uint32_t calculated_voltage = static_cast<uint32_t>(adc_mv) *
                                  (config_.divider_top_ohms + config_.divider_bottom_ohms) /
                                  config_.divider_bottom_ohms;

    out.voltage_mv = static_cast<uint16_t>(calculated_voltage);



    return ESP_OK;
}

bool BatteryMonitor::is_initialized() const
{
    return initialized_;
}

esp_err_t BatteryMonitor::validate_config() const
{
    if (config_.divider_bottom_ohms == 0) {
        ESP_LOGE(TAG, "Invalid config: divider_bottom_ohms cannot be 0");
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

} // namespace battery_monitor
