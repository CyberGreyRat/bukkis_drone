#include "bukkis_bt.h"
#include <uni.h>
#include <btstack_port_esp32.h> // <-- NEU: Für btstack_init()
#include <btstack_run_loop.h>   // <-- NEU: Für btstack_run_loop_execute()
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BT_CTRL";

// Globale Variablen für den Controller
int32_t bt_throttle = 0;
int32_t bt_yaw = 0;
int32_t bt_pitch = 0;
int32_t bt_roll = 0;

// --- NEUE DUMMY FUNKTIONEN ---
static void my_on_init_complete(void) {
    // Wird aufgerufen, wenn BT bereit ist
    uni_bt_start_scanning_and_autoconnect_unsafe();
    uni_bt_allow_incoming_connections(true); //wirfst den esp raus
}



static const uni_property_t* my_get_property(uni_property_idx_t key) {
    // Gibt einfach "Nichts" (NULL) zurück, da wir keine Properties brauchen
    return NULL; 
}

// -----------------------------

static void my_on_device_connected(uni_hid_device_t *d) {
    ESP_LOGI(TAG, "Controller verbunden!");
}

static void my_on_device_disconnected(uni_hid_device_t *d) {
    ESP_LOGI(TAG, "Controller getrennt! Setze Werte auf 0.");
    bt_throttle = 0; bt_yaw = 0; bt_pitch = 0; bt_roll = 0;
}

static void my_on_controller_data(uni_hid_device_t *d, uni_controller_t *ctl) {
    bt_throttle = ctl->gamepad.axis_ry; 
    bt_yaw      = ctl->gamepad.axis_rx; 
    bt_pitch    = ctl->gamepad.axis_y;  
    bt_roll     = ctl->gamepad.axis_x;  
}

static void my_init(int argc, const char** argv) {}

static uni_error_t my_on_device_ready(uni_hid_device_t* d) {
    return UNI_ERROR_SUCCESS; 
}

// ---------------------------------------------------------
// NEU: Der FreeRTOS Task für Bluepad32
// ---------------------------------------------------------
static void bluepad_task(void *pvParameters) {
  
    ESP_LOGI(TAG, "Starte Bluepad32 im Hintergrund-Task...");
vTaskDelay(pdMS_TO_TICKS(10000));
    // 1. Zwingend VOR uni_init: BTstack Hardware & Speicher hochfahren
    btstack_init();

    // 2. Deine Plattform registrieren
    static struct uni_platform platform = {0};
    platform.name = "bukkis_drone";
    platform.init = my_init;
    platform.on_init_complete = my_on_init_complete;
    platform.on_device_ready = my_on_device_ready;
    platform.on_device_connected = my_on_device_connected;
    platform.on_device_disconnected = my_on_device_disconnected;
    platform.on_controller_data = my_on_controller_data;
    platform.get_property = my_get_property;                 
    uni_platform_set_custom(&platform);

    // 3. Bluepad-Kern starten
    uni_init(0, NULL);

    // 4. Endlosschleife für Bluetooth starten! (Kehrt niemals zurück)
    btstack_run_loop_execute();
    
    // Wird im Idealfall nie erreicht
    vTaskDelete(NULL); 
}

// ---------------------------------------------------------
// Wird von deiner app_main() aufgerufen
// ---------------------------------------------------------
void bluetooth_init(void) {
    // Starte den BT-Task. 8192 Bytes RAM, Priorität 5, auf keinem festen Core gepinnt
    xTaskCreate(bluepad_task, "bluepad_task", 8192, NULL, 5, NULL);
}