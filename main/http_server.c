#include "http_server.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include <string.h>

static const char *TAG = "http_server";
static httpd_handle_t s_server = NULL;
static volatile http_server_status_t s_server_status = HTTP_SERVER_STOPPED;

#define HTTP_SERVER_PORT 80
#define HTTP_SERVER_MAX_URI_LEN 512
#define HTTP_SERVER_MAX_HANDLERS 10

esp_err_t http_server_init(void)
{
    if (s_server != NULL) 
    {
        ESP_LOGW(TAG, "HTTP server is already running");
        return ESP_OK;
    }

    s_server_status = HTTP_SERVER_STARTING;
    ESP_LOGI(TAG, "Starting HTTP server...");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = HTTP_SERVER_PORT;
    config.max_uri_handlers = HTTP_SERVER_MAX_HANDLERS;
    config.max_resp_headers = 8;
    config.max_open_sockets = 7;
    config.stack_size = 8192;
    config.task_priority = 5;

    esp_err_t ret = httpd_start(&s_server, &config);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
        s_server_status = HTTP_SERVER_ERROR;
        return ret;
    }

    s_server_status = HTTP_SERVER_RUNNING;
    ESP_LOGI(TAG, "HTTP server started on port %d", HTTP_SERVER_PORT);
    return ESP_OK;
}

esp_err_t http_server_stop(void)
{
    if (s_server == NULL) 
    {
        ESP_LOGW(TAG, "HTTP server is not running");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Stopping HTTP server...");
    esp_err_t ret = httpd_stop(s_server);
    if (ret == ESP_OK) 
    {
        s_server = NULL;
        s_server_status = HTTP_SERVER_STOPPED;
        ESP_LOGI(TAG, "HTTP server stopped");
    } 
    else 
    {
        ESP_LOGE(TAG, "Failed to stop HTTP server: %s", esp_err_to_name(ret));
        s_server_status = HTTP_SERVER_ERROR;
    }
    
    return ret;
}

http_server_status_t http_server_get_status(void)
{
    return s_server_status;
}

httpd_handle_t http_server_get_handle(void)
{
    return s_server;
}

esp_err_t http_server_register_handler(const httpd_uri_t *uri_handler)
{
    if (s_server == NULL) 
    {
        ESP_LOGE(TAG, "HTTP server is not running. Cannot register handler for URI: %s", 
                 uri_handler ? uri_handler->uri : "unknown");
        return ESP_ERR_INVALID_STATE;
    }

    if (uri_handler == NULL) 
    {
        ESP_LOGE(TAG, "URI handler is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = httpd_register_uri_handler(s_server, uri_handler);
    if (ret == ESP_OK) 
    {
        ESP_LOGI(TAG, "Registered handler for URI: %s", uri_handler->uri);
    } 
    else 
    {
        ESP_LOGE(TAG, "Failed to register handler for URI %s: %s", 
                 uri_handler->uri, esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t http_server_unregister_handler(const char *uri, httpd_method_t method)
{
    if (s_server == NULL) 
    {
        ESP_LOGE(TAG, "HTTP server is not running. Cannot unregister handler for URI: %s", 
                 uri ? uri : "unknown");
        return ESP_ERR_INVALID_STATE;
    }

    if (uri == NULL) 
    {
        ESP_LOGE(TAG, "URI is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = httpd_unregister_uri_handler(s_server, uri, method);
    if (ret == ESP_OK) 
    {
        ESP_LOGI(TAG, "Unregistered handler for URI: %s", uri);
    } 
    else 
    {
        ESP_LOGE(TAG, "Failed to unregister handler for URI %s: %s", 
                 uri, esp_err_to_name(ret));
    }
    
    return ret;
}
