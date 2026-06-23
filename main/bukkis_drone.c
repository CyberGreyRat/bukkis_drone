#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mpu6050.h"
#include "webserver.h"
#include "pid.h"
#include "motor.h"

static const char *TAG = "MAIN";

// Das Flag vom Webserver, damit wir es in der Schleife abfragen können
extern volatile bool start_motor_test_flag;

float clamp(float value, float min, float max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starte Drohnen-System...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(mpu6050_init());
    ESP_ERROR_CHECK(webserver_init());
    ESP_ERROR_CHECK(motor_init());

    // Optional: Die Motoren piepsen/zucken beim Hochfahren kurz als Status-Check
    motor_test_sequence();

    // PID Regler initialisieren (kp, ki, kd, i_limit, out_limit)
    pid_ctrl_t pid_pitch, pid_roll;
    pid_init(&pid_pitch, 1.2f, 0.0f, 0.5f, 20.0f, 40.0f);
    pid_init(&pid_roll, 1.2f, 0.0f, 0.5f, 20.0f, 40.0f);

    mpu6050_angles_t aktuelle_winkel;

    // Zeit-Setup für exakt 100 Hz (10ms)
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t loop_delay = pdMS_TO_TICKS(10);
    const float dt = 0.010f;

    while (1)
    {
        // 1. Webserver Button abfragen
        if (start_motor_test_flag)
        {
            start_motor_test_flag = false;        // Direkt wieder scharfschalten
            motor_test_sequence();                // Test läuft durch (blockierend)
            last_wake_time = xTaskGetTickCount(); // Zeit-Anker neu setzen!
        }

        // 2. Sensoren lesen & Flug berechnen
        if (mpu6050_read_angles(&aktuelle_winkel) == ESP_OK)
        {
            web_controls_t cmd = webserver_get_controls();
            float m_fl = 0, m_fr = 0, m_bl = 0, m_br = 0;

            if (cmd.throttle > 2.0f)
            {
                // PID Berechnungen
                float out_pitch = pid_compute(&pid_pitch, cmd.pitch_setpoint, aktuelle_winkel.pitch, dt);
                float out_roll = pid_compute(&pid_roll, cmd.roll_setpoint, aktuelle_winkel.roll, dt);

                // Mixer
                //m_fl = cmd.throttle - out_pitch + out_roll;
                //m_fr = cmd.throttle - out_pitch - out_roll;
                //m_bl = cmd.throttle + out_pitch + out_roll;
                //m_br = cmd.throttle + out_pitch - out_roll;

                // Mixer (Vorzeichen invertiert)
                m_fl = cmd.throttle + out_pitch - out_roll;
                m_fr = cmd.throttle + out_pitch + out_roll;
                m_bl = cmd.throttle - out_pitch - out_roll;
                m_br = cmd.throttle - out_pitch + out_roll;

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

            // Telemetrie an Webserver übergeben
            webserver_update_data(aktuelle_winkel.pitch, aktuelle_winkel.roll, m_fl, m_fr, m_bl, m_br);
        }

        // 3. Präzises Warten
        vTaskDelayUntil(&last_wake_time, loop_delay);
    }
}