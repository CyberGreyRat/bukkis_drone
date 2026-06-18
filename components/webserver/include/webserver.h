#pragma once
#include "esp_err.h"

// NEU: Struktur für die eingehenden Befehle vom Browser
typedef struct {
    float throttle;
    float pitch_setpoint;
    float roll_setpoint;
} web_controls_t;

esp_err_t webserver_init(void);

// GEÄNDERT: Wir übergeben jetzt auch die 4 Motorwerte an das Web-UI
void webserver_update_data(float pitch, float roll, float m_fl, float m_fr, float m_bl, float m_br);

// NEU: Die Main-Schleife holt sich hiermit die aktuellen Browser-Befehle
web_controls_t webserver_get_controls(void);