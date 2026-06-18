#include "motor.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MOTOR";

#define MOTOR_PWM_FREQ_HZ 5000
#define MOTOR_PWM_RESOLUTION LEDC_TIMER_10_BIT

esp_err_t motor_init(void) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = MOTOR_PWM_RESOLUTION,
        .freq_hz          = MOTOR_PWM_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    int pins[4] = {MOTOR_FL_PIN, MOTOR_FR_PIN, MOTOR_BL_PIN, MOTOR_BR_PIN};
    
    for (int i = 0; i < 4; i++) {
        ledc_channel_config_t ledc_channel = {
            .speed_mode     = LEDC_LOW_SPEED_MODE,
            .channel        = (ledc_channel_t)i,
            .timer_sel      = LEDC_TIMER_0,
            .intr_type      = LEDC_INTR_DISABLE,
            .gpio_num       = pins[i],
            .duty           = 0,
            .hpoint         = 0
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    }
    
    ESP_LOGI(TAG, "Motoren (PWM) initialisiert.");
    return ESP_OK;
}

esp_err_t motor_set_speed(motor_id_t motor, float speed_percent) {
    if (speed_percent < 0.0f) speed_percent = 0.0f;
    if (speed_percent > 100.0f) speed_percent = 100.0f;

    uint32_t duty = (uint32_t)((speed_percent / 100.0f) * 1023.0f);
    
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)motor, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)motor));
    
    return ESP_OK;
}

void motor_test_sequence(void) {
    ESP_LOGI(TAG, "Starte Motor-Testsequenz...");
    for (int i = 0; i < 4; i++) {
        motor_set_speed((motor_id_t)i, 20.0f);
        vTaskDelay(pdMS_TO_TICKS(250));
        motor_set_speed((motor_id_t)i, 0.0f);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    ESP_LOGI(TAG, "Motoren bereit.");
}