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
    TIMEOUT_UPDATE_SCREEN       = 19*TIMEOUT_SEC,
    TIMEOUT_MINUTE              = 60*TIMEOUT_SEC,
    TIMEOUT_FOUR_MINUTE         = 4*TIMEOUT_MINUTE,
    TIMEOUT_HOUR                = 60*TIMEOUT_MINUTE,
    TIMEOUT_4_HOUR              = 4*TIMEOUT_HOUR,
    DELAY_UPDATE_FORECAST       = 32*TIMEOUT_MINUTE,
    LONG_PRESS_TIME             = TIMEOUT_SEC,
};

enum TaskDelay{
    DELAY_SERV      = 100,
    DELAY_MAIN_TASK = 100,
};

static int delay_update_forecast = 2*TIMEOUT_MINUTE;
static float temp, pres;

static void show_screen();
static bool is_long_pressed(const int but_id);
static void sig_end_update_screen_handler();
static void sig_end_but_inp_handler();




static void main_task(void *pv)
{
    float volt_val = 0;
    unsigned bits;
    int but_val = NO_DATA;
    unsigned timeout = TIMEOUT_10_SEC;
    long long counter, work_time = 0;
    int volt_fail = 0;
    set_offset(device_get_offset());
    device_set_state(BIT_UPDATE_FORECAST_DATA);
    for(;;){

        device_start_timer();
        counter = esp_timer_get_time();
        device_set_pin(PIN_BM280_EN, 1);
        device_set_pin(PIN_EP_EN, 1);
        device_set_pin(PIN_DHT10_EN, 1);
        if(bmp280_init() == ESP_OK){
            bmp280_read_data(&temp, &pres);
        }
        device_set_pin(PIN_BM280_EN, 0);
        device_set_pin(PIN_DHT10_EN, 0);
        epaper_init();

        while(1){
            
            vTaskDelay(DELAY_MAIN_TASK/portTICK_PERIOD_MS);

            but_val = device_get_touch_but_state();

            bits = device_get_state();

            if(but_val != NO_DATA){
                device_set_state(BIT_WAIT_BUT_INPUT);
                if(is_long_pressed(but_val)){
                    create_periodic_task(sig_end_but_inp_handler, TIMEOUT_10_SEC, 1);
                    if(but_val == TOUCH_BUT_LEFT){
                        if(bits&BIT_SERVER_RUN){
                            device_clear_state(BIT_SERVER_RUN);
                        } else {
                            device_set_state(BIT_START_SERVER);
                        }
                    } else {
                        device_set_state(BIT_FORCE_UPDATE_FORECAST_DATA);
                    }
                }
            }

            if(bits&BIT_CHECK_BAT){
                volt_val = device_get_voltage();
                if(volt_val > 3.2){
                    if(volt_val < 3.5){
                        if(! (bits&BIT_IS_LOW_BAT) ){
                            device_set_state(BIT_IS_LOW_BAT);
                        }
                        device_set_state(BIT_NEW_DATA);
                        start_signale_series(50, 20);
                    } else if(bits&BIT_IS_LOW_BAT){
                        device_clear_state(BIT_IS_LOW_BAT);
                        device_set_state(BIT_NEW_DATA);
                    }
                    device_clear_state(BIT_CHECK_BAT);
                    volt_fail = 0;
                } else if(volt_fail < 20){
                    volt_fail += 1;
                } else {
                    device_set_pin(PIN_EP_EN, 0);
                    start_signale_series(100, 20);
                    vTaskDelay(2000/portTICK_PERIOD_MS);
                    esp_deep_sleep(UINT64_MAX);
                }
            }

            if(bits&BIT_NEW_DATA){
                epaper_clear();
                show_screen();
                device_clear_state(BIT_NEW_DATA);
                if(epaper_update()){
                    device_set_state(BIT_UPDATE_SCREEN);
                    create_periodic_task(sig_end_update_screen_handler, TIMEOUT_UPDATE_SCREEN, 1);
                }
            } else if( !(bits&DENIED_SLEEP_BITS)){
                work_time = (esp_timer_get_time() - counter) / 1000;
                if(work_time > timeout){
                    break; 
                } 
            }
        }

        device_set_pin(PIN_EP_EN, 0);
        esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_TOUCH_RIGHT, 1);
        esp_sleep_enable_timer_wakeup(delay_update_forecast * 1000);
        device_stop_timer();
        esp_light_sleep_start();
        if(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER){
            device_set_state(BIT_UPDATE_FORECAST_DATA);
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
    bool fail_init_sntp = false;
    struct tm * tinfo;
    for(;;){
        device_wait_bits_untile(BIT_UPDATE_FORECAST_DATA
                        |BIT_FORCE_UPDATE_FORECAST_DATA
                        |BIT_START_SERVER, 
                            portMAX_DELAY);

        bits = device_set_state(BIT_WAIT_PROCCESS);
        if(bits & BIT_START_SERVER){
            if(start_ap() == ESP_OK ){
                bits = device_wait_bits(BIT_IS_AP_CONNECTION);
                if(bits & BIT_IS_AP_CONNECTION && init_server(network_buf) == ESP_OK){
                    wait_client_timeout = 0;
                    device_set_state(BIT_SERVER_RUN);
                    open_sesion = false;
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

                    }
                    deinit_server();
                    bool change_settings = device_commit_changes();
                    if(change_settings && ! (bits&BIT_FORECAST_OK) ){
                        bits = device_set_state(BIT_UPDATE_FORECAST_DATA);
                    }
                }
                wifi_stop();
            }
            device_clear_state(BIT_START_SERVER);
        }

        if(bits&BIT_UPDATE_FORECAST_DATA || bits&BIT_FORCE_UPDATE_FORECAST_DATA){
            esp_res = connect_sta(device_get_ssid(),device_get_pwd());
            if(esp_res == ESP_OK){
                device_set_state(BIT_STA_CONF_OK);
                if(! (bits&BIT_IS_TIME)){
                    init_sntp();
                    device_wait_bits(BIT_IS_TIME);
                    stop_sntp();
                }
                esp_res = update_forecast_data(device_get_city_name(),device_get_api_key());
            }
            if(esp_res == ESP_OK){
                tinfo = get_cur_time_tm();
                if(fail_init_sntp || service_data.update_data_time > tinfo->tm_hour){
                    esp_restart();
                }
                service_data.update_data_time = tinfo->tm_hour;
                if(! (bits&BIT_FORECAST_OK)){
                    delay_update_forecast = DELAY_UPDATE_FORECAST;
                    device_set_state(BIT_FORECAST_OK);
                }
            } else {

                device_clear_state(BIT_FORECAST_OK);

                if(!fail_init_sntp && service_data.update_data_time == NO_DATA){
                    fail_init_sntp = true;
                }
                if(bits&BIT_UPDATE_FORECAST_DATA){
                    if(delay_update_forecast < DELAY_UPDATE_FORECAST){
                        delay_update_forecast *= 2;
                    }
                }
            }
            wifi_stop();
            device_set_state(BIT_NEW_DATA);
            device_clear_state(BIT_UPDATE_FORECAST_DATA|BIT_FORCE_UPDATE_FORECAST_DATA);
        }
        device_clear_state(BIT_WAIT_PROCCESS);
    }
}



