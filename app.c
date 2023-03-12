/***************************************************************************//**
 * @file
 * @brief Empty NCP-host Example Project.
 *
 * Reference implementation of an NCP (Network Co-Processor) host, which is
 * typically run on a central MCU without radio. It can connect to an NCP via
 * VCOM to access the Bluetooth stack of the NCP and to control it using BGAPI.
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include "app.h"
#include "app_gattdb.h"
#include "ncp_host.h"
#include "app_log.h"
#include "app_log_cli.h"
#include "app_assert.h"
#include "sl_bt_api.h"
#include <getopt.h>
#include <sys/stat.h> //for umask
#include <time.h>
#include <inttypes.h>
#include <stdint.h>
#include <assert.h>

// Optstring argument for getopt.
#define OPTSTRING      NCP_HOST_OPTSTRING APP_LOG_OPTSTRING "hv"

// Usage info.
#define USAGE          APP_LOG_NL "%s " NCP_HOST_USAGE APP_LOG_USAGE " [-h]" \
                        APP_LOG_NL OPTIONS

// Options info.
#define OPTIONS    \
  "\nOPTIONS\n"    \
  NCP_HOST_OPTIONS \
  APP_LOG_OPTIONS  \
"  -h                      Print help message\n"\
"  -u <UART port name>                        \n"\
"  -t <tcp address>\n"\
"  -b <baud_rate, default 115200>\n"\
"  -f                      Enable hardware flow control\n"\
"  --version               Print version number defined in application.\n"\
"  --time   <duration ms>      Set time for test in milliseconds, 0 for infinite mode (exit with control-c)\n"\
"  --packet_type <payload/modulation type, 0:PBRS9 packet payload, 1:11110000 packet payload, 2:10101010 packet payload, 4:111111 packet payload, 5:00000000 packet payload, 6:00001111 packet payload, 7:01010101 packet payload, 253:PN9 continuously modulated, 254:unmodulated carrier>\n"\
"  --power  <power level>      Set power level for test in 0.1dBm steps\n"\
"  --channel <channel index>   Set channel index for test, frequency=2402 MHz + 2*channel>\n"\
"  --len <test packet length>  Set test packet length, ignored for unmodulated carrier>\n"\
"  --rx                        TM receive test. Prints number of received DTM packets.\n"\
"  --ctune_set <ctune val>     Set 16-bit crystal tuning value, e.g. 0x0136\n"\
"  --addr_set  <MAC>           Set 48-bit MAC address, e.g. 01:02:03:04:05:06\n"\
"  --ctune_get                 Read 16-bit crystal tuning value\n"\
"  --fwver_get                 Read FW revision string from Device Information (GATT)\n"\
"  --adv                       Enter an advertisement mode with scan response for TIS/TRP chamber and connection testing (default period = 100ms)\n"\
"  --adv_period  <period>      Set advertising period for the advertisement mode, in units of 0.625ms\n"\
"  --cust <ASCII hex string>   Allows verification/running custom BGAPI commands. Example of ASCII hex string: a50102feed\n"\
"  --phy  <PHY selection for test packets/waveforms/RX mode, 1:1Mbps, 2:2Mbps, 3:125k LR coded, 4:500k LR coded.>\n"\
"  --advscan                   Return RSSI, channel, and MAC address for advertisement scan results\n"\
"  --advscan=<MAC>             Set optional MAC address for advertising scan filtering, e.g. 01:02:03:04:05:06\n"\
"  --rssi_avg <number of packets to include in RSSI average reports for advscan>\n"\
"  --conn=<MAC>                Connect as central to 48-bit MAC address, e.g. 01:02:03:04:05:06\n"\
"  --conn_int <conn interval>  Set connection interval for central connection, in units of 1.25ms\n"\
"  --coex                      Enable coexistence on the target if available\n"\
"  --throughput <0 or 1>       Push dummy throughput data when connected as central to another unit running BLEtest as an advertiser, with ack (1) or without ack (0)\n"\
"  --report  <interval>        Print the channel map and throughput (if applicable) at the specified interval in milliseconds\n"

  #define LONG_OPT_VERSION 0
  #define LONG_OPT_TIME 1
  #define LONG_OPT_PACKET_TYPE 2
  #define LONG_OPT_POWER 3
  #define LONG_OPT_CHANNEL 4
  #define LONG_OPT_LEN 5
  #define LONG_OPT_RX 6
  #define LONG_OPT_CTUNE_SET 7
  #define LONG_OPT_CTUNE_GET 8
  #define LONG_OPT_ADDR_SET 9
  #define LONG_OPT_FWVER 10
  #define LONG_OPT_ADV 11
  #define LONG_OPT_DAEMON 12
  #define LONG_OPT_CUST 13
  #define LONG_OPT_PHY 14
  #define LONG_OPT_ADVSCAN 15u
  #define LONG_OPT_RSSI_AVG 16u
  #define LONG_OPT_CONN 17u
  #define LONG_OPT_CONN_INT 18u
  #define LONG_OPT_ADV_PERIOD 19u
  #define LONG_OPT_COEX 20u
  #define LONG_OPT_THROUGHPUT 21u
  #define LONG_OPT_REPORT 22u

  static struct option long_options[] = {
             {"version",    no_argument,       0,  LONG_OPT_VERSION },
             {"time",       required_argument, 0,  LONG_OPT_TIME },
             {"packet_type",required_argument, 0,  LONG_OPT_PACKET_TYPE },
             {"power",      required_argument, 0,  LONG_OPT_POWER },
             {"channel",    required_argument, 0,  LONG_OPT_CHANNEL },
             {"len",        required_argument, 0,  LONG_OPT_LEN },
             {"rx",         no_argument,       0,  LONG_OPT_RX },
             {"ctune_set",  required_argument, 0,  LONG_OPT_CTUNE_SET },
             {"ctune_get",  no_argument,       0,  LONG_OPT_CTUNE_GET },
             {"addr_set",   required_argument, 0,  LONG_OPT_ADDR_SET },
             {"fwver_get",  no_argument,       0,  LONG_OPT_FWVER },
             {"adv",        no_argument,       0,  LONG_OPT_ADV },
             {"cust",       required_argument, 0,  LONG_OPT_CUST },
             {"phy",        required_argument, 0,  LONG_OPT_PHY },
             {"advscan",    optional_argument, 0,  LONG_OPT_ADVSCAN },
             {"rssi_avg",   required_argument, 0,  LONG_OPT_RSSI_AVG },
             {"conn",       required_argument, 0,  LONG_OPT_CONN },
             {"conn_int",   required_argument, 0,  LONG_OPT_CONN_INT },
             {"adv_period", required_argument, 0,  LONG_OPT_ADV_PERIOD },
             {"coex",       no_argument,       0,  LONG_OPT_COEX },
             {"throughput", required_argument, 0,  LONG_OPT_THROUGHPUT},
             {"report",     required_argument, 0,  LONG_OPT_REPORT},
             {0,           0,                 0,  0  }};

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

// Handle for dynamic GATT Database session.
uint16_t gattdb_session;

#define VERSION_MAJ	2u
#define VERSION_MIN	10u

#define TRUE   1u
#define FALSE  0u

#define DEFAULT_PHY test_phy_1m	//default to use 1Mbit PHY

/* Program defaults - will use these if not specified on command line */
#ifndef DEFAULT_DURATION
  #define DEFAULT_DURATION		0	//default=0 (can override with global #define)
