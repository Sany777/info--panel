#include "screen_handler.h"

#include "periodic_task.h"
#include "device_common.h"
#include "forecast_http_client.h"
#include "sound_generator.h"

#include "adc_reader.h"
#include "toolbox.h"

#include "esp_sleep.h"
#include "wifi_service.h"
#include "stdbool.h"

#include "sdkconfig.h"
#include "clock_module.h"
#include "setting_server.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "portmacro.h"
#include "epaper_adapter.h"

#include "esp_log.h"



enum FuncId{
    SCREEN_MAIN,
    SCREEN_TIMER,
    SCREEN_SETTING,
    SCREEN_DEVICE_INFO,
    SCREEN_FORECAST_DETAIL,
};

enum TimeoutConst{
    TIMEOUT_SEC   = 1000,
    TIMEOUT_2_SEC = 2*TIMEOUT_SEC,
    ONE_MINUTE    = 60*TIMEOUT_SEC,
    TIMEOUT_4_HOUR= 4*60*ONE_MINUTE,
    TIMEOUT_5_SEC = 5*TIMEOUT_SEC,
    TIMEOUT_7_SEC = 7*TIMEOUT_SEC,
    TIMEOUT_6_SEC = 6*TIMEOUT_SEC,
    TIMEOUT_20_SEC= 20*TIMEOUT_SEC,
    HALF_MINUTE   = 30*TIMEOUT_SEC,
};



static void timer_func(int cmd);
static void setting_func(int cmd);
static void main_func(int cmd);
static void device_info_func(int cmd);
static void weather_info_func(int cmd);


typedef void(*handler_func_t)(int);

static const handler_func_t func_list[] = {
    main_func,
    timer_func,
    setting_func,
    device_info_func,
    weather_info_func,
};

static const int SCREEN_LIST_SIZE = sizeof(func_list)/sizeof(func_list[0]);

enum {

    CMD_PRESS,
    CMD_INC,
    CMD_DEC,
    CMD_INIT,
    CMD_DEINIT,
    CMD_UPDATE_DATA,
    CMD_UPDATE_TIME,
    CMD_UPDATE_ROT_VAL,
    CMD_UPDATE_TIMER_TIME
};

volatile static int next_screen;
volatile static bool screen_inp;



void set_default_screen_handler()
{
    next_screen = SCREEN_MAIN;
}

static void update_forecast_handler()
{
    device_set_state(BIT_UPDATE_FORECAST_DATA);
}

static void main_task(void *pv)
{
    unsigned bits;
    int screen = NO_DATA;
    int cmd = NO_DATA;
    uint64_t sleep_time_ms;
    int timeout = TIMEOUT_6_SEC;
    device_set_state(BIT_UPDATE_FORECAST_DATA);
    device_set_pin(PIN_EP_EN, 1);
    vTaskDelay(200/portTICK_PERIOD_MS);

    next_screen = SCREEN_MAIN;
    for(;;){
        screen_inp = false;
        vTaskDelay(100/portTICK_PERIOD_MS);
        restart_timer();   
        do{ 
            bits = device_get_state();

            
            if(screen != next_screen) {
                if(next_screen >= SCREEN_LIST_SIZE){
                    next_screen = 0;
                } else if(next_screen < 0){
                    next_screen = SCREEN_LIST_SIZE-1;
                }
                screen = next_screen;
                cmd = CMD_INIT;
            }else if(bits&BIT_BUT_PRESSED){
                cmd = CMD_PRESS;
                device_clear_state(BIT_BUT_PRESSED);
            } else if(bits&BIT_NEW_DATA) {
                cmd = CMD_UPDATE_DATA;
                device_clear_state(BIT_NEW_DATA);
            } else if(bits&BIT_NEW_MIN) {
                device_clear_state(BIT_NEW_MIN);
                cmd = CMD_UPDATE_TIME; 
                if(bits & BIT_IS_TIME && bits&BIT_NOTIF_ENABLE){
                    if(is_signale(get_time_tm())){
                        start_signale_series(75, 5, 2000);
                    }
                }
            } else if( ! (bits&BIT_WAIT_SIGNALE)
                            && ! (bits&BIT_WAIT_BUT_INPUT)
                                && ! (bits&BIT_WAIT_PROCCESS)
                                    && get_timer_ms() > timeout){
                break;
            }
            
            
            if(cmd != NO_DATA){

                // func_list[screen](cmd);
                if(screen == next_screen){
epaper_update();
                }
                cmd = NO_DATA;
            }

            vTaskDelay(200/portTICK_PERIOD_MS);

        } while(1);
   

        wifi_stop();
        sleep_time_ms = ONE_MINUTE - get_timer_ms()%ONE_MINUTE;

        esp_sleep_enable_timer_wakeup(sleep_time_ms * 1000);
        esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_WAKEUP, 0);
        esp_light_sleep_start();
 
        if(true){
            timeout = TIMEOUT_7_SEC;
        } else {
            timeout = 1;
            if(next_screen != SCREEN_TIMER && next_screen != SCREEN_MAIN){
                next_screen = SCREEN_MAIN;
            }
        }

        device_set_pin(PIN_EP_EN, 1);
        device_set_state(BIT_NEW_DATA);
    }
}

