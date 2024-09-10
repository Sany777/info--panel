#include "i2c_adapter.h"

#include "driver/i2c.h"
#include "device_macro.h"
#include "device_common.h"

#include "esp_log.h"

static const char *TAG = "I2C";

  


void I2C_scan() 
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        ESP_LOGE(TAG, "Failed to create I2C command link");
        return;
    }

    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, addr << 1 | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Device found at address: 0x%02X", addr);
        } else if (err == ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "Timeout occurred while scanning address 0x%02X", addr);
        } else {
            ESP_LOGE(TAG, "Error occurred while scanning address 0x%02X: %s", addr, esp_err_to_name(err));
        }
        
        i2c_cmd_link_delete(cmd);
        cmd = i2c_cmd_link_create();
        if (cmd == NULL) {
            ESP_LOGE(TAG, "Failed to create new I2C command link");
            return;
        }
    }
    i2c_cmd_link_delete(cmd);
}

int I2C_init() 
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    CHECK_AND_RET_ERR(i2c_param_config(I2C_MASTER_NUM, &conf));
    CHECK_AND_RET_ERR(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));

    return ESP_OK;
}

int I2C_read(uint8_t addr, uint8_t reg_addr, uint8_t *data, unsigned len) 
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    CHECK_AND_RET_ERR(i2c_master_start(cmd));
    CHECK_AND_RET_ERR(i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true));
    CHECK_AND_RET_ERR(i2c_master_write_byte(cmd, reg_addr, true));
    CHECK_AND_RET_ERR(i2c_master_start(cmd));
    CHECK_AND_RET_ERR(i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true));
    CHECK_AND_RET_ERR(i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK));
    CHECK_AND_RET_ERR(i2c_master_stop(cmd));
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100);
    i2c_cmd_link_delete(cmd);
    return ret;
}


int I2C_write(uint8_t addr, uint8_t reg_addr, uint8_t data) 
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    CHECK_AND_RET_ERR(i2c_master_start(cmd));
    CHECK_AND_RET_ERR(i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true));
    CHECK_AND_RET_ERR(i2c_master_write_byte(cmd, reg_addr, true));
    CHECK_AND_RET_ERR(i2c_master_write_byte(cmd, data, true));
    CHECK_AND_RET_ERR(i2c_master_stop(cmd));
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100);
    i2c_cmd_link_delete(cmd);
    return ret;
}