#endif
#ifndef DEFAULT_PACKET_TYPE
  #define DEFAULT_PACKET_TYPE		sl_bt_test_pkt_carrier 	//unmodulated carrier
#endif
#define DEFAULT_CHANNEL			0	//2.402 GHz
#define DEFAULT_POWER_LEVEL		50	//5 dBm
#define DEFAULT_PACKET_LENGTH	25
#define CMP_LENGTH	2

#define CANCEL_TIMEOUT_SECONDS 3 //quit after 3 seconds if no response

/**
 * Configurable parameters that can be modified to match the test setup.
 */

/* The modulation type */
static uint8_t packet_type=DEFAULT_PACKET_TYPE;

/* The power level */
static int16_t power_level=DEFAULT_POWER_LEVEL;

/* The duration in us */
static uint32_t duration_usec=DEFAULT_DURATION;

/* channel */
static uint8_t channel=DEFAULT_CHANNEL;

/* packet length */
static uint8_t packet_length=DEFAULT_PACKET_LENGTH;

/* phy */
static uint8_t selected_phy=DEFAULT_PHY;

/* advertising test mode */
#define TEST_ADV_INTERVAL_MS_DEFAULT 100 //100ms default
#define ADV_INTERVAL_UNIT 0.625 //per API guide

#define MAX_POWER_LEVEL 200 //deci-dBm
#define MIN_POWER_LEVEL -300 //deci-dBm

static uint16_t adv_period = TEST_ADV_INTERVAL_MS_DEFAULT/ADV_INTERVAL_UNIT;
static uint8_t adv_data[] = {0x02, 0x01, 0x06, 0x03, 0x03, 0x02, 0x18}; //LE general discoverable, FIND ME service (0x1802)
/* 20 byte length, 0x09 (full name), "Blue Gecko Test App" */
static uint8_t scan_rsp_data[] = {20,0x09,0x42,0x6c,0x75,0x65,0x20,0x47,0x65,0x63,0x6b,0x6f,0x20,0x54,0x65,0x73,0x74,0x20,0x41,0x70,0x70};

/* flash write state machine */
static enum ps_states {
  ps_none,
  ps_write_mac,
  ps_write_ctune,
  ps_read_ctune,
  ps_read_gatt_fwversion //note that this isn't really a PS command, but we'll leave it here
} ps_state = ps_none;

/* application state machine */
static enum app_states {
  adv_test_advertising,
  adv_test_connected,
  dtm_rx_begin,
  dtm_tx_begin,
  dtm_rx_started,
  dtm_tx_started,
  default_state,
  verify_custom_bgapi,
  advscan_wait,
  advscan_run,
  conn_initiate,
  conn_pending,
  connected
} app_state =   default_state;

/*
   Enumeration of possible throughput states
 */
enum throughput_states {
  THROUGHPUT_NONE,   // default state
  THROUGHPUT_CONNECT,
  THROUGHPUT_FIND_SERVICES,  // find throughput service
  THROUGHPUT_FIND_CHARACTERISTICS,  // find throughput control&data attributes
  THROUGHPUT_NOACK,  // Running throughput test no ACK
  THROUGHPUT_ACK,   // Running throughput test with ACK
} throughput_state = THROUGHPUT_NONE;
void throughput_change_state(enum throughput_states new_state);

static uint8_t bletest_throughput_ack = false;

//Throughput handles
static uint32_t bletest_throughput_service_handle = 0xFFFFFFFF;
static uint16_t bletest_throughput_write_with_response_handle = 0xFFFF;
static uint16_t bletest_throughput_write_no_response_handle = 0xFFFF;

#define UUID_LEN                                    16

// these are defined in app_gattdb.c
extern const uint8_t bletest_throughput_service_uuid[];
extern const uint8_t bletest_throughput_write_with_response_characteristic_uuid[];
extern const uint8_t bletest_throughput_write_no_response_characteristic_uuid[];

static void process_procedure_complete_event(sl_bt_msg_t *evt);
static void check_characteristic_uuid(sl_bt_msg_t *evt);

uint8_t bletest_throughput_payload_data[244] = { 0, };

uint64_t bletest_throughput_total_bytes = 0;

void timer_on_report(void);

#define REPORT_TIMER_HANDLE 42u //random number for timer handle

#define MAX_CUST_BGAPI_STRING_LEN  16u
uint8_t cust_bgapi_data[MAX_CUST_BGAPI_STRING_LEN/2]; //half of string length due to ascii hex data in string
size_t cust_bgapi_len; //how many bytes in cust_bgapi_data

#define CTUNE_PSKEY_LENGTH	2u
#define MAX_CTUNE_VALUE 511u

#define GATT_FWREV_LO 0x26
#define GATT_FWREV_HI 0x2A
#define FWREV_TYPE_LEN 2
static uint8_t fwrev_type_data[FWREV_TYPE_LEN] = {GATT_FWREV_LO,GATT_FWREV_HI};

static uint16_t ctune_value=0; //unsigned int representation of ctune
static uint8_t ctune_array[CTUNE_PSKEY_LENGTH]; //ctune value to be stored (little endian)
static bd_addr new_address; //mac address to be stored
static bd_addr scan_filt_address; //mac address for adv scan filtering
static uint8_t scan_filt_flag=false; //true if scan filtering is specified
static uint32_t scan_counter; //scan result count
static uint32_t rssi_len; //number of packets to average
static int32_t rssi_sum; //signed sum of RSSI
static uint32_t rssi_count;

static size_t ctune_ret_len;

static int64_t start_time_us=0;
static int64_t last_send_time_us=0;
static int64_t last_report_time_us=0;

/* Store NCP version information */
static uint8_t version_major;
static uint8_t version_minor;
static uint8_t version_patch;

static bd_addr conn_address; //mac address for connection
static uint8_t conn_handle;
static uint8_t timeout_count=0;
#define CONN_INTERVAL_DEFAULT 16u // 16/1.25ms = 20ms
#define CONN_INTERVAL_UNIT_MS 1.25
static uint16_t conn_interval=CONN_INTERVAL_DEFAULT; //connection interval for central connection
#define SUP_TIMEOUT_FACTOR 4u   //how many connection intervals pass before timeout occurs
#define SUP_TIMEOUT_VAL_MIN 10u //minimum timeout value in API

static uint8_t coex_enabled=false;

static uint16_t map_interval_ms=0;

static void print_address(bd_addr address);
static void initiate_connection(void);
void print_packet_counters(void);
void print_coex_counters(void);

static int64_t cur_time_us(void);

unsigned int toInt(char c) {
  /* Convert ASCII hex to binary, return -1 if error */
  if (c >= '0' && c <= '9') {
    return      c - '0';
  } else if (c >= 'A' && c <= 'F') {
    return 10 + c - 'A';
  } else if (c >= 'a' && c <= 'f') {
    return 10 + c - 'a';
  } else {
    return -1; //error
  }

}

// Initialize GATT database dynamically.
static void initialize_gatt_database(void);

