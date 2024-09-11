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


int device_get_touch_but_state()
{
    uint16_t touch_val;
    
    touch_pad_read(TOUCH_BUT_RIGHT, &touch_val);

    if(touch_val < TOUCH_THRESHOLD){
        return TOUCH_BUT_RIGHT;
    }

    touch_pad_read(TOUCH_BUT_LEFT, &touch_val);

    if(touch_val < TOUCH_THRESHOLD){
        return TOUCH_BUT_LEFT;
    }

    return NO_DATA;
}

void device_gpio_init() 
{
    device_set_pin(PIN_BUZZER, 0);
    device_set_pin(PIN_BM280_EN, 0);
    device_set_pin(PIN_EP_EN, 0);
    device_set_pin(PIN_DHT10_EN, 0);
    gpio_set_direction((gpio_num_t)32, GPIO_MODE_INPUT);
    gpio_set_level((gpio_num_t)32, 0);
    touch_pad_init();
    touch_pad_config(TOUCH_BUT_LEFT, TOUCH_THRESHOLD);
    touch_pad_config(TOUCH_BUT_RIGHT, TOUCH_THRESHOLD);
}

int IRAM_ATTR device_set_pin(int pin, unsigned state) 
{
    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT_OUTPUT);
    return gpio_set_level((gpio_num_t)pin, state);
}
