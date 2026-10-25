idf_component_register(SRCS "dht11.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES esp_driver_gpio esp_timer)