static void main_app_handler(void);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(int argc, char *argv[])
{
  sl_status_t sc;
  int opt;
  int option_index = 0;
  char *temp;
  uint8_t i; //local counter variable
  int values[8]; //local vars for bluetooth address

  /* locals for custom bgapi string */
  uint8_t string_len;
  int8_t upper_nib;
  int8_t lower_nib;

  // Process command line options.
  while ((opt = getopt_long(argc, argv, OPTSTRING, long_options, &option_index)) != -1) {
    switch (opt) {
      // Print help.
      case 'h':
        app_log(USAGE, argv[0]);
        app_log(OPTIONS);
        exit(EXIT_SUCCESS);

      case 'v':
      case LONG_OPT_VERSION:
        printf("%s version %d.%d\n",argv[0],VERSION_MAJ,VERSION_MIN);
        break;

      case LONG_OPT_TIME:
        duration_usec = atoi(optarg)*1000;
        break;

      case LONG_OPT_PACKET_TYPE:
        /* packet / modulation type */
        packet_type = (uint8_t) strtoul(optarg, &temp, 0);
        break;

      case LONG_OPT_POWER:
        power_level = atoi(optarg);
        if (power_level>200)
        {
          printf("Error in power level: max value 200 (20.0 dBm)\n");
          exit(EXIT_FAILURE);
        }
        break;

      case LONG_OPT_CHANNEL:
        channel = atoi(optarg);
        if (channel>39)
        {
          printf("Error in channel: max value 39\n");
          exit(EXIT_FAILURE);
        }
        break;

      case LONG_OPT_LEN:
        packet_length = atoi(optarg);
        if (packet_length>255 || packet_length < 0)
        {
          printf("Error in packet length %d, must be in the range 0-255\n", packet_length);
          exit(EXIT_FAILURE);
        }
        break;

      case LONG_OPT_RX:
        /* DTM receive */
        app_state = dtm_rx_begin;
        break;

      case LONG_OPT_CTUNE_SET:
        /* xtal ctune */
        ctune_value = (uint16_t) strtoul(optarg,&temp,0);
        /* check range */
        if (ctune_value > MAX_CTUNE_VALUE)
        {
          printf("Error! Ctune value entered (0x%2x) is larger than the maximum allowed value of 0x%2x.\n",
          ctune_value, MAX_CTUNE_VALUE);
          exit(EXIT_FAILURE);
        } else
        {
          ps_state = ps_write_ctune; //init ps state machine
          /* store in ctune_array as little endian */
          /* Test for a little-endian machine */
          #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            /* ctune_value is little endian - LSB in low byte */
            ctune_array[0] = (uint8_t) (ctune_value & 0xff);
            ctune_array[1] = (uint8_t) ((ctune_value >> 8) & 0xff);
          #else
            /* ctune_value is big endian - LSB in high byte */
            ctune_array[1] = (uint8_t) ((ctune_value & 0xff);
            ctune_array[0] = (uint8_t) ((ctune_value >> 8) & 0xff);
          #endif
        }
        break;

      case LONG_OPT_CTUNE_GET:
        ps_state = ps_read_ctune; //init ps state machine
        break;

      case LONG_OPT_ADDR_SET:
        /* set bluetooth address */
        ps_state = ps_write_mac;
        if( 6 == sscanf(optarg, "%x:%x:%x:%x:%x:%x", &values[5], &values[4],
          &values[3], &values[2], &values[1], &values[0]) )
        {
          /* convert to uint8_t */
          for( i = 0; i < 6; ++i )
            new_address.addr[i] = (uint8_t) values[i];
        }
        else
        {
          /* invalid mac */
          printf("Error in mac address - enter 6 ascii hex bytes separated by ':'\n");
          exit(EXIT_FAILURE);
        }
        break;

      case LONG_OPT_FWVER:
        /* read gatt fw version - can be overwritten by other options */
        ps_state = ps_read_gatt_fwversion;
        break;

      case LONG_OPT_ADV:
        /* advertise test for TIS / TRP and connection testing */
        app_state = adv_test_advertising;
        break;

      case LONG_OPT_CUST:
        /* Verify custom BGAPI by sending a custom BGAPI command and printing the response */
        string_len = strlen(optarg);
        cust_bgapi_len = string_len/2;
        if (string_len > MAX_CUST_BGAPI_STRING_LEN)
        {
          printf("String too long in -b argument: string length %zu, max = %d\n",strlen(optarg),MAX_CUST_BGAPI_STRING_LEN);
          exit(EXIT_FAILURE);
        }

        for (i=0; i != cust_bgapi_len; i++) {
          /* Convert arg string (e.g. "040101") to binary data */
          upper_nib = toInt(optarg[2*i]);
          lower_nib = toInt(optarg[2*i+1]);
          if (lower_nib < 0 || upper_nib < 0) {
            /* Problem with conversion - print error and exit */
            printf("Error! \"%s\" is an invalid ascii hex string. The characters need to be A-F, a-f, or 0-9.\n",optarg);
            exit(EXIT_FAILURE);
          } else {
            cust_bgapi_data[i] = (uint8_t) (16*upper_nib) + lower_nib;
          }
        }
        app_state = verify_custom_bgapi;
        break;

      case LONG_OPT_PHY:
        /* Select PHY for test packets/waveforms */
        selected_phy = atoi(optarg);
        if (selected_phy != test_phy_1m && selected_phy != test_phy_2m &&
          selected_phy != test_phy_125k && selected_phy != test_phy_500k ) {
            printf("Error! Invalid phy argument, 0x%02x\n",selected_phy);
            exit(EXIT_FAILURE);
        }
        break;

      case LONG_OPT_RSSI_AVG:
        rssi_len = atoi(optarg);
        break;

      case LONG_OPT_ADVSCAN:
        app_state = advscan_wait;

        if (optarg) {
          /* Optional parameter is present, so process it */
          /* set bluetooth address */
          if( 6 == sscanf(optarg, "%x:%x:%x:%x:%x:%x", &values[5], &values[4],
            &values[3], &values[2], &values[1], &values[0]) )
          {
            /* convert to uint8_t */
            for( i = 0; i < 6; ++i ) {
              scan_filt_address.addr[i] = (uint8_t) values[i];
            }
            scan_filt_flag = true;
          }
          else
          {
            /* invalid mac */
            printf("Error in advscan mac address - enter 6 ascii hex bytes separated by ':'\n");
            exit(EXIT_FAILURE);
          }
        }
        break;

      case LONG_OPT_CONN:
        app_state = conn_initiate;
        /* set bluetooth address */
        if( 6 == sscanf(optarg, "%x:%x:%x:%x:%x:%x", &values[5], &values[4],
          &values[3], &values[2], &values[1], &values[0]) )
        {
          /* convert to uint8_t */
          for( i = 0; i < 6; ++i ) {
            conn_address.addr[i] = (uint8_t) values[i];
          }
        }
        else
        {
          /* invalid mac */
          printf("Error in conn mac address - enter 6 ascii hex bytes separated by ':'\n");
          exit(EXIT_FAILURE);
        }
        break;

      case LONG_OPT_CONN_INT:
        /* connection interval for central connection */
        conn_interval = atoi(optarg);
        break;

      case LONG_OPT_ADV_PERIOD:
        /* advertising period for advertising mode */
        adv_period = atoi(optarg);
        break;

      case LONG_OPT_COEX:
        /* enable coexistence on the target */
        coex_enabled = true;
        break;

      case LONG_OPT_THROUGHPUT:
        /* enable throughput when connecting as a central */
        throughput_state = THROUGHPUT_CONNECT;
        if (atoi(optarg) != 0) {
          bletest_throughput_ack = true;
        }
        break;

      case LONG_OPT_REPORT:
        /* set the report interval */
        map_interval_ms = atoi(optarg);
        break;

      // Process options for other modules.
      default:
        sc = ncp_host_set_option((char)opt, optarg);
        if (sc == SL_STATUS_NOT_FOUND) {
          sc = app_log_set_option((char)opt, optarg);
        }
        if (sc != SL_STATUS_OK) {
          app_log(USAGE, argv[0]);
          exit(EXIT_FAILURE);
        }
        break;
    }
  }

  // Initialize NCP connection.
  sc = ncp_host_init();
  if (sc == SL_STATUS_INVALID_PARAMETER) {
    app_log(USAGE, argv[0]);
    exit(EXIT_FAILURE);
  }
  app_assert_status(sc);

  printf("\n------------------------\n");
  printf("Waiting for boot pkt...\n");
  // Reset NCP to ensure it gets into a defined state.
  // Once the chip successfully boots, boot event should be received.
  sl_bt_system_reset(sl_bt_system_boot_mode_normal);

  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
  sl_status_t sc;
  if (app_state == advscan_run && duration_usec != 0) {
    // Check for advscan timeout here (deinit to stop, print, exit)
    if (cur_time_us() > start_time_us + duration_usec ) {
      app_deinit();
    }
  }

  // if we are in throughput noack mode, write values as fast as possible
  if (throughput_state == THROUGHPUT_NOACK && bletest_throughput_ack == false ) {
      if (cur_time_us() > last_send_time_us + 1000) {
        // send notifications from central to peripheral if enabled
        uint16_t bytes_written;
        printf(".");
        fflush(stdout);
        sc = sl_bt_gatt_write_characteristic_value_without_response(conn_handle,
                                                  bletest_throughput_write_no_response_handle,
                                                  sizeof(bletest_throughput_payload_data),
                                                  bletest_throughput_payload_data,
                                                  &bytes_written);
        app_assert_status(sc);
        last_send_time_us = cur_time_us();
        bletest_throughput_total_bytes += sizeof(bletest_throughput_payload_data);
      }

  }
}

/**************************************************************************//**
 * Application Deinit.
 *****************************************************************************/
void app_deinit(void)
{
  time_t start_time;
  uint8_t exitwhile=false;
  sl_status_t sc;
  sl_bt_msg_t evt;

  /* if DTM is in process, end it prior to closing */
  if (app_state == dtm_rx_begin || app_state == dtm_tx_begin)
  {
    printf("Canceling DTM in progress...\n");
    start_time = time(NULL);
    sc = sl_bt_test_dtm_end();
    app_assert_status(sc);
    do {
      sc = sl_bt_pop_event(&evt);

      if (sc == SL_STATUS_OK) {
        if (SL_BGAPI_MSG_ID(evt.header) == sl_bt_evt_test_dtm_completed_id) {
          if (app_state == dtm_rx_begin) {
            // Process dtm_completed thrown when started
            app_state = dtm_rx_started;
          } else if (app_state == dtm_tx_begin) {
            // Process dtm_completed thrown when started
            app_state = dtm_tx_started;
          } else if (app_state == dtm_tx_started) {
          printf("DTM completed, number of packets transmitted: %d\n",evt.data.evt_test_dtm_completed.number_of_packets);
          exitwhile = true;
        } else if (app_state == dtm_rx_started) {
          printf("DTM completed, number of packets received: %d\n",evt.data.evt_test_dtm_completed.number_of_packets);
          exitwhile = true;
        } else {
          printf("sl_bt_evt_test_dtm_completed_id with unknown app state\n");
          exitwhile = true;
        }
        }
      }

      if ((time(NULL) - start_time) >= CANCEL_TIMEOUT_SECONDS) { //timeout in seconds
        exitwhile = true;
      }

    } while (exitwhile == false);

  } else if (app_state == advscan_run) {
    // Turn off scan and print the number of scan results received
    printf("Exiting scan mode, total scan packets received = %u\r\n", scan_counter);
    sc = sl_bt_scanner_stop();
    app_assert_status(sc);
  } else if (app_state == connected || app_state == adv_test_connected) {
    // clean up connetion if connected
    printf("Disconnecting...\r\n");
    start_time = time(NULL);
    sc = sl_bt_connection_close(conn_handle);
    app_log_debug("sl_bt_connection_close, status=0x%x\r\n", sc);
    do {
      sc = sl_bt_pop_event(&evt);
      app_log_debug("sl_bt_pop_event, status=0x%x\r\n", sc);
      if (sc == SL_STATUS_OK) {
        if (SL_BGAPI_MSG_ID(evt.header) == sl_bt_evt_connection_closed_id) {
          printf("Disconnected!\r\n");
          exitwhile = true;
        }
      }

      if ((time(NULL) - start_time) >= CANCEL_TIMEOUT_SECONDS) { //timeout in seconds
        exitwhile = true;
      }

    } while (exitwhile == false);
    print_packet_counters();
  } else if (app_state == adv_test_advertising) {
    printf("Stopping advertisements...\r\n");
    sc = sl_bt_advertiser_stop(advertising_set_handle);
    app_assert_status(sc);
  }
  if (app_state == adv_test_connected || app_state == adv_test_advertising || app_state == connected || app_state == conn_pending \
        || app_state == conn_initiate) {
    app_log_debug("Supervision timeout count: %d\r\n", timeout_count);
    if (coex_enabled == true) {
      /* try to print coex counters */
      print_coex_counters();
    }
  }
  ncp_host_deinit();

  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application deinit code here!                       //
  // This is called once during termination.                                 //
  /////////////////////////////////////////////////////////////////////////////

  // Force exit here
  exit(EXIT_SUCCESS);
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;
  int16_t power_level_set_min, power_level_set_max;
  uint16_t null_var;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      /* Store version info */
      version_major = evt->data.evt_system_boot.major;
      version_minor = evt->data.evt_system_boot.minor;
      version_patch = evt->data.evt_system_boot.patch;
      // Print boot message.
      printf("\nboot pkt rcvd: gecko_evt_system_boot(%d, %d, %d, %d, 0x%8x, %d)\n",
               version_major,
               version_minor,
               version_patch,
               evt->data.evt_system_boot.build,
               evt->data.evt_system_boot.bootloader,
               evt->data.evt_system_boot.hw);
      // Extract unique ID from BT Address.
      sc = sl_bt_system_get_identity_address(&address, &address_type);
      app_assert_status(sc);
      printf("MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
        address.addr[5], // <-- address is little-endian
        address.addr[4],
        address.addr[3],
        address.addr[2],
        address.addr[1],
        address.addr[0]);

      // Set power limits to max (note - max will generally be internally
      // limited to 10 dBm without AFH component)
      sc = sl_bt_system_set_tx_power(MIN_POWER_LEVEL, MAX_POWER_LEVEL, &power_level_set_min, &power_level_set_max);
      app_assert_status(sc);

      // Initialize GATT database dynamically.
      initialize_gatt_database();

      if (throughput_state != THROUGHPUT_NONE) {
        /* Init test data buffer with alphabet pattern */
        for (int i=0; i < (sizeof(bletest_throughput_payload_data)/sizeof(*bletest_throughput_payload_data)); i++){
          bletest_throughput_payload_data[i] = 'a' + (i % 26);
        }
      }

      main_app_handler();
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:

      printf("Connection opened." APP_LOG_NL);
      // reset connection packet debug counters
      sc = sl_bt_system_get_counters(true, &null_var, &null_var,
                                    &null_var, &null_var);
      
      sc = sl_bt_connection_set_preferred_phy(conn_handle,
                                              selected_phy,
                                              0xff);
      // get connection RSSI
      sc = sl_bt_connection_get_rssi(evt->data.evt_connection_opened.connection);
      app_assert_status(sc);
      if (app_state == conn_pending) {
        // handle connection mode as central
        app_state = connected;
        if (throughput_state != THROUGHPUT_NONE) {
        sc = sl_bt_gatt_discover_primary_services_by_uuid(conn_handle,
                                                        UUID_LEN,
                                                        bletest_throughput_service_uuid);
        app_assert_status(sc);     
        throughput_state = THROUGHPUT_FIND_SERVICES;              
        }
      } else if (app_state == adv_test_advertising) {
        app_state = adv_test_connected;
        conn_handle = evt->data.evt_connection_opened.connection;
      }
      if (map_interval_ms != 0) {
        sc = sl_bt_system_set_lazy_soft_timer((uint32_t) 32 * map_interval_ms,
                                          0,
                                          REPORT_TIMER_HANDLE,
                                          false);
        app_assert_status(sc);
      }
      break;

    case sl_bt_evt_dfu_boot_id:
      printf("DFU Boot, Version 0x%x\n",evt->data.evt_dfu_boot.version);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
    // first stop report timer
      sc = sl_bt_system_set_lazy_soft_timer((uint32_t) 0u,
                                          0,
                                          REPORT_TIMER_HANDLE,
                                          false);
      app_assert_status(sc);
      printf("Disconnected from central" APP_LOG_NL);
      app_log_debug("Disconnect reason:0x%2x\r\n",evt->data.evt_connection_closed.reason);
      print_packet_counters();
      if (evt->data.evt_connection_closed.reason == SL_STATUS_BT_CTRL_CONNECTION_TIMEOUT) {
        // increment supervision timeout counter
        timeout_count++;
      }

      // Restart advertising after client has disconnected.
      if (app_state == adv_test_connected)
      {
        sc = sl_bt_advertiser_start(
          advertising_set_handle,
          sl_bt_advertiser_general_discoverable,
          sl_bt_advertiser_connectable_scannable);
        app_assert_status(sc);
        printf("Started advertising." APP_LOG_NL);
        app_state = adv_test_advertising;
      } else if (app_state == connected)
      {
        // we were connected as central, so try to reconnect
        initiate_connection();
      }
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_dfu_boot_failure_id:
      /* DFU boot failure detected */
      printf("DFU boot failure detected, reason 0x%2x: ",evt->data.evt_dfu_boot_failure.reason);
      /* Print a more useful reason */
      switch (evt->data.evt_dfu_boot_failure.reason) {
        case SL_STATUS_SECURITY_IMAGE_CHECKSUM_ERROR:
          printf("device firmware signature/checksum verification failed.\n");
          break;
        default:
          break;
        }
        printf("Run the uart_dfu application to update the firmware.\n");
        exit(EXIT_FAILURE); //terminate
      break;

      case sl_bt_evt_test_dtm_completed_id:
        if (app_state == dtm_rx_begin) {
          //This is just an acknowledgement of the DTM start - set a flag for the next event which is the end
          app_state = dtm_rx_started;
        } else if (app_state == dtm_tx_begin ) {
          //This is just an acknowledgement of the DTM start - set a flag for the next event which is the end
          app_state = dtm_tx_started;

        } else if (app_state == dtm_rx_started) {
          //This is the event received at the end of the test
          printf("DTM receive completed. Number of packets received: %d\n",evt->data.evt_test_dtm_completed.number_of_packets);
          exit(EXIT_SUCCESS);	//test done - terminate
        } else if (app_state == dtm_tx_started ) {
          /* Not sure how we got here - but exit anyways */
          printf("DTM completed, number of packets transmitted: %d\n",evt->data.evt_test_dtm_completed.number_of_packets);
          exit(EXIT_SUCCESS); //test done - terminate
        }
        break;

      case sl_bt_evt_scanner_legacy_advertisement_report_id:
        if (scan_filt_flag == true) {
          if (evt->data.evt_scanner_legacy_advertisement_report.address.addr[5] != scan_filt_address.addr[5] ||
            evt->data.evt_scanner_legacy_advertisement_report.address.addr[4] != scan_filt_address.addr[4] ||
            evt->data.evt_scanner_legacy_advertisement_report.address.addr[3] != scan_filt_address.addr[3] ||
            evt->data.evt_scanner_legacy_advertisement_report.address.addr[2] != scan_filt_address.addr[2] ||
            evt->data.evt_scanner_legacy_advertisement_report.address.addr[1] != scan_filt_address.addr[1] ||
            evt->data.evt_scanner_legacy_advertisement_report.address.addr[0] != scan_filt_address.addr[0] )
           {
            // scan doesn't match the filter - discard
            break;
          }
        }
        scan_counter++;
        // print scan packet info if not averaging
        if (rssi_len == 0) {
          printf("ADV RCVD from MAC ");
	        print_address(evt->data.evt_scanner_legacy_advertisement_report.address);
          printf(", Channel: %d",evt->data.evt_scanner_legacy_advertisement_report.channel);
			    printf(", RSSI: %d\r\n", evt->data.evt_scanner_legacy_advertisement_report.rssi);
        } else {
          // Handle averaging
          rssi_sum += evt->data.evt_scanner_legacy_advertisement_report.rssi;
          rssi_count++;
          if (rssi_count >= rssi_len) {
            // print average and reset totals
            printf("AVG RSSI REPORT: %2.2f\r\n",(float)rssi_sum/(float)rssi_count);
            rssi_count = 0;
            rssi_sum = 0;
          }
        }
    break;

    case sl_bt_evt_connection_parameters_id:
      app_log_debug("Conn params interval=%3f ms, timeout: %d ms\r\n",
                    (float)evt->data.evt_connection_parameters.interval * 1.25,
                    evt->data.evt_connection_parameters.timeout * 10);
      break;

    case sl_bt_evt_connection_rssi_id:
      printf("Connection RSSI=%d dBm\r\n", evt->data.evt_connection_rssi.rssi);
      break;

    case sl_bt_evt_gatt_mtu_exchanged_id:
      app_log_debug("MTU exchanged, MTU:%d\r\n", evt->data.evt_gatt_mtu_exchanged.mtu);
      break;

    case sl_bt_evt_gatt_procedure_completed_id:
      process_procedure_complete_event(evt);
      break;

    case sl_bt_evt_gatt_characteristic_id:
      check_characteristic_uuid(evt);
      break;
    
    case sl_bt_evt_system_soft_timer_id:
      if (evt->data.evt_system_soft_timer.handle == REPORT_TIMER_HANDLE){
        timer_on_report();
      }
      break;

    case sl_bt_evt_gatt_service_id:
      if (evt->data.evt_gatt_service.uuid.len == UUID_LEN) {
        if (memcmp(bletest_throughput_service_uuid, evt->data.evt_gatt_service.uuid.data, UUID_LEN) == 0) {
          bletest_throughput_service_handle = evt->data.evt_gatt_service.service;
        }
      }
      break;
    
    case sl_bt_evt_gatt_server_attribute_value_id:
      // receiving throughput data as server - show something
      printf(".");
      fflush(stdout);
      break;

    case sl_bt_evt_connection_phy_status_id:
      // report phy changes
      app_log_info("PHY update procedure completed, new phy = 0x%x\r\n", (uint8_t) evt->data.evt_connection_phy_status.phy);

    break;

    case sl_bt_evt_connection_remote_used_features_id:
    case sl_bt_evt_connection_tx_power_id:
    case sl_bt_evt_gatt_characteristic_value_id: // receives notifications from central
      // Do nothing
      break;

    // -------------------------------
    // Default event handler.
    default:
    // Print unhandled events with debug log level
      app_log_debug("Unhandled event received, event ID: 0x%2x\n", SL_BT_MSG_ID(evt -> header));
      break;
  }
}

