#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "esp_http_server.h"
#include "esp_err.h"

// HTTP Server status
typedef enum {
    HTTP_SERVER_STOPPED,
    HTTP_SERVER_STARTING,
    HTTP_SERVER_RUNNING,
    HTTP_SERVER_ERROR
} http_server_status_t;

// Initialize and start the HTTP server
esp_err_t http_server_init(void);

// Stop the HTTP server
esp_err_t http_server_stop(void);

// Get current server status
http_server_status_t http_server_get_status(void);

// Get the server handle (for registering custom handlers)
httpd_handle_t http_server_get_handle(void);

// Register a URI handler
esp_err_t http_server_register_handler(const httpd_uri_t *uri_handler);

// Unregister a URI handler
esp_err_t http_server_unregister_handler(const char *uri, httpd_method_t method);

#endif // HTTP_SERVER_H
