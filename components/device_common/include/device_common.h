#ifndef DEVICE_SYSTEM_H
#define DEVICE_SYSTEM_H

// #include "driver/touch_sensor_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"
#include "time.h"



enum BasicConst{
    NO_DATA = -100,
    WEEK_DAYS_NUM           = 7,
    MAX_STR_LEN             = 32,
    API_LEN                 = 32,
    FORBIDDED_NOTIF_HOUR    = 6*60,
    DESCRIPTION_SIZE        = 20,
    FORECAST_LIST_SIZE      = 6,
    NET_BUF_LEN             = 6000,
};

enum Bits{
    BIT_FORECAST_OK             = (1<<0),
    BIT_ERR_SSID_NOT_FOUND      = (1<<1),
    BIT_STA_CONF_OK             = (1<<2),
    BIT_IS_TIME                 = (1<<3),
    BIT_UPDATE_SCREEN           = (1<<4),
    BIT_IS_AP_CONNECTION        = (1<<5),
    BIT_IS_STA_CONNECTION       = (1<<6),
    BIT_SERVER_RUN              = (1<<8),
    BIT_IS_AP_CLIENT            = (1<<9),
    BIT_WAIT_PROCCESS           = (1<<10),
    BIT_START_SERVER            = (1<<11),
    BIT_UPDATE_FORECAST_DATA    = (1<<12),
    BIT_NEW_PERIOD              = (1<<13),
    BIT_WAIT_BUT_INPUT          = (1<<15),
    BIT_NEW_DATA                = (1<<16),
    BIT_WAIT_SIGNALE            = (1<<18),
    BIT_IS_LOW_BAT              = (1<<19),
    BIT_CHECK_BAT               = (1<<20),
    DENIED_SLEEP_BITS           = (BIT_UPDATE_FORECAST_DATA|BIT_SERVER_RUN|BIT_UPDATE_SCREEN|BIT_WAIT_SIGNALE|BIT_WAIT_BUT_INPUT|BIT_WAIT_PROCCESS)
};

typedef struct {
    char ssid[MAX_STR_LEN+1];
    char pwd[MAX_STR_LEN+1];
    char city_name[MAX_STR_LEN+1];
    char api_key[API_LEN+1];
    unsigned flags;
    int time_offset;
} settings_data_t;


typedef struct {
    int update_data_time;
    int sunrise_hour;
    int sunset_hour;
    int sunrise_min;
    int sunset_min;
    int id_list[FORECAST_LIST_SIZE];
    int pop_list[FORECAST_LIST_SIZE];
    int temp_list[FORECAST_LIST_SIZE];
    char desciption[FORECAST_LIST_SIZE][DESCRIPTION_SIZE+1];
} service_data_t;

// --------------------------------------- GPIO
void device_gpio_init(void);
int device_set_pin(int pin, unsigned state);
int device_get_touch_but_state();


#define PIN_EP_RES          5
#define PIN_EP_DC           17
#define PIN_EP_BUSY         34
#define PIN_EP_CS           16
#define PIN_EP_SCL          4
#define PIN_EP_SDA          15
#define PIN_EP_EN           18
#define PIN_TOUCH_RIGHT     33




#define PIN_BUZZER          25

#define PIN_DHT10_EN        27
#define PIN_DHT10_IO        22

#define PIN_BM280_EN        26
#define PIN_BM280_SDA       19
#define PIN_BM280_SCL       23

#define PIN_WAKEUP          8
#define PIN_SIG_OUT         PIN_BUZZER 
#define I2C_MASTER_SCL_IO   PIN_BM280_SCL       
#define I2C_MASTER_SDA_IO   PIN_BM280_SDA        


#define TOUCH_THRESHOLD     400 
// 33
#define TOUCH_BUT_LEFT      9 
// 32
#define TOUCH_BUT_RIGHT     8

// --------------------------------------- common
int device_get_offset();
void device_set_pwd(const char *str);
void device_set_ssid(const char *str);
void device_set_city(const char *str);
void device_set_key(const char *str);
bool device_commit_changes();
unsigned device_get_state();
unsigned device_wait_bits_untile(unsigned bits, unsigned time_ms);
char *device_get_ssid();
char *device_get_pwd();
char *device_get_api_key();
char *device_get_city_name();
unsigned device_clear_state(unsigned bits);
void device_set_state_isr(unsigned bits);
void device_clear_state_isr(unsigned bits);
unsigned device_set_state(unsigned bits);
void device_init();
void device_set_offset(int time_offset);



#define device_wait_bits(bits) \
    device_wait_bits_untile(bits, 10000/portTICK_PERIOD_MS)
    
#define get_notif_size(schema) \
    (get_notif_num(schema)*sizeof(unsigned))


extern service_data_t service_data;

extern char network_buf[];






#ifdef __cplusplus
}
#endif



#endif