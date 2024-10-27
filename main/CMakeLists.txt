set(srcs "esp_hid_host_main.c"
         "esp_hid_gap.c"
         "esp_mqtt5_custom.c")
set(include_dirs ".")

idf_component_register(SRCS "tp_read_main.c" "esp_mqtt_custom.c" "app_main.c" "${srcs}"
                       INCLUDE_DIRS "${include_dirs}"
                       REQUIRES esp_hid
                       REQUIRES driver
                       PRIV_REQUIRES nvs_flash
                       mqtt)
