#include "sound_generator.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

#include "device_common.h"
#include "periodic_task.h"
#include "driver/gpio.h"

#define DEFAULT_DELAY           100


static void IRAM_ATTR stop_signale();

static unsigned _delay;

static void IRAM_ATTR continue_signale()
{
    device_set_state_isr(BIT_WAIT_SIGNALE);
    gpio_set_level((gpio_num_t)PIN_BUZZER, 1);
    create_periodic_task_isr(stop_signale, _delay/2, 1);
}

void start_single_signale(unsigned delay)
{
    start_signale_series(delay, 1);
}


void short_signale()
{
    start_signale_series(5, 1);
}

void long_signale()
{
    start_signale_series(50, 2);
}

void sig_disable()
{
    start_single_signale(70);
}

void sound_off()
{
    remove_task(continue_signale);
    remove_task(stop_signale);
    stop_signale();
}

void start_signale_series(unsigned delay, unsigned count)
{
    if(!(device_get_state()&BIT_WAIT_SIGNALE) && delay){
        _delay = delay;
        continue_signale();
        if(count>1){
            create_periodic_task(continue_signale, _delay, count-1);
        }
    }
}



static IRAM_ATTR void stop_signale()
{
    gpio_set_level((gpio_num_t)PIN_BUZZER, 0);
    device_clear_state(BIT_WAIT_SIGNALE);
}


