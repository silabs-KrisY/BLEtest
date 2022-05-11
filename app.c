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
  "   -h                Print this help message.\n" \
  "   --version         Print the software version defined in the application.\n" \
  "   --time <duration of test in milliseconds>, 0 for infinite mode (exit with control-c)\n" \
  "   --packet_type <payload/modulation type, 0:PBRS9, 1:11110000 packet payload, 2:10101010 packet payload, 253:PN9 continuously modulated, 254:unmodulated carrier>\n" \
  "   --power <power level in 0.1dBm steps>\n" \
  "   --channel <channel, 2402 MHz + 2*channel>\n" \
  "   --len <packet length, ignored for unmodulated carrier>\n" \
  "   --rx              DTM receive test. Prints number of received DTM packets.\n" \
  "   --ctune_set <Set 16-bit crystal tuning value, e.g. 0x0136>\n" \
  "   --addr_set <Set 48-bit MAC address, e.g. 01:02:03:04:05:06>\n" \
  "   --ctune_get       Read 16-bit crystal tuning value\n" \
  "   --fwver_get       Read FW revision string from Device Information (GATT)\n" \
  "   --adv             Enter a fast advertisement mode with scan response for TIS/TRP chamber testing\n" \
  "   --cust <ASCII hex command string> Allows verification/running custom BGAPI commands.\n" \
  "   --phy <PHY selection for test packets/waveforms/RX mode, 1:1Mbps, 2:2Mbps, 3:125k LR coded, 4:500k LR coded.>\n"

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
             {0,           0,                 0,  0  }};

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

// Handle for dynamic GATT Database session.
uint16_t gattdb_session;

#define VERSION_MAJ	2u
#define VERSION_MIN	5u

#define TRUE   1u
#define FALSE  0u

#define DEFAULT_PHY test_phy_1m	//default to use 1Mbit PHY

/* Program defaults - will use these if not specified on command line */
#define DEFAULT_DURATION		1000*1000	//1 second
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
static uint16_t power_level=DEFAULT_POWER_LEVEL;

/* The duration in us */
static uint32_t duration_usec=DEFAULT_DURATION;

/* channel */
static uint8_t channel=DEFAULT_CHANNEL;

/* packet length */
static uint8_t packet_length=DEFAULT_PACKET_LENGTH;

/* phy */
static uint8_t selected_phy=DEFAULT_PHY;

/* advertising test mode */
#define TEST_ADV_INTERVAL_MS 50
#define ADV_INTERVAL_UNIT 0.625 //per API guide

#define MAX_POWER_LEVEL 200 //deci-dBm
#define MIN_POWER_LEVEL -100 //deci-dBm

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
  adv_test,
  dtm_rx_begin,
  dtm_tx_begin,
  dtm_rx_started,
  dtm_tx_started,
  default_state,
  verify_custom_bgapi
} app_state =   default_state;

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
static size_t ctune_ret_len;

/* Store NCP version information */
static uint8_t version_major;
static uint8_t version_minor;
static uint8_t version_patch;

//static void print_usage(void);

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
        /* advertise test for TIS / TRP */
        app_state = adv_test;
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
      // limited to 10 dBm)
      sc = sl_bt_system_set_tx_power(MIN_POWER_LEVEL, MAX_POWER_LEVEL, &power_level_set_min, &power_level_set_max);
      app_assert_status(sc);

      // Initialize GATT database dynamically.
      initialize_gatt_database();

      main_app_handler();
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      printf("Connection opened." APP_LOG_NL);
      break;

    case sl_bt_evt_dfu_boot_id:
      printf("DFU Boot, Version 0x%x\n",evt->data.evt_dfu_boot.version);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      printf("Disconnected from central" APP_LOG_NL);
      // Restart advertising after client has disconnected.
      if (app_state == adv_test)
      {
        sc = sl_bt_advertiser_start(
          advertising_set_handle,
          sl_bt_advertiser_general_discoverable,
          sl_bt_advertiser_connectable_scannable);
        app_assert_status(sc);
        printf("Started advertising." APP_LOG_NL);
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
    // -------------------------------
    // Default event handler.
    default:
    printf("Unhandled event received, event ID: 0x%2x\n", SL_BT_MSG_ID(evt -> header));
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

  /* Default to PTA disabled so PTA doesn't gate any of the DTM outputs. */
  sc = sl_bt_coex_set_options(coex_option_enable, FALSE);
  if (sc == SL_STATUS_NOT_SUPPORTED) {
    app_log_debug("Attempted to disable PTA, but Coex not available in the NCP target.\n");
  } else if (sc) {
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
    printf("Writing MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
    new_address.addr[5], // <-- address is little-endian
    new_address.addr[4],
    new_address.addr[3],
    new_address.addr[2],
    new_address.addr[1],
    new_address.addr[0]
    );
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
    usleep(duration_usec);  /* sleep during test */
    sc = sl_bt_test_dtm_end();
    app_assert_status(sc);
  }
  else if (app_state == adv_test)
  {
    /* begin advertisement test */
    printf("\n--> Starting fast advertisements for testing\n");
    sc = sl_bt_system_set_tx_power(power_level, power_level, &power_level_set_min, &power_level_set_max);
    printf("Attempted power setting of %.1f dBm, actual setting %.1f dBm\n",(float)power_level/10,(float)power_level_set_max/10);
    printf("Press 'control-c' to end...\n");
    sc = sl_bt_advertiser_create_set(&advertising_set_handle);
    app_assert_status(sc);
    sc = sl_bt_advertiser_set_data(advertising_set_handle, 0, sizeof(adv_data),adv_data); //advertising data
    sc = sl_bt_advertiser_set_data(advertising_set_handle, 1, sizeof(scan_rsp_data), scan_rsp_data); //scan response data
    sc = sl_bt_advertiser_set_timing(advertising_set_handle, TEST_ADV_INTERVAL_MS/ADV_INTERVAL_UNIT,TEST_ADV_INTERVAL_MS/ADV_INTERVAL_UNIT,0,0);
    sc = sl_bt_advertiser_start(advertising_set_handle, sl_bt_advertiser_general_discoverable,
      sl_bt_advertiser_connectable_scannable);

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
        // Infinte mode
        // wait here for control-c, sleeping periodically
        usleep(1000);
      }
    }
    sc = sl_bt_test_dtm_end();
    app_assert_status(sc);
  }
}
