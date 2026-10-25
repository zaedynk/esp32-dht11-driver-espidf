// The DHT11 is an ultra-low-cost, pre-calibrated digital temperature and humidity sensor module commonly used with microcontrollers like Arduino and Raspberry Pi.

// Key Specifications

// Temperature Range: 0°C to 50°C (accuracy of ±2°C)
// Humidity Range: 20% to 90% RH (accuracy of ±5% RH)
// Operating Voltage: 3.3V to 5.5V DC
// Output Signal: Single-bus digital signal
// Sampling Rate: Once every 2 seconds

#include "dht11.hpp"

extern "C" {

    void app_main () {

        Aerospace::DHT11 sensor(GPIO_NUM_4);


    }

}