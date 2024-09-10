idf_component_register(SRC_DIRS "src"
                        INCLUDE_DIRS "include"
                        PRIV_REQUIRES toolbox   periodic_task  wifi_service setting_server device_common forecast sound_generator adc_reader driver DHT10 epaper_adapter BMP280 clock_module esp_timer display_icons
                    ) 