#pragma once
#include "driver/gpio.h"
#include "esp_err.h"
#include <cstdint>

namespace Aerospace {

    class DHT11 {
    public:
        
        explicit DHT11(gpio_num_t pin);

        esp_err_t Read();
        
        uint8_t Get_Temp() const;

        uint8_t Get_Humidity() const;

    private:
        void Send_Start_Signal();

        int64_t Wait_For_Level(int level, int64_t timeout);

        esp_err_t error;
        gpio_num_t pin;
        uint8_t data[5] = {};
        int64_t last_read_us{0};
        uint8_t temperature {0};
        uint8_t humidity {0}; 
        bool has_valid_reading{false};
    };

}