/**************************************************************************//**
 * Initialize GATT database dynamically.
 *****************************************************************************/
static void initialize_gatt_database(void)
{
  sl_status_t sc;

  // New session
  sc = sl_bt_gattdb_new_session(&gattdb_session);

  if (sc == SL_STATUS_NOT_SUPPORTED) {
    /* If dynamic GATT is not supported, just ignore for now */
    app_log_debug("warning: NCP firmware does not support dynamic GATT\r\n");
    return;
  } else {
    app_assert_status(sc);
  }

  // Add services
  for (service_index_t i = (service_index_t)0; i < SERVICES_COUNT; i++) {
    sc = app_gattdb_add_service(gattdb_session, &services[i]);
    app_assert_status(sc);
  }

  // Add characteristics
  for (characteristic_index_t i = (characteristic_index_t)0; i < CHARACTERISTICS_COUNT; i++) {
    sc = app_gattdb_add_characteristic(gattdb_session, &characteristics[i]);
    app_assert_status(sc);
  }

  // Start services and child characteristics
  for (service_index_t i = (service_index_t)0; i < SERVICES_COUNT; i++) {
    sc = sl_bt_gattdb_start_service(gattdb_session, services[i].handle);
    app_assert_status(sc);
  }

  // Commit changes
  sc = sl_bt_gattdb_commit(gattdb_session);
  app_assert_status(sc);
}

