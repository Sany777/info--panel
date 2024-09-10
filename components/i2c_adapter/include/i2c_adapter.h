#ifndef I2C_MODULE_H
#define I2C_MODULE_H



#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"


#define I2C_MASTER_NUM              I2C_NUM_0 
#define I2C_MASTER_FREQ_HZ          100000   
#define I2C_MASTER_TIMEOUT_MS       1000

#define NO_DATA_REG 0xffff

void I2C_scan() ;

int I2C_init();
int I2C_read(uint8_t addr, uint8_t reg_addr, uint8_t *data, unsigned len);
int I2C_write(uint8_t addr, uint8_t reg_addr, uint8_t data);

#ifdef __cplusplus
}
#endif



#endif



