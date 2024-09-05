#include "sound_generator.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include <math.h>

#include "device_common.h"
#include "periodic_task.h"


#define DEFAULT_DELAY           100


static void IRAM_ATTR stop_signale();

static unsigned _loud, _delay;

static void IRAM_ATTR continue_signale()
{
    device_set_pin(PIN_BUZZER, 1);
    create_periodic_isr_task(stop_signale, _delay/2, 1);
}

void start_single_signale(unsigned delay, unsigned freq)
{
    start_signale_series(delay, 1, freq);
}

void set_loud(unsigned loud)
{
    _loud = loud;
}

void alarm()
{
    start_signale_series(100, 5, 1200);
}

void start_alarm()
{
    create_periodic_isr_task(alarm, 1000, 5);
}

void sound_off()
{
    remove_isr_task(alarm);
    remove_isr_task(continue_signale);
    remove_isr_task(stop_signale);
    stop_signale();
}

void start_signale_series(unsigned delay, unsigned count, unsigned freq)
{
    if(!(device_get_state()&BIT_WAIT_SIGNALE) ){

        if(delay == 0)_delay = DEFAULT_DELAY;
        else _delay = delay;
        if(count>1){
            create_periodic_isr_task(continue_signale, _delay, count-1);
        }
        create_periodic_isr_task(stop_signale, _delay/2, 1);
    }
}

static IRAM_ATTR void stop_signale()
{
    device_set_pin(PIN_BUZZER, 0);
    device_clear_state(BIT_WAIT_SIGNALE);
}


