#include "motor.h"
#include "driver/ledc.h"
#include "driver/gpio.h" // WICHTIG: Für die gpio_ Funktionen hinzugefügt
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MOTOR";

// Frequenz auf 16kHz erhöht für weicheren, leiseren Motorlauf
#define MOTOR_PWM_FREQ_HZ 16000 
#define MOTOR_PWM_RESOLUTION LEDC_TIMER_10_BIT

esp_err_t motor_init(void) {
    int pins[4] = {MOTOR_FL_PIN, MOTOR_FR_PIN, MOTOR_BL_PIN, MOTOR_BR_PIN};

    // 1. SICHERHEITS-SCHRITT: Pins explizit auf 0 (GND) ziehen, bevor PWM übernimmt.
    // Das verhindert das "Vollgas" beim Einschalten des ESP32.
    for (int i = 0; i < 4; i++) {
        gpio_reset_pin(pins[i]);
        gpio_set_direction(pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(pins[i], 0);
    }

    // 2. Timer konfigurieren
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = MOTOR_PWM_RESOLUTION,
        .freq_hz          = MOTOR_PWM_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // 3. Kanäle konfigurieren
    for (int i = 0; i < 4; i++) {
        ledc_channel_config_t ledc_channel = {
            .speed_mode     = LEDC_LOW_SPEED_MODE,
            .channel        = (ledc_channel_t)i,
            .timer_sel      = LEDC_TIMER_0,
            .intr_type      = LEDC_INTR_DISABLE,
            .gpio_num       = pins[i],
            .duty           = 0, // Garantiert 0% beim Start
            .hpoint         = 0
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    }
    
    ESP_LOGI(TAG, "Motoren (PWM) sicher initialisiert.");
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
    ESP_LOGI(TAG, "Starte sanfte Motor-Testsequenz (Ramp-Up bis 15%)...");
    
    for (int i = 0; i < 4; i++) {
        ESP_LOGI(TAG, "Teste Motor %d...", i);
        
        // Sanftes Hochfahren von 0 auf 15% in 1%-Schritten
        for (float speed = 0.0f; speed <= 15.0f; speed += 1.0f) {
            motor_set_speed((motor_id_t)i, speed);
            vTaskDelay(pdMS_TO_TICKS(50)); // 50 Millisekunden warten pro Prozentpunkt
        }
        
        // Für eine halbe Sekunde auf 15% halten
        vTaskDelay(pdMS_TO_TICKS(500)); 
        
        // Motor sofort abschalten
        motor_set_speed((motor_id_t)i, 0.0f);
        
        // Kurze Pause, bevor der nächste Motor an der Reihe ist
        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
    
    ESP_LOGI(TAG, "Testsequenz beendet. Alle Motoren sind aus.");
}