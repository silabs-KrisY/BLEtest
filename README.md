# BLEtest

This an example application that demonstrates Bluetooth Low Energy (BLE) peripheral connectivity using BGLib C function definitions. It resets the device, then reads the MAC address and executes one or more test commands. The functionality can be used for RF testing and manufacturing verification purposes.

This is targeted to run on a Posix/Linux/Cygwin host. The BLE NCP can be connected to any /dev/ttyx including hardware UARTs as well as USB virtual COM ports. This includes the Silicon Labs Wireless Starter Kit (WSTK) development board (or Silicon Labs Thunderboard) connected over USB (virtual COM) with an applicable NCP firmware image programmed into the EFR32.

# Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

1. Blue Gecko SDK version v3.3.1 (Gecko SDK 4.0.1) or greater installed via Simplicity Studio v5 or from github. *NOTE: This tool is no longer compatible with Blue Gecko SDK version 2.x. For 2.x compatibility, revert to previous 1.x versions of BLEtest.*
2. Linux/Posix build environment (OSX, Raspberry Pi, etc.) or Windows (Cygwin/MinGW).
3. Blue Gecko / Mighty Gecko device running a serial UART NCP (Network Co-Processor) firmware image v3.3.1 (Gecko SDK 4.0.1) or greater. See the "To Run" section below for more details.

### Installing

This project can be built as supplied within the Blue Gecko SDK frameworks.

#### For Cygwin/OSX/Linux with Gecko SDK installed

Clone or copy the contents of this repository into the Gecko SDK, into a subfolder of app/bluetooth/example_host. Commands shown here are from OSX using Gecko SDK Suite v3.3, but will be similar in Linux/Cygwin.

```
$ cd ~/SimplicityStudio/SDKs/gecko_sdk/app/bluetooth/example_host/
$ git clone https://github.com/kryoung-silabs/BLEtest.git
$ cd BLEtest
$ make
```

#### For Raspberry Pi

1. Clone Gecko SDK (4.0.1 or higher) to the Raspberry Pi.

```
$ git clone https://github.com/SiliconLabs/gecko_sdk.git
```

2. Clone BLEtest to the Raspberry Pi:
```
$ git clone https://github.com/kryoung-silabs/BLEtest.git
```
2. Cd to the BLEtest folder and invoke make, specifying the SDK root directory via the SDK_DIR parameter.
```
$ cd BLEtest
$ make SDK_DIR=../gecko_sdk
```
#### To Run

