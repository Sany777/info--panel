idf_component_register(SRC_DIRS "src"
                        INCLUDE_DIRS "include"
                        REQUIRES device_macro
                        PRIV_REQUIRES driver device_memory wifi_service adc_reader i2c_adapter  periodic_task sound_generator   DHT10 esp_timer clock_module
                    ) 