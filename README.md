# ESP32 DHT11 Driver (C++, ESP-IDF)

A bare-metal driver for the DHT11 temperature and humidity sensor, written in C++ for the ESP32 using ESP-IDF v6.1 and FreeRTOS. It talks to the sensor directly over its single-wire protocol, with no third-party sensor libraries. I built it as a learning project to practice low-level embedded programming from a datasheet.

## Hardware

| Item | Details |
|---|---|
| Board | Inland ESP32-WROOM-32D |
| Sensor | DHT11 |
| Data pin | GPIO 4 |
| Power | 3.3 V |
| Pin mode | Open-drain input/output, internal pull-up enabled |

**Why open-drain with a pull-up:** on this bus, both the ESP32 and the sensor only ever pull the data line LOW. When neither is pulling, the pull-up resistor brings the line back HIGH. Because nothing ever actively drives the line HIGH, the two devices can never fight each other (one driving high while the other drives low), and the ESP32 can release the line and read it on the same pin. The ESP32's internal pull-up is weak (~45 kΩ), so a 4.7–10 kΩ external pull-up is recommended if your sensor board doesn't already have one.

## How it works

The protocol comes from section 5 of the DHT11 datasheet.

| Step | Who drives the line | What happens | Time | Driver timeout |
|---|---|---|---|---|
| Start signal | ESP32 | Pull LOW, then release | 20 ms (datasheet minimum: 18 ms) | n/a |
| Response 1 | Sensor | LOW | ~80 µs | 100 µs |
| Response 2 | Sensor | HIGH | ~80 µs | 100 µs |
| Each bit (×40) | Sensor | LOW, then HIGH | 50 µs LOW + 26–28 µs HIGH = `0`, ~70 µs HIGH = `1` | 100 µs per edge |

**Decoding bits:** every bit starts with the same 50 µs LOW, so only the HIGH part carries information. The driver measures how long the line stays HIGH. Anything longer than **49 µs** is a `1`, anything shorter is a `0`. 49 µs is the midpoint between ~28 µs and ~70 µs, so a pulse would have to be off by about 21 µs before it was misread.

**Building bytes:** the 40 bits arrive most-significant bit first and fill 5 bytes. Bit `i` goes into byte `i / 8` (integer division). For each bit, that byte is shifted left by one and the new bit is OR'd into the lowest position.

| Byte | Contents |
|---|---|
| 0 | Humidity (integer part) |
| 1 | Humidity (decimal part, 0 on DHT11) |
| 2 | Temperature (integer part) |
| 3 | Temperature (decimal part) |
| 4 | Checksum |

**Checksum:** bytes 0–3 are added together in a `uint8_t`. The datasheet says the checksum is the *last 8 bits* of that sum, and a `uint8_t` automatically wraps around at 256, which keeps only the last 8 bits. If the result doesn't match byte 4, the reading is rejected.

## Error handling

`Read()` returns an `esp_err_t`:

| Return value | Meaning |
|---|---|
| `ESP_OK` | New reading, or the last good reading if called again within 2 s |
| `ESP_ERR_TIMEOUT` | The line didn't change within 100 µs (sensor disconnected or not responding) |
| `ESP_ERR_INVALID_CRC` | Data received, but the checksum didn't match |
| Error from `gpio_config` | Pin setup failed in the constructor |

A C++ constructor can't return a value, and ESP-IDF disables exceptions by default. So the constructor stores the result of `gpio_config()`, and `Read()` checks it first and returns it if setup failed.

## Timing protection

The bit timing is in microseconds, so an interrupt or task switch in the middle of a pulse could make a `0` look like a `1`. The driver uses FreeRTOS critical sections (`portENTER_CRITICAL` / `portEXIT_CRITICAL`) around the sensor response and around each bit.

The 20 ms start signal is deliberately **not** inside a critical section. Its timing only has to be *at least* 18 ms, so extra delay is harmless, and keeping interrupts disabled for 20 ms would block the rest of the system for far too long.

## Rate limiting

The datasheet specifies one reading about every 2 seconds. If `Read()` is called again within 2 s of a good reading, it returns `ESP_OK` with the previous values and doesn't touch the sensor. A `has_valid_reading` flag makes sure the very first call always performs a real read. `app_main` also waits 1 s after power-up before the first read, as the datasheet recommends.

## Bugs I found and fixed

1. **Timeout never triggered.** In `Wait_For_Level`, the current time was read once before the loop and never updated inside it, so the elapsed time was always 0. With the sensor unplugged, the loop would have run forever. Fix: read the time on every pass of the loop.
2. **Unbalanced critical sections.** The timeout paths in the sensor-response phase called `portEXIT_CRITICAL` without a matching `portENTER_CRITICAL`, and after adding one, the success path was missing its `EXIT`, which would have left interrupts disabled after a good read. Fix: traced every path through `Read()` so each `ENTER` has exactly one `EXIT`.
3. **First read returned zeros.** The 2-second rate-limit check ran before any reading existed, so for the first 2 s after boot `Read()` returned `ESP_OK` with temperature and humidity still at 0. Fix: added `has_valid_reading`, so cached values are only returned after a real read has succeeded.
4. **Pin setup errors were ignored.** The constructor stored the `gpio_config()` result, but nothing checked it. Fix: `Read()` now checks it first and returns the error.

## Testing

- Normal operation: readings logged every 2 s.
- Data wire disconnected: `Read()` returns `ESP_ERR_TIMEOUT` every 2 s without crashing.
- Reset: the first logged reading shows real values, not zeros.

## Limitations and next steps

- The DHT11 is a low-cost sensor: ±2 °C and ±5 %RH accuracy, whole-number readings only.
- The driver polls the line (bit-banging), so it uses the CPU while reading (~4 ms per read).
- Next steps: read the sensor with the ESP32's RMT peripheral instead of polling, and unit-test the bit-decoding and checksum logic on a PC.

## Project structure

```text
esp32-dht11-driver-espidf/
├── main/
│   ├── CMakeLists.txt
│   └── main.cpp            # app_main: creates the sensor and logs readings
├── components/
│   └── dht11/
│       ├── CMakeLists.txt  # depends on esp_driver_gpio and esp_timer
│       ├── dht11.hpp       # DHT11 class declaration
│       └── dht11.cpp       # protocol implementation
└── CMakeLists.txt
```

## Build and flash

```bash
idf.py set-target esp32
idf.py build flash monitor
```

Example log format:

```text
I (3012) MAIN_APP: Temp: 24 C | Humidity: 45 %
E (5034) MAIN_APP: Sensor read failed. Error code: ESP_ERR_TIMEOUT
```
