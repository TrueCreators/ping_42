#!/bin/bash

# Log file
LOG_FILE="ping_test_results.log"

# Clear previous log
> "$LOG_FILE"

# Function to run test and compare results
run_ping_test() {
    local test_name="$1"
    local ping_args="$2"
    local ft_ping_args="$3"
    
    echo "=== TEST: $test_name ===" >> "$LOG_FILE"
    
    # Output full commands
    echo "[PING COMMAND] ping $ping_args" >> "$LOG_FILE"
    echo "[FT_PING COMMAND] ./ft_ping $ft_ping_args" >> "$LOG_FILE"
    
    # Execute system ping
    echo -e "\n[SYSTEM PING]" >> "$LOG_FILE"
    echo "[COMMAND]: ping $ping_args" >> "$LOG_FILE"
    ping $ping_args >> "$LOG_FILE" 2>&1
    
    echo -e "\n[FT_PING]" >> "$LOG_FILE"
    echo "[COMMAND]: ./ft_ping $ft_ping_args" >> "$LOG_FILE"
    # Execute ft_ping
    ./ft_ping $ft_ping_args >> "$LOG_FILE" 2>&1
    
    echo -e "\n\n" >> "$LOG_FILE"
}

# Test with various parameters
echo "Running tests for ft_ping. Results in $LOG_FILE"

# Test 1: Basic ping with hostname
run_ping_test "Basic Ping" "8.8.8.8 -c 4" "8.8.8.8 -c 4"

# Test 2: Ping with custom packet size
run_ping_test "Custom Packet Size" "8.8.8.8 -c 4 -s 100" "8.8.8.8 -c 4 -s 100"

# Test 3: Ping with TTL setup
run_ping_test "Custom TTL" "8.8.8.8 -c 4 --ttl 10" "8.8.8.8 -c 4 --ttl 10"

# Test 4: Verbose mode
run_ping_test "Verbose Mode" "8.8.8.8 -c 4 -v" "8.8.8.8 -c 4 -v"

# Test 5: Ping localhost
run_ping_test "Localhost Ping" "localhost -c 4" "localhost -c 4"

# Test 6: Ping with invalid host (should handle errors correctly)
run_ping_test "Invalid Host Ping" "nonexistent.host -c 4" "nonexistent.host -c 4"

# Final report
echo "Testing completed. Full report in $LOG_FILE"