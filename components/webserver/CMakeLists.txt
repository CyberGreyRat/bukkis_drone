idf_component_register(SRCS "webserver.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_wifi esp_http_server nvs_flash log)