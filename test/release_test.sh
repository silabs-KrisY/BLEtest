#!/bin/bash

# This is a bash/zsh script to perform some release testing on BLEtest. 
# Running this test requires two seaprate NCP devices, with 
# UART ports configured in the script as UART1 and UART2. 
# Tested on Mac OSX and Raspberry Pi.

#*******************************************************************************
# * # License
# * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
# *******************************************************************************
# *
# * SPDX-License-Identifier: Zlib
# *
# * The licensor of this software is Silicon Laboratories Inc.
# *
# * This software is provided 'as-is', without any express or implied
# * warranty. In no event will the authors be held liable for any damages
# * arising from the use of this software.
# *
# * Permission is granted to anyone to use this software for any purpose,
# * including commercial applications, and to alter it and redistribute it
# * freely, subject to the following restrictions:
# *
# * 1. The origin of this software must not be misrepresented; you must not
# *    claim that you wrote the original software. If you use this software
# *    in a product, an acknowledgment in the product documentation would be
# *    appreciated but is not required.
# * 2. Altered source versions must be plainly marked as such, and must not be
# *    misrepresented as being the original software.
# * 3. This notice may not be removed or altered from any source distribution.
# *
# ******************************************************************************/

# --- Configuration ---
APP_NAME="BLEtest"
APP_PATH="./exe/$APP_NAME" # Adjust path as needed
LOG_FILE="/tmp/${APP_NAME}_release_test.log"
TEST_DATA_DIR="/tmp/${APP_NAME}_test_data"

# --- These correspond to the two connected test NCP devices ----
UART1="/dev/ttyACM0"
UART2="/dev/ttyACM1"

# --- Functions ---

# Function to log messages
log_message() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $1" | tee -a "$LOG_FILE"
}

# Function to check output file for forbidden text (assert)
assertion_failure() {
  local pattern="Assertion failed"
  local file="$1"
  if grep -Fq "$pattern" "$file"; then
    printf 'FAIL: assert message "%s" found in %s\n' "$pattern" "$file"
    exit 1
  else
    printf 'PASS: no "%s" found\n' "$pattern"
  fi
}

# Function to check if a command succeeded
check_success() {
    if [ $? -eq 0 ]; then
        log_message "SUCCESS: $1"
    else
        log_message "FAILURE: $1"
        exit 1 # Exit on first failure
    fi
}

# Function that waits for two background processes represented by PID1 and PID2
# to complete

pid_wait() {

    echo "Waiting for PID1 process to complete..."
    wait $PID1 # Wait specifically for the process with PID1
    echo "PID1 process has completed."
    echo "Waiting for PID2 process to complete..."
    wait $PID2 # Wait specifically for the process with PID2
    echo "PID2 process has completed."
}

# --- Pre-test Setup ---
log_message "Starting release tests for $APP_NAME..."
rm -f "$LOG_FILE" # Clear previous log
mkdir -p "$TEST_DATA_DIR"
check_success "Pre-test setup completed"

# --- Test Cases ---

# 1. Check if application executable exists
log_message "Test 1: Checking for application executable..."
if [ -f "$APP_PATH" ]; then
    check_success "Application executable found at $APP_PATH"
else
    check_success "Application executable NOT found at $APP_PATH"
fi

# 2. Check application version
log_message "Test 2: Checking application version..."
"$APP_PATH" --version > "$TEST_DATA_DIR/version.txt" 2>&1
check_success "Application version retrieved"
log_message "Version: $(cat "$TEST_DATA_DIR/version.txt")"

# 3. Testing NCP on UART1
log_message "Test 3: Performing UART1 NCP init test..."
"$APP_PATH" -u "$UART1" --time 1 > "$TEST_DATA_DIR/basic_output.txt" 2>&1
check_success "UART1 init test completed"
assertion_failure "$TEST_DATA_DIR/basic_output.txt"
MAC_ADDR1="$(
  # Example line: MAC address: 0C:43:14:F0:2F:8E
  sed -n -E 's/^MAC address: ([0-9A-F]{2}(:[0-9A-F]{2}){5})$/\1/p' "$TEST_DATA_DIR/basic_output.txt" | head -n1
)"
printf '  MAC1: %s\n' "${MAC_ADDR1:-<none>}"
printf '  NCP1 ver: ' 
awk '/gecko_evt_system_boot/{
  sub(/.*gecko_evt_system_boot\(/,""); sub(/\).*/,""); gsub(/[[:space:]]/,"");
  split($0,a,","); if (length(a)>=3) print a[1]","a[2]","a[3]; exit
}' "$TEST_DATA_DIR/basic_output.txt"
printf ' \n'
# Add checks for expected output in basic_output.txt if needed

