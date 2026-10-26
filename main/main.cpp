#include "dht11.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "MAIN_APP";

extern "C" {

    void app_main () {

        Aerospace::DHT11 sensor(GPIO_NUM_4);

        // 1s delay before reading for the first time
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        while(true) {
            esp_err_t status = sensor.Read();

            if(status == ESP_OK) {
                ESP_LOGI(TAG, "Temp: %d C | Humidity: %d %%", 
                        sensor.Get_Temp(), 
                        sensor.Get_Humidity());
            } else {
                ESP_LOGE(TAG, "Sensor read failed. Error code: %s", esp_err_to_name(status));
            }
            
            // Delay before reading again
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }

}