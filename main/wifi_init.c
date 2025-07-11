#include "wifi_init.h"
#include "wifi_config.h"
#include "http_server.h"
#include "ota_update.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "wifi_init";
static EventGroupHandle_t s_wifi_event_group;
static volatile wifi_status_t s_wifi_status = WIFI_STATUS_DISCONNECTED;
static esp_netif_t *s_sta_netif = NULL;
static bool s_http_server_started = false;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_RECONNECT_DELAY_MS 5000 // 5 seconds
#define WIFI_TASK_STACK_SIZE 4096
#define WIFI_TASK_PRIORITY 5

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "WiFi started, attempting to connect...");
        s_wifi_status = WIFI_STATUS_CONNECTING;
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGW(TAG, "WiFi disconnected (reason: %d), will retry in %d seconds",
                 event->reason, WIFI_RECONNECT_DELAY_MS / 1000);
        s_wifi_status = WIFI_STATUS_DISCONNECTED;

        // Stop HTTP server when WiFi disconnects
        if (s_http_server_started)
        {
            ESP_LOGI(TAG, "Stopping HTTP server due to WiFi disconnection");
            http_server_stop();
            s_http_server_started = false;
        }

        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_wifi_status = WIFI_STATUS_CONNECTED;

        // Start HTTP server and services when WiFi connects
        if (!s_http_server_started)
        {
            ESP_LOGI(TAG, "Starting HTTP server...");
            esp_err_t ret = http_server_init();
            if (ret == ESP_OK)
            {
                ESP_LOGI(TAG, "HTTP server started successfully");
                s_http_server_started = true;

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
            }
            else
            {
                ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
            }
        }

        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_task(void *pvParameters)
{
    ESP_LOGI(TAG, "WiFi task started");

    while (1)
    {
        if (s_wifi_status == WIFI_STATUS_DISCONNECTED)
        {
            ESP_LOGI(TAG, "Attempting WiFi connection...");
            s_wifi_status = WIFI_STATUS_CONNECTING;

            esp_err_t ret = esp_wifi_connect();
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "WiFi connect failed: %s", esp_err_to_name(ret));
                s_wifi_status = WIFI_STATUS_DISCONNECTED;
            }
        }

        // Wait for connection result or timeout
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                               pdTRUE,  // Clear bits after waiting
                                               pdFALSE, // Wait for any bit
                                               pdMS_TO_TICKS(WIFI_RECONNECT_DELAY_MS));

        if (bits & WIFI_CONNECTED_BIT)
        {
            ESP_LOGI(TAG, "WiFi connection successful");
            // Stay connected, wait for disconnection event
            xEventGroupWaitBits(s_wifi_event_group, WIFI_FAIL_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
        }
        else if (bits & WIFI_FAIL_BIT)
        {
            ESP_LOGW(TAG, "WiFi connection failed, retrying in %d seconds", WIFI_RECONNECT_DELAY_MS / 1000);
        }
        else
        {
            // Timeout occurred
            ESP_LOGW(TAG, "WiFi connection timeout, retrying...");
            s_wifi_status = WIFI_STATUS_DISCONNECTED;
        }

        // Small delay before next attempt if not connected
        if (s_wifi_status != WIFI_STATUS_CONNECTED)
        {
            vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second delay before retry
        }
    }
}

void wifi_init_task(void)
{
    // Initialize event group
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL)
    {
        ESP_LOGE(TAG, "Failed to create event group");
        return;
    }

    // Initialize network interface
    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(ret));
        return;
    }

    s_sta_netif = esp_netif_create_default_wifi_sta();
    if (s_sta_netif == NULL)
    {
        ESP_LOGE(TAG, "esp_netif_create_default_wifi_sta failed");
        return;
    }

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(ret));
        return;
    }

    // Register event handlers
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ret = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                              &wifi_event_handler, NULL, &instance_any_id);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_event_handler_instance_register WIFI_EVENT failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                              &wifi_event_handler, NULL, &instance_got_ip);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_event_handler_instance_register IP_EVENT failed: %s", esp_err_to_name(ret));
        return;
    }

    // Configure WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
    };

    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(ret));
        return;
    }

    // Create WiFi management task
    BaseType_t task_ret = xTaskCreate(wifi_task, "wifi_task", WIFI_TASK_STACK_SIZE,
                                      NULL, WIFI_TASK_PRIORITY, NULL);
    if (task_ret != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create WiFi task");
        return;
    }

    ESP_LOGI(TAG, "WiFi initialization completed - running as background task");
}

wifi_status_t wifi_get_status(void)
{
    return s_wifi_status;
}

bool wifi_get_ip_address(char *ip_str, size_t ip_str_len)
{
    if (s_wifi_status != WIFI_STATUS_CONNECTED || s_sta_netif == NULL || ip_str == NULL)
    {
        return false;
    }

    esp_netif_ip_info_t ip_info;
    esp_err_t ret = esp_netif_get_ip_info(s_sta_netif, &ip_info);
    if (ret == ESP_OK && ip_info.ip.addr != 0)
    {
        snprintf(ip_str, ip_str_len, IPSTR, IP2STR(&ip_info.ip));
        return true;
    }

    return false;
}

bool wifi_is_http_server_running(void)
{
    return s_http_server_started && (s_wifi_status == WIFI_STATUS_CONNECTED);
}