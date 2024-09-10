#include "device_common.h"


#include "stdlib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "semaphore.h"
#include "string.h"
#include "portmacro.h"
#include "esp_sleep.h"

#include "device_macro.h"
#include "wifi_service.h"
#include "device_memory.h"

#include "i2c_adapter.h"
#include "dht10.h"
#include "adc_reader.h"

#include "periodic_task.h"
#include "sound_generator.h"
#include "clock_module.h"

#include "esp_log.h"


static bool changes_main_data;
static settings_data_t main_data = {0};
service_data_t service_data = {0};
char network_buf[NET_BUF_LEN];

static EventGroupHandle_t clock_event_group;
static const char *MAIN_DATA_NAME = "main_data";

static int read_data();

void device_set_offset(int time_offset)
{
    set_offset(time_offset - main_data.time_offset);
    main_data.time_offset = time_offset;
    changes_main_data = true;
}


void device_set_pwd(const char *str)
{
    const int len = strnlen(str, MAX_STR_LEN);
    memcpy(main_data.pwd, str, len);
    main_data.pwd[len] = 0;
    changes_main_data = true;
}

void device_set_ssid(const char *str)
{
    const int len = strnlen(str, MAX_STR_LEN);
    memcpy(main_data.ssid, str, len);
    main_data.ssid[len] = 0;
    changes_main_data = true;
}

void device_set_city(const char *str)
{
    const int len = strnlen(str, MAX_STR_LEN);
    memcpy(main_data.city_name, str, len);
    main_data.city_name[len] = 0;
    changes_main_data = true;
}

void device_set_key(const char *str)
{
    if(strnlen(str, API_LEN+1) == API_LEN){
        memcpy(main_data.api_key, str, API_LEN);
        changes_main_data = true;
        main_data.api_key[API_LEN] = 0;
    }
}


bool device_commit_changes()
{
    if(changes_main_data){
        CHECK_AND_RET_ERR(write_flash(MAIN_DATA_NAME, (uint8_t *)&main_data, sizeof(main_data)));
        return true;
    }
    return false;
}

unsigned device_get_state()
{
    return xEventGroupGetBits(clock_event_group);
} 

unsigned  device_set_state(unsigned bits)
{
    return xEventGroupSetBits(clock_event_group, (EventBits_t) (bits));
}

unsigned  device_clear_state(unsigned bits)
{
    return xEventGroupClearBits(clock_event_group, (EventBits_t) (bits));
}

unsigned device_wait_bits_untile(unsigned bits, unsigned time_ticks)
{
    return xEventGroupWaitBits(clock_event_group,
                                (EventBits_t) (bits),
                                pdFALSE,
                                pdFALSE,
                                time_ticks);
}


char *  device_get_ssid()
{
    return main_data.ssid;
}
char *  device_get_pwd()
{
    return main_data.pwd;
}
char *  device_get_api_key()
{
    return main_data.api_key;
}
char *  device_get_city_name()
{
    return main_data.city_name;
}

int device_get_offset()
{
    return main_data.time_offset;
}


static int read_data()
{
    service_data.update_data_time = NO_DATA;
    CHECK_AND_RET_ERR(read_flash(MAIN_DATA_NAME, (unsigned char *)&main_data, sizeof(main_data)));
    return ESP_OK;
}

void device_init()
{
    clock_event_group = xEventGroupCreate();
    device_gpio_init();
    read_data();
    I2C_init();
    wifi_init();
}



void device_set_state_isr(unsigned bits)
{
    BaseType_t pxHigherPriorityTaskWoken;
    xEventGroupSetBitsFromISR(clock_event_group, (EventBits_t)bits, &pxHigherPriorityTaskWoken);
    portYIELD_FROM_ISR( pxHigherPriorityTaskWoken );
}

void  device_clear_state_isr(unsigned bits)
{
    xEventGroupClearBitsFromISR(clock_event_group, (EventBits_t)bits);
}
