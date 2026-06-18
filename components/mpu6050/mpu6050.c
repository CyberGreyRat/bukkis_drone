#include <math.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "mpu6050.h"

#define I2C_MASTER_SCL_IO           22
#define I2C_MASTER_SDA_IO           21
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_TIMEOUT_MS       1000

#define MPU6050_ADDR                0x68
#define MPU6050_PWR_MGMT_1_REG      0x6B
#define MPU6050_ACCEL_XOUT_H_REG    0x3B

static const char *TAG = "MPU6050";

// Ref: SYS001 - MPU6050 I2C Kommunikation initialisieren
esp_err_t mpu6050_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) return err;
    
    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK) return err;

    // Sensor aufwecken
    uint8_t write_buf[2] = {MPU6050_PWR_MGMT_1_REG, 0x00};
    err = i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDR, write_buf, sizeof(write_buf), pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Initialisiert und aufgeweckt.");
    } else {
        ESP_LOGE(TAG, "Fehler beim Aufwecken!");
    }
    return err;
}

// Ref: SYS001 - Winkelberechnung aus Rohdaten
esp_err_t mpu6050_read_angles(mpu6050_angles_t *angles) {
    uint8_t reg = MPU6050_ACCEL_XOUT_H_REG;
    uint8_t data[6];

    esp_err_t err = i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDR, &reg, 1, data, 6, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    
    if (err == ESP_OK) {
        int16_t accel_x = (data[0] << 8) | data[1];
        int16_t accel_y = (data[2] << 8) | data[3];
        int16_t accel_z = (data[4] << 8) | data[5];

        angles->roll  = atan2(accel_y, accel_z) * 180.0 / M_PI;
        angles->pitch = atan2(-accel_x, sqrt(accel_y * accel_y + accel_z * accel_z)) * 180.0 / M_PI;
    }
    return err;
}