#include <stdio.h>
#include "wifi_init.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "camera_init.h"
#include "video_stream.h"
#include "http_server.h"

static const char *TAG = "main";

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Starting application...");

    // Initialize camera first
    ESP_LOGI(TAG, "Initializing camera...");
    esp_err_t camera_ret = camera_init();
    if (camera_ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera initialization failed: %s", esp_err_to_name(camera_ret));
        ESP_LOGE(TAG, "Application will continue without camera functionality");
    }
    else
    {
        ESP_LOGI(TAG, "Camera initialized successfully");
    }

    // Initialize WiFi as a background task (non-blocking)
    wifi_init_task();
    ESP_LOGI(TAG, "WiFi initialization started in background");

    while (1)
    {
        wifi_status_t status = wifi_get_status();

        if (status == WIFI_STATUS_CONNECTED)
        {
            if (wifi_is_http_server_running())
            {
                // Initialize video streaming if camera is ready and streaming not started
                if (camera_get_status() == CAM_STATUS_READY &&
                    video_stream_get_status() != VIDEO_STREAM_RUNNING)
                {

                    httpd_handle_t server_handle = http_server_get_handle();
                    if (server_handle != NULL)
                    {
                        esp_err_t stream_ret = video_stream_init(server_handle);
                        if (stream_ret == ESP_OK)
                        {
                            ESP_LOGI(TAG, "Video streaming initialized successfully");
                            ESP_LOGI(TAG, "Access the camera stream at: http://<device_ip>/");
                        }
                        else
                        {
                            ESP_LOGE(TAG, "Failed to initialize video streaming: %s", esp_err_to_name(stream_ret));
                        }
                    }
                }
            }
        }
        else if (status == WIFI_STATUS_CONNECTING)
        {
            ESP_LOGI(TAG, "WiFi connecting...");
        }
        else
        {
            ESP_LOGI(TAG, "WiFi disconnected, waiting for connection...");
        }

        vTaskDelay(pdMS_TO_TICKS(30000)); // Check every 30 seconds
    }
}
