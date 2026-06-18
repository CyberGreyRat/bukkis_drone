#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"

// I2C Konfiguration
#define I2C_MASTER_SCL_IO           22
#define I2C_MASTER_SDA_IO           21
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_TIMEOUT_MS       1000

// MPU6050 Register
#define MPU6050_ADDR                0x68
#define MPU6050_PWR_MGMT_1_REG      0x6B
#define MPU6050_ACCEL_XOUT_H_REG    0x3B

static const char *TAG = "DROHNE";

// I2C Bus initialisieren
static esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

// MPU6050 aus dem Sleep-Modus aufwecken
static void mpu6050_wake_up(void) {
    uint8_t write_buf[2] = {MPU6050_PWR_MGMT_1_REG, 0x00}; // Schreibe 0x00 in das Power Management Register
    i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDR, write_buf, sizeof(write_buf), pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    ESP_LOGI(TAG, "MPU6050 aufgeweckt!");
}

// Sensordaten lesen und Winkel berechnen
static void read_and_print_angles(void) {
    uint8_t reg = MPU6050_ACCEL_XOUT_H_REG;
    uint8_t data[6]; // Wir brauchen 6 Bytes (X, Y, Z jeweils High und Low Byte)

    // Sende die Startadresse (0x3B) und lies direkt 6 Bytes aus
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDR, &reg, 1, data, 6, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));

    if (ret == ESP_OK) {
        // Bytes zu 16-Bit Integer zusammenbauen
        int16_t accel_x = (data[0] << 8) | data[1];
        int16_t accel_y = (data[2] << 8) | data[3];
        int16_t accel_z = (data[4] << 8) | data[5];

        // Winkel in Grad berechnen (Pitch = Nicken, Roll = Rollen)
        float roll  = atan2(accel_y, accel_z) * 180.0 / M_PI;
        float pitch = atan2(-accel_x, sqrt(accel_y * accel_y + accel_z * accel_z)) * 180.0 / M_PI;

        ESP_LOGI(TAG, "Pitch: %6.1f° | Roll: %6.1f°", pitch, roll);
    } else {
        ESP_LOGE(TAG, "Verbindung zum Sensor verloren!");
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(i2c_master_init());
    mpu6050_wake_up();
    
    // Die Echtzeit-Schleife (10 mal pro Sekunde)
    while(1) {
        read_and_print_angles();
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}