static void service_task(void *pv)
{
    uint32_t bits;
    int timeout = 0;
    bool open_sesion = false;
    vTaskDelay(100/portTICK_PERIOD_MS);
    int fail_count = 2;
    int esp_res;
    for(;;){
        device_wait_bits_untile(BIT_UPDATE_FORECAST_DATA|BIT_START_SERVER, 
                            portMAX_DELAY);

        bits = device_set_state(BIT_WAIT_PROCCESS);
        if(bits & BIT_START_SERVER){
            if(start_ap() == ESP_OK ){
                bits = device_wait_bits(BIT_IS_AP_CONNECTION);
                if(bits & BIT_IS_AP_CONNECTION && init_server(network_buf) == ESP_OK){
                    device_set_state(BIT_NEW_DATA|BIT_SERVER_RUN);
                    open_sesion = false;
                    while(bits = device_get_state(), bits&BIT_SERVER_RUN){
                        if(open_sesion){
                            if(!(bits&BIT_IS_AP_CLIENT) ){
                                device_clear_state(BIT_SERVER_RUN);
                            }
                        } else if(bits&BIT_IS_AP_CLIENT){
                            open_sesion = true;
                        } else if(timeout>600){
                            device_clear_state(BIT_SERVER_RUN);
                        } else {
                            timeout += 1;
                        }
                        vTaskDelay(100/portTICK_PERIOD_MS);
                    }
                    deinit_server();
                    device_commit_changes();
                    vTaskDelay(1000/portTICK_PERIOD_MS);
                }
            }
            device_clear_state(BIT_START_SERVER);
            bits = device_set_state(BIT_NEW_DATA);
        }

        if(bits&BIT_UPDATE_FORECAST_DATA){
            esp_res = connect_sta(device_get_ssid(),device_get_pwd());
            if(esp_res == ESP_OK){
                if(! (bits&BIT_STA_CONF_OK)){
                    device_set_state(BIT_STA_CONF_OK);
                }
                if(! (bits&BIT_IS_TIME) ){
                    init_sntp();
                    device_wait_bits(BIT_IS_TIME);
                }
                esp_res = get_weather(device_get_city_name(),device_get_api_key());
            }
            
            if(esp_res == ESP_OK){
                if(! (bits&BIT_FORECAST_OK)){
                    create_periodic_task(update_forecast_handler, 60*30, FOREVER);
                }
                device_set_state(BIT_FORECAST_OK|BIT_NEW_DATA);
            } else if(bits&BIT_FORECAST_OK){
                fail_count = 10;
                create_periodic_task(update_forecast_handler, fail_count*60, 1);
                device_clear_state(BIT_FORECAST_OK);
            } else if(fail_count < 30){
                create_periodic_task(update_forecast_handler, fail_count*60, FOREVER);
                fail_count += 5;
            }
            device_clear_state(BIT_UPDATE_FORECAST_DATA);
        }

        device_clear_state(BIT_WAIT_PROCCESS);
    }
}



int tasks_init()
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




static int timer_counter = 0;

void timer_counter_handler()
{
    if(timer_counter > 0){
        timer_counter -= 1;
    } else {
        remove_task(timer_counter_handler);
    }
    device_set_state(BIT_NEW_MIN);
}



void print_temp_indoor()
{
    float t;
    // if(dht20_read_data(&t, NULL) == ESP_OK){
        // lcd_printf(16, 51, FONT_SIZE_9, BLACK, "%.0fC*", t);
        // lcd_draw_house(15, 51, 40, 10, BLACK);
    // }
    // lcd_draw_line(0, 61, 128, BLACK, HORISONTAL, 0);
    // lcd_draw_line(1, 62, 127, BLACK, HORISONTAL, 1);
    // lcd_draw_line(0, 63, 128, BLACK, HORISONTAL, 1);
}

