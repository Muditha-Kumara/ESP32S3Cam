#include "ota_update.h"
#include "http_server.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "OTA";

esp_err_t ota_handler(httpd_req_t *req)
{
    esp_ota_handle_t ota_handle = 0;
    const esp_partition_t *ota_partition = NULL;
    esp_err_t err = ESP_OK;
    size_t remaining = req->content_len;

    ESP_LOGI(TAG, "Starting OTA update, expected size: %d bytes", remaining);

    // Validate content length
    if (remaining == 0)
    {
        ESP_LOGE(TAG, "No content in OTA request");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No firmware data");
        return ESP_FAIL;
    }

    ota_partition = esp_ota_get_next_update_partition(NULL);
    if (ota_partition == NULL)
    {
        ESP_LOGE(TAG, "Failed to find OTA partition");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA partition not found");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Starting OTA to partition %s at offset 0x%lx",
             ota_partition->label, (unsigned long)ota_partition->address);

    err = esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_begin failed, error=%s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA begin failed");
        return err;
    }

    char buffer[1024];
    int data_read;
    size_t total_received = 0;

    while (remaining > 0)
    {
        int recv_len = (remaining > sizeof(buffer)) ? sizeof(buffer) : remaining;
        data_read = httpd_req_recv(req, buffer, recv_len);

        if (data_read < 0)
        {
            ESP_LOGE(TAG, "httpd_req_recv failed, error=%d", data_read);
            esp_ota_abort(ota_handle);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to receive data");
            return ESP_FAIL;
        }
        else if (data_read == 0)
        {
            // Connection closed
            ESP_LOGE(TAG, "Connection closed prematurely");
            esp_ota_abort(ota_handle);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Connection closed");
            return ESP_FAIL;
        }

        err = esp_ota_write(ota_handle, buffer, data_read);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_ota_write failed, error=%s", esp_err_to_name(err));
            esp_ota_abort(ota_handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA write failed");
            return err;
        }

        remaining -= data_read;
        total_received += data_read;

        // Log progress every 64KB
        if (total_received % (64 * 1024) == 0 || remaining == 0)
        {
            ESP_LOGI(TAG, "OTA progress: %d/%d bytes (%.1f%%)",
                     total_received, req->content_len,
                     (float)total_received / req->content_len * 100);
        }
    }

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_end failed, error=%s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA end failed");
        return err;
    }

    err = esp_ota_set_boot_partition(ota_partition);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed, error=%s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to set boot partition");
        return err;
    }

    ESP_LOGI(TAG, "OTA update successful (%d bytes), restarting in 2 seconds...", total_received);

    // Send success response before restarting
    httpd_resp_send(req, "OTA update successful, device will restart", -1);

    // Give time for response to be sent
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();

    return ESP_OK;
}

esp_err_t ota_init(void)
{
    ESP_LOGI(TAG, "Initializing OTA functionality...");

    // Define the OTA URI handler
    httpd_uri_t ota_uri = {
        .uri = "/ota",
        .method = HTTP_POST,
        .handler = ota_handler,
        .user_ctx = NULL};

    // Register the OTA handler with the HTTP server
    esp_err_t ret = http_server_register_handler(&ota_uri);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "OTA handler registered successfully");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to register OTA handler: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t ota_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing OTA functionality...");

    esp_err_t ret = http_server_unregister_handler("/ota", HTTP_POST);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "OTA handler unregistered successfully");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to unregister OTA handler: %s", esp_err_to_name(ret));
    }

    return ret;
}

void start_ota_update(void)
{
    ESP_LOGI(TAG, "Starting OTA update service (legacy function)...");

    // Start the HTTP server if not already running
    if (http_server_get_status() != HTTP_SERVER_RUNNING)
    {
        esp_err_t ret = http_server_init();
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to start HTTP server for OTA");
            return;
        }
    }

    // Initialize OTA functionality
    ota_init();
}