static void show_screen()
{
    static int i = 0;
    int data_indx, rect_x0, rect_x1, udt;
    int cur_hour = get_cur_time_tm()->tm_hour;
    const bool is_day = cur_hour <= service_data.sunset_hour 
                && cur_hour > service_data.sunrise_hour;
    const unsigned bits = device_get_state();
    float voltage = device_get_voltage();
    epaper_display_image(-8, -8, 64, 64, RED, house);
    epaper_printf(7, 25, FONT_SIZE_16, BLACK, "%+d", (int)temp);
    epaper_display_image(265, 111, 24, 24, voltage < 3.5 ? RED : BLACK, 
                        get_battery_icon_bitmap(battery_voltage_to_percentage(voltage)));
    if(service_data.update_data_time == NO_DATA && !(bits&BIT_FORECAST_OK) ){
        if(bits&BIT_STA_CONF_OK){
            epaper_print_centered_str(80, FONT_SIZE_16, BLACK, "Updating data");
        } else if(bits&BIT_ERR_SSID_NOT_FOUND){
            epaper_print_centered_str(80, FONT_SIZE_16, BLACK, "No wifi network found");
        } else{
            epaper_print_centered_str(80, FONT_SIZE_16, BLACK, "No data available");
        } 
    } else {
        udt = service_data.update_data_time;
        data_indx = get_actual_forecast_data_index(cur_hour, udt);
        if(data_indx == NO_DATA){
            epaper_printf_centered(60, FONT_SIZE_16, BLACK, "Data update time %d:00", udt);
        } else {
            epaper_printf(50, 118, FONT_SIZE_12, BLACK, "Sunrise:%d:%2.2d Sunset:%d:%2.2d", 
                                    service_data.sunrise_hour, service_data.sunrise_min,
                                    service_data.sunset_hour, service_data.sunset_min);
            epaper_display_image(230, -7, 64, 64, BLACK, 
                        get_forecast_data_icon(service_data.id_list[data_indx], is_day));
            epaper_printf(100, 17, FONT_SIZE_20, BLACK,
                                     "%+d*C", 
                                    service_data.temp_list[data_indx]);
            epaper_printf_centered(48, FONT_SIZE_20, BLACK, "%s", 
                                    service_data.desciption[data_indx]);
            for(int i = 0; i<FORECAST_LIST_SIZE; ++i){
                if(udt>23)udt %= 24;
                rect_x0 = 13+i*46;
                rect_x1 = rect_x0+40;
                draw_rect(rect_x0, 68, rect_x1, 112, RED, false);
                epaper_printf(16+i*46, 70, FONT_SIZE_12, BLACK, "%d:00", udt);
                epaper_printf(22+i*46, 85, FONT_SIZE_12, BLACK, "%+d", service_data.temp_list[i]);
                epaper_printf(23+i*46, 100, FONT_SIZE_12, BLACK, "%d%%", service_data.pop_list[i]);
                udt += 3;
            }
        }
    }
}


static bool is_long_pressed(const int but_id)
{
    const long long pres_but_time = esp_timer_get_time();
    bool pressed = false;
    while(device_get_touch_but_state() == but_id){
        vTaskDelay(100/portTICK_PERIOD_MS);
        if(pressed){
            vTaskDelay(500/portTICK_PERIOD_MS);
            short_signale();
        } else if ( (esp_timer_get_time()-pres_but_time) > LONG_PRESS_TIME){
            pressed = true;
        }
    };
    return pressed;
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