static void timer_func(int cmd)
{
    static int init_val = 0;
    static bool timer_run;

    if(cmd == CMD_INIT){
        timer_run = false;
        timer_counter = init_val = 1;
    } else if(cmd == CMD_INC || cmd == CMD_DEC) {
        if(timer_run){
            timer_run = false;
            remove_task(timer_counter_handler);
            create_periodic_task(set_default_screen_handler, 55, 1);
        } 

 
        if(init_val <= 0){
            next_screen = SCREEN_MAIN;
            init_val = timer_counter = 0;
            return;
        }
    } else if(cmd == CMD_PRESS){
        if(init_val > 0){
            timer_run = !timer_run;
            if(timer_run){
                restart_timer();
                create_periodic_task(timer_counter_handler, 60, FOREVER);
                remove_task(set_default_screen_handler);
            } else {
                remove_task(timer_counter_handler);
            }
        }
    }

    print_temp_indoor();
    // lcd_print_str(70, 50, FONT_SIZE_9, BLACK, snprintf_time("%H:%M"));

    if(timer_run){
        if(timer_counter == 0){
            timer_run = false;
            // lcd_print_centered_str(20, FONT_SIZE_18, BLACK, "0");
            // lcd_update();
            start_alarm();
            // lcd_fill(WHITE);
            print_temp_indoor();
            // lcd_print_str(70, 50, FONT_SIZE_9, BLACK, snprintf_time("%H:%M"));
            int timeout = 50;
            int light_en = 0;
            do{
                vTaskDelay(100/portTICK_PERIOD_MS);
                if(timeout%5){
                    light_en = !light_en;

                }
                // if(get_but_state()){
                //     sound_off();
  
                //     do{
                //         vTaskDelay(100/portTICK_PERIOD_MS);
                //     }while(get_but_state());
                //     device_clear_state(BIT_BUT_PRESSED|BIT_BUT_LONG_PRESSED);
                //     next_screen = SCREEN_MAIN;
                //     return;
                // }
                timeout -= 1;
            }while(timeout);

            timer_run = true;
            timer_counter = init_val;
            create_periodic_task(timer_counter_handler, 60, FOREVER);
        }
    }

    // lcd_printf_centered(15, FONT_SIZE_18, BLACK, "%i", timer_counter);
    // lcd_print_centered_str(35, FONT_SIZE_9, BLACK, timer_run ? "min" : "Pausa");
}


static void setting_func(int cmd)
{
    unsigned bits = device_get_state();
    
    // lcd_draw_line(1, 0, 127, BLACK, HORISONTAL, 1);
    // lcd_draw_line(0, 1, 128, BLACK, HORISONTAL, 1);
    // lcd_draw_line(1, 2, 127, BLACK, HORISONTAL, 1);
    // lcd_draw_line(1, 61, 127, BLACK, HORISONTAL, 1);
    // lcd_draw_line(0, 62, 128, BLACK, HORISONTAL, 1);
    // lcd_draw_line(1, 63, 127, BLACK, HORISONTAL, 1);

    if(cmd == CMD_INC || cmd == CMD_DEC){

        device_clear_state(BIT_SERVER_RUN|BIT_START_SERVER);
        return;
    }

    if(bits&BIT_SERVER_RUN){
        if(cmd == CMD_PRESS){
            device_clear_state(BIT_SERVER_RUN|BIT_START_SERVER);
        }
        // lcd_print_centered_str(2, FONT_SIZE_9, BLACK, "Server run!");
        // lcd_print_centered_str(12, FONT_SIZE_9, BLACK, "http://");
        // lcd_print_centered_str(22, FONT_SIZE_9, BLACK,"192.168.4.1");
        // lcd_print_centered_str(32, FONT_SIZE_9, BLACK, "SSID:" CONFIG_WIFI_AP_SSID);
        // lcd_print_centered_str(42, FONT_SIZE_9, BLACK, "Password:");
        // lcd_print_centered_str(52, FONT_SIZE_9, BLACK, CONFIG_WIFI_AP_PASSWORD);
    } else {
        if(cmd == CMD_PRESS){
            device_set_state(BIT_START_SERVER|BIT_WAIT_PROCCESS);
        }
        // lcd_print_centered_str(12, FONT_SIZE_9, BLACK, "Press button");
        // lcd_print_centered_str(24, FONT_SIZE_9, BLACK, "for starting");
        // lcd_print_centered_str(36, FONT_SIZE_9, BLACK, "settings server");
    }
}


static void main_func(int cmd)
{

    if(cmd == CMD_INC || cmd == CMD_DEC){

        return;
    }

    const unsigned bits = device_get_state();
    int ver_desc;
    epaper_print_centered_str(10, FONT_SIZE_20, BLACK, "Work!");
    if(bits&BIT_IS_LOW_BAT){
        // epaper_printf_centered(1, 1, 18, BLACK, "BAT!");
        // lcd_draw_rectangle(0, 0, 32, 10, BLACK);
        // lcd_draw_rectangle(32, 3, 4, 4, BLACK);
        ver_desc = 11;
    } else {
        ver_desc = 8;
    }
    
    print_temp_indoor();

    int data_indx = get_actual_forecast_data_index(get_time_tm(), service_data.update_data_time);

    if(data_indx != NO_DATA){
        // lcd_printf(70, 49, FONT_SIZE_9, BLACK, "%dC*", service_data.temp_list[data_indx]);
        // lcd_print_centered_str(ver_desc, FONT_SIZE_9, BLACK, service_data.desciption[data_indx]);
    }
    
    if(bits&BIT_IS_TIME) {
        // lcd_print_centered_str(20, FONT_SIZE_18, BLACK, snprintf_time("%H:%M"));
        // lcd_print_centered_str(37, FONT_SIZE_9, BLACK, snprintf_time("%d %a"));
    } else {
        // lcd_print_centered_str(25, FONT_SIZE_18, BLACK, snprintf_time("--:--"));
    }
}