/**********************************************************
*   This is the main application handler, called by the boot event.
***********************************************************/
void main_app_handler(void) {

  sl_status_t sc;

  uint8_t i;
  uint16_t attribute_handle;
  uint8_t rev_str[50];
  size_t rev_str_len;
  int16_t power_level_set_min, power_level_set_max;
  size_t user_message_response_len;
  uint8_t user_message_response[50];

  /* Immediately exit if not the right NCP version */
  if ((version_major < 3) || ((version_major == 3) && (version_minor < 3) && (version_patch < 1))) {
    printf("ERROR: This software version requires Blue Gecko NCP version 3.3.1 or higher!\n");
    exit(EXIT_FAILURE);
  }

  /* Set up PTA/coexistence. Disabled by default so PTA doesn't gate the
      DTM outputs. Ignore and print debug message if not supported on target */
  if (coex_enabled == TRUE) {
    app_log_debug("Attempting to enable coexistence on the target\n");
    sc = sl_bt_coex_set_options(coex_option_enable, coex_option_enable);
    if (sc == SL_STATUS_OK) {
      app_log_debug("Coexistence enabled successfully!\n");
    }
  } else {
    app_log_debug("Attempting to disable coexistence on the target\n");
    sc = sl_bt_coex_set_options(coex_option_enable, 0x0000);
    if (sc == SL_STATUS_OK) {
      app_log_debug("Coexistence disabled successfully!\n");
    }
  }
  sc = sl_bt_coex_set_options(coex_option_enable, coex_enabled ? coex_option_enable : 0);
  if (sc == SL_STATUS_NOT_SUPPORTED) {
    if (coex_enabled) {
      app_log_debug("Could not enable coexistence - not available in the NCP target.\n");
    } else {
      app_log_debug("Could not disable coexistence - not available in the NCP target.\n");
    }
  } else if (sc) {
    // assert for other errors
    app_assert_status(sc);
  }
  /* Run test commands */
  /* deal with flash writes first, since reboot is needed to take effect */
  if(ps_state == ps_write_ctune) {
    /* Write ctune value and reboot */
    printf("Writing ctune value to 0x%04x\n", ctune_value);
    sc = sl_bt_nvm_save(SL_BT_NVM_KEY_CTUNE, sizeof(ctune_array), ctune_array);
    app_assert_status(sc);
    ps_state = ps_none; /* reset state machine */
    sl_bt_system_reset(sl_bt_system_boot_mode_normal);/* reset to take effect */
    printf("Rebooting with new ctune value...\n");
  } else if (ps_state == ps_read_ctune)
  {
    /* read the NVM3 key for CTUNE */
    printf("Reading flash NVM3 value for CTUNE\n");
    sc = sl_bt_nvm_load(SL_BT_NVM_KEY_CTUNE, sizeof(ctune_array), &ctune_ret_len, ctune_array);
    if (sc == SL_STATUS_BT_PS_KEY_NOT_FOUND ) {
      printf("CTUNE value not loaded in PS flash!\n");
    } else if (sc) {
      app_assert_status(sc);
    }
    else {
      printf("Stored CTUNE = 0x%04x\nRebooting...\n", (uint16_t)(ctune_array[0] | ctune_array[1] << 8));
    }
      /* Reset again and proceed with other commands */
      ps_state = ps_none; /* reset state machine */
      sl_bt_system_reset(sl_bt_system_boot_mode_normal);/* reset to take effect */

  } else if (ps_state == ps_read_gatt_fwversion)
  {
    /* Find the firmware revision string (UUID 0x2a26) in the local GATT and print it out */
    sc = sl_bt_gatt_server_find_attribute(0,FWREV_TYPE_LEN,fwrev_type_data, &attribute_handle);
    if (sc == SL_STATUS_BT_ATT_ATT_NOT_FOUND) {
        printf("Firmware revision string not found in the local GATT.\nRebooting...\n");
    } else if (sc)
    {
      app_assert_status(sc);
    } else {
      printf("Found firmware revision string at handle %d\n",attribute_handle);

      /* This is the FW revision string from the GATT - load it and print it as an ASCII string */
      sc = sl_bt_gatt_server_read_attribute_value(attribute_handle,0,sizeof(rev_str), &rev_str_len, rev_str); //read from the beginning (offset 0)
      app_assert_status(sc);
      printf("FW Revision string (length = %zu): ",rev_str_len);
      for (i = 0; i < rev_str_len; i++)
      {
        printf("%c",rev_str[i]);
      }
      printf("\n");
    }

    /* Reset again and proceed with other commands (not flash commands)*/
    ps_state = ps_none; /* reset state machine */
    sl_bt_system_reset(sl_bt_system_boot_mode_normal);/* reset to take effect */
  }
  else if (ps_state == ps_write_mac) {
    /* write MAC value and reboot */
    printf("Writing MAC address: ");
    print_address(new_address);
    printf("\r\n");
    sc = sl_bt_system_set_identity_address(new_address, 0); //set public address
    app_assert_status(sc);
    ps_state = ps_none; /* reset state machine */
    sl_bt_system_reset(sl_bt_system_boot_mode_normal);/* reset to take effect */
    printf("Rebooting with new MAC address...\n");
  }
  else if (app_state == dtm_rx_begin)
  {
    printf("DTM receive enabled, freq=%d MHz, phy=0x%02X\n",2402+(2*channel), selected_phy);
    sc = sl_bt_test_dtm_rx(channel,selected_phy);
    app_assert_status(sc);
    if (duration_usec != 0) {
      usleep(duration_usec);	/* sleep during test */
    } else {
        // Infinite mode
        printf("Infinite mode. Press control-c to exit...\r\n");
        while(1) {
          // wait here for control-c, sleeping periodically to save host CPU cycles
          usleep(1000);
        }
    }
    sc = sl_bt_test_dtm_end();
    app_assert_status(sc);
  }
  else if (app_state == adv_test_advertising)
  {
    /* begin advertisement test */
    printf("\nStarting advertisements at period = %dms\r\n", (uint16_t) (adv_period * ADV_INTERVAL_UNIT));
    sc = sl_bt_system_set_tx_power(power_level, power_level, &power_level_set_min, &power_level_set_max);
    app_assert_status(sc);
    printf("Attempted power setting of %.1f dBm, actual setting %.1f dBm\n",(float)power_level/10,(float)power_level_set_max/10);
    printf("Press 'control-c' to end...\n");
    sc = sl_bt_advertiser_create_set(&advertising_set_handle);
    app_assert_status(sc);
    sc = sl_bt_advertiser_set_data(advertising_set_handle, 0, sizeof(adv_data),adv_data); //advertising data
    app_assert_status(sc);
    sc = sl_bt_advertiser_set_data(advertising_set_handle, 1, sizeof(scan_rsp_data), scan_rsp_data); //scan response data
    app_assert_status(sc);
    sc = sl_bt_advertiser_set_timing(advertising_set_handle, adv_period,adv_period,0,0);
    app_assert_status(sc);
    sc = sl_bt_advertiser_start(advertising_set_handle, sl_bt_advertiser_general_discoverable,
      sl_bt_advertiser_connectable_scannable);
    app_assert_status(sc);
  } else if (app_state == verify_custom_bgapi) {
    /* Send custom BGAPI command and print response */
    printf("Sending custom user message to target...\n");
    sc = sl_bt_user_message_to_target(cust_bgapi_len,cust_bgapi_data,
      sizeof(user_message_response), &user_message_response_len,
      user_message_response);
    if (sc != SL_STATUS_OK)
    {
      printf("Custom BGAPI error returned from NCP: result=0x%04X\n",sc);
      exit(EXIT_FAILURE);
    } else {
      /* Print returned payload */
      if (user_message_response_len != 0) {
        uint8_t i;
        printf("BGAPI success! Returned payload (len=%zu): ",user_message_response_len);
        for (i=0; i != user_message_response_len;i++) {
          printf("%02X ",user_message_response[i]);
        }
        printf("\n");
      } else {
        printf("BGAPI success! No return payload\n");
      }
      exit(EXIT_SUCCESS);
    }
  } else if (app_state == advscan_wait) {
    if (scan_filt_flag == true) {
      printf("Enabling advertising scan with MAC address filter ");
      print_address(scan_filt_address);
      printf(". ");
    } else {
      printf("Enabling advertising scan. ");
    }
    printf("\r\n");
    if (rssi_len != 0) {
      printf("RSSI averaging over %d packets\r\n", rssi_len);
    } else {
      printf("No RSSI averaging - info from every packet will be printed\r\n");
    }
    sc = sl_bt_scanner_start(1u,sl_bt_scanner_discover_observation); // scan with 1M PHY
    app_assert_status(sc);
    if (duration_usec == 0) {
      // infinite mode
      printf("Infinite mode. Press control-c to exit.\r\n");
    } else {
      printf("Scanning for %d milliseconds\r\n", duration_usec/1000);
      start_time_us = cur_time_us();
    }
    app_state = advscan_run;
  } else if (app_state == conn_initiate) {
      initiate_connection();
  }
  else if (ps_state == ps_none) {
    app_state = dtm_tx_begin;
    printf("Outputting modulation type 0x%02X for %d ms at %d MHz at %.1f dBm, phy=0x%02X\n",
      packet_type, duration_usec/1000, 2402+(2*channel), (float)power_level/10,
      selected_phy);
    if (duration_usec == 0) {
      printf("Infinite mode. Press control-c to exit...\r\n");
    }
    /* Run test command using test_dtm_tx_v4 */
    if ((packet_type != sl_bt_test_pkt_carrier) && (packet_type != sl_bt_test_pkt_pn9)) {
      // units of dBm for packet commands using v4 cmd
      sc = sl_bt_test_dtm_tx_v4(packet_type,packet_length,channel,selected_phy,
        (int8_t) (power_level/10));
    } else {
      // units of deci-dBm for unmodulated/PN9 modulated carrier using
      // sl_bt_test_dtm_tx_cw() API available in BLE SDK v3.3 (with output
      // bug fixed in v3.3.1)
      sc = sl_bt_test_dtm_tx_cw(packet_type,channel,selected_phy,
        power_level);
    }

    if (sc)
    {
      printf("Error running DTM TX command, result=0x%02X\n",sc);
      exit(EXIT_FAILURE);
    }
    if (duration_usec != 0) {
      usleep(duration_usec);	/* sleep during test */
    } else {
      while(1) {
        // Infinite mode
        // wait here for control-c, sleeping periodically
        usleep(1000);
      }
    }
    sc = sl_bt_test_dtm_end();
    app_assert_status(sc);
  }

}

