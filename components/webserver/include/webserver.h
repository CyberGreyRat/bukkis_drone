#pragma once
#include "esp_err.h"

esp_err_t webserver_init(void);
void webserver_update_data(float pitch, float roll);