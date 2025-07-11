#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include "esp_err.h"

// Initialize OTA functionality (registers OTA handler with HTTP server)
esp_err_t ota_init(void);

// Remove OTA functionality (unregisters OTA handler)
esp_err_t ota_deinit(void);

// Legacy function for backward compatibility
void start_ota_update(void);

#endif // OTA_UPDATE_H