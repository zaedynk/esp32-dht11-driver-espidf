#include "dht11.hpp"
#include "esp_rom_sys.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


// Initialize the spinlock structure to a completely unlocked state at boot
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

Aerospace::DHT11::DHT11(gpio_num_t pin) {
    gpio_config_t config = {};
    this->pin = pin;
    config.pin_bit_mask = 1ULL << pin; 
    config.mode = GPIO_MODE_INPUT_OUTPUT_OD;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;

    error = gpio_config(&config);

    if(error == ESP_OK) {
        gpio_set_level(pin, 1);
    }
}

esp_err_t Aerospace::DHT11::Read() {
    if(error != ESP_OK) {
        return error;
    }

    int64_t result {0};
    int64_t now = esp_timer_get_time();

    // If 2 seconds haven't passed since the last read, drop out early
    if (now - last_read_us < 2000000 && has_valid_reading) {
        return ESP_OK; // Gracefully return cached temperature/humidity values
    }

    Send_Start_Signal();

    // Disable interrupts
    portENTER_CRITICAL(&spinlock);

    // Wait low
    result = Wait_For_Level(0, 100);
    if(result == -1) {
        portEXIT_CRITICAL(&spinlock);
        return ESP_ERR_TIMEOUT;
    }

    // Wait high
    result = Wait_For_Level(1, 100);
    if(result == -1) {
        portEXIT_CRITICAL(&spinlock);
        return ESP_ERR_TIMEOUT;
    }

    // Wait low
    result = Wait_For_Level(0, 100);
    if(result == -1) {
        portEXIT_CRITICAL(&spinlock);
        return ESP_ERR_TIMEOUT;
    }

    // Enable interrupts
    portEXIT_CRITICAL(&spinlock);

    // Clear previous data
    for (int d = 0; d < 5; ++d) { data[d] = 0; }

    // Loop over the 5 bytes (40 bits)
    for (int i = 0; i < 40; ++i) {
        
        // Disable interrupts
        portENTER_CRITICAL(&spinlock);

        result = Wait_For_Level(1, 100);
        if(result == -1) {
            portEXIT_CRITICAL(&spinlock);
            return ESP_ERR_TIMEOUT;
        }

        int64_t high_duration = Wait_For_Level(0, 100);
        if(high_duration == -1) {
            portEXIT_CRITICAL(&spinlock);
            return ESP_ERR_TIMEOUT;
        }

        // Shift the byte to open up the lowest bit slot
        data[i / 8] <<= 1;

        // Evaluate the threshold
        if(high_duration > 49) {
            data[i / 8] |= 1;
        }

        // Enable interrupts
        portEXIT_CRITICAL(&spinlock);
    }

    // Calculate and mask the 8-bit checksum sum
    uint8_t raw_sum {0};
    raw_sum += data[0];
    raw_sum += data[1];
    raw_sum += data[2];
    raw_sum += data[3];

    // Validate integrity
    if(raw_sum != data[4]) {
        return ESP_ERR_INVALID_CRC;
    }

    // Unpack valid payload
    humidity = data[0];
    temperature = data[2];

    // Save successful timestamp boundary for the next 2-second check
    last_read_us = esp_timer_get_time();
    has_valid_reading = true;

    // Everything succeeded
    return ESP_OK;
}

uint8_t Aerospace::DHT11::Get_Temp() const {
    return temperature;
}

uint8_t Aerospace::DHT11::Get_Humidity() const {
    return humidity;
}

void Aerospace::DHT11::Send_Start_Signal() {
    // Drive low
    gpio_set_level(pin, 0);

    // Wait atleast 18ms
    esp_rom_delay_us(20000); // 20ms to be safe

    // Release
    gpio_set_level(pin, 1);
}

int64_t Aerospace::DHT11::Wait_For_Level(int level, int64_t timeout) {
    int64_t start_read_us, current_read_us = 0;

    start_read_us = esp_timer_get_time();
    
    while (gpio_get_level(pin) != level) {
        current_read_us = esp_timer_get_time();
        if(current_read_us - start_read_us > timeout) {
            return -1;
        }
    }

    return esp_timer_get_time() - start_read_us;
}
