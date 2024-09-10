#include "device_task.h"

#include "esp_sleep.h"
#include "stdbool.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "portmacro.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "display_icons.h"
#include "forecast_http_client.h"
#include "sound_generator.h"
#include "periodic_task.h"
#include "device_common.h"
#include "setting_server.h"
#include "toolbox.h"
#include "wifi_service.h"
#include "epaper_adapter.h"
#include "bmp280.h"
#include "clock_module.h"
#include "adc_reader.h"



enum TimeoutMS{
    TIMEOUT_SEC                 = 1000,
    TIMEOUT_10_SEC              = 10*TIMEOUT_SEC,
    TIMEOUT_UPDATE_SCREEN       = 18*TIMEOUT_SEC,
    TIMEOUT_MINUTE              = 60*TIMEOUT_SEC,
    TIMEOUT_FOUR_MINUTE         = 4*TIMEOUT_MINUTE,
    TIMEOUT_HOUR                = 60*TIMEOUT_MINUTE,
    TIMEOUT_4_HOUR              = 4*TIMEOUT_HOUR,
    TIMEOUT_UPDATE_DATA         = 8*TIMEOUT_HOUR,
    DELAY_UPDATE_FORECAST       = TIMEOUT_HOUR / 2,
    DELAY_FIRST_UPDATE_FORECAST = 2*TIMEOUT_MINUTE,
    LONG_PRESS_TIME             = TIMEOUT_SEC
};

enum TaskDelay{
    DELAY_SERV      = 100,
    DELAY_MAIN_TASK = 100,
};

static int delay_update_forecast = DELAY_FIRST_UPDATE_FORECAST;
static float temp, pres;
static void show_screen();
static bool is_long_pressed(const int but_id);

static void update_forecast_handler();
static void update_data_handler();
static void sig_end_update_screen_handler();
static void sig_end_but_inp_handler();




static void main_task(void *pv)
{
    float volt_val;
    unsigned bits;
    int but_val = NO_DATA;
    unsigned timeout = TIMEOUT_10_SEC*1000;
    long long counter, time_work = 0;
    const struct tm * tinfo = get_time_tm();
    device_set_state(BIT_UPDATE_FORECAST_DATA|BIT_CHECK_BAT|BIT_WAIT_PROCCESS);
    create_periodic_task(update_data_handler, TIMEOUT_UPDATE_DATA, FOREVER);

    for(;;){
// device_set_state(BIT_NEW_DATA);
        device_set_pin(PIN_BM280_EN, 1);
        device_set_pin(PIN_EP_EN, 1);
        device_set_pin(PIN_DHT10_EN, 1);

        if(bmp280_init() == ESP_OK){
            bmp280_read_data(&temp, &pres);
        }
        device_start_timer();
        device_set_pin(PIN_BM280_EN, 0);
        device_set_pin(PIN_DHT10_EN, 0);
        epaper_init();

        counter = esp_timer_get_time();

        while(1){
            
            vTaskDelay(DELAY_MAIN_TASK/portTICK_PERIOD_MS);

            but_val = device_get_touch_but_state();

            if(but_val != NO_DATA){
                
                device_set_state(BIT_WAIT_BUT_INPUT);
                create_periodic_task(sig_end_but_inp_handler, TIMEOUT_10_SEC, 1);

                if(is_long_pressed(but_val)){
                    if(but_val == TOUCH_BUT_LEFT){
                        device_set_state(BIT_START_SERVER|BIT_WAIT_PROCCESS);
                    } else {
                        device_set_state(BIT_UPDATE_FORECAST_DATA|BIT_WAIT_PROCCESS);
                    }
                    long_signale();
                } else {
                    short_signale();
                }
            }

            do{
                vTaskDelay(50/portTICK_PERIOD_MS);
            }while(bits = device_get_state(), bits&BIT_WAIT_PROCCESS);

            if(bits&BIT_CHECK_BAT){
                volt_val = device_get_voltage();
                if(volt_val > 2.0){
                    if(volt_val < 3.5){
                        if(volt_val < 3.3){
                            device_set_pin(PIN_EP_EN, 0);
                            esp_deep_sleep(UINT64_MAX);
                        }
                        if(! (bits&BIT_IS_LOW_BAT) ){
                            device_set_state(BIT_IS_LOW_BAT);
                        }
                        start_signale_series(30, 20);
                        device_set_state(BIT_NEW_DATA);
                    } else if(bits&BIT_IS_LOW_BAT){
                        device_clear_state(BIT_IS_LOW_BAT);
                        device_set_state(BIT_NEW_DATA);
                    }
                    device_clear_state(BIT_CHECK_BAT);
                }
            }

            if(bits&BIT_NEW_DATA){
                device_clear_state(BIT_NEW_DATA);
                epaper_clear();
                show_screen();
                if(epaper_update()){
                    bits = device_set_state(BIT_UPDATE_SCREEN);
                    create_periodic_task(sig_end_update_screen_handler, TIMEOUT_UPDATE_SCREEN, 1);
                }
            } else if( !(bits&DENIED_SLEEP_BITS)){
                time_work = (esp_timer_get_time() - counter);
                if(time_work > timeout) break; 
            }
        }
   
        device_set_pin(PIN_EP_EN, 0);
        esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_TOUCH_RIGHT, 1);
        esp_sleep_enable_timer_wakeup(delay_update_forecast * 1000);
        device_stop_timer();
        esp_light_sleep_start();
        if(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER){
            timeout = 1;
        } else {
            timeout = TIMEOUT_10_SEC;
        }
    }
}



