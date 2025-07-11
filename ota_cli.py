#!/usr/bin/env python3
"""
ESP32S3 Camera OTA Update CLI Tool

This script provides command-line interface to check and perform OTA updates
for the ESP32S3 Camera project.
"""

import argparse
import requests
import sys
import time
import os
from pathlib import Path

class ESP32OTAClient:
    def __init__(self, device_ip, port=80):
        """Initialize OTA client with device IP and port."""
        self.device_ip = device_ip
        self.port = port
        self.base_url = f"http://{device_ip}:{port}"
        
    def check_device_status(self):
        """Check if the ESP32 device is reachable."""
        try:
            # First try the root endpoint
            response = requests.get(f"{self.base_url}/", timeout=5)
            if response.status_code == 200:
                print(f"✓ Device at {self.device_ip} is reachable")
                return True
            elif response.status_code == 404:
                # 404 is fine - device is responding, just no root handler
                print(f"✓ Device at {self.device_ip} is reachable (no root endpoint)")
                return True
            else:
                print(f"✗ Device responded with status code: {response.status_code}")
                return False
        except requests.ConnectionError:
            print(f"✗ Cannot connect to device at {self.device_ip}")
            return False
        except requests.Timeout:
            print(f"✗ Connection timeout to device at {self.device_ip}")
            return False
        except Exception as e:
            print(f"✗ Error checking device status: {e}")
            return False
    
    def get_device_info(self):
        """Get device information if available."""
        try:
            # Try to get device info (you may need to implement this endpoint on ESP32)
            response = requests.get(f"{self.base_url}/info", timeout=5)
            if response.status_code == 200:
                info = response.json()
                print("Device Information:")
                for key, value in info.items():
                    print(f"  {key}: {value}")
                return info
            else:
                print("Device info endpoint not available")
                return None
        except:
            print("Device info endpoint not available")
            return None
    
    def perform_ota_update(self, firmware_path, progress_callback=None):
        """Perform OTA update with given firmware file."""
        if not os.path.exists(firmware_path):
            print(f"✗ Firmware file not found: {firmware_path}")
            return False
        
        file_size = os.path.getsize(firmware_path)
        print(f"Starting OTA update...")
        print(f"Firmware file: {firmware_path}")
        print(f"File size: {file_size} bytes")
        
        try:
            with open(firmware_path, 'rb') as firmware_file:
                # Prepare headers
                headers = {
                    'Content-Type': 'application/octet-stream',
                    'Content-Length': str(file_size)
                }
                
                # Send POST request to /ota endpoint
                response = requests.post(
                    f"{self.base_url}/ota",
                    data=firmware_file,
                    headers=headers,
                    timeout=300,  # 5 minutes timeout
                    stream=True
                )
                
                if response.status_code == 200:
                    print("✓ OTA update completed successfully!")
                    print("Device should restart automatically...")
                    
                    # Wait for device to restart
                    print("Waiting for device to restart...")
                    time.sleep(5)
                    
                    # Check if device comes back online
                    for i in range(30):  # Wait up to 30 seconds
                        time.sleep(1)
                        if self.check_device_status():
                            print("✓ Device is back online!")
                            return True
                        print(f"Waiting for device... ({i+1}/30)")
                    
                    print("⚠ Device may have restarted but is not responding")
                    return True
                    
                else:
                    print(f"✗ OTA update failed with status code: {response.status_code}")
                    print(f"Response: {response.text}")
                    return False
                    
        except requests.ConnectionError:
            print("✗ Connection lost during OTA update")
            print("This may be normal if the device is restarting...")
            return True
        except requests.Timeout:
            print("✗ OTA update timeout")
            return False
        except Exception as e:
            print(f"✗ Error during OTA update: {e}")
            return False
    
    def ping_device(self):
        """Simple ping to check device responsiveness."""
        try:
            start_time = time.time()
            response = requests.get(f"{self.base_url}/", timeout=5)
            end_time = time.time()
            
            if response.status_code == 200:
                response_time = (end_time - start_time) * 1000
                print(f"✓ Device responded in {response_time:.2f}ms")
                return True
            else:
                print(f"✗ Device responded with error: {response.status_code}")
                return False
        except Exception as e:
            print(f"✗ Ping failed: {e}")
            return False

def find_firmware_files():
    """Find available firmware files in the project."""
    firmware_paths = [
        "./build/ESP32S3Cam.bin",
        "../myota/build/myota.bin",
        "../myota/ota/firmware.bin"
    ]
    
    available_files = []
    for path in firmware_paths:
        if os.path.exists(path):
            size = os.path.getsize(path)
            available_files.append((path, size))
    
    return available_files

def main():
    parser = argparse.ArgumentParser(
        description="ESP32S3 Camera OTA Update CLI Tool",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s check 192.168.1.100              # Check device status
  %(prog)s ping 192.168.1.100               # Ping device
  %(prog)s info 192.168.1.100               # Get device info
  %(prog)s update 192.168.1.100 firmware.bin # Perform OTA update
  %(prog)s list                             # List available firmware files
        """
    )
    
    subparsers = parser.add_subparsers(dest='command', help='Available commands')
    
    # Check command
    check_parser = subparsers.add_parser('check', help='Check device connectivity')
    check_parser.add_argument('ip', help='ESP32 device IP address')
    check_parser.add_argument('--port', type=int, default=80, help='HTTP port (default: 80)')
    
    # Ping command
    ping_parser = subparsers.add_parser('ping', help='Ping device')
    ping_parser.add_argument('ip', help='ESP32 device IP address')
    ping_parser.add_argument('--port', type=int, default=80, help='HTTP port (default: 80)')
    
    # Info command
    info_parser = subparsers.add_parser('info', help='Get device information')
    info_parser.add_argument('ip', help='ESP32 device IP address')
    info_parser.add_argument('--port', type=int, default=80, help='HTTP port (default: 80)')
    
    # Update command
    update_parser = subparsers.add_parser('update', help='Perform OTA update')
    update_parser.add_argument('ip', help='ESP32 device IP address')
    update_parser.add_argument('firmware', help='Path to firmware binary file')
    update_parser.add_argument('--port', type=int, default=80, help='HTTP port (default: 80)')
    
    # List command
    list_parser = subparsers.add_parser('list', help='List available firmware files')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    if args.command == 'list':
        print("Available firmware files:")
        firmware_files = find_firmware_files()
        if firmware_files:
            for path, size in firmware_files:
                print(f"  {path} ({size} bytes)")
        else:
            print("  No firmware files found")
        return 0
    
    # Create OTA client
    client = ESP32OTAClient(args.ip, args.port)
    
    if args.command == 'check':
        success = client.check_device_status()
        return 0 if success else 1
    
    elif args.command == 'ping':
        success = client.ping_device()
        return 0 if success else 1
    
    elif args.command == 'info':
        if client.check_device_status():
            client.get_device_info()
        return 0
    
    elif args.command == 'update':
        if not client.check_device_status():
            print("Cannot reach device. Aborting OTA update.")
            return 1
        
        success = client.perform_ota_update(args.firmware)
        return 0 if success else 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
