#include "ota_update.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"

static const char *TAG = "OTA";

esp_err_t ota_handler(httpd_req_t *req)
{
    esp_ota_handle_t ota_handle;
    const esp_partition_t *ota_partition = esp_ota_get_next_update_partition(NULL);
    if (ota_partition == NULL)
    {
        ESP_LOGE(TAG, "Failed to find OTA partition");
        return ESP_FAIL;
    }

    esp_err_t err = esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_begin failed, error=%d", err);
        return err;
    }

    char buffer[1024];
    int data_read;
    while ((data_read = httpd_req_recv(req, buffer, sizeof(buffer))) > 0)
    {
        err = esp_ota_write(ota_handle, buffer, data_read);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_ota_write failed, error=%d", err);
            esp_ota_end(ota_handle);
            return err;
        }
    }

    if (data_read < 0)
    {
        ESP_LOGE(TAG, "httpd_req_recv failed, error=%d", data_read);
        esp_ota_end(ota_handle);
        return ESP_FAIL;
    }

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_end failed, error=%d", err);
        return err;
    }

    err = esp_ota_set_boot_partition(ota_partition);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed, error=%d", err);
        return err;
    }

    ESP_LOGI(TAG, "OTA update successful, restarting...");
    esp_restart();

    return ESP_OK;
}

void start_ota_update(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_start(&server, &config);

    httpd_uri_t ota_uri = {
        .uri = "/ota",
        .method = HTTP_POST,
        .handler = ota_handler,
        .user_ctx = NULL};

    httpd_register_uri_handler(server, &ota_uri);
    ESP_LOGI(TAG, "OTA update server started");
}