static void service_task(void *pv)
{
    uint32_t bits;
    bool open_sesion;
    vTaskDelay(100/portTICK_PERIOD_MS);
    int esp_res, wait_client_timeout;
    for(;;){
        device_wait_bits_untile(BIT_UPDATE_FORECAST_DATA|BIT_START_SERVER, 
                            portMAX_DELAY);

        bits = device_set_state(BIT_WAIT_PROCCESS);
        if(bits & BIT_START_SERVER){
            if(start_ap() == ESP_OK ){
                bits = device_wait_bits(BIT_IS_AP_CONNECTION);
                if(bits & BIT_IS_AP_CONNECTION && init_server(network_buf) == ESP_OK){
                    wait_client_timeout = 0;
                    device_set_state(BIT_SERVER_RUN);
                    open_sesion = false;
                    while(device_get_touch_but_state() != NO_DATA){
                        start_single_signale(10);
                        vTaskDelay(150/portTICK_PERIOD_MS);
                    }
                    while(bits = device_get_state(), bits&BIT_SERVER_RUN){
                        if(open_sesion){
                            if(!(bits&BIT_IS_AP_CLIENT) ){
                                device_clear_state(BIT_SERVER_RUN);
                            }
                        } else if(bits&BIT_IS_AP_CLIENT){
                            wait_client_timeout = 0;
                            open_sesion = true;
                        } else if(wait_client_timeout>TIMEOUT_MINUTE){
                            device_clear_state(BIT_SERVER_RUN);
                        } else {
                            wait_client_timeout += DELAY_SERV;
                        }

                        vTaskDelay(DELAY_SERV/portTICK_PERIOD_MS);

                        if(is_long_pressed(TOUCH_BUT_LEFT)){
                            device_clear_state(BIT_SERVER_RUN);
                            long_signale();
                            is_long_pressed(TOUCH_BUT_LEFT);
                        }

                    }
                    deinit_server();
                    device_commit_changes();
                }
            }
            device_clear_state(BIT_START_SERVER);
        }

        if(bits&BIT_UPDATE_FORECAST_DATA){
            esp_res = connect_sta(device_get_ssid(),device_get_pwd());
            if(esp_res == ESP_OK){
                if(! (bits&BIT_STA_CONF_OK)){
                    device_set_state(BIT_STA_CONF_OK);
                }
                if(! (bits&BIT_IS_TIME) ){
                    init_sntp();
                    bits = device_wait_bits(BIT_IS_TIME);
                }
                esp_res = get_weather(device_get_city_name(),device_get_api_key());
            }
            
            if(esp_res == ESP_OK){
                if(! (bits&BIT_FORECAST_OK)){
                    delay_update_forecast = DELAY_UPDATE_FORECAST;
                    create_periodic_task(update_forecast_handler, delay_update_forecast, FOREVER);
                    device_set_state(BIT_FORECAST_OK);
                }
                device_set_state(BIT_NEW_DATA);
            } else {
                if(bits&BIT_FORECAST_OK){
                    device_clear_state(BIT_FORECAST_OK);
                }
                if(delay_update_forecast < DELAY_UPDATE_FORECAST){
                    create_periodic_task(update_forecast_handler, delay_update_forecast, FOREVER);
                    delay_update_forecast *= 2;
                }
            }
            device_clear_state(BIT_UPDATE_FORECAST_DATA);
        }
        wifi_stop();
        device_clear_state(BIT_WAIT_PROCCESS);
    }
}



