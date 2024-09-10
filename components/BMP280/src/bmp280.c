#include "bmp280.h"

#include <stdio.h>
#include "i2c_adapter.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "device_macro.h"

#define BMP280_ADDR 0x76

#define BMP280_REG_ID 0xD0
#define BMP280_REG_CTRL_MEAS 0xF4
#define BMP280_REG_CONFIG 0xF5
#define BMP280_REG_DATA 0xF7
#define BMP280_REG_READ_CALIB_DATA 0x88

#define BMP280_EXPECTED_ID 0x58

#define DELAY_REQUEST   (40/portTICK_PERIOD_MS)
#define TIMEOUT_REQUEST (1000/portTICK_PERIOD_MS)

typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} bmp280_calib_data;

static bmp280_calib_data calib_data;
static int32_t t_fine;

static uint32_t bmp280_compensate_pressure(int32_t adc_P);
static int32_t bmp280_compensate_temperature(int32_t adc_T);
static void bmp280_read_calibration_data();

int bmp280_init() 
{
    uint8_t id = 0;
    int timeout = 0;
    do{
        vTaskDelay(DELAY_REQUEST);
        CHECK_AND_RET_ERR(I2C_read(BMP280_ADDR, BMP280_REG_ID, &id, 1));
        if(id == BMP280_EXPECTED_ID){
            break;
        }
        if(timeout >= TIMEOUT_REQUEST){
            return ESP_ERR_TIMEOUT;
        }
        timeout += DELAY_REQUEST;
    }while(1);

    CHECK_AND_RET_ERR(I2C_write(BMP280_ADDR, BMP280_REG_CTRL_MEAS, 0x27)); // Normal mode, Oversampling Temp x1, Pressure x1
    CHECK_AND_RET_ERR(I2C_write(BMP280_ADDR, BMP280_REG_CONFIG, 0xA0)); // Standby 0.5ms, filter off
    bmp280_read_calibration_data();
    return ESP_OK;
}

static void bmp280_read_calibration_data() 
{
    uint8_t calib[24];
    I2C_read(BMP280_ADDR, BMP280_REG_READ_CALIB_DATA, calib, sizeof(calib));
    calib_data.dig_T1 = (calib[1] << 8) | calib[0];
    calib_data.dig_T2 = (int16_t)((calib[3] << 8) | calib[2]);
    calib_data.dig_T3 = (int16_t)((calib[5] << 8) | calib[4]);
    calib_data.dig_P1 = (uint16_t)((calib[7] << 8) | calib[6]);
    calib_data.dig_P2 = (int16_t)((calib[9] << 8) | calib[8]);
    calib_data.dig_P3 = (int16_t)((calib[11] << 8) | calib[10]);
    calib_data.dig_P4 = (int16_t)((calib[13] << 8) | calib[12]);
    calib_data.dig_P5 = (int16_t)((calib[15] << 8) | calib[14]);
    calib_data.dig_P6 = (int16_t)((calib[17] << 8) | calib[16]);
    calib_data.dig_P7 = (int16_t)((calib[19] << 8) | calib[18]);
    calib_data.dig_P8 = (int16_t)((calib[21] << 8) | calib[20]);
    calib_data.dig_P9 = (int16_t)((calib[23] << 8) | calib[22]);
}

static int32_t bmp280_compensate_temperature(int32_t adc_T) 
{
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)calib_data.dig_T1 << 1))) * ((int32_t)calib_data.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib_data.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib_data.dig_T1))) >> 12) * ((int32_t)calib_data.dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

static uint32_t bmp280_compensate_pressure(int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib_data.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib_data.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib_data.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib_data.dig_P3) >> 8) + ((var1 * (int64_t)calib_data.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib_data.dig_P1) >> 33;
    
    if (var1 == 0) {
        return 0; 
    }
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib_data.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib_data.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib_data.dig_P7) << 4);
    return (uint32_t)p;
}


int bmp280_read_data(float *temperature, float *pressure) 
{
    uint8_t data[6];
    int32_t adc_T, adc_P;

    CHECK_AND_RET_ERR(I2C_read(BMP280_ADDR, BMP280_REG_DATA, data, sizeof(data)));

    adc_P = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    adc_T = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);

    if (temperature) {
        *temperature = bmp280_compensate_temperature(adc_T) / 100.0;
    }
    if (pressure) {
        *pressure = bmp280_compensate_pressure(adc_P) / 25600.0; 
    }
    return ESP_OK;
}
