#include "device_common.h"
#include "sound_generator.h"

#include "periodic_task.h"
#include "device_macro.h"
#include "freertos/FreeRTOS.h"
#include "portmacro.h"
#include "esp_log.h"

#include "esp_timer.h"
#include "driver/gpio.h"

#include "driver/touch_pad.h"





static void IRAM_ATTR button_isr_handler(void* arg) 
{

}

void device_gpio_init() 
{
    touch_pad_init();
    touch_pad_config(TOUCH_BUT_LEFT, TOUCH_THRESHOLD);
    touch_pad_config(TOUCH_BUT_RIGHT, TOUCH_THRESHOLD);
}

int IRAM_ATTR device_set_pin(int pin, unsigned state) 
{
    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT_OUTPUT);
    return gpio_set_level((gpio_num_t)pin, state);
}