void print_address(bd_addr address)
{
    for (int i = 5; i >= 0; i--)
    {
        printf("%02X", address.addr[i]);
        if (i > 0)
            printf(":");
    }
}

static int64_t cur_time_us(void) {
  /* Return current time in us */
  struct timespec tms;

  assert(clock_gettime(CLOCK_REALTIME,&tms) != -1);

  /* seconds, multiplied with 1 million */
  int64_t micros = tms.tv_sec * 1000000;
  /* Add full microseconds */
  micros += tms.tv_nsec/1000;
  /* round up if necessary */
  if (tms.tv_nsec % 1000 >= 500) {
      ++micros;
  }
  return micros;
}

static void initiate_connection(void) {
  sl_status_t sc;
  uint16_t supervision_timeout;
  int16_t power_level_set_min, power_level_set_max;

  printf("Initiating connection as central with connection interval=%3f ms"
          " to MAC ", (float)(conn_interval * CONN_INTERVAL_UNIT_MS));
  print_address(conn_address);
  printf("\r\n");
  supervision_timeout = (conn_interval * CONN_INTERVAL_UNIT_MS *
      SUP_TIMEOUT_FACTOR) / 10;
  if (supervision_timeout < SUP_TIMEOUT_VAL_MIN) {
    supervision_timeout = SUP_TIMEOUT_VAL_MIN;
  }
  sc = sl_bt_system_set_tx_power(power_level, power_level, &power_level_set_min, &power_level_set_max);
  app_assert_status(sc);
  printf("Attempted power setting of %.1f dBm, actual setting %.1f dBm\n",(float)power_level/10,(float)power_level_set_max/10);
  sc = sl_bt_connection_set_default_parameters(conn_interval, //min_interval
                                           conn_interval, //max_interval
                                            0u, //latency
                                            supervision_timeout, //supervision timeout
                                            0u,//min_ce_length
                                            0xffff);//max_ce_length
  app_assert_status(sc);

  // proceed with connection using sl_bt_gap_phy_1m
  sc = sl_bt_connection_open(conn_address,
                             sl_bt_gap_public_address,
                            sl_bt_gap_phy_1m,
                             &conn_handle);
  app_assert_status(sc);
  app_state = conn_pending;
}

