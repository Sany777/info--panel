#ifndef ADD_FUNCTIONS_H
#define ADD_FUNCTIONS_H



#ifdef __cplusplus
extern "C" {
#endif

#include "time.h"


unsigned get_num(char *data, unsigned size);
char * num_to_str(char *buf, unsigned num, unsigned char digits, const unsigned char base);
unsigned num_arr_to_str(char *dst, unsigned *src, unsigned char dst_digits, unsigned src_size);
int get_actual_forecast_data_index(int cur_hour, int update_data_time);
uint8_t battery_voltage_to_percentage(float voltage);


#ifdef __cplusplus
}
#endif


#endif



