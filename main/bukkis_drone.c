#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"       // NEU: Für WLAN benötigt
#include "mpu6050.h"
#include "webserver.h"       // NEU: Dein neues Modul

static const char *TAG = "MAIN";

void app_main(void) {
    ESP_LOGI(TAG, "Starte Drohnen-System...");

    // 1. NVS (Flash) initialisieren - PFLICHT für WLAN!
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Hardware & Netzwerk initialisieren
    ESP_ERROR_CHECK(mpu6050_init());
    ESP_ERROR_CHECK(webserver_init());
    
    mpu6050_angles_t aktuelle_winkel;

    // 3. Hauptschleife (Jetzt mit Web-Update)
    while(1) {
        if (mpu6050_read_angles(&aktuelle_winkel) == ESP_OK) {
            // Daten an den Webserver übergeben
            webserver_update_data(aktuelle_winkel.pitch, aktuelle_winkel.roll);
            
            // Optional: Konsolenausgabe verlangsamen, damit sie nicht so schnell scrollt
            // ESP_LOGI(TAG, "Pitch: %6.1f° | Roll: %6.1f°", aktuelle_winkel.pitch, aktuelle_winkel.roll);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // 10 Hz Loop
    }
}