void print_packet_counters(void) {
  /* Print packet counters */
    uint16_t tx_packets, rx_packets, crc_errors, failures;
    sl_status_t sc;
    // print counters with no reset
    sc = sl_bt_system_get_counters(false, &tx_packets, &rx_packets,
                                  &crc_errors, &failures);
    app_assert_status(sc);
    app_log_debug("Last connection packets TX:%d, RX:%d, CRC ERR:%d, failures:%d\r\n",
                                tx_packets, rx_packets, crc_errors, failures);
}

void print_coex_counters(void) {
  /* Print all the uint32_t coex counters */
  sl_status_t sc;
  size_t len;
  struct coex_counters_t {
    uint32_t low_pri_requested;
    uint32_t high_pri_requested;
    uint32_t low_pri_denied;
    uint32_t high_pri_denied;
    uint32_t low_pri_tx_abort;
    uint32_t high_pri_tx_abort;
  } coex_counters;
  sc = sl_bt_coex_get_counters(0u,
                               sizeof(coex_counters),
                               &len,
                               (uint8_t*)&coex_counters);
  if (sc == SL_STATUS_NOT_SUPPORTED) {
   app_log_debug("Cannot print coex counters as coexistence is not available in the NCP target.\n");
  } else if (sc) {
   app_assert_status(sc);
 } else {
   app_log_debug("coex counters loaded %lu bytes\r\n", len);
   printf("Coex counters: low_pri_requested=%u, high_pri_requested=%u, "
           "low_pri_denied=%u, high_pri_denied=%u, "
           "low_pri_tx_abort=%u, high_pri_tx_abort=%u\r\n",
           coex_counters.low_pri_requested, coex_counters.high_pri_requested,
           coex_counters.low_pri_denied,coex_counters.high_pri_denied,
           coex_counters.low_pri_tx_abort, coex_counters.high_pri_tx_abort);
 }
}


