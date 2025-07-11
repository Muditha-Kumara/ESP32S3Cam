#ifndef CUSTOM_HANDLERS_H
#define CUSTOM_HANDLERS_H

#include "esp_err.h"

// Initialize all custom HTTP handlers
esp_err_t custom_handlers_init(void);

// Remove all custom HTTP handlers
esp_err_t custom_handlers_deinit(void);

#endif // CUSTOM_HANDLERS_H
