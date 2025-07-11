#include "custom_handlers.h"
#include "http_server.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include <string.h>

static const char *TAG = "custom_handlers";

// Example: Status/Info handler
static esp_err_t status_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Status endpoint accessed");
    
    const char* response = "{"
        "\"status\": \"running\","
        "\"device\": \"ESP32S3Cam\","
        "\"version\": \"1.0.0\","
        "\"free_heap\": %d"
        "}";
    
    char json_response[256];
    snprintf(json_response, sizeof(json_response), response, esp_get_free_heap_size());
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, strlen(json_response));
    
    return ESP_OK;
}

// Example: Device control handler
static esp_err_t control_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Control endpoint accessed");
    
    char buffer[100];
    int ret = httpd_req_recv(req, buffer, sizeof(buffer) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    buffer[ret] = '\0';
    ESP_LOGI(TAG, "Received control command: %s", buffer);
    
    // Process your control commands here
    // Example: parse JSON commands for device control
    
    const char* response = "{\"result\": \"command_processed\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    
    return ESP_OK;
}

// Example: Camera stream handler (placeholder for future development)
static esp_err_t camera_stream_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Camera stream endpoint accessed");
    
    // This is where you'll implement camera streaming in the future
    const char* response = "{\"error\": \"camera_not_implemented_yet\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    
    return ESP_OK;
}

esp_err_t custom_handlers_init(void)
{
    ESP_LOGI(TAG, "Initializing custom HTTP handlers...");
    
    // Status/Info endpoint
    httpd_uri_t status_uri = {
        .uri = "/status",
        .method = HTTP_GET,
        .handler = status_handler,
        .user_ctx = NULL
    };
    
    // Device control endpoint
    httpd_uri_t control_uri = {
        .uri = "/control",
        .method = HTTP_POST,
        .handler = control_handler,
        .user_ctx = NULL
    };
    
    // Camera stream endpoint (for future implementation)
    httpd_uri_t camera_uri = {
        .uri = "/camera",
        .method = HTTP_GET,
        .handler = camera_stream_handler,
        .user_ctx = NULL
    };
    
    // Register all handlers
    esp_err_t ret;
    
    ret = http_server_register_handler(&status_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register status handler");
        return ret;
    }
    
    ret = http_server_register_handler(&control_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register control handler");
        return ret;
    }
    
    ret = http_server_register_handler(&camera_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register camera handler");
        return ret;
    }
    
    ESP_LOGI(TAG, "Custom HTTP handlers initialized successfully");
    ESP_LOGI(TAG, "Available endpoints:");
    ESP_LOGI(TAG, "  GET  /status  - Device status and info");
    ESP_LOGI(TAG, "  POST /control - Device control commands");
    ESP_LOGI(TAG, "  GET  /camera  - Camera stream (placeholder)");
    ESP_LOGI(TAG, "  POST /ota     - OTA firmware updates");
    
    return ESP_OK;
}

esp_err_t custom_handlers_deinit(void)
{
    ESP_LOGI(TAG, "Removing custom HTTP handlers...");
    
    esp_err_t ret1 = http_server_unregister_handler("/status", HTTP_GET);
    esp_err_t ret2 = http_server_unregister_handler("/control", HTTP_POST);
    esp_err_t ret3 = http_server_unregister_handler("/camera", HTTP_GET);
    
    if (ret1 == ESP_OK && ret2 == ESP_OK && ret3 == ESP_OK) {
        ESP_LOGI(TAG, "Custom HTTP handlers removed successfully");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Some handlers failed to unregister");
        return ESP_FAIL;
    }
}
