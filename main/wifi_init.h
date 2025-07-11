#ifndef WIFI_INIT_H
#define WIFI_INIT_H

#include "esp_wifi.h"

// WiFi connection status
typedef enum
{
    WIFI_STATUS_DISCONNECTED,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED
} wifi_status_t;

// Initialize WiFi as a separate task (non-blocking)
void wifi_init_task(void);

// Get current WiFi connection status
wifi_status_t wifi_get_status(void);

// Get WiFi IP address (returns true if connected and IP is available)
bool wifi_get_ip_address(char *ip_str, size_t ip_str_len);

// Check if HTTP server is running
bool wifi_is_http_server_running(void);

#endif // WIFI_INIT_H