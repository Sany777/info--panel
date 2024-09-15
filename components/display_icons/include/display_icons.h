#ifndef DISPLAY_ICONS_H
#define DISPLAY_ICONS_H

#ifdef __cplusplus
extern "C" {
#endif




#include "bitmap_icons.h"

const unsigned char * get_battery_icon_bitmap(const int percentage);
const unsigned char *get_forecast_data_icon(int id, int day);

#ifdef __cplusplus
}
#endif



#endif