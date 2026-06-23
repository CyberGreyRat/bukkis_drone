#pragma once
#include "esp_err.h"

// Pins für die 4 Motoren
#define MOTOR_FL_PIN 13 // Vorne Links
#define MOTOR_FR_PIN 32 // Vorne Rechts
#define MOTOR_BL_PIN 14 // Hinten Links
#define MOTOR_BR_PIN 27 // Hinten Rechts

typedef enum {
    MOTOR_FL = 0,
    MOTOR_FR,
    MOTOR_BL,
    MOTOR_BR
} motor_id_t;

esp_err_t motor_init(void);
esp_err_t motor_set_speed(motor_id_t motor, float speed_percent);


void motor_test_sequence(void);