#ifndef VIDEO_STREAM_H
#define VIDEO_STREAM_H

#include "esp_err.h"
#include "esp_http_server.h"

// Video streaming status
typedef enum {
    VIDEO_STREAM_STOPPED,
    VIDEO_STREAM_STARTING,
    VIDEO_STREAM_RUNNING,
    VIDEO_STREAM_ERROR
} video_stream_status_t;

// Streaming configuration
#define STREAM_CONTENT_TYPE "multipart/x-mixed-replace;boundary=123456789000000000000987654321"
#define STREAM_BOUNDARY "\r\n--123456789000000000000987654321\r\n"
#define STREAM_PART "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n"

// Function declarations
esp_err_t video_stream_init(httpd_handle_t server);
esp_err_t video_stream_stop(void);
video_stream_status_t video_stream_get_status(void);

// HTTP handlers
esp_err_t stream_handler(httpd_req_t *req);
esp_err_t capture_handler(httpd_req_t *req);
esp_err_t index_handler(httpd_req_t *req);

#endif // VIDEO_STREAM_H