static void device_info_func(int cmd)
{
    unsigned bits = device_get_state();

    if(cmd == CMD_INC || cmd == CMD_DEC){
        // next_screen += get_encoder_val();
        // reset_encoder_val();
        return;
    }

    // lcd_printf(5, 7, FONT_SIZE_9, BLACK, "Battery:%.2fV", adc_reader_get_voltage());

    // lcd_draw_line(1, 16, 127, BLACK, HORISONTAL, 1);
    // lcd_draw_line(0, 17, 128, BLACK, HORISONTAL, 1);
    // lcd_draw_line(1, 18, 127, BLACK, HORISONTAL, 1);

    if(bits&BIT_IS_STA_CONNECTION){
        // lcd_print_str(2, 20, FONT_SIZE_9, BLACK, "WiFi:connected");
    } else if(bits&BIT_ERR_SSID_NOT_FOUND){
        // lcd_print_str(2, 20, FONT_SIZE_9, WHITE, "SSID:not found");
    } else if( ! (bits&BIT_STA_CONF_OK)){
        // lcd_print_str(2, 20, FONT_SIZE_9, WHITE, "WiFi pwd is wrong");
    } else {
        // lcd_print_str(2, 20, FONT_SIZE_9, BLACK, "WiFi:disable");
    }
    if(bits&BIT_IS_AP_CLIENT){
        // lcd_print_str(2, 30, FONT_SIZE_9, BLACK, "AP:is a client");
    }else if(bits&BIT_IS_AP_CONNECTION){
        // lcd_print_str(2, 30, FONT_SIZE_9, BLACK, "AP:enable");
    } else {
        // lcd_print_str(2, 30, FONT_SIZE_9, BLACK, "AP:disable");
    }
    if(bits&BIT_SNTP_OK){
        // lcd_print_str(2, 40, FONT_SIZE_9, BLACK, "SNTP:Ok");
    } else {
        // lcd_print_str(2, 40, FONT_SIZE_9, WHITE, "SNTP:NOk");
    }
    if(bits&BIT_FORECAST_OK){
        // lcd_print_str(2, 50, FONT_SIZE_9, BLACK, "Openweath.:Ok");
    } else {
        // lcd_print_str(2, 50, FONT_SIZE_9, WHITE, "Openweath.:Nok");
    }
}


static void weather_info_func(int cmd)
{
    // lcd_draw_line(0, 10, 128, BLACK, HORISONTAL, 1);
    // lcd_draw_line(1, 11, 127, BLACK, HORISONTAL, 1);
    if(cmd == CMD_PRESS){
        device_set_state(BIT_UPDATE_FORECAST_DATA);
    }
    if(cmd == CMD_INC || cmd == CMD_DEC){

        return;
    }
    if(service_data.update_data_time == NO_DATA){
        // lcd_printf_centered(1, FONT_SIZE_9, BLACK, "No data");
        return;
    }

    int dt = service_data.update_data_time;
    int data_indx = get_actual_forecast_data_index(get_time_tm(), dt);

    if(data_indx != NO_DATA){
        // lcd_print_centered_str(1, FONT_SIZE_9, BLACK, service_data.desciption[data_indx]);
    } else {
        // lcd_printf_centered(1, FONT_SIZE_9, BLACK, "Update time %d:00", dt);
    }
    for(int i=0; i<FORECAST_LIST_SIZE; ++i){
        if(dt>23)dt %= 24;
        // lcd_printf(dt>9 ? 1 : 9, 
                    // 14+i*10, 
                    // FONT_SIZE_9, 
                    // BLACK, 
                    // "%d:00", dt);
        // lcd_printf(service_data.temp_list[i]/10 ? 45 : 50, 
                        // 14+i*10, 
                        // FONT_SIZE_9, 
                        // BLACK, 
                        // "%dC*", service_data.temp_list[i]);
        // lcd_printf(service_data.pop_list[i]/10 ? 95 : 100, 
        //             14+i*10, 
        //             FONT_SIZE_9, 
        //             BLACK, 
        //             "%d%%", service_data.pop_list[i]);
        // dt += 3;
    }
    // lcd_draw_line(42, 12, 64, BLACK, VERTICAL, 1);
    // lcd_draw_line(90, 12, 64, BLACK, VERTICAL, 1);
}









