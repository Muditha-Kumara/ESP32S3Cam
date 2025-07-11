#include <stdio.h>
#include "ota_update.h"
#include "http_server.h"
#include "custom_handlers.h"
#include "wifi_init.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

    // Initialize WiFi as a background task (non-blocking)
    wifi_init_task();
    ESP_LOGI(TAG, "WiFi initialization started in background");

    ESP_LOGI(TAG, "Application ready - WiFi connecting in background");

    // Wait for WiFi connection before starting HTTP server and services
    bool http_server_started = false;
    while (1)
    {
        wifi_status_t status = wifi_get_status();

        // Start HTTP server and services only once when WiFi connects
        if (status == WIFI_STATUS_CONNECTED && !http_server_started)
        {
            char ip_str[16];
            if (wifi_get_ip_address(ip_str, sizeof(ip_str)))
            {
                ESP_LOGI(TAG, "WiFi Connected (IP: %s)", ip_str);

                // Start HTTP server
                esp_err_t ret = http_server_init();
                if (ret == ESP_OK)
                {
                    ESP_LOGI(TAG, "HTTP server started successfully");

                    // Initialize OTA functionality
                    ret = ota_init();
                    if (ret == ESP_OK)
                    {
                        ESP_LOGI(TAG, "OTA service initialized");
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Failed to initialize OTA service");
                    }

                    // Initialize custom handlers for future development
                    ret = custom_handlers_init();
                    if (ret == ESP_OK)
                    {
                        ESP_LOGI(TAG, "Custom handlers initialized");
                        ESP_LOGI(TAG, "Web interface available at: http://%s", ip_str);
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Failed to initialize custom handlers");
                    }

                    http_server_started = true;
                }
                else
                {
                    ESP_LOGE(TAG, "Failed to start HTTP server");
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10000)); // Check every 10 seconds
    }
}
