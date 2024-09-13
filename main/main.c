#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "device_common.h"
#include "device_task.h"
#include "sound_generator.h"
#include "periodic_task.h"
#include "adc_reader.h"




void app_main() 
{
    device_init();
    device_init_timer();
    adc_reader_init();
    task_init();
}
