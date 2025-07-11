# ESP32S3 Camera OTA Configuration
# Edit this file to set your default device settings

# Default device IP address
DEVICE_IP=192.168.1.100

# Default HTTP port
HTTP_PORT=80

# Default firmware paths (relative to project root)
FIRMWARE_PATHS=(
    "build/ESP32S3Cam.bin"
    "../myota/build/myota.bin"
    "../myota/ota/firmware.bin"
)

# OTA update timeout (seconds)
OTA_TIMEOUT=300

# Device restart wait time (seconds)
RESTART_WAIT=30
