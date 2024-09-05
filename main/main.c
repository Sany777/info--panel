#include <stdio.h>
#include "sound_generator.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "device_common.h"
#include "screen_handler.h"
#include "epaper_adapter.h"



void app_main() 
{
    // device_init();
    // start_signale_series(40, 3, 1000);
    // tasks_init();
    // device_set_pin(PIN_BUZZER, 1);
    device_set_pin(PIN_EP_EN, 1);
    device_set_pin(PIN_BM280_EN, 1);
    device_set_pin(PIN_DHT10_EN, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    epaper_init();
    epaper_clear();
    vTaskDelay(pdMS_TO_TICKS(1000));
    epaper_print_str(10, 20, FONT_SIZE_34, WHITE, "WORK!!!");
    epaper_print_str(100, 20, FONT_SIZE_34, BLACK, "WORK!!!");
    epaper_print_str(5, 100, FONT_SIZE_34, RED, "WORK!!!");
    epaper_update();

    vTaskDelay(pdMS_TO_TICKS(500000));



}