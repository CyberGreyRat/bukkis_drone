#pragma once

#include "esp_err.h"

// Ein sauberes Struct, um die Daten später an den Regler oder WebSocket zu übergeben
typedef struct {
    float pitch;
    float roll;
} mpu6050_angles_t;

// Öffentliche Funktionen
esp_err_t mpu6050_init(void);
esp_err_t mpu6050_read_angles(mpu6050_angles_t *angles);