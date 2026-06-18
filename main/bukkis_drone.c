#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mpu6050.h" // Deine neue Komponente!

static const char *TAG = "MAIN";

void app_main(void) {
    ESP_LOGI(TAG, "Starte Drohnen-System...");

    // 1. Hardware initialisieren
    ESP_ERROR_CHECK(mpu6050_init());
    
    mpu6050_angles_t aktuelle_winkel;

    // 2. Hauptschleife
    while(1) {
        if (mpu6050_read_angles(&aktuelle_winkel) == ESP_OK) {
            ESP_LOGI(TAG, "Pitch: %6.1f° | Roll: %6.1f°", aktuelle_winkel.pitch, aktuelle_winkel.roll);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}