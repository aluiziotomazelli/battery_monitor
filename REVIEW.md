# Code Review Report — `battery_monitor` Component

> **Date:** 2026-07-09
> **Scope:** Full static review of `src/`, `include/`, `host_test/`
> **Reviewer:** Antigravity (AI-assisted)

---

## Strengths

### Architecture and Design
- **Abstract and Testable HAL**: All hardware dependencies (ADC, calibration, timer) are fully abstracted via ABC interfaces. This enables host-based unit testing without any real hardware.
- **Constructor-Based Dependency Injection**: `AdcBatteryReader` and `BatteryMonitor` receive all their dependencies via their constructors, eliminating global state and implicit coupling.
- **Single Responsibility Principle (SRP) Well Respected**: `AdcBatteryReader` is exclusively responsible for hardware reading; `BatteryMonitor` handles the business logic (voltage divider, percentage, state classification). Responsibilities are not mixed.
- **Graceful Calibration Fallback**: If `adc_cali_create_scheme_curve_fitting()` fails (unprogrammed eFuse or unsupported hardware), the component continues to operate using linear conversion fallback. Calibration is "best-effort" without blocking initialization.
- **Cleanup on `init()` Failure**: If `config_channel()` fails, `del_unit()` is called before returning, preventing resource leaks of the ADC handle.
- **Conditional Compilation for Host**: Fake `esp_adc` types inside `#if defined(__linux__)` allow compiling all business logic on the host without depending on target-specific IDF headers.
- **Test Coverage**: 26 tests covering 96.7% of production lines and 100% of public functions. Critical error paths (initialization failures, reading failures, division by zero) are thoroughly tested.

### Code Quality
- **Prevention of Double-Init/Deinit**: Both classes check `initialized_` at the beginning of `init()`, `deinit()`, and `read()` and return `ESP_ERR_INVALID_STATE` immediately if invalid.
- **Controlled Overflow in Percentage**: The percentage calculation does an upcast to `uint32_t` before multiplying by 100, preventing overflow for voltage values near limits.
- **Delay Only Between Samples**: The sampling loop calls `delay_us()` only between samples, not after the last one — correct behavior that avoids unnecessary latency.

---

## Weaknesses and Potential Improvements

### [MEDIUM] 1. Delayed Configuration Validation (Fixed)

**File:** `src/battery_monitor.cpp`

Previously, `divider_bottom_ohms == 0` was only checked inside `read()`, and other invalid settings (e.g., `full_mv <= empty_mv`) were not checked.

**Status:** Fixed. Added `validate_config()` called inside `init()` to fail fast with `ESP_ERR_INVALID_ARG` on invalid configurations.

---

### [MEDIUM] 2. Opaque Clock Source Configuration (Kept with Comments)

**File:** `src/adc_battery_reader.cpp` (init)

```cpp
// Current: literal integer value without semantics
.clk_src = static_cast<adc_oneshot_clk_src_t>(0),
```

**Discussion:** Casting to `0` works because ESP-IDF internally treats `0` as the default clock source. However, not all ESP32 targets support `ADC_RTC_CLK_SRC_DEFAULT` natively. 

**Decision:** Kept as `0` for maximum target compatibility, but added an explanatory comment documenting this decision.

---

### [LOW] 3. Unused `get_time_us()` in `IBmHalTimer` (Kept)

**File:** `include/interfaces/i_bm_hal_timer.hpp`

The `get_time_us()` method is declared in the interface and implemented in `BmHalTimer` / `MockBmHalTimer`, but is currently not called.

**Decision:** Kept in the interface defensively. It remains useful for future improvements (e.g., sample loop timeouts) or log timestamps.

---

### [LOW] 4. Absence of Timeout in the Sampling Loop (Kept)

**File:** `src/adc_battery_reader.cpp` (read_adc_mv)

The sampling loop blockingly calls `timer_hal_.delay_us()` `(sample_count - 1)` times, blocking for ~15 ms under default configuration (16 samples, 1000 µs delay).

**Decision:** Kept as is. Since `adc_oneshot_read()` is synchronous and extremely fast (~10–100 µs), there is no blocked I/O or DMA. An ADC freeze is a catastrophic hardware failure, not a normal timeout scenario. The risk is negligible.

---

### [LOW] 5. Absence of Hysteresis in State Thresholds (Kept)

**File:** `src/battery_monitor.cpp`

Battery state thresholds (`CRITICAL`, `LOW`, `NORMAL`, `FULL`) are checked without hysteresis. A voltage oscillating around `low_mv` could toggle the state rapidly.

**Decision:** Kept without hysteresis. In cyclic deep sleep devices, state is not preserved between sleep cycles unless persisted to NVS or RTC memory. For this application, the current behavior is an acceptable trade-off.

---

### [LOW] 6. Struct Partially Filled on Error (Fixed)

**File:** `src/battery_monitor.cpp`

Previously, `out.adc_mv` was filled before verifying `divider_bottom_ohms == 0`, potentially returning a partially populated struct to the caller.

**Status:** Fixed. With the configuration validation moved to `init()`, `read()` is guaranteed to operate under valid configs, preventing this failure mode entirely.

---

## Executive Summary

| # | Item | Severity | Status / Resolution |
|---|---|---|---|
| 1 | Delayed config validation | Medium | **Fixed** (validated during `init()`) |
| 2 | Opaque ADC clock source | Medium | **Kept** (documented with comments) |
| 3 | Unused `get_time_us()` in timer interface | Low | **Kept** (defensive design) |
| 4 | No timeout in sampling loop | Low | **Kept** (low risk, synchronous ADC) |
| 5 | No hysteresis in thresholds | Low | **Kept** (acceptable deep sleep trade-off) |
| 6 | Struct partially filled on error | Low | **Fixed** (resolved by #1) |
