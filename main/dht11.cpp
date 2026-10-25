#include "dht11.hpp"
#include "esp_rom_sys.h"
#include "esp_timer.h"

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
    return ESP_OK;
}

uint8_t Aerospace::DHT11::Get_Temp() const {
    return temperature;
}

uint8_t Aerospace::DHT11::Get_Humidity() const {
    return humidity;
}

void Aerospace::DHT11::Send_Start_Signal() {

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
