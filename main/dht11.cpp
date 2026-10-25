#include "dht11.hpp"
#include "esp_rom_sys.h"

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