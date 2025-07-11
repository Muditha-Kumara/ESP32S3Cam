#include "video_stream.h"
#include "camera_init.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "video_stream";
static volatile video_stream_status_t s_stream_status = VIDEO_STREAM_STOPPED;
static httpd_handle_t s_server_handle = NULL;

// HTML page for video streaming
static const char* index_html = 
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"    <title>ESP32S3 Camera Live Stream</title>\n"
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
"    <style>\n"
"        body {\n"
"            font-family: Arial, sans-serif;\n"
"            margin: 0;\n"
"            padding: 20px;\n"
"            background-color: #f0f0f0;\n"
"            text-align: center;\n"
"        }\n"
"        .container {\n"
"            max-width: 800px;\n"
"            margin: 0 auto;\n"
"            background: white;\n"
"            padding: 20px;\n"
"            border-radius: 10px;\n"
"            box-shadow: 0 4px 6px rgba(0,0,0,0.1);\n"
"        }\n"
"        h1 {\n"
"            color: #333;\n"
"            margin-bottom: 20px;\n"
"        }\n"
"        img {\n"
"            max-width: 100%;\n"
"            height: auto;\n"
"            border: 2px solid #ddd;\n"
"            border-radius: 8px;\n"
"            box-shadow: 0 2px 4px rgba(0,0,0,0.1);\n"
"        }\n"
"        .controls {\n"
"            margin: 20px 0;\n"
"        }\n"
"        button {\n"
"            background-color: #4CAF50;\n"
"            color: white;\n"
"            padding: 10px 20px;\n"
"            border: none;\n"
"            border-radius: 4px;\n"
"            cursor: pointer;\n"
"            margin: 5px;\n"
"            font-size: 16px;\n"
"        }\n"
"        button:hover {\n"
"            background-color: #45a049;\n"
"        }\n"
"        .info {\n"
"            background-color: #e7f3ff;\n"
"            border: 1px solid #b3d9ff;\n"
"            border-radius: 4px;\n"
"            padding: 10px;\n"
"            margin: 10px 0;\n"
"        }\n"
"    </style>\n"
"</head>\n"
"<body>\n"
"    <div class=\"container\">\n"
"        <h1>ESP32S3 Camera Live Stream</h1>\n"
"        <div class=\"info\">\n"
"            <p>XIAO ESP32S3 Sense - OV2640 Camera Module</p>\n"
"        </div>\n"
"        <div class=\"controls\">\n"
"            <button onclick=\"location.reload()\">Refresh Stream</button>\n"
"            <button onclick=\"captureImage()\">Capture Image</button>\n"
"        </div>\n"
"        <div>\n"
"            <img id=\"stream\" src=\"/stream\" alt=\"Video Stream\">\n"
"        </div>\n"
"        <div class=\"info\">\n"
"            <p>Stream URL: <strong>/stream</strong></p>\n"
"            <p>Capture URL: <strong>/capture</strong></p>\n"
"        </div>\n"
"    </div>\n"
"    <script>\n"
"        function captureImage() {\n"
"            window.open('/capture', '_blank');\n"
"        }\n"
"        \n"
"        // Auto-refresh if stream fails\n"
"        document.getElementById('stream').onerror = function() {\n"
"            setTimeout(function() {\n"
"                document.getElementById('stream').src = '/stream?' + new Date().getTime();\n"
"            }, 5000);\n"
"        };\n"
"    </script>\n"
"</body>\n"
"</html>";

esp_err_t video_stream_init(httpd_handle_t server)
{
    if (server == NULL) {
        ESP_LOGE(TAG, "HTTP server handle is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    if (s_stream_status == VIDEO_STREAM_RUNNING) {
        ESP_LOGW(TAG, "Video stream is already running");
        return ESP_OK;
    }

    s_stream_status = VIDEO_STREAM_STARTING;
    s_server_handle = server;
    ESP_LOGI(TAG, "Starting video stream...");

    // Register the index page handler
    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = NULL
    };
    esp_err_t ret = httpd_register_uri_handler(server, &index_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register index handler: %s", esp_err_to_name(ret));
        s_stream_status = VIDEO_STREAM_ERROR;
        return ret;
    }

    // Register the stream handler
    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL
    };
    ret = httpd_register_uri_handler(server, &stream_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register stream handler: %s", esp_err_to_name(ret));
        s_stream_status = VIDEO_STREAM_ERROR;
        return ret;
    }

    // Register the capture handler
    httpd_uri_t capture_uri = {
        .uri = "/capture",
        .method = HTTP_GET,
        .handler = capture_handler,
        .user_ctx = NULL
    };
    ret = httpd_register_uri_handler(server, &capture_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register capture handler: %s", esp_err_to_name(ret));
        s_stream_status = VIDEO_STREAM_ERROR;
        return ret;
    }

    s_stream_status = VIDEO_STREAM_RUNNING;
    ESP_LOGI(TAG, "Video stream started successfully");
    return ESP_OK;
}

esp_err_t video_stream_stop(void)
{
    if (s_stream_status == VIDEO_STREAM_STOPPED) {
        ESP_LOGW(TAG, "Video stream is already stopped");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Stopping video stream...");
    
    if (s_server_handle != NULL) {
        httpd_unregister_uri_handler(s_server_handle, "/", HTTP_GET);
        httpd_unregister_uri_handler(s_server_handle, "/stream", HTTP_GET);
        httpd_unregister_uri_handler(s_server_handle, "/capture", HTTP_GET);
    }
    
    s_stream_status = VIDEO_STREAM_STOPPED;
    s_server_handle = NULL;
    ESP_LOGI(TAG, "Video stream stopped");
    return ESP_OK;
}

video_stream_status_t video_stream_get_status(void)
{
    return s_stream_status;
}

esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Expires", "0");
    
    return httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
}

esp_err_t capture_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;

    if (camera_get_status() != CAM_STATUS_READY) {
        ESP_LOGE(TAG, "Camera is not ready for capture");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    fb = camera_get_frame();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Expires", "0");

    res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
    camera_return_frame(fb);

    if (res == ESP_OK) {
        ESP_LOGI(TAG, "Image captured and sent, size: %zu bytes", fb->len);
    } else {
        ESP_LOGE(TAG, "Failed to send captured image");
    }

    return res;
}

esp_err_t stream_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char part_buf[64];

    if (camera_get_status() != CAM_STATUS_READY) {
        ESP_LOGE(TAG, "Camera is not ready for streaming");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
        return res;
    }

    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Expires", "0");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    ESP_LOGI(TAG, "Starting video stream for client");

    while (true) {
        fb = camera_get_frame();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
            break;
        }

        if (fb->format != PIXFORMAT_JPEG) {
            ESP_LOGE(TAG, "Non-JPEG frame received");
            camera_return_frame(fb);
            res = ESP_FAIL;
            break;
        }

        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;

        res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
        if (res != ESP_OK) {
            camera_return_frame(fb);
            break;
        }

        size_t hlen = snprintf(part_buf, 64, STREAM_PART, _jpg_buf_len);
        res = httpd_resp_send_chunk(req, part_buf, hlen);
        if (res != ESP_OK) {
            camera_return_frame(fb);
            break;
        }

        res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        if (res != ESP_OK) {
            camera_return_frame(fb);
            break;
        }

        camera_return_frame(fb);
        fb = NULL;
        _jpg_buf = NULL;
        
        // Small delay to prevent overwhelming the client
        vTaskDelay(pdMS_TO_TICKS(33)); // ~30 FPS
    }

    ESP_LOGI(TAG, "Video stream ended for client");
    return res;
}
