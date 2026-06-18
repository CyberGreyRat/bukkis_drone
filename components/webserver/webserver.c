#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "webserver.h"

static const char *TAG = "WEBSERVER";

// Globale Variablen für den Datenaustausch
static float current_pitch = 0.0;
static float current_roll = 0.0;

// Das Frontend (HTML + JavaScript)
static const char* index_html = 
    "<!DOCTYPE html><html><head><title>Drohnen Dashboard</title>"
    "<style>body{font-family:Arial;text-align:center;background:#222;color:#fff;margin-top:50px;}"
    ".val{font-size:4em;font-weight:bold;color:#0f0;}</style></head><body>"
    "<h1>Flight Controller</h1>"
    "<div>Pitch: <span id='pitch' class='val'>0.0</span>&deg;</div>"
    "<div>Roll: <span id='roll' class='val'>0.0</span>&deg;</div>"
    "<script>"
    "setInterval(() => {"
    "  fetch('/data').then(r => r.json()).then(data => {"
    "    document.getElementById('pitch').innerText = data.pitch.toFixed(1);"
    "    document.getElementById('roll').innerText = data.roll.toFixed(1);"
    "  });"
    "}, 100);" // 10 mal pro Sekunde abfragen
    "</script></body></html>";

// Update-Funktion (wird von main.c aufgerufen)
void webserver_update_data(float pitch, float roll) {
    current_pitch = pitch;
    current_roll = roll;
}

// Handler für die Webseite ("/")
static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
}

// Handler für die Sensordaten ("/data")
static esp_err_t data_handler(httpd_req_t *req) {
    char json_response[100];
    snprintf(json_response, sizeof(json_response), "{\"pitch\": %f, \"roll\": %f}", current_pitch, current_roll);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
}

// Ref: SYS002 - WLAN AP und Webserver starten
esp_err_t webserver_init(void) {
    // 1. WLAN Initialisieren (Access Point)
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "Bukkis_Drone",
            .ssid_len = strlen("Bukkis_Drone"),
            .channel = 1,
            .password = "12345678",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK
        },
    };
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();
    ESP_LOGI(TAG, "WLAN AP gestartet. Verbinde dich mit 'Bukkis_Drone', PW: '12345678'");

    // 2. HTTP Server starten
    httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &server_config) == ESP_OK) {
        httpd_uri_t index_uri = { .uri = "/", .method = HTTP_GET, .handler = index_handler };
        httpd_register_uri_handler(server, &index_uri);

        httpd_uri_t data_uri = { .uri = "/data", .method = HTTP_GET, .handler = data_handler };
        httpd_register_uri_handler(server, &data_uri);
        
        ESP_LOGI(TAG, "Webserver läuft auf http://192.168.4.1");
    }
    return ESP_OK;
}