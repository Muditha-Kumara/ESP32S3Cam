#include "camera_init.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_psram.h"

static const char *TAG = "camera_init";
static volatile cam_status_t s_camera_status = CAM_STATUS_NOT_INITIALIZED;

esp_err_t camera_init(void)
{
    if (s_camera_status == CAM_STATUS_READY) {
        ESP_LOGW(TAG, "Camera is already initialized");
        return ESP_OK;
    }

    s_camera_status = CAM_STATUS_INITIALIZING;
    ESP_LOGI(TAG, "Initializing camera...");

    // Check PSRAM availability
    bool psram_found = esp_psram_is_initialized();
    ESP_LOGI(TAG, "PSRAM found: %s", psram_found ? "Yes" : "No");
    
    // Use appropriate settings based on PSRAM availability
    framesize_t frame_size;
    int fb_count;
    camera_fb_location_t fb_location;
    int jpeg_quality;
    
    if (psram_found) {
        // PSRAM available - use better settings
        frame_size = CAMERA_FRAME_SIZE;
        fb_count = CAMERA_FB_COUNT;
        fb_location = CAMERA_FB_IN_PSRAM;
        jpeg_quality = CAMERA_JPEG_QUALITY;
        ESP_LOGI(TAG, "Using PSRAM for frame buffers");
    } else {
        // No PSRAM - use conservative settings
        frame_size = FRAMESIZE_CIF;  // 352x288
        fb_count = 1;
        fb_location = CAMERA_FB_IN_DRAM;
        jpeg_quality = 20;  // Lower quality to save memory
        ESP_LOGI(TAG, "Using internal DRAM for frame buffers");
    }

    camera_config_t config = {
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sscb_sda = CAM_PIN_SIOD,
        .pin_sscb_scl = CAM_PIN_SIOC,
        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,
        
        .xclk_freq_hz = 10000000,  // 10MHz for better stability
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        
        .pixel_format = CAMERA_PIXEL_FORMAT,
        .frame_size = frame_size,
        .jpeg_quality = jpeg_quality,
        .fb_count = fb_count,
        .fb_location = fb_location,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
        .sccb_i2c_port = 0  // Use I2C port 0 (default)
    };

    ESP_LOGI(TAG, "Camera config: frame_size=%d, fb_count=%d, fb_location=%s, jpeg_quality=%d", 
             frame_size, fb_count, fb_location == CAMERA_FB_IN_PSRAM ? "PSRAM" : "DRAM", jpeg_quality);

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        s_camera_status = CAM_STATUS_ERROR;
        return err;
    }

    // Set initial sensor settings for better image quality
    sensor_t * s = esp_camera_sensor_get();
    if (s != NULL) {
        // Initial settings for OV2640
        s->set_brightness(s, 0);     // -2 to 2
        s->set_contrast(s, 0);       // -2 to 2
        s->set_saturation(s, 0);     // -2 to 2
        s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
        s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
        s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
        s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
        s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
        s->set_aec2(s, 0);           // 0 = disable , 1 = enable
        s->set_ae_level(s, 0);       // -2 to 2
        s->set_aec_value(s, 300);    // 0 to 1200
        s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
        s->set_agc_gain(s, 0);       // 0 to 30
        s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
        s->set_bpc(s, 0);            // 0 = disable , 1 = enable
        s->set_wpc(s, 1);            // 0 = disable , 1 = enable
        s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
        s->set_lenc(s, 1);           // 0 = disable , 1 = enable
        s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
        s->set_vflip(s, 0);          // 0 = disable , 1 = enable
        s->set_dcw(s, 1);            // 0 = disable , 1 = enable
        s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
    }

    s_camera_status = CAM_STATUS_READY;
    ESP_LOGI(TAG, "Camera initialized successfully");
    return ESP_OK;
}

esp_err_t camera_deinit(void)
{
    if (s_camera_status == CAM_STATUS_NOT_INITIALIZED) {
        ESP_LOGW(TAG, "Camera is not initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Deinitializing camera...");
    esp_err_t err = esp_camera_deinit();
    if (err == ESP_OK) {
        s_camera_status = CAM_STATUS_NOT_INITIALIZED;
        ESP_LOGI(TAG, "Camera deinitialized successfully");
    } else {
        ESP_LOGE(TAG, "Camera deinit failed with error 0x%x", err);
        s_camera_status = CAM_STATUS_ERROR;
    }
    return err;
}

cam_status_t camera_get_status(void)
{
    return s_camera_status;
}

camera_fb_t* camera_get_frame(void)
{
    if (s_camera_status != CAM_STATUS_READY) {
        ESP_LOGE(TAG, "Camera is not ready");
        return NULL;
    }

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
    }
    return fb;
}

void camera_return_frame(camera_fb_t* fb)
{
    if (fb != NULL) {
        esp_camera_fb_return(fb);
    }
}

esp_err_t camera_set_quality(int quality)
{
    if (s_camera_status != CAM_STATUS_READY) {
        ESP_LOGE(TAG, "Camera is not ready");
        return ESP_ERR_INVALID_STATE;
    }

    if (quality < 0 || quality > 63) {
        ESP_LOGE(TAG, "Quality must be between 0-63");
        return ESP_ERR_INVALID_ARG;
    }

    sensor_t * s = esp_camera_sensor_get();
    if (s != NULL) {
        s->set_quality(s, quality);
        ESP_LOGI(TAG, "Camera quality set to %d", quality);
        return ESP_OK;
    }
    
    return ESP_ERR_NOT_FOUND;
}

esp_err_t camera_set_framesize(framesize_t framesize)
{
    if (s_camera_status != CAM_STATUS_READY) {
        ESP_LOGE(TAG, "Camera is not ready");
        return ESP_ERR_INVALID_STATE;
    }

    sensor_t * s = esp_camera_sensor_get();
    if (s != NULL) {
        s->set_framesize(s, framesize);
        ESP_LOGI(TAG, "Camera framesize set to %d", framesize);
        return ESP_OK;
    }
    
    return ESP_ERR_NOT_FOUND;
}
