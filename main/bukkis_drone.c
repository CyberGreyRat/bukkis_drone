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

float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void app_main(void) {
    ESP_LOGI(TAG, "Starte Drohnen-System...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(mpu6050_init());
    ESP_ERROR_CHECK(webserver_init());
    ESP_ERROR_CHECK(motor_init());

    motor_test_sequence();

    pid_ctrl_t pid_pitch, pid_roll;
    pid_init(&pid_pitch, 0.8f, 0.0f, 0.2f, 30.0f);
    pid_init(&pid_roll, 0.8f, 0.0f, 0.2f, 30.0f);

    mpu6050_angles_t aktuelle_winkel;
    TickType_t last_time = xTaskGetTickCount();

    while(1) {
        TickType_t now = xTaskGetTickCount();
        float dt = (now - last_time) / (float)configTICK_RATE_HZ;
        last_time = now;

        if (mpu6050_read_angles(&aktuelle_winkel) == ESP_OK) {
            web_controls_t cmd = webserver_get_controls();
            float m_fl = 0, m_fr = 0, m_bl = 0, m_br = 0;

            if (cmd.throttle > 2.0f) {
                float out_pitch = pid_compute(&pid_pitch, cmd.pitch_setpoint, aktuelle_winkel.pitch, dt);
                float out_roll = pid_compute(&pid_roll, cmd.roll_setpoint, aktuelle_winkel.roll, dt);

                m_fl = cmd.throttle - out_pitch + out_roll;
                m_fr = cmd.throttle - out_pitch - out_roll;
                m_bl = cmd.throttle + out_pitch + out_roll;
                m_br = cmd.throttle + out_pitch - out_roll;

                m_fl = clamp(m_fl, 0.0f, 100.0f);
                m_fr = clamp(m_fr, 0.0f, 100.0f);
                m_bl = clamp(m_bl, 0.0f, 100.0f);
                m_br = clamp(m_br, 0.0f, 100.0f);

                motor_set_speed(MOTOR_FL, m_fl);
                motor_set_speed(MOTOR_FR, m_fr);
                motor_set_speed(MOTOR_BL, m_bl);
                motor_set_speed(MOTOR_BR, m_br);
            } else {
                motor_set_speed(MOTOR_FL, 0.0f); motor_set_speed(MOTOR_FR, 0.0f);
                motor_set_speed(MOTOR_BL, 0.0f); motor_set_speed(MOTOR_BR, 0.0f);
                
                pid_pitch.integral = 0; pid_pitch.prev_error = 0;
                pid_roll.integral = 0;  pid_roll.prev_error = 0;
            }

            webserver_update_data(aktuelle_winkel.pitch, aktuelle_winkel.roll, m_fl, m_fr, m_bl, m_br);
        }
        vTaskDelay(pdMS_TO_TICKS(20)); 
    }
}