# ESP32 Bare-Metal DHT11 Bit-Banged Driver

A highly deterministic, fault-tolerant C++ peripheral driver for the DHT11 temperature and humidity sensor built natively on the **ESP-IDF (v6.1)** framework. This project was developed from the ground up without external abstraction libraries to demonstrate safety-critical firmware engineering principles required in aerospace and defense environments.

## Key Engineering & Safety Features

*   **Defensive Guard Clauses:** Implements absolute microsecond timeout thresholds on all hardware state transitions to eliminate infinite blocking loops and ensure deterministic system behavior.
*   **Asynchronous Timing Protection:** Utilizes FreeRTOS critical sections (`portENTER_CRITICAL`) to temporarily disable hardware interrupts, preventing CPU task scheduling from distorting sub-millisecond bitstream pulses.
*   **Software Rate-Limiting:** Enforces a defensive 2-second rate limit using the hardware system timer (`esp_timer_get_time()`) to protect the sensor against data corruption or thermal strain caused by flooding requests.
*   **Uniform Initialization:** Follows modern C++ safety patterns by strictly using braced initialization `{0}` for private data structures to entirely eliminate compile-time narrowing conversions and uninitialized memory bugs.
*   **Checksum Verification:** Mandates strict 8-bit checksum packet validation before exposing unpacked telemetry constants to the main application layer.

## Hardware Bus Topography & Configuration

Safety-critical communication over a single wire relies on an Open-Drain bus topography. This allows multiple silicon gates to interface safely without risk of an electrical short circuit.

*   **Bus Mode:** Open-Drain Input/Output (`GPIO_MODE_INPUT_OUTPUT_OD`)
*   **Biasing:** Internal Pull-Up Resistor Enabled / Pull-Down Disabled
*   **Data Pin:** Target target mapping default is set to `GPIO_NUM_4`
*   **Target Hardware:** Inland ESP32-WROOM-32D Module (Xtensa Dual-Core LX6)

### Sensor Specifications

The DHT11 is an ultra-low-cost, pre-calibrated digital temperature and humidity sensor module operating under the following performance constraints:

*   **Temperature Range:** 0°C to 50°C (accuracy of ±2°C)
*   **Humidity Range:** 20% to 90% RH (accuracy of ±5% RH)
*   **Operating Voltage:** 3.3V to 5.5V DC
*   **Output Signal:** Asynchronous single-bus digital signal
*   **Sampling Rate:** Once every 2 seconds (0.5 Hz)

### Project Directory Structure


```text
esp32-dht11-driver-espidf/
├── main/
│   ├── CMakeLists.txt     # Application-level compilation rules
│   └── main.cpp           # Application entry loop & scheduler
├── components/
│   └── dht11/
│       ├── CMakeLists.txt # Component-level build configuration (Links to esp_driver_gpio & esp_timer)
│       ├── dht11.hpp      # Object-oriented C++ Class definition & API contract
│       └── dht11.cpp      # Isolated low-level peripheral state machine
├── CMakeLists.txt         # Root project build rules
└── README.md              # Engineering documentation and verification blueprint
```

## Protocol Specifications & Timing Parameters

The single-wire communication protocol is calculated based on the following exact microsecond threshold windows:

| Phase | Description | Logic State | Target Duration | Safety Timeout |
| :--- | :--- | :--- | :--- | :--- |
| **Phase 1** | Master Start Assertion | Logic LOW (0) | 20,000 µs (20 ms) | N/A (Forced Wait) |
| **Phase 2** | Sensor Handshake Start | Logic LOW (0) | 80 µs | 100 µs |
| **Phase 3** | Sensor Handshake Sync | Logic HIGH (1) | 80 µs | 100 µs |
| **Phase 4** | Binary `0` Payload Data | Logic HIGH (1) | 26–28 µs | 100 µs (Threshold > 49 µs) |
| **Phase 5** | Binary `1` Payload Data | Logic HIGH (1) | 70 µs | 100 µs (Threshold > 49 µs) |

## 🧠 Engineering Notes & Debugging Log

Developing this bare-metal driver required overcoming several critical hardware-software race conditions that are common in safety-critical real-time embedded systems:

### 1. Eliminating Infinite Loops via Deterministic Time Tracking
Initially, a classic asynchronous bug was identified in the polling loop where the elapsed microsecond tracking stayed stuck at zero, risking an infinite loop lockup if the physical sensor wire was pulled. I resolved this by shifting `esp_timer_get_time()` inside the active `while` condition. Tracing the CPU execution across failure paths proved that the driver now successfully fails-fast and yields an `ESP_ERR_TIMEOUT` within exactly 100 µs of a hardware bus stall.

### 2. Restructuring Critical Sections to Prevent OS Watchdog Resets
An initial implementation trapped the entire 40-bit bit-banging data stream inside a single global critical section. Holding the `portENTER_CRITICAL` spinlock across the entire read routine starved the FreeRTOS scheduler ticks for over 4 milliseconds, triggering immediate CPU crash dumps (`Backtrace:...`). I re-architected the timing blocks to tightly wrap only the individual microsecond bit-payload windows, allowing the underlying operating system to breathe and preventing system task watchdog lockups.

### 3. Graceful System Degradation via the Cached Data Pattern
The DHT11 silicon registers demand a mandatory 2-second settling window between physical reads. To handle instances where the main system application accidentally floods the polling entry loop too quickly, I implemented a defensive rate-limit gate using `last_read_us`. Instead of throwing a system error that could break application telemetry, the driver utilizes a "Graceful Degradation" pattern—returning `ESP_OK` and allowing the application to safely reuse the previously cached data values until the hardware bus recharges.

## Verification & Compilation Blueprint

To build, flash, and read the real-time telemetry output, activate your target framework path and run the compilation toolchain:

```bash
# Set the target hardware controller
idf.py set-target esp32

# Cross-compile, flash over UART, and spawn the serial monitor
idf.py build flash monitor
```

### Expected Telemetry Console Output
```text
I (5322) MAIN_APP: Temp: 25 C | Humidity: 47 %
I (7322) MAIN_APP: Temp: 25 C | Humidity: 47 %
I (9322) MAIN_APP: Temp: 26 C | Humidity: 46 %
```