// Helper function to make the discovery and subscribing flow correct.
// Excerpted from throughput_central.c
static void process_procedure_complete_event(sl_bt_msg_t *evt)
{
  uint16_t procedure_result =  evt->data.evt_gatt_procedure_completed.result;
  sl_status_t sc;

  switch (throughput_state) {
    case THROUGHPUT_FIND_SERVICES:
      app_assert_status(procedure_result);
      if (!procedure_result) {
        // Discover successful, start characteristic discovery.
        sc = sl_bt_gatt_discover_characteristics(conn_handle, bletest_throughput_service_handle);
        app_assert_status(sc);
        throughput_state = THROUGHPUT_FIND_CHARACTERISTICS;
      }
      break;
    case THROUGHPUT_FIND_CHARACTERISTICS:
      app_assert_status(procedure_result);
      if (!procedure_result) {
        if (bletest_throughput_write_with_response_handle != 0xFFFF && bletest_throughput_write_no_response_handle != 0xFFFF) {
          last_report_time_us = cur_time_us();
          if (bletest_throughput_ack == true) {
            throughput_state = THROUGHPUT_ACK;
                      printf("Running throughput test with ack\r\n");
            // write with response (next write is handled by the procedure complete event)
            sc = sl_bt_gatt_write_characteristic_value(conn_handle,
                                                      bletest_throughput_write_with_response_handle,
                                                      sizeof(bletest_throughput_payload_data),
                                                      bletest_throughput_payload_data);
            app_assert_status(sc);
          } else if (bletest_throughput_ack == false) {
            // throughput noack on the advertiser side results in enabling high throughput notifications from the central
            throughput_state = THROUGHPUT_NOACK;
            printf("Running throughput test with no ack (enabling notifications on central)\r\n");
          } 
        } else {
          printf("BLEtest throughput characteristics not found - skipping throughput test\r\n");
          throughput_state = THROUGHPUT_NONE;
        }
      }
    break;

    case THROUGHPUT_ACK:
    app_assert_status(procedure_result);
      if (!procedure_result) {
        if (bletest_throughput_write_with_response_handle != 0xFFFF && bletest_throughput_write_no_response_handle != 0xFFFF) {
          bletest_throughput_total_bytes += sizeof(bletest_throughput_payload_data);
          printf(".");
          fflush(stdout);
          sc = sl_bt_gatt_write_characteristic_value(conn_handle,
                                                    bletest_throughput_write_with_response_handle,
                                                    sizeof(bletest_throughput_payload_data),
                                                    bletest_throughput_payload_data);
          app_assert_status(sc);
        }
      }
      break;

    case THROUGHPUT_NONE:
    case THROUGHPUT_NOACK:
    case THROUGHPUT_CONNECT:
    default:
      printf("DEBUG: hit default case in process_procedure_complete_event\r\n");
      break;
  }
}

// Check if found characteristic matches the UUIDs that we are searching for.
static void check_characteristic_uuid(sl_bt_msg_t *evt)
{
  if (evt->data.evt_gatt_characteristic.uuid.len == UUID_LEN) {
    if (memcmp(bletest_throughput_write_with_response_characteristic_uuid, evt->data.evt_gatt_characteristic.uuid.data, UUID_LEN) == 0) {
      bletest_throughput_write_with_response_handle = evt->data.evt_gatt_characteristic.characteristic;
      app_log_debug("bletest_throughput_write_with_response_handle=0x%x\r\n", bletest_throughput_write_with_response_handle);
    } else if (memcmp(bletest_throughput_write_no_response_characteristic_uuid, evt->data.evt_gatt_characteristic.uuid.data, UUID_LEN) == 0) {
      bletest_throughput_write_no_response_handle = evt->data.evt_gatt_characteristic.characteristic;
      app_log_debug("bletest_throughput_write_no_response_handle=0x%x\r\n", bletest_throughput_write_no_response_handle);
    } 
  }
}
/**************************************************************************//**
 * Event handler for report
 *****************************************************************************/
void timer_on_report(void)
{
  sl_status_t sc;
  unsigned char channel_map[5];
  size_t map_size;
  int64_t elapsed_time_us;
  uint32_t sent_bits;
    sc = sl_bt_connection_read_channel_map(conn_handle,
                                            sizeof(channel_map),
                                             &map_size,
                                            channel_map);
    app_assert_status(sc);
    app_log_info("\r\nChannel Map: 0x%x[4] 0x%x[3] 0x%x[2] 0x%x[1] 0x%x[0]" \
            APP_LOG_NL,channel_map[4],channel_map[3],channel_map[2],
            channel_map[1],channel_map[0]);
    if (throughput_state != THROUGHPUT_NONE) {
      // also print throughput since last report if running
      elapsed_time_us = cur_time_us() - last_report_time_us;
      sent_bits = bletest_throughput_total_bytes * 8;
      app_log_info("Throughput since last report: %0.2f bps\r\n", ((float) (sent_bits * 1e6) / (float) (elapsed_time_us)));
      // reset for next report
      last_report_time_us = cur_time_us();
      bletest_throughput_total_bytes = 0;
    }
}
