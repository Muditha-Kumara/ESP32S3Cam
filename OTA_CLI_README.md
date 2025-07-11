# ESP32S3 Camera OTA CLI Tools

This directory contains command-line tools for managing Over-The-Air (OTA) updates for your ESP32S3 Camera project.

## Files

- `ota_cli.py` - Python CLI tool for OTA operations
- `ota.sh` - Bash wrapper script with additional features
- `ota_config.sh` - Configuration file for default settings
- `requirements.txt` - Python dependencies

## Prerequisites

1. **Python 3.6+** with pip
2. **ESP32 device** running the OTA-enabled firmware
3. **Network connectivity** between your computer and ESP32

## Installation

1. Install Python dependencies:
```bash
pip3 install -r requirements.txt
```

2. Make scripts executable:
```bash
chmod +x ota_cli.py ota.sh
```

3. Edit `ota_config.sh` to set your device's IP address.

## Usage

### Using the Bash Wrapper (Recommended)

The bash wrapper (`ota.sh`) provides a simplified interface:

```bash
# Check device connectivity
./ota.sh check

# Check specific device
./ota.sh check 192.168.1.100

# List available firmware files
./ota.sh list

# Perform OTA update
./ota.sh update 192.168.1.100 build/ESP32S3Cam.bin

# Build firmware and perform OTA update in one command
./ota.sh build-and-update

# Get device information
./ota.sh info

# Ping device
./ota.sh ping

# Monitor device connectivity
./ota.sh monitor-logs
```

### Using the Python CLI Directly

For more control, use the Python CLI directly:

```bash
# Check device status
python3 ota_cli.py check 192.168.1.100

# Ping device
python3 ota_cli.py ping 192.168.1.100

# Get device info
python3 ota_cli.py info 192.168.1.100

# Perform OTA update
python3 ota_cli.py update 192.168.1.100 build/ESP32S3Cam.bin

# List available firmware
python3 ota_cli.py list
```

## Commands Reference

### check
Check if the ESP32 device is reachable and responding.
```bash
./ota.sh check [IP_ADDRESS]
```

### ping
Send a simple HTTP request to test device responsiveness.
```bash
./ota.sh ping [IP_ADDRESS]
```

### info
Get device information (requires `/info` endpoint on ESP32).
```bash
./ota.sh info [IP_ADDRESS]
```

### update
Perform an OTA firmware update.
```bash
./ota.sh update [IP_ADDRESS] <FIRMWARE_FILE>
```

### list
List all available firmware files in the project.
```bash
./ota.sh list
```

### build-and-update
Build the firmware using ESP-IDF and then perform OTA update.
```bash
./ota.sh build-and-update [IP_ADDRESS]
```

### monitor-logs
Continuously monitor device connectivity (useful for debugging).
```bash
./ota.sh monitor-logs [IP_ADDRESS]
```

## Configuration

Edit `ota_config.sh` to customize default settings:

- `DEVICE_IP`: Default IP address of your ESP32 device
- `HTTP_PORT`: HTTP port (default: 80)
- `FIRMWARE_PATHS`: Paths where firmware files are located
- `OTA_TIMEOUT`: Timeout for OTA operations
- `RESTART_WAIT`: Time to wait for device restart

## Troubleshooting

### Device Not Reachable
```bash
✗ Cannot connect to device at 192.168.1.100
```
**Solutions:**
- Check if the ESP32 is powered on and connected to WiFi
- Verify the IP address is correct
- Ensure the HTTP server is running on the ESP32
- Check firewall settings

### OTA Update Failed
```bash
✗ OTA update failed with status code: 500
```
**Solutions:**
- Check if there's enough flash memory space
- Verify the firmware file is valid
- Ensure the ESP32 is not running other intensive tasks
- Check ESP32 logs via serial monitor

### Connection Lost During Update
```bash
✗ Connection lost during OTA update
```
**Note:** This is often normal behavior as the ESP32 restarts after a successful update.

### Python Dependencies Issues
```bash
ModuleNotFoundError: No module named 'requests'
```
**Solution:**
```bash
pip3 install -r requirements.txt
```

## ESP32 Side Requirements

Your ESP32 firmware must:

1. **HTTP Server**: Running on port 80 (or configured port)
2. **OTA Handler**: Registered at `/ota` endpoint (POST method)
3. **WiFi Connection**: Active and stable
4. **Partition Table**: Configured for OTA updates

## Security Considerations

- This tool sends firmware over unencrypted HTTP
- For production use, consider implementing:
  - HTTPS/TLS encryption
  - Authentication/authorization
  - Firmware signature verification
  - Network segmentation

## Example Workflow

1. **Development cycle:**
```bash
# Edit code
vim main/ESP32S3Cam.c

# Build and update in one command
./ota.sh build-and-update

# Monitor device
./ota.sh monitor-logs
```

2. **Quick update:**
```bash
# Check device status
./ota.sh check

# List available firmware
./ota.sh list

# Update with specific firmware
./ota.sh update 192.168.1.100 build/ESP32S3Cam.bin
```

## Integration with VS Code

You can integrate these tools with VS Code tasks by adding to `.vscode/tasks.json`:

```json
{
    "label": "OTA Update",
    "type": "shell",
    "command": "./ota.sh",
    "args": ["build-and-update"],
    "group": "build",
    "dependsOn": "ESP-IDF: Build"
}
```

## Tips

- Use `./ota.sh list` to quickly see available firmware files
- The tool automatically waits for device restart after OTA update
- Use `monitor-logs` command to continuously check device status during development
- Set your device IP in `ota_config.sh` to avoid typing it repeatedly
