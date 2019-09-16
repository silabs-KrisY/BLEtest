
 /***********************************************************************************************//**
  * \file   main.c
  * \brief  BLEtest NCP host project for RF testing and manufacturing
 */
 /*******************************************************************************
 * @section License
 * <b>Copyright 2016 Silicon Laboratories, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.@n
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.@n
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/

/**
 * This an example application that demonstrates Bluetooth Smart peripheral
 * connectivity using BGLib C function definitions. It resets the device, then
 * reads the MAC address and executes a test command. The functionality can be used for
 * RF testing and manufacturing purposes. Please refer to the accompanying
 * README.txt for details.
 *
 * Most of the functionality in BGAPI uses a request-response-event pattern
 * where the module responds to each command with a response, indicating that
 * it has processed the command. Events which occur asynchonously or with non-
 * predictable timing may be sent from the module to the host at any time. For
 * more information, please see the WSTK BGAPI GPIO Demo Application Note.
 *
 * This is targeted to run on a Linux/Cygwin host. The BLE NCP can be connected to any /dev/ttyx
 * including hardware UARTs as well as USB virtual COM ports. This includes the WSTK development board
 * connected over USB (virtual COM) with an applicable NCP image programmed into the EFR32 module.
 */

 #include <stdlib.h>
 #include <stdio.h>
 #include <stdint.h>
 #include <signal.h>
 #include <errno.h>
 #include <string.h>
 #include <unistd.h>
 #include <sys/types.h>
 #include <sys/stat.h>

#include "gecko_bglib.h"
#include "uart.h"

BGLIB_DEFINE();

#define VERSION_MAJ	1u
#define VERSION_MIN	6u

#define TRUE   1u
#define FALSE  0u

#define PHY test_phy_1m	//always use 1Mbit PHY for now

/* Default to use HW flow control? (compile time option)
NOTE: Default can be overridden with -h command line option */
#ifndef USE_RTS_CTS
  #define USE_RTS_CTS FALSE
#endif

/* Program defaults - will use these if not specified on command line */
#define DEFAULT_DURATION		1000*1000	//1 second
#ifndef DEFAULT_MODULATION
  #define DEFAULT_MODULATION		253	//unmodulated ('3' for early NCPs, changed to '253' in later SDKs)
#endif
#define DEFAULT_CHANNEL			0	//2.402 GHz
#define DEFAULT_POWER_LEVEL		50	//5 dBm
#define DEFAULT_PACKET_LENGTH	25
#define CMP_LENGTH	2

/**
 * Configurable parameters that can be modified to match the test setup.
 */

/** The default serial port to use for BGAPI communication if port not specified on command line. */
static char *default_uart_port = "/dev/ttyAMA0";	//RPi target

/** The default baud rate to use if not specified on command line. */
uint32_t default_baud_rate = 115200;

/** The serial port to use for BGAPI communication. */
static char *uart_port = NULL;

/** The baud rate to use. */
static uint32_t baud_rate = 0;

/* The modulation type */
static uint8_t mod_type=DEFAULT_MODULATION;

/* The power level */
static uint16_t power_level=DEFAULT_POWER_LEVEL;

/* The duration in us */
static uint32_t duration_usec=DEFAULT_DURATION;

/* channel */
static uint8_t channel=DEFAULT_CHANNEL;

/* packet length */
static uint8_t packet_length=DEFAULT_PACKET_LENGTH;

/* run as daemon? */
static uint8_t daemon_flag = 0;

/* advertising test mode */
#define TEST_ADV_INTERVAL_MS 50
#define ADV_INTERVAL_UNIT 0.625 //per API guide
#define CHANNEL_MAP_ALL 0x07 //all channels

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
} ps_state;

/* application state machine */
static enum app_states {
	adv_test,
	dtm_begin,
	dtm_receive_started,
	default_state,
  verify_custom_bgapi
} app_state;

#define MAX_CUST_BGAPI_STRING_LEN  16u
uint8_t cust_bgapi_data[MAX_CUST_BGAPI_STRING_LEN/2]; //half of string length due to ascii hex data in string
uint8_t cust_bgapi_len; //how many bytes in cust_bgapi_data

