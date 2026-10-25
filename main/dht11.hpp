#pragma once
#include "driver/gpio.h"
#include "esp_err.h"

#include <cstdint>

/*

Communication Process: Serial Interface
https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf

40 bit data total.
Without the start signal, it wont give the response signal.
Once data is collected, DHT11 will change to the low power consumption mode until it receives a start signal again.


https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html

Set up pin	    gpio_config()
Drive low	    gpio_set_level()
Release	        gpio_set_level()
Read line	    gpio_get_level()

https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/internal-unstable.html
Wait N µs	    esp_rom_delay_us()

https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/esp_timer.html
Current time	esp_timer_get_time()	



----- 5.2 MCU Sends out Start Signal to DHT -----

(1) drive low, (2) wait ≥18 ms, (3) release, (4) wait until the line goes low, but give up if it takes too long.



----- 5.3 DHT Responses to MCU -----

(1) read low, (2) wait until reading high, (3) prepare for data transmission - loop read after 80us, every bit of data begins with 50us low, then a high - length of high determines whether each bit is 0 or 1. >= 49us(center between 28 and 70) means 1
Each step should be verified as it goes.

*/

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
    };

}