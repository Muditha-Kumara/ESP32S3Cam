idf_component_register(SRCS "video_stream.c" "camera_init.c" "ESP32S3Cam.c" "wifi_init.c" "ota_update.c" "http_server.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash esp_http_server esp_wifi esp_event esp_netif log app_update esp_partition esp32-camera esp_psram)
