#ifndef CAMERA_INIT_H
#define CAMERA_INIT_H

#include "esp_camera.h"
#include "esp_err.h"

// Camera initialization status
typedef enum {
    CAM_STATUS_NOT_INITIALIZED,
    CAM_STATUS_INITIALIZING, 
    CAM_STATUS_READY,
    CAM_STATUS_ERROR
} cam_status_t;

// XIAO ESP32S3 Sense camera pin definitions (OV2640) - Using standard I2C pins
#define CAM_PIN_PWDN    -1  // Power down is not used
#define CAM_PIN_RESET   -1  // Software reset will be performed
#define CAM_PIN_XCLK    10
#define CAM_PIN_SIOD    40  // SDA
#define CAM_PIN_SIOC    39  // SCL

#define CAM_PIN_D7      48  // Y9
#define CAM_PIN_D6      11  // Y8
#define CAM_PIN_D5      12  // Y7
#define CAM_PIN_D4      14  // Y6
#define CAM_PIN_D3      16  // Y5
#define CAM_PIN_D2      18  // Y4
#define CAM_PIN_D1      17  // Y3
#define CAM_PIN_D0      15  // Y2
#define CAM_PIN_VSYNC   38  // VSYNC
#define CAM_PIN_HREF    47  // HREF
#define CAM_PIN_PCLK    13  // PCLK

#define LED_GPIO_NUM 21 // LED GPIO for status indication

// Camera configuration
#define CAMERA_FRAME_SIZE FRAMESIZE_VGA   // 640x480 with PSRAM
#define CAMERA_PIXEL_FORMAT PIXFORMAT_JPEG
#define CAMERA_JPEG_QUALITY 12  // 0-63 lower means higher quality
#define CAMERA_FB_COUNT 2       // Use dual buffers with PSRAM

// Function declarations
esp_err_t camera_init(void);
esp_err_t camera_deinit(void);
cam_status_t camera_get_status(void);
camera_fb_t* camera_get_frame(void);
void camera_return_frame(camera_fb_t* fb);
esp_err_t camera_set_quality(int quality);
esp_err_t camera_set_framesize(framesize_t framesize);

#endif // CAMERA_INIT_H