static void show_screen()
{
    struct tm * tinfo = get_time_tm();
    epaper_display_image(-8, -8, 64, 64, RED, house);
    epaper_printf_centered(118, FONT_SIZE_12, BLACK, "sunrise:%d: %s %d", snprintf_time("%A"), tinfo->tm_mday);
    epaper_printf(7, 25, FONT_SIZE_16, BLACK, "%+d", (int)temp);



    const unsigned bits = device_get_state();
    int udt, data_indx;
    udt = service_data.update_data_time;

    if(udt == NO_DATA){
        if(bits&BIT_STA_CONF_OK){
            epaper_print_centered_str(80, FONT_SIZE_20, BLACK, "Updating data");
        } else if(bits&BIT_ERR_SSID_NOT_FOUND){
            epaper_print_centered_str(80, FONT_SIZE_20, BLACK, "WIFI network not found");
        } else{
            epaper_print_centered_str(80, FONT_SIZE_20, BLACK, "No data available");
        } 
    } else {
        data_indx = get_actual_forecast_data_index(get_time_tm(), udt);
        if(data_indx == NO_DATA){
            epaper_printf_centered(60, FONT_SIZE_20, BLACK, "Data update time %d:00", udt);
        } else {
            epaper_display_image(270, -5, 64, 64, RED, 
                        get_bitmap(service_data.id_list[data_indx], 
                        service_data.pop_list[data_indx],
                        tinfo->tm_hour > service_data.sunset_hour));
            epaper_printf(100, 17, FONT_SIZE_20, BLACK,
                                     "%+d*C", 
                                    service_data.temp_list[data_indx]);
            epaper_printf_centered(48, FONT_SIZE_20, BLACK,"%s", 
                                    service_data.desciption[data_indx]);
            for(int i=data_indx; i<FORECAST_LIST_SIZE; ++i){
                if(udt>23)udt %= 24;
                epaper_printf(10+i*50, 70, FONT_SIZE_12, BLACK, "%d:00", udt);
                epaper_printf(10+i*50, 85, FONT_SIZE_12, BLACK, "%+d", service_data.temp_list[i]);
                epaper_printf(10+i*50, 100, FONT_SIZE_12, BLACK, "%d%%", service_data.pop_list[i]);
                udt += 3;
            }
        }
    }


 
}


static bool is_long_pressed(const int but_id)
{
    const long long pres_but_time = esp_timer_get_time();
    while(device_get_touch_but_state() == but_id){
        vTaskDelay(100/portTICK_PERIOD_MS);
        if((esp_timer_get_time()-pres_but_time) > LONG_PRESS_TIME){
            return true;
        }
    };
    return false;
}


static void update_forecast_handler()
{
    device_set_state_isr(BIT_UPDATE_FORECAST_DATA);
}


static void update_data_handler()
{
    create_periodic_task_isr(update_forecast_handler, delay_update_forecast, FOREVER);
    device_set_state_isr(BIT_IS_TIME);
    device_set_state_isr(BIT_UPDATE_FORECAST_DATA);
}


static void sig_end_update_screen_handler()
{
    device_clear_state_isr(BIT_UPDATE_SCREEN);
}


static void sig_end_but_inp_handler()
{
    device_clear_state_isr(BIT_WAIT_BUT_INPUT);
}


int task_init()
{
    if(xTaskCreate(
            service_task, 
            "service",
            20000, 
            NULL, 
            3,
            NULL) != pdTRUE
        || xTaskCreate(
            main_task, 
            "main",
            20000, 
            NULL, 
            4,
            NULL) != pdTRUE 
    ){
        ESP_LOGE("","task create failure");
        return ESP_FAIL;
    }
    return ESP_OK;   
}




