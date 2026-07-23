idf_component_register(SRCS "bukkis_drone.c"
                       INCLUDE_DIRS "."
                       REQUIRES bukkis_bt nvs_flash pid motor mpu6050)
                       