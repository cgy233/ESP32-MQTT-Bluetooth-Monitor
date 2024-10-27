set(srcs "esp_hid_host_main.c"
         "esp_hid_gap.c"
        )
set(include_dirs ".")

idf_component_register(SRCS "door_sensor_driver.c" "tp_read_main.c" "esp_mqtt_custom.c" "app_main.c" "${srcs}"
                       INCLUDE_DIRS "${include_dirs}"
                       REQUIRES esp_hid
                       REQUIRES driver
                       PRIV_REQUIRES nvs_flash
                       mqtt)
