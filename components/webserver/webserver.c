#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "webserver.h"

static const char *TAG = "WEBSERVER";

static float current_pitch = 0.0;
static float current_roll = 0.0;
static float cur_m_fl = 0, cur_m_fr = 0, cur_m_bl = 0, cur_m_br = 0;

static web_controls_t current_controls = {0, 0, 0};

void webserver_update_data(float pitch, float roll, float m_fl, float m_fr, float m_bl, float m_br) {
    current_pitch = pitch;
    current_roll = roll;
    cur_m_fl = m_fl;
    cur_m_fr = m_fr;
    cur_m_bl = m_bl;
    cur_m_br = m_br;
}

web_controls_t webserver_get_controls(void) {
    return current_controls;
}

static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    const char* fallback_html = "<html><body><h1>Bukkis Drone Server laeuft</h1></body></html>";
    return httpd_resp_send(req, fallback_html, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t data_handler(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); 
    
    char buf[100];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        char param[32];
        if (httpd_query_key_value(buf, "t", param, sizeof(param)) == ESP_OK) current_controls.throttle = atof(param);
        if (httpd_query_key_value(buf, "p", param, sizeof(param)) == ESP_OK) current_controls.pitch_setpoint = atof(param);
        if (httpd_query_key_value(buf, "r", param, sizeof(param)) == ESP_OK) current_controls.roll_setpoint = atof(param);
    }

    char json_response[200];
    snprintf(json_response, sizeof(json_response), 
             "{\"pitch\":%.1f,\"roll\":%.1f,\"fl\":%.1f,\"fr\":%.1f,\"bl\":%.1f,\"br\":%.1f}", 
             current_pitch, current_roll, cur_m_fl, cur_m_fr, cur_m_bl, cur_m_br);
             
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
}

esp_err_t webserver_init(void) {
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
    ESP_LOGI(TAG, "WLAN AP bereit.");

    httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &server_config) == ESP_OK) {
        httpd_uri_t index_uri = { .uri = "/", .method = HTTP_GET, .handler = index_handler };
        httpd_register_uri_handler(server, &index_uri);

        httpd_uri_t data_uri = { .uri = "/data", .method = HTTP_GET, .handler = data_handler };
        httpd_register_uri_handler(server, &data_uri);
    }
    return ESP_OK;
}