# 4. Testing NCP on UART2
log_message "Test 4: Performing UART2 NCP init test..."
"$APP_PATH" -u "$UART2" --time 1 > "$TEST_DATA_DIR/basic_output.txt" 2>&1
check_success "UART2 init test completed"
MAC_ADDR2="$(
  # Example line: MAC address: 0C:43:14:F0:2F:8E
  sed -n -E 's/^MAC address: ([0-9A-F]{2}(:[0-9A-F]{2}){5})$/\1/p' "$TEST_DATA_DIR/basic_output.txt" | head -n1
)"
printf '  MAC2: %s\n' "${MAC_ADDR2:-<none>}"
printf '  NCP2 ver: ' 
awk '/gecko_evt_system_boot/{
  sub(/.*gecko_evt_system_boot\(/,""); sub(/\).*/,""); gsub(/[[:space:]]/,"");
  split($0,a,","); if (length(a)>=3) print a[1]","a[2]","a[3]; exit
}' "$TEST_DATA_DIR/basic_output.txt"
printf ' \n'
# Add checks for expected output in basic_output.txt if needed

# 5. Testing advertising
log_message "Test 5: Performing basic advertising test..."
# Advertiser
"$APP_PATH" -u "$UART1" --adv --time 3000 > "$TEST_DATA_DIR/advertiser_output.txt" 2>&1 &
PID1=$!
# Scanner
"$APP_PATH" -u "$UART2" --advscan --time 2000 > "$TEST_DATA_DIR/scanner_output.txt" 2>&1 &
PID2=$!
pid_wait
COUNT="$(grep -Foc "MAC $MAC_ADDR1," "$TEST_DATA_DIR/scanner_output.txt" )"
if [ $COUNT -gt 0 ]; then
        log_message "SUCCESS: TargetMAC appears in scanner output at least once"
        printf ' Target MAC count in scan results %s\n' "${COUNT:-<none>}"
    else
        log_message "Failure: TargetMAC does not appear in scanner output at least once"
        exit 1 # Exit on failure
    fi
# Also check output files for assertions just in case
assertion_failure "$TEST_DATA_DIR/advertiser_output.txt"
assertion_failure "$TEST_DATA_DIR/scanner_output.txt" 

# 6. Testing DTM TX->RX
log_message "Test 6: Performing basic DTM TX->RX test..."
# DTM RX
"$APP_PATH" -u "$UART2" --time 5000 --rx  > "$TEST_DATA_DIR/rx.txt" 2>&1 &
PID1=$!
# DTM TX
"$APP_PATH" -u "$UART1" --time 2000 --packet_type 0 > "$TEST_DATA_DIR/tx.txt" 2>&1 &
PID2=$!
pid_wait
# Check for packet RX and TX counts and make sure they are reasonable (within +/- 15)
# Example line:
# DTM completed, number of packets transmitted: 8018
PKT_TX="$(awk '/^DTM completed/ { print $NF; exit }' "$TEST_DATA_DIR/tx.txt")"   
PKT_RX="$(awk '/^DTM receive completed/ { print $NF; exit }' "$TEST_DATA_DIR/rx.txt")"   
printf '  TX count: %s\n' "${PKT_TX:-<none>}"
printf '  RX count: %s\n' "${PKT_RX:-<none>}"
 if [ $PKT_TX -gt $((PKT_RX - 15)) ] && [ $PKT_TX -lt $((PKT_RX + 15)) ] && [ $PKT_RX != 0 ]; then
        log_message "SUCCESS: TX and RX counts are within +/-15"
    else
        log_message "FAILURE: TX and RX counts differ"
        exit 1 # Exit on failure
    fi
# Also check output files for assertions since the return value for
# bg tasks is not performed
assertion_failure "$TEST_DATA_DIR/tx.txt"
assertion_failure "$TEST_DATA_DIR/rx.txt"

# 6a. Testing DTM TX->RX with 2M PHY
log_message "Test 6a: Performing basic DTM TX->RX test with 2M PHY..."
# DTM RX
"$APP_PATH" -u "$UART2" --time 5000 --rx --phy 2 > "$TEST_DATA_DIR/rx.txt" 2>&1 &
PID1=$!
# DTM TX
"$APP_PATH" -u "$UART1" --time 2000 --packet_type 0 --phy 2 > "$TEST_DATA_DIR/tx.txt" 2>&1 &
PID2=$!
pid_wait
# Check for packet RX and TX counts and make sure they are reasonable (within +/- 15)
# Example line:
# DTM completed, number of packets transmitted: 8018
PKT_TX="$(awk '/^DTM completed/ { print $NF; exit }' "$TEST_DATA_DIR/tx.txt")"   
PKT_RX="$(awk '/^DTM receive completed/ { print $NF; exit }' "$TEST_DATA_DIR/rx.txt")"   
printf '  TX count: %s\n' "${PKT_TX:-<none>}"
printf '  RX count: %s\n' "${PKT_RX:-<none>}"
 if [ $PKT_TX -gt $((PKT_RX - 15)) ] && [ $PKT_TX -lt $((PKT_RX + 15)) ] && [ $PKT_RX != 0 ]; then
        log_message "SUCCESS: TX and RX counts are within +/-15"
    else
        log_message "FAILURE: TX and RX counts differ"
        exit 1 # Exit on failure
    fi
