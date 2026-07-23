#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mpu6050.h"
#include "pid.h"
#include "motor.h"
#include "bukkis_bt.h"

static const char *TAG = "MAIN";

// Hilfsfunktion zum Begrenzen der Motorwerte
float clamp(float value, float min, float max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Hilfsfunktion zum Umrechnen (Mappen) der Controller-Werte
float map_value(int value, int in_min, int in_max, float out_min, float out_max) {
    return (value - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}

void app_main(void)
{
    // 1. NVS initialisieren (Zwingend erforderlich für Bluetooth!)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Starte Drohnen-System...");

    // 2. Hardware und Subsysteme initialisieren
    ESP_ERROR_CHECK(mpu6050_init());
    ESP_ERROR_CHECK(motor_init());
    bluetooth_init(); // <-- Bluetooth starten

    motor_test_sequence();

    // 3. PID Regler initialisieren
    pid_ctrl_t pid_pitch, pid_roll;
    pid_init(&pid_pitch, 1.2f, 0.0f, 0.5f, 20.0f, 40.0f);
    pid_init(&pid_roll, 1.2f, 0.0f, 0.5f, 20.0f, 40.0f);

    mpu6050_angles_t aktuelle_winkel;

    // 4. Zeit-Setup für exakt 100 Hz (10ms)
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t loop_delay = pdMS_TO_TICKS(10);
    const float dt = 0.010f;

    while (1)
    {
        // Sensoren lesen & Flug berechnen
        if (mpu6050_read_angles(&aktuelle_winkel) == ESP_OK)
        {
            float m_fl = 0, m_fr = 0, m_bl = 0, m_br = 0;

            // 1. Controller-Daten abgreifen und umrechnen
            // Stick nach Oben = negativ (-512) bei Bluepad32. 
            // Wir mappen -20 bis -512 auf 0 bis 100% (mit kleiner Deadzone).
            float throttle_cmd = 0.0f;
            if (bt_throttle < -20) { 
                throttle_cmd = map_value(bt_throttle, -20, -512, 0.0f, 100.0f);
            }

            // Stick X/Y Mappen auf maximal +-30 Grad Neigung (mit kleiner Deadzone)
            float pitch_cmd = 0.0f;
            float roll_cmd = 0.0f;
            if (bt_pitch > 20 || bt_pitch < -20) pitch_cmd = map_value(bt_pitch, -512, 511, -30.0f, 30.0f);
            if (bt_roll > 20 || bt_roll < -20)   roll_cmd  = map_value(bt_roll, -512, 511, -30.0f, 30.0f);

            // 2. Motorsteuerung anwenden
            if (throttle_cmd > 2.0f)
            {
                // PID Berechnungen mit Controller-Inputs
                float out_pitch = pid_compute(&pid_pitch, pitch_cmd, aktuelle_winkel.pitch, dt);
                float out_roll  = pid_compute(&pid_roll, roll_cmd, aktuelle_winkel.roll, dt);

                // Mixer
                m_fl = throttle_cmd + out_pitch - out_roll;
                m_fr = throttle_cmd + out_pitch + out_roll;
                m_bl = throttle_cmd - out_pitch - out_roll;
                m_br = throttle_cmd - out_pitch + out_roll;

                // Begrenzen
                m_fl = clamp(m_fl, 0.0f, 100.0f);
                m_fr = clamp(m_fr, 0.0f, 100.0f);
                m_bl = clamp(m_bl, 0.0f, 100.0f);
                m_br = clamp(m_br, 0.0f, 100.0f);

                // Motoren ansteuern
                motor_set_speed(MOTOR_FL, m_fl);
                motor_set_speed(MOTOR_FR, m_fr);
                motor_set_speed(MOTOR_BL, m_bl);
                motor_set_speed(MOTOR_BR, m_br);
            }
            else
            {
                // Gas ist weg -> Motoren sofort aus
                motor_set_speed(MOTOR_FL, 0.0f);
                motor_set_speed(MOTOR_FR, 0.0f);
                motor_set_speed(MOTOR_BL, 0.0f);
                motor_set_speed(MOTOR_BR, 0.0f);

                // PID zurücksetzen (Anti-Windup)
                pid_reset(&pid_pitch);
                pid_reset(&pid_roll);
            }
        }

        // Präzises Warten
        vTaskDelayUntil(&last_wake_time, loop_delay);
    }
}