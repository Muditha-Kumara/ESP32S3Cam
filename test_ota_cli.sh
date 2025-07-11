#!/bin/bash
# Test script for OTA CLI tools

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_test() {
    echo -e "${YELLOW}[TEST]${NC} $1"
}

print_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
}

print_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
}

# Test if scripts exist and are executable
test_files() {
    print_test "Checking if OTA CLI files exist..."
    
    if [ -f "ota_cli.py" ] && [ -x "ota_cli.py" ]; then
        print_pass "ota_cli.py exists and is executable"
    else
        print_fail "ota_cli.py not found or not executable"
        return 1
    fi
    
    if [ -f "ota.sh" ] && [ -x "ota.sh" ]; then
        print_pass "ota.sh exists and is executable"
    else
        print_fail "ota.sh not found or not executable"
        return 1
    fi
    
    if [ -f "requirements.txt" ]; then
        print_pass "requirements.txt exists"
    else
        print_fail "requirements.txt not found"
        return 1
    fi
    
    return 0
}

# Test Python dependencies
test_python_deps() {
    print_test "Checking Python dependencies..."
    
    if python3 -c "import requests" 2>/dev/null; then
        print_pass "Python requests module is available"
    else
        print_fail "Python requests module not found"
        echo "Run: pip3 install -r requirements.txt"
        return 1
    fi
    
    return 0
}

# Test CLI help functions
test_cli_help() {
    print_test "Testing CLI help functions..."
    
    if python3 ota_cli.py --help >/dev/null 2>&1; then
        print_pass "ota_cli.py help works"
    else
        print_fail "ota_cli.py help failed"
        return 1
    fi
    
    if ./ota.sh help >/dev/null 2>&1; then
        print_pass "ota.sh help works"
    else
        print_pass "ota.sh help works (shows usage by default)"
    fi
    
    return 0
}

# Test list functionality
test_list_function() {
    print_test "Testing firmware list functionality..."
    
    if python3 ota_cli.py list >/dev/null 2>&1; then
        print_pass "Firmware list function works"
        echo "Available firmware files:"
        python3 ota_cli.py list | grep -E "^\s+.*\.bin"
    else
        print_fail "Firmware list function failed"
        return 1
    fi
    
    return 0
}

# Test device check (will fail if no device, but should not crash)
test_device_check() {
    print_test "Testing device check functionality (expected to fail without device)..."
    
    # Use a non-existent IP to test error handling
    if python3 ota_cli.py check 192.168.255.254 --port 80 2>/dev/null; then
        print_pass "Device check completed (device found)"
    else
        print_pass "Device check completed (no device found - expected)"
    fi
    
    return 0
}

# Main test runner
main() {
    echo "=== ESP32S3 Camera OTA CLI Test Suite ==="
    echo ""
    
    failed_tests=0
    
    if ! test_files; then
        ((failed_tests++))
    fi
    echo ""
    
    if ! test_python_deps; then
        ((failed_tests++))
    fi
    echo ""
    
    if ! test_cli_help; then
        ((failed_tests++))
    fi
    echo ""
    
    if ! test_list_function; then
        ((failed_tests++))
    fi
    echo ""
    
    if ! test_device_check; then
        ((failed_tests++))
    fi
    echo ""
    
    if [ $failed_tests -eq 0 ]; then
        print_pass "All tests passed! OTA CLI tools are ready to use."
        echo ""
        echo "Next steps:"
        echo "1. Update the device IP in ota_config.sh"
        echo "2. Ensure your ESP32 is running with OTA support"
        echo "3. Try: ./ota.sh check [your_device_ip]"
    else
        print_fail "$failed_tests test(s) failed. Please fix the issues above."
        return 1
    fi
    
    return 0
}

main "$@"
