#!/bin/bash
# ESP32S3 Camera OTA CLI Wrapper Script

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default device IP (change this to your ESP32's IP)
DEFAULT_IP="192.168.1.224"

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_title() {
    echo -e "${BLUE}=== $1 ===${NC}"
}

# Check if Python is available
check_python() {
    if ! command -v python3 &> /dev/null; then
        print_error "Python3 is not installed"
        exit 1
    fi
}

# Install Python dependencies
install_deps() {
    print_status "Installing Python dependencies..."
    pip3 install -r requirements.txt
}

# Main script
main() {
    print_title "ESP32S3 Camera OTA CLI Tool"
    
    check_python
    
    # Install dependencies if requirements.txt exists
    if [ -f "requirements.txt" ]; then
        if ! python3 -c "import requests" &> /dev/null; then
            install_deps
        fi
    fi
    
    case "$1" in
        "check"|"ping"|"info")
            IP=${2:-$DEFAULT_IP}
            print_status "Running command '$1' on device $IP"
            python3 ota_cli.py "$1" "$IP"
            ;;
        "update")
            IP=${2:-$DEFAULT_IP}
            FIRMWARE=${3:-"build/ESP32S3Cam.bin"}
            
            # If no arguments provided, use defaults
            if [ $# -eq 1 ]; then
                IP=$DEFAULT_IP
                FIRMWARE="build/ESP32S3Cam.bin"
            fi
            
            # Check if firmware file exists
            if [ ! -f "$FIRMWARE" ]; then
                print_error "Firmware file not found: $FIRMWARE"
                echo "Available firmware files:"
                python3 ota_cli.py list
                exit 1
            fi
            
            print_status "Updating device $IP with firmware $FIRMWARE"
            python3 ota_cli.py update "$IP" "$FIRMWARE"
            ;;
        "list")
            python3 ota_cli.py list
            ;;
        "build-and-update")
            IP=${2:-$DEFAULT_IP}
            print_status "Building firmware..."
            idf.py build
            if [ $? -eq 0 ]; then
                print_status "Build successful, starting OTA update..."
                python3 ota_cli.py update "$IP" "build/ESP32S3Cam.bin"
            else
                print_error "Build failed"
                exit 1
            fi
            ;;
        "monitor-logs")
            IP=${2:-$DEFAULT_IP}
            print_status "Monitoring device logs at $IP"
            while true; do
                echo "$(date): Pinging device..."
                python3 ota_cli.py ping "$IP"
                sleep 5
            done
            ;;
        *)
            echo "ESP32S3 Camera OTA CLI Tool"
            echo ""
            echo "Usage: $0 <command> [options]"
            echo ""
            echo "Commands:"
            echo "  check [IP]              - Check device connectivity (default IP: $DEFAULT_IP)"
            echo "  ping [IP]               - Ping device"
            echo "  info [IP]               - Get device information"
            echo "  update [IP] [firmware]  - Perform OTA update (defaults: $DEFAULT_IP, build/ESP32S3Cam.bin)"
            echo "  list                    - List available firmware files"
            echo "  build-and-update [IP]   - Build firmware and perform OTA update"
            echo "  monitor-logs [IP]       - Monitor device connectivity"
            echo ""
            echo "Examples:"
            echo "  $0 check                                    # Check default device"
            echo "  $0 update                                   # Update with defaults"
            echo "  $0 update 192.168.1.100                    # Update specific device with default firmware"
            echo "  $0 update 192.168.1.100 build/ESP32S3Cam.bin  # Update with specific firmware"
            echo "  $0 build-and-update                        # Build and update"
            echo ""
            ;;
    esac
}

main "$@"
