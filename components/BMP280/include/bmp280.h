#pragma once
#include <string.h>
#include <stdint.h>

#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif


int bmp280_read_data(float *temperature, float *pressure);
int bmp280_init();
int bmp280_read_raw_data(int32_t *temperature, int32_t *pressure);

#ifdef __cplusplus
}
#endif