#define MAC_PSKEY_LENGTH	6u
#define CTUNE_PSKEY_LENGTH	2u
#define MAX_CTUNE_VALUE 511u

#define GATT_FWREV_LO 0x26
#define GATT_FWREV_HI 0x2A
#define FWREV_TYPE_LEN 2
static uint8_t fwrev_type_data[FWREV_TYPE_LEN] = {GATT_FWREV_LO,GATT_FWREV_HI};

static uint16_t ctune_value=0; //unsigned int representation of ctune
static uint8_t ctune_array[CTUNE_PSKEY_LENGTH]; //ctune value to be stored (little endian)
static uint8_t mac_address[MAC_PSKEY_LENGTH]; //mac address to be stored

/* Store NCP version information */
static uint8_t version_major;
static uint8_t version_minor;

static uint8_t hwflow_enable = USE_RTS_CTS; //hwflow defaulted from define

static void print_usage(void);

unsigned int toInt(char c) {
  /* Convert ASCII hex to binary */
  if (c >= '0' && c <= '9') return      c - '0';
  if (c >= 'A' && c <= 'F') return 10 + c - 'A';
  if (c >= 'a' && c <= 'f') return 10 + c - 'a';
  return -1;
}
void print_usage(void)
{
	printf("\n Arguments: \n");
	printf("-v Print version number defined in application.\n");
	printf("-t <duration in ms>\n");
	printf("-m <payload/modulation type, 0:PBRS9, 1:11110000 packet payload, 2:10101010 packet payload, 3:unmodulated carrier>\n");
	printf("-p <power level in 0.1dBm steps>\n");
	printf("-u <UART port name>\n");
	printf("-c <channel, 2402 MHz + 2*channel>\n");
	printf("-l <packet length, ignored for unmodulated carrier>\n");
	printf("-r DTM receive test. Prints number of received DTM packets.\n");
	printf("-x <16-bit crystal tuning value, e.g. 0x0136>\n");
	printf("-a <48-bit MAC address, e.g. 01:02:03:04:05:06>\n\n");
	printf("-z Read 16-bit crystal tuning value\n");
	printf("-f Read FW revision string from Device Information (GATT)\n");
	printf("-e Enter a fast advertisement mode with scan response for TIS/TRP chamber testing\n");
	printf("-d Run as a daemon process.\n");
  printf("-h <0/1> Disable/Enable hardware flow control (RTS/CTS). Default HW flow control state set by compile time define.\n");
  printf("-b <ASCII hex command string> Verify custom BGAPI command.\n");
	printf("Example - transmit PRBS9 payload of length=25 for 10 seconds on 2402 MHz at 5.5dBm output power level on device connected to serial port /dev/ttyAMA0 :\n\tBLEtest -t 10000 -m 0 -p 55 -u /dev/ttyAMA0 -c 0 -l 25\n\n\n");
}

/**
 * Function called when a message needs to be written to the serial port.
 * @param msg_len Length of the message.
 * @param msg_data Message data, including the header.
 * @param data_len Optional variable data length.
 * @param data Optional variable data.
 */
static void on_message_send(uint32_t msg_len, uint8* msg_data)
{
    /** Variable for storing function return values. */
    int ret;

#ifdef _DEBUG
	printf("on_message_send()\n");
#endif /* DEBUG */

    ret = uartTx(msg_len, msg_data);
    if (ret < 0)
    {
        printf("on_message_send() - failed to write to serial port %s, ret: %d, errno: %d\n", uart_port, ret, errno);
        exit(EXIT_FAILURE);
    }
}

/* Handle posix signals and clean up if possible */
void sig_handler(int signo)
{
	printf("sig handler %d received\n", signo);
  if (signo == SIGINT || signo == SIGTERM)
	{
		printf("Cleaning up and exiting\n");
		gecko_cmd_system_reset(0); /* reset NCP */
		sleep(1);
		uartClose();
		exit(EXIT_SUCCESS);
	}
}

