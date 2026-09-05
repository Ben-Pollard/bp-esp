idf_component_register(SRCS "xpt2046_touch.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_driver_gpio esp_rom esp_timer)