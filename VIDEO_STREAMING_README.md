# ESP32S3 Camera Live Video Streaming

This branch adds live video streaming capabilities to the XIAO ESP32S3 Sense board using the OV2640 camera module.

## Features

- **Live Video Streaming**: Real-time MJPEG video stream accessible via web browser
- **Single Image Capture**: Capture and download individual JPEG images
- **Web Interface**: User-friendly HTML interface for viewing the stream
- **Camera Configuration**: Adjustable quality and frame size settings
- **Auto-reconnection**: Automatic stream recovery if connection is lost

## Hardware Requirements

- **XIAO ESP32S3 Sense** with built-in OV2640 camera
- WiFi connection for streaming access

## Camera Specifications

- **Sensor**: OV2640 CMOS camera sensor
- **Resolution**: Up to VGA (640x480) for streaming
- **Format**: JPEG compression for efficient streaming
- **Frame Rate**: ~30 FPS (adjustable)
- **Quality**: Configurable (0-63, lower = higher quality)

## Pin Configuration (XIAO ESP32S3 Sense)

The camera pins are pre-configured for the XIAO ESP32S3 Sense:

```c
#define CAM_PIN_XCLK    10
#define CAM_PIN_SIOD    40  // SDA
#define CAM_PIN_SIOC    39  // SCL
#define CAM_PIN_D7      48
#define CAM_PIN_D6      11
#define CAM_PIN_D5      12
#define CAM_PIN_D4      14
#define CAM_PIN_D3      16
#define CAM_PIN_D2      18
#define CAM_PIN_D1      17
#define CAM_PIN_D0      15
#define CAM_PIN_VSYNC   38
#define CAM_PIN_HREF    47
#define CAM_PIN_PCLK    13
```

## Build and Flash

1. **Configure WiFi**: Update `main/wifi_config.h` with your WiFi credentials
2. **Build the project**:
   ```bash
   idf.py build
   ```
3. **Flash to device**:
   ```bash
   idf.py flash monitor
   ```

## Usage

1. **Power on** the XIAO ESP32S3 Sense
2. **Wait for WiFi connection** (check serial monitor for IP address)
3. **Open web browser** and navigate to `http://<device_ip>/`
4. **View the live stream** on the web interface

## API Endpoints

- `GET /` - Web interface with live video stream
- `GET /stream` - Raw MJPEG video stream
- `GET /capture` - Capture single JPEG image

## Web Interface Features

- **Live Video Display**: Real-time streaming video
- **Refresh Button**: Manually refresh the stream
- **Capture Button**: Download a snapshot image
- **Responsive Design**: Works on desktop and mobile devices
- **Auto-recovery**: Automatically attempts to reconnect if stream fails

## Configuration Options

### Camera Quality
```c
camera_set_quality(12);  // 0-63, lower = higher quality
```

### Frame Size
```c
camera_set_framesize(FRAMESIZE_VGA);  // Various sizes available
```

### Stream Frame Rate
The frame rate can be adjusted by modifying the delay in `video_stream.c`:
```c
vTaskDelay(pdMS_TO_TICKS(33)); // ~30 FPS (33ms delay)
```

## Memory Configuration

The project is configured to use PSRAM for camera frame buffers:

- **PSRAM**: Enabled for ESP32S3
- **Frame Buffers**: 2 buffers in PSRAM
- **Buffer Size**: Optimized for VGA JPEG frames

## Troubleshooting

### Camera Not Initializing
- Check that you're using XIAO ESP32S3 **Sense** (with camera)
- Verify PSRAM is enabled in configuration
- Check serial monitor for camera initialization errors

### Poor Video Quality
- Increase JPEG quality (lower number = better quality)
- Check WiFi signal strength
- Reduce frame rate if bandwidth is limited

### Stream Not Loading
- Verify WiFi connection and IP address
- Check firewall settings
- Try accessing `/capture` endpoint first to test camera

### Memory Issues
- Ensure PSRAM is properly configured
- Check partition table has sufficient space
- Monitor heap usage in serial output

## Development Notes

### File Structure
```
main/
├── camera_init.c/h     # Camera initialization and control
├── video_stream.c/h    # HTTP streaming server
├── http_server.c/h     # Base HTTP server
├── wifi_init.c/h       # WiFi management
└── ESP32S3Cam.c        # Main application
```

### Key Functions
- `camera_init()` - Initialize camera with optimal settings
- `video_stream_init()` - Start streaming server
- `stream_handler()` - Handle MJPEG stream requests
- `capture_handler()` - Handle single image capture

### Performance Optimization
- JPEG compression reduces bandwidth requirements
- Frame buffering prevents blocking during capture
- Chunked HTTP responses for smooth streaming
- PSRAM usage prevents main RAM exhaustion

## Dependencies

- **esp32-camera**: Camera driver component
- **esp_http_server**: Web server functionality
- **esp_wifi**: WiFi connectivity
- **nvs_flash**: Configuration storage

## License

This project maintains the same license as the original ESP32S3Cam project.
