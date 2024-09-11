#ifndef DISPLAY_ICONS_H
#define DISPLAY_ICONS_H

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"
#include "stdbool.h"

#include "bitmap_icons.h"

const uint8_t *get_battery_icon_bitmap(float bat_voltage);
const uint8_t *update_forecast_data_icon_bitmap(int id, int clouds, bool day);

#ifdef __cplusplus
}
#endif



#endif