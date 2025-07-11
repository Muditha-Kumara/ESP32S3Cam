#include <stdio.h>
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
    ESP_LOGI(TAG, "HTTP server will start automatically when WiFi connects");

    // Main application loop - can add other tasks here
    while (1)
    {
        wifi_status_t status = wifi_get_status();

        if (status == WIFI_STATUS_CONNECTED)
        {
            if (wifi_is_http_server_running())
            {
                // Application is fully running
                // You can add other periodic tasks here
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