int hw_init(int argc, char* argv[])
{
  int argCount = 1;  /* Used for parsing command line arguments */
  char *temp;
  int values[8];
  uint8_t i;

  /**
  * Handle the command-line arguments.
  */
  baud_rate = default_baud_rate;
  uart_port = default_uart_port;

	/* Let's go ahead and start parsing the command line arguments */
  if (argc > 1)
  {
	  if ((!strncasecmp(argv[1],"-h",CMP_LENGTH)) || (!strncasecmp(argv[1],"-?",CMP_LENGTH)))
	  {
		print_usage();
		exit(EXIT_FAILURE);
	  }
  }

  while (argCount < argc)
  {
	if(!strncasecmp(argv[argCount],"-t",CMP_LENGTH))
	{
		duration_usec = atoi(argv[argCount+1])*1000;
	} else if(!strncasecmp(argv[argCount],"-m",CMP_LENGTH))
	{
		mod_type = atoi(argv[argCount+1]);

	} else if(!strncasecmp(argv[argCount],"-p",CMP_LENGTH))
	{
		power_level = atoi(argv[argCount+1]);
		if (power_level>200)
		{
		  printf("Error in power level: max value 200 (20.0 dBm)\n");
		  exit(EXIT_FAILURE);
		}
	} else if(!strncasecmp(argv[argCount],"-u",CMP_LENGTH))
	{
		uart_port = (char *)argv[argCount+1];
	}
	else if(!strncasecmp(argv[argCount],"-c",CMP_LENGTH))
	{
		channel = atoi(argv[argCount+1]);
		if (channel>39)
		{
		  printf("Error in channel: max value 39\n");
		  exit(EXIT_FAILURE);
		}
	}
	else if(!strncasecmp(argv[argCount],"-l",CMP_LENGTH))
	{
		packet_length = atoi(argv[argCount+1]);
		if (packet_length>46)
		{
		  printf("Error in packet length: max value 46\n");
		  exit(EXIT_FAILURE);
		}
	}
	else if(!strncasecmp(argv[argCount],"-r",CMP_LENGTH))
	{
		/* DTM receive */
		app_state = dtm_begin;
	}
	else if(!strncasecmp(argv[argCount],"-e",CMP_LENGTH))
	{
		/* advertise test for TIS / TRP */
		app_state = adv_test;
	}
	else if(!strncasecmp(argv[argCount],"-x",CMP_LENGTH))
	{
		/* xtal ctune */
		ctune_value = (uint16_t) strtoul(argv[argCount+1],&temp,0);
		/* check range */
		if (ctune_value > MAX_CTUNE_VALUE)
		{
				printf("Ctune value entered (0x%2x) is larger than the maximum allowed value of 0x%2x. Ignoring argument.\n",
						ctune_value, MAX_CTUNE_VALUE);
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
	}
	else if(!strncasecmp(argv[argCount],"-v",CMP_LENGTH))
	{
			/* Print #defined version number */
			printf("%s version %d.%d\n",argv[0],VERSION_MAJ,VERSION_MIN);
	}
	else if(!strncasecmp(argv[argCount],"-z",CMP_LENGTH))
	{
			ps_state = ps_read_ctune; //init ps state machine
	}
	else if(!strncasecmp(argv[argCount],"-f",CMP_LENGTH))
	{
			ps_state = ps_read_gatt_fwversion; //init ps state machine (note - this will overwrite states from other arguments)
	}
	else if(!strncasecmp(argv[argCount],"-d",CMP_LENGTH))
	{
			daemon_flag = 1; //run as daemon (fork the process, etc.)
			printf("Running %s as daemon...\n",argv[0]);
	}
	else if(!strncasecmp(argv[argCount],"-a",CMP_LENGTH))
	{
		/* mac address */
		ps_state = ps_write_mac;
		if( 6 == sscanf( argv[argCount+1], "%x:%x:%x:%x:%x:%x", &values[5], &values[4], &values[3], &values[2], &values[1], &values[0]) )
		{
			/* convert to uint8_t */
			for( i = 0; i < 6; ++i )
				mac_address[i] = (uint8_t) values[i];
		}
		else
		{
			/* invalid mac */
			printf("Error in mac address - enter 6 ascii hex bytes separated by ':'\n");
			exit(EXIT_FAILURE);
		}
	}
  else if(!strncasecmp(argv[argCount],"-h",CMP_LENGTH))
	{
		/* enable/disable HW flow control (RTS/CTS) */
    hwflow_enable = atoi(argv[argCount+1]);
		if (hwflow_enable!= 0 && hwflow_enable!= 1 )
		{
		  printf("Error in hw flow argument: '0' for disabled, '1' for enabled\n");
		  exit(EXIT_FAILURE);
		}
	}
  else if(!strncasecmp(argv[argCount],"-b",CMP_LENGTH))
	{
    /* Verify custom BGAPI by sending a custom BGAPI command and printing the response */
    uint8_t string_len;
    string_len = strlen(argv[argCount+1]);
    cust_bgapi_len = string_len/2;
    if (string_len > MAX_CUST_BGAPI_STRING_LEN)
    {
      printf("String too long in -b argument: string length %lu, max = %d\n",strlen(argv[argCount+1]),MAX_CUST_BGAPI_STRING_LEN);
		  exit(EXIT_FAILURE);
    }

    for (i=0; i != cust_bgapi_len; i++) {
      /* Convert arg string "040101" to binary data */
      cust_bgapi_data[i] = 16 * toInt(argv[argCount+1][2*i]) + toInt(argv[argCount+1][2*i+1]);
    }
    app_state = verify_custom_bgapi;
	}
		argCount=argCount+1;
  }

    /**
    * Initialise the serial port.
    */
    return uartOpen((int8_t*)uart_port, baud_rate,hwflow_enable,100);
}

/**
 * The main program.
 */
int main(int argc, char* argv[])
{
	struct gecko_cmd_packet *evt;
	void *rsp;
	ps_state = ps_none;
	app_state = default_state;

	/* register the signal interrupt handler */
	if (signal(SIGINT, sig_handler) == SIG_ERR)
	{
		printf("\ncError registering SIGINT\n");
	}

	/**
    * Initialize BGLIB with our output function for sending messages.
    */

    BGLIB_INITIALIZE(on_message_send, uartRx);

	if (hw_init(argc, argv) < 0)
    {
        printf("Hardware initialization failure, check serial port and baud rate values\n");
        exit(EXIT_FAILURE);
    }

	/* handle the daemon processing */
	if (daemon_flag == 1)
	{
		pid_t process_id = 0;
		pid_t sid = 0;

		// Create child process
		process_id = fork();

		// Indication of fork() failure
		if (process_id < 0)
		{
			printf("daemon fork failed! Exiting.\n");
			// Return failure in exit status
			exit(EXIT_FAILURE);
		}
		// PARENT PROCESS. Need to kill it.
		if (process_id > 0)
		{
			printf("Child process %d created\n", process_id);
			// return success in exit status
			exit(EXIT_SUCCESS);
		}

		//unmask the file mode
		umask(0);

		//set new session
		sid = setsid();
		if(sid < 0)
		{
			// Return failure
			exit(EXIT_FAILURE);
		}

		// Change the current working directory to root.
		chdir("/");

		// Close stdin. stdout and stderr
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);

	}

	printf("\n------------------------\n");

	// trigger reset manually with an API command
	printf("Waiting for boot pkt...\n");
	rsp = gecko_cmd_system_reset(0);

	// ========================================================================================
    // This infinite loop is similar to the BGScript interpreter environment, and each "case"
	// represents one of the event handlers that you would define using BGScript code. You can
	// restructure this code to use your own function calls instead of placing all of the event
	// handler logic right inside each "case" block.
	// ========================================================================================
	while (1)
  {
		// blocking wait for event API packet
		evt = gecko_wait_event();

		// if a non-blocking implementation is needed, use gecko_peek_event() instead
		// (will return NULL instead of packet struct pointer if no event is ready)

        switch (BGLIB_MSG_ID(evt -> header)) {
		    // SYSTEM BOOT (power-on/reset)
        case gecko_evt_system_boot_id:
        /* Store version info */
        version_major = evt->data.evt_system_boot.major;
        version_minor = evt->data.evt_system_boot.minor;

        printf("\nboot pkt rcvd: gecko_evt_system_boot(%d, %d, %d, %d, 0x%8x, %d)\n",
               version_major,
               version_minor,
               evt->data.evt_system_boot.patch,
               evt->data.evt_system_boot.build,
               evt->data.evt_system_boot.bootloader,
               evt->data.evt_system_boot.hw
        );

			/* Read and print MAC address */
			rsp=gecko_cmd_system_get_bt_address();
			printf("MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
				((struct gecko_msg_system_get_bt_address_rsp_t *)rsp)->address.addr[5], // <-- address is little-endian
				((struct gecko_msg_system_get_bt_address_rsp_t *)rsp)->address.addr[4],
				((struct gecko_msg_system_get_bt_address_rsp_t *)rsp)->address.addr[3],
				((struct gecko_msg_system_get_bt_address_rsp_t *)rsp)->address.addr[2],
				((struct gecko_msg_system_get_bt_address_rsp_t *)rsp)->address.addr[1],
				((struct gecko_msg_system_get_bt_address_rsp_t *)rsp)->address.addr[0]
				);

        /* Default to PTA disabled so PTA doesn't gate any of the DTM outputs.
          Don't print any status messages. If coex support can't be enabled
          because it isn't built-in to the NCP, it's OK.
          NOTE: This requires building host with BG SDK v2.6 or later */

        /* Version 1.x NCPs don't respond to this command at all, which hangs up
        the app waiting for the response. Only use on version 2.6+ */
        if ((version_major == 2 && version_minor >= 6) || version_major > 2) {
          gecko_cmd_coex_set_options(coex_option_enable, FALSE);
        }

			/* Run test commands */
			/* deal with flash writes first, since reboot is needed to take effect */
			if(ps_state == ps_write_ctune) {
				/* Write ctune value and reboot */
				printf("Writing ctune value to 0x%04x\n", ctune_value);
        gecko_cmd_flash_ps_save(FLASH_PS_KEY_CTUNE, CTUNE_PSKEY_LENGTH, ctune_array); //write out key
				ps_state = ps_none; /* reset state machine */
				rsp = gecko_cmd_system_reset(0); /* reset to take effect */
				printf("Rebooting with new ctune value...\n");
			} else if (ps_state == ps_read_ctune)
			{
				/* read the PS key for CTUNE */
				printf("Reading flash PS value for CTUNE (%d)\n", FLASH_PS_KEY_CTUNE);
        rsp = gecko_cmd_flash_ps_load(FLASH_PS_KEY_CTUNE);
				errorcode_t err;
				err = ((struct gecko_msg_flash_ps_load_rsp_t *)rsp)->result;
				if (err == bg_err_hardware_ps_key_not_found)
				{
					printf("CTUNE value not loaded in PS flash!\n");
				} else {

					/* CTUNE is stored in ps key as little endian with LSB first. Need to print correctly
					as uint16 depending on endianness of host processor. NOTE: Not tested on big endian host */
					/* Test for a little-endian machine */
					#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
						/* ctune_value is little endian - LSB goes in first (low) byte */
						ctune_array[0] = ((struct gecko_msg_flash_ps_load_rsp_t *)rsp)->value.data[0];
						ctune_array[1] = ((struct gecko_msg_flash_ps_load_rsp_t *)rsp)->value.data[1];
					#else
						/* ctune_value is big endian - LSB goes in second (high) byte */
						ctune_array[1] = ((struct gecko_msg_flash_ps_load_rsp_t *)rsp)->value.data[0];
						ctune_array[0] = ((struct gecko_msg_flash_ps_load_rsp_t *)rsp)->value.data[1];
					#endif
					printf("Stored CTUNE = 0x%04x\nRebooting...\n", (uint16_t)(ctune_array[0] | ctune_array[1] << 8));
				}
					/* Reset again and proceed with other commands */
					ps_state = ps_none; /* reset state machine */
					rsp = gecko_cmd_system_reset(0); /* reset to take effect */

			} else if (ps_state == ps_read_gatt_fwversion)
			{
				/* Find the firmware revision string (UUID 0x2a26) in the local GATT and print it out */
				rsp = gecko_cmd_gatt_server_find_attribute(0,FWREV_TYPE_LEN,fwrev_type_data);
				errorcode_t err;
				err = ((struct gecko_msg_gatt_server_find_attribute_rsp_t *)rsp)->result;
				if (err == bg_err_att_att_not_found)
				{
						printf("Firmware revision string not found in the local GATT.\nRebooting...\n");
				} else
				{
					uint16_t attribute = ((struct gecko_msg_gatt_server_find_attribute_rsp_t *)rsp)->attribute;
					printf("Found firmware revision string at handle %d\n",attribute);

					/* This is the FW revision string from the GATT - load it and print it as an ASCII string */
					rsp = gecko_cmd_gatt_server_read_attribute_value(attribute,0); //read from the beginning (offset 0)
					uint8_t value_len = ((struct gecko_msg_gatt_server_read_attribute_value_rsp_t *)rsp)->value.len;
					uint8 i = 0;
					printf("FW Revision string (length = %d): ",value_len);
					for (i = 0; i < value_len; i++)
					{
						printf("%c",((struct gecko_msg_gatt_server_read_attribute_value_rsp_t *)rsp)->value.data[i]);
					}
					printf("\n");
				}

				/* Reset again and proceed with other commands (not flash commands)*/
				ps_state = ps_none; /* reset state machine */
				rsp = gecko_cmd_system_reset(0); /* reset to take effect */
			}
			else if (ps_state == ps_write_mac) {
				/* write MAC value and reboot */
				printf("Writing MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
				mac_address[5], // <-- address is little-endian
				mac_address[4],
				mac_address[3],
				mac_address[2],
				mac_address[1],
				mac_address[0]
				);
				gecko_cmd_flash_ps_save(FLASH_PS_KEY_LOCAL_BD_ADDR, MAC_PSKEY_LENGTH, (uint8 *)&mac_address); //write out key
				ps_state = ps_none; /* reset state machine */
				rsp = gecko_cmd_system_reset(0); /* reset to take effect */
				printf("Rebooting with new MAC address...\n");
			}
			else if (app_state == dtm_begin)
			{
        printf("DTM receive enabled, freq=%d MHz\n",2402+(2*channel));
        gecko_cmd_test_dtm_rx(channel,PHY);
        usleep(duration_usec);  /* sleep during test */
        gecko_cmd_test_dtm_end();
      }
			else if (app_state == adv_test)
			{
				/* begin advertisement test */
				printf("\n--> Starting fast advertisements for testing\n");
				rsp = gecko_cmd_system_set_tx_power(power_level);
				uint8_t ret_power_level = ((struct gecko_msg_system_set_tx_power_rsp_t *)rsp)->set_power;
				printf("Attempted power setting of %.1f dBm, actual setting %.1f dBm\n",(float)power_level/10,(float)ret_power_level/10);
				printf("Press 'control-c' to end...\n");
				//rsp = gecko_cmd_system_set_tx_power(100); //10.0 dBm output power
        rsp = gecko_cmd_le_gap_set_adv_data(0, sizeof(adv_data),adv_data); //advertising data
        rsp = gecko_cmd_le_gap_set_adv_data(1, sizeof(scan_rsp_data), scan_rsp_data); //scan response data
        rsp = gecko_cmd_le_gap_set_adv_parameters(TEST_ADV_INTERVAL_MS/ADV_INTERVAL_UNIT,TEST_ADV_INTERVAL_MS/ADV_INTERVAL_UNIT,CHANNEL_MAP_ALL); //advertise on all channels at specified interval
        rsp = gecko_cmd_le_gap_set_mode(le_gap_user_data,le_gap_undirected_connectable);

			} else if (app_state == verify_custom_bgapi) {
        /* Send custom BGAPI command and print response */

        rsp = gecko_cmd_user_message_to_target(cust_bgapi_len,cust_bgapi_data);
        if (((struct gecko_msg_user_message_to_target_rsp_t  *)rsp)->result != bg_err_success)
        {
          printf("Custom BGAPI error returned from NCP: result=0x%02x\n", ((struct gecko_msg_user_message_to_target_rsp_t  *)rsp)->result);
          exit(EXIT_FAILURE);
        } else {
          /* Print returned payload */
          if (((struct gecko_msg_user_message_to_target_rsp_t  *)rsp)->data.len != 0) {
            uint8_t i;
            printf("BGAPI success! Returned payload: ");
            for (i=0; i != ((struct gecko_msg_user_message_to_target_rsp_t  *)rsp)->data.len;i++) {
              printf("%02X ",((struct gecko_msg_user_message_to_target_rsp_t  *)rsp)->data.data[i]);
            }
            printf("\n");
          } else {
            printf("BGAPI success! No payload\n");
          }
          exit(EXIT_SUCCESS);
        }
      }
			else if (ps_state == ps_none) {
				printf("Outputting modulation type %u for %d ms at %d MHz at %.1f dBm\n", mod_type, duration_usec/1000, 2402+(2*channel),(float)power_level/10);
				/*Run test commands */
				gecko_cmd_system_set_tx_power(power_level);
				rsp = gecko_cmd_test_dtm_tx(mod_type,packet_length,channel,PHY);
        if (((struct gecko_msg_test_dtm_tx_rsp_t  *)rsp)->result)
        {
          printf("Error in DTM TX command, result=0x%02X\n",((struct gecko_msg_test_dtm_tx_rsp_t  *)rsp)->result);
          exit(EXIT_FAILURE);
        }
				usleep(duration_usec);	/* sleep during test */
				gecko_cmd_test_dtm_end();
			}
		break;

		case gecko_evt_test_dtm_completed_id:

				if (app_state == dtm_begin)
				{
						//This is just an acknowledgement of the DTM start - set a flag for the next event which is the end
						app_state = dtm_receive_started;
				} else if (app_state == dtm_receive_started)
				{
					//This is the event received at the end of the test
					printf("DTM receive completed. Number of packets received: %d\n",evt->data.evt_test_dtm_completed.number_of_packets);
					exit(EXIT_SUCCESS);	//test done - terminate
				}
			  else {
				/* Not sure how we got here - but exit anyways */
				printf("Test completed!\n");
				exit(EXIT_SUCCESS); //test done - terminate
			}
		  /* evt->data.evt_test_dtm_completed.number_of_packets */

			break;
			case gecko_evt_dfu_boot_id:
					/* DFU boot detected  */
					printf("DFU Boot, Version 0x%x\n",evt->data.evt_dfu_boot.version);
					break;
			case gecko_evt_le_connection_closed_id:
					/* Connection has closed - resume advertising if in if app_state == adv_test */
					printf("Disconnected from central\n");
					if (app_state == adv_test)
					{
						rsp = gecko_cmd_le_gap_set_mode(le_gap_user_data,le_gap_undirected_connectable );
					}
					break;
			case gecko_evt_dfu_boot_failure_id:
					/* DFU boot failure detected */
					printf("DFU boot failure detected, reason 0x%2x: ",evt->data.evt_dfu_boot_failure.reason);
					/* Print a more useful reason */
					switch (evt->data.evt_dfu_boot_failure.reason)
					{
						case bg_err_security_image_signature_verification_failed:
							printf("device firmware signature verification failed.\n");
							break;
						case bg_err_security_file_signature_verification_failed:
							printf("bootloading file signature verification failed.\n");
							break;
						case bg_err_security_image_checksum_error:
							printf("device firmware checksum is not valid.\n");
							break;
						default:
							printf("\n"); //add a lf when no detailed reason provided.
							break;
					}
					printf("Run the uart_dfu application to update the firmware.\n");
					exit(EXIT_FAILURE); //terminate
			default:
		       printf("Unhandled event received, event ID: 0x%2x\n", BGLIB_MSG_ID(evt -> header));
		      break;
		}
  }

    return 0;
}
