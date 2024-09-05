#include "test_periph.h"



#include "device_common.h"
#include "sound_generator.h"
#include "screen_handler.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"



#include "esp_log.h"
#include "driver/touch_pad.h"
#include "esp_system.h"
#include "esp_err.h"

#include "epaper_adapter.h"


void test()
{
    device_set_pin(PIN_EP_EN, 1);
    vTaskDelay(100);
    epaper_init();
    epaper_clear(WHITE);
    vTaskDelay(100);
    epaper_print_centered_str(20, 16, BLACK, "Hello");
    epaper_update();

}