# Also check output files for assertions since the return value for
# bg tasks is not performed
assertion_failure "$TEST_DATA_DIR/tx.txt"
assertion_failure "$TEST_DATA_DIR/rx.txt"

# 7. Testing CTUNE read/write
"$APP_PATH" -u "$UART1" --time 1 --ctune_set 0x1a5 > "$TEST_DATA_DIR/ctune.txt" 2>&1
check_success "CTUNE set"
"$APP_PATH" -u "$UART1" --ctune_get --time 1 > "$TEST_DATA_DIR/ctune.txt" 2>&1
check_success "CTUNE get"
CTUNE="$(awk '/^Stored CTUNE/ { print $NF; exit }' "$TEST_DATA_DIR/ctune.txt")"
if [ "$CTUNE" = "0x01a5" ]; then
        log_message "SUCCESS: CTUNE read equal to write"
    else
        log_message "FAILURE: CTUNE read not equal to write"
        printf "CTUNE read: %s\n" "$CTUNE"
        exit 1 # Exit on failure
    fi

# 8. Testing custom BGAPI
# NCP examples set up cust command 04 as a loopback
"$APP_PATH" -u "$UART1" --cust 04 > "$TEST_DATA_DIR/cust_bgapi.txt" 2>&1
check_success "custom BGAPI command"
# Expected string "BGAPI success! Returned payload (len=1): 04"
LINE="$(grep 'Returned payload' "$TEST_DATA_DIR/cust_bgapi.txt")"
LEN="$(echo "$LINE" | sed -E 's/.*len=([0-9]+).*/\1/')"
# Grab everything after the colon, stripping trailing spaces
PAYLOAD="$(echo "$LINE" | sed -E 's/.*: *//' | sed 's/[[:space:]]*$//')" 

if [ $LEN -eq 1 ] && [ "$PAYLOAD" = "04" ]; then
        log_message "SUCCESS: cust BGAPI return value equal to input"
    else
        log_message "FAILURE: cust BGAPI return value not equal to input"
        printf "BGAPI return value: %s, len=%d\n" "$PAYLOAD" "$LEN"
        exit 1 # Exit on failure
    fi

# 9. Testing throughput / GATT connection
log_message "Test 9: Testing throughput / GATT connection..."
# Advertiser (20 seconds)
"$APP_PATH" -u "$UART1" --adv --time 20000 > "$TEST_DATA_DIR/advertiser_output.txt" 2>&1 &
PID1=$!
"$APP_PATH" -u "$UART2" --conn="$MAC_ADDR1" --time 5000 --report 500 --throughput 0 > "$TEST_DATA_DIR/central_output.txt" 2>&1 
check_success "throughput test connection established"
log_message "waiting for background PID"
wait $PID1
assertion_failure "$TEST_DATA_DIR/advertiser_output.txt" # check advertiser log for assertion
COUNT="$(grep -Foc "Throughput since last report" "$TEST_DATA_DIR/central_output.txt" )"
if [ $COUNT -gt 5 ]; then
        log_message "SUCCESS: 5 or more throughput reports observed"
        printf 'Number of throughput reports observed: %s\n' "${COUNT:-<none>}"
    else
        log_message "Failure: Less than 5 throughput reports observed"
        printf 'Number of throughput reports observed: %s\n' "${COUNT:-<none>}"
        exit 1 # Exit on failure
    fi

# 10. Test setting custom MAC address
log_message "Test 10: Performing custom MAC address test..."
"$APP_PATH" -u "$UART1" --addr_set 01:02:03:04:05:06 --time 1 > "$TEST_DATA_DIR/basic_output.txt" 2>&1
check_success "custom MAC address set"
"$APP_PATH" -u "$UART1" --time 1 > "$TEST_DATA_DIR/basic_output.txt" 2>&1
MAC_ADDR="$(
  # Example line: MAC address: 0C:43:14:F0:2F:8E
  sed -n -E 's/^MAC address: ([0-9A-F]{2}(:[0-9A-F]{2}){5})$/\1/p' "$TEST_DATA_DIR/basic_output.txt" | head -n1
)"
printf 'MAC read: %s\n' "${MAC_ADDR:-<none>}"
if [ "$MAC_ADDR" = "01:02:03:04:05:06" ]; then
        log_message "SUCCESS: custom MAC read equal to write"
    else
        log_message "FAILURE: custom MAC read not equal to write"
        exit 1 # Exit on failure
    fi


# 11. Get fwrev 
log_message "Test 11: Performing fwver_get test..."
"$APP_PATH" -u "$UART2" --fwver_get > "$TEST_DATA_DIR/basic_output.txt" 2>&1
check_success "fwver_get"

# --- Post-test Cleanup ---
log_message "Cleaning up test data..."
rm -rf "$TEST_DATA_DIR"
check_success "Cleanup completed"

log_message "All release tests completed successfully for $APP_NAME."
exit 0