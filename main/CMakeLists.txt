idf_component_register(SRC_DIRS .
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES BMP280 sound_generator device_task  device_common driver epaper_adapter freertos i2c_adapter periodic_task adc_reader
                    )
