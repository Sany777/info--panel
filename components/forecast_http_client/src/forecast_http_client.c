#include "forecast_http_client.h"

#include "esp_http_client.h"
#include "clock_module.h"
#include "device_common.h"
#include "device_macro.h"

#define SIZE_URL_BUF 250
#define MAX_KEY_NUM 10

static char url_buf[SIZE_URL_BUF];


static size_t get_value_ptrs(char ***value_list, char *data_buf, const size_t buf_len, const char *key)
{
    char *buf_oper[MAX_KEY_NUM] = { 0 };
    if(data_buf == NULL) 
        return 0;
    const size_t key_size = strlen(key);
    char *data_ptr = data_buf;
    char **buf_ptr = buf_oper;
    const char *data_end = data_buf+buf_len;
    size_t value_list_size = 0;
    while(data_ptr = strstr(data_ptr, key), 
            data_ptr 
            && data_end > data_ptr 
            && MAX_KEY_NUM > value_list_size ){
        data_ptr += key_size;
        buf_ptr[value_list_size++] = data_ptr;
    }
    const size_t data_size = value_list_size * sizeof(char *);
    *value_list = (char **)malloc(data_size);
    if(value_list == NULL)
        return 0;
    memcpy(*value_list, buf_oper, data_size);
    return value_list_size;
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer; 
    static int output_len;      
    switch(evt->event_id) {
    case HTTP_EVENT_ON_DATA:
    {
        if(!esp_http_client_is_chunked_response(evt->client)){
            int copy_len = 0;
            if(evt->user_data){
                copy_len = MIN(evt->data_len, (NET_BUF_LEN - output_len));
                if (copy_len) {
                    memcpy(evt->user_data + output_len, evt->data, copy_len);
                }
            } else {
                const int buffer_len = esp_http_client_get_content_length(evt->client);
                if (output_buffer == NULL) {
                    output_buffer = (char *) malloc(buffer_len);
                    output_len = 0;
                    if (output_buffer == NULL) {
                        return ESP_FAIL;
                    }
                }
                copy_len = MIN(evt->data_len, (buffer_len - output_len));
                if (copy_len) {
                    memcpy(output_buffer + output_len, evt->data, copy_len);
                }
            }
            output_len += copy_len;
        }
        break;
    }
    case HTTP_EVENT_ON_FINISH:
    {
        if(output_buffer != NULL){
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    }
    case HTTP_EVENT_DISCONNECTED:
    {
        if (output_buffer != NULL){
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    }
    default:break;
    }
    return ESP_OK;
}



static void split(char *data_buf, size_t data_size, const char *split_chars_str)
{
    char *ptr = data_buf;
    const char *end_data_buf = data_buf + data_size;
    const size_t symb_list_size = strlen(split_chars_str);
    char c;
    while(ptr != end_data_buf){
        c = *(ptr);
        for(int i=0; i<symb_list_size; ++i){
            if(split_chars_str[i] == c){
                *(ptr) = 0;
                break;
            }
        }
        ++ptr;
    }
}

int get_weather(const char *city, const char *api_key)
{
    int res = ESP_FAIL;
    char **feels_like_list = NULL, **description_list = NULL, **pop_list = NULL, **dt_list = NULL; 

    if(strnlen(city, MAX_STR_LEN) == 0 || strnlen(api_key, MAX_STR_LEN) != API_LEN)
        return ESP_FAIL;

    snprintf(url_buf, SIZE_URL_BUF, 
    "https://api.openweathermap.org/data/2.5/forecast?q=%s&units=metric&cnt=%d&appid=%s", 
    city, FORECAST_LIST_SIZE, api_key);

    esp_http_client_config_t config = {
        .url = url_buf,
        .event_handler = http_event_handler,
        .user_data = (void*)network_buf,    
        .method = HTTP_METHOD_GET,
        .buffer_size = NET_BUF_LEN,
        .auth_type = HTTP_AUTH_TYPE_NONE,
        .skip_cert_common_name_check = true
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_perform(client);
    const size_t data_size = esp_http_client_get_content_length(client);
    if(data_size){

        network_buf[data_size] = 0;
        
        const size_t pop_num = get_value_ptrs(&pop_list, network_buf, data_size, "\"pop\":");
        const size_t feels_like_num = get_value_ptrs(&feels_like_list, network_buf, data_size, "\"feels_like\":");
        const size_t description_num = get_value_ptrs(&description_list, network_buf, data_size, "\"description\":\"");
        get_value_ptrs(&dt_list, network_buf, data_size,"\"dt\":");

        split(network_buf, data_size, "},\"");

        if(description_list){
            memset(service_data.desciption, 0, sizeof(service_data.desciption));
            for(int i=0; i<description_num && i<FORECAST_LIST_SIZE; ++i){
                strncpy(service_data.desciption[i], description_list[i], sizeof(service_data.desciption[0]));
            }
            free(description_list);
            description_list = NULL;
        }

        if(dt_list){
            time_t time_now  = atol(dt_list[0]);
            struct tm * tinfo = localtime(&time_now);
            service_data.update_data_time = tinfo->tm_hour;
            free(dt_list);
            dt_list = NULL;
        }

        if(feels_like_list){
            for(int i=0; i<feels_like_num && i<FORECAST_LIST_SIZE; ++i){
                service_data.temp_list[i] = atof(feels_like_list[i]);
            }
            free(feels_like_list);
            feels_like_list = NULL;
        }

        if(pop_list){
            for(int i=0; i<pop_num && i<FORECAST_LIST_SIZE; ++i){
                service_data.pop_list[i] = atof(pop_list[i])*100;
            }
            free(pop_list);
            pop_list = NULL;
        }
        res = ESP_OK;
    }
    
    esp_http_client_cleanup(client);

    return res;
}

