#include "adc_reader.h"

#include <stdio.h>
#include "driver/adc.h"
#include "esp_log.h"



#define ADC_CHANNEL ADC1_CHANNEL_4  // GPIO32 (ESP32)
#define ADC_ATTEN ADC_ATTEN_DB_0     
#define ADC_MAX_VALUE 4095          // 12-bit ADC maximum value
#define VOLT_DIV_CONST 4.15F
#define VREF 1100


static const char *TAG = "ADC_READER";

void adc_reader_init(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);
    ESP_LOGI(TAG, "ADC Initialized");
}


float device_get_voltage(void)
{
    int adc_value = adc1_get_raw(ADC_CHANNEL);
    return ((float)adc_value * VREF * VOLT_DIV_CONST) / ADC_MAX_VALUE;
}