1. Program your NCP firmware image (using either the "Bluetooth - NCP Empty" or "Bluetooth - NCP" example projects) into your target board. Instructions on how to implement this, both on custom hardware and on Silicon Labs wireless starter kit (WSTK) radio boards are provided in [AN1259: Using the v3.x Silicon Labs Bluetooth(R) Stack in Network Co-Processor Mode](https://www.silabs.com/documents/public/application-notes/an1259-bt-ncp-mode-sdk-v3x.pdf).
2. Connect your device running the Bluetooth NCP firmware to the host (via USB, uart, etc.).
3. Run the host application, pointing it to the correct serial port. Note that the default serial port created when plugging a Silicon Labs WSTK or Silicon Labs Thunderboard into a Raspberry Pi is "/dev/ttyACM0". The command line usage is:
```
BLEtest -u <serial port>
```

Arguments:
```
-h          Print help message
-u <UART port name>
-t <tcp address>
-b <baud_rate, default 115200>
-f          Enable hardware flow control
--version   Print version number defined in application.
--time <duration of test in milliseconds>, 0 for infinite mode (exit with control-c)
--packet_type <payload/modulation type, 0:PBRS9, 1:11110000 packet payload, 2:10101010 packet payload, 253:PN9 continuously modulated, 254:unmodulated carrier>
--power <power level in 0.1dBm steps>
--channel <channel, 2402 MHz + 2*channel>
--len <packet length, ignored for unmodulated carrier>
--rx        DTM receive test. Prints number of received DTM packets.
--ctune_set <Set 16-bit crystal tuning value, e.g. 0x0136>
--addr_set <Set 48-bit MAC address, e.g. 01:02:03:04:05:06>
--ctune_get Read 16-bit crystal tuning value
--fwver_get Read FW revision string from Device Information (GATT)
--adv       Enter a fast advertisement mode with scan response for TIS/TRP chamber testing
--cust <ASCII hex command string> Allows verification/running custom BGAPI commands.
--phy <PHY selection for test packets/waveforms/RX mode, 1:1Mbps, 2:2Mbps, 3:125k LR coded, 4:500k LR coded.>
```

Refer to the [Blue Gecko API documentation](https://docs.silabs.com/bluetooth/latest/) for more details about the various arguments.

When you run with only the serial port argument, you'll get some simple default behavior:
```
$ ./exe/BLEtest -u /dev/ttyACM0

------------------------
Waiting for boot pkt...

boot pkt rcvd: gecko_evt_system_boot(3, 2, 2, 267, 0x 1090002, 1)
MAC address: 00:0D:6F:20:B2:D6
Outputting modulation type 0xFE for 1000 ms at 2402 MHz at 5.0 dBm, phy=0x01
DTM completed, number of packets transmitted: 0
```

Examples:

1. Transmit an unmodulated carrier for 10 seconds on 2402 at 5.0 dBm output power level on the device connected to serial port /dev/ttyACM0:
```
$ ./exe/BLEtest --time 10000 --packet_type 254 --power 50 -u /dev/ttyACM0 --channel 0

------------------------
Waiting for boot pkt...

boot pkt rcvd: gecko_evt_system_boot(3, 2, 2, 267, 0x 1090002, 1)
MAC address: 00:0D:6F:20:B2:D6
Outputting modulation type 0xFE for 10000 ms at 2402 MHz at 5.0 dBm, phy=0x01
DTM completed, number of packets transmitted: 0
```

2. Transmit DTM (direct test mode) packets containing a pseudorandom PRBS9 payload of length=25 for 10 seconds on 2404 MHz at 8dBm output power level on device connected to serial port /dev/ttyACM0
```
$ ./exe/BLEtest --time 10000 --packet_type 0 --power 80 -u /dev/ttyACM0 --channel 1 --len 25

------------------------
Waiting for boot pkt...

boot pkt rcvd: gecko_evt_system_boot(3, 2, 4, 297, 0x 1090002, 1)
MAC address: 00:0D:6F:08:62:3F
Outputting modulation type 0x00 for 10000 ms at 2404 MHz at 8.0 dBm, phy=0x01
DTM completed, number of packets transmitted: 16014
```

3. Receive DTM (direct test mode) packets for 10 seconds on 2404 MHz on device connected to serial port /dev/ttyACM0. Note that the printout below shows an example of 100% packet reception rate when running example 2 (TX with PRBS9 DTM length=25) on a separate device.
```
$ ./exe/BLEtest --time 20000 --rx -u /dev/ttyACM0 --channel 1

------------------------
Waiting for boot pkt...

boot pkt rcvd: gecko_evt_system_boot(3, 2, 2, 267, 0x 1090002, 1)
MAC address: 00:0D:6F:20:B2:D6
DTM receive enabled, freq=2404 MHz
DTM receive completed. Number of packets received: 16214
```

4. Program custom MAC address 01:02:03:04:05:06 on device connected to virtual COM port /dev/ttyACM0:
```
$ ./exe/BLEtest -u /dev/ttyACM0 --addr_set 01:02:03:04:05:06

------------------------
Waiting for boot pkt...

boot pkt rcvd: gecko_evt_system_boot(3, 2, 4, 297, 0x 1090002, 1)
MAC address: 00:0D:6F:08:62:3F
Writing MAC address: 01:02:03:04:05:06
Rebooting with new MAC address...

boot pkt rcvd: gecko_evt_system_boot(3, 2, 4, 297, 0x 1090002, 1)
MAC address: 01:02:03:04:05:06
Outputting modulation type 0xFE for 1000 ms at 2402 MHz at 5.0 dBm, phy=0x01
DTM completed, number of packets transmitted: 0
```

5. Write crystal tuning value of 0x0155 to device connected to virtual COM port /dev/ttyACM0 and output test tone of 2402 MHz at 5.0 dBm for 10 seconds to verify frequency using a spectrum analyzer (set the instrument to 500 kHz bandwidth).
```
$ ./exe/BLEtest -u /dev/ttyACM0 --ctune_set 0x0155 --packet_type 254 --time 10000 --channel 0 --power 50

------------------------
Waiting for boot pkt...

boot pkt rcvd: gecko_evt_system_boot(3, 2, 2, 267, 0x 1090002, 1)
MAC address: 00:0D:6F:20:B2:D6
Writing ctune value to 0x0155
Rebooting with new ctune value...
boot pkt rcvd: gecko_evt_system_boot(3, 2, 2, 267, 0x 1090002, 1)
MAC address: 00:0D:6F:20:B2:D6
Outputting modulation type 0xFE for 10000 ms at 2402 MHz at 5.0 dBm, phy=0x01
DTM completed, number of packets transmitted: 0
```

6. Send custom BGAPI payload "0x01" and print response payload. In our NCP example projects, custom BGAPI handlers for commands "0x01" and "0x02" are implemented as an example with functionality implemented to echo back the command in the response payload.
```
$ ./exe/BLEtest -u /dev/ttyACM0 --cust 01

------------------------
Waiting for boot pkt...

boot pkt rcvd: gecko_evt_system_boot(3, 2, 2, 267, 0x 1090002, 1)
MAC address: 00:0D:6F:20:B2:D6
Sending custom user message to target...
BGAPI success! Returned payload (len=1): 01
```

7. Transmit a PN9 continuously modulated output (100% duty cycle) on logical channel 19 (physical channel 21, 2444 MHz) at 10dBm for 5 seconds using the 2Mbps PHY.
```
$ ./exe/BLEtest -u /dev/ttyACM0 --packet_type 253 --channel 21 --power 100 --time 5000 --phy 2

------------------------
Waiting for boot pkt...

boot pkt rcvd: gecko_evt_system_boot(3, 2, 2, 267, 0x 1090002, 1)
MAC address: 00:0D:6F:20:B2:D6
Outputting modulation type 253 for 5000 ms at 2402 MHz at 10.0 dBm, phy=0x02
Test completed!
```

8. Run in fast advertisement mode as a background process for TIS/TRP chamber testing.
```
$ ./exe/BLEtest -u /dev/ttyACM0 --adv > /dev/null 2>&1 &
[1] 25670
$
```

## Deployment

For a commercially deployed system (i.e. embedded gateway, etc.), use the supplied makefile and source files from the Blue Gecko SDK to cross-compile for the desired platform.

## Support

I am a Field Applications Engineer for Silicon Labs, not a full time software developer, so I've created this application in my "spare time" to provide an example for Silicon Labs customers to use to bring up their hardware, do some testing, and perhaps form as the basis to extend with additional functionality. If needed, I can provide limited support for this specific software via email <<kris.young@silabs.com>>. For support on building NCP firmware images, bringing up NCP firmware, and building target firmware images using examples under Simplicity Studio, please obtain support through the [official Silicon Labs support portal](http://silabs.com/support).

## Versioning

I plan to use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/host-thermometer-client/tags). All notable changes to this project will be documented in [CHANGELOG.md](CHANGELOG.md).

## Authors

* **Kris Young** - *Initial work* - [Kris Young](https://github.com/kryoung-silabs) <<kris.young@silabs.com>>

## License

This AS-IS example project is licensed under Zlib by Silicon Laboratories Inc. See the [LICENSE.md](LICENSE.md) file for details
