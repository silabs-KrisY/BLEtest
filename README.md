# BLEtest

This an example application that demonstrates Bluetooth Low Energy (BLE) peripheral connectivity using BGLib C function definitions. It resets the device, then reads the MAC address and executes a test command. The functionality can be used for RF testing and manufacturing purposes.

This is targeted to run on a Linux/Cygwin host. The BLE NCP can be connected to any /dev/ttyx including hardware UARTs as well as USB virtual COM ports. This includes the WSTK development board connected over USB (virtual COM) with an applicable NCP image programmed into the EFR32 module.

# Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

1. Blue Gecko SDK version 2.x (installed via Simplicity Studio). NOTE: This tool is not compatible with Blue Gecko SDK version 3.x.
2. Linux/Posix build environment (OSX, Cygwin, Raspberry Pi, etc.).
3. Blue Gecko / Mighty Gecko device running a serial UART NCP (Network Co-Processor) firmware image. See the "To Run" section below for more details.

### Installing

This project can be built as supplied within the Blue Gecko SDK frameworks.

#### For Cygwin/OSX/Linux with SDK installed

Clone or copy the contents of this repository into the Blue Gecko SDK, into a subfolder of app/bluetooth/examples_ncp_host. Commands shown here are from OSX using Gecko SDK Suite v2.6, but will be similar in Linux/Cygwin.

```
$ cd /Applications/SimplicityStudiov4.app/Contents/Eclipse/developer/sdks/gecko_sdk_suite/v2.6/app/bluetooth/examples_ncp_host/
$ git clone https://github.com/kryoung-silabs/BLEtest.git
$ cd BLEtest
$ make
```

#### For Raspberry Pi

1. On the host with SDK installed, create a compressed file archive containing the required SDK source files (command line example here on OS X using Gecko SDK Suite v2.6).

```
$ cd /Applications/SimplicityStudiov4.app/Contents/Eclipse/developer/sdks/gecko_sdk_suite/v2.6
$ tar -cvf ble_2_12_2.tgz "protocol/bluetooth/ble_stack/inc" "protocol/bluetooth/ble_stack/src" "app/bluetooth/examples_ncp_host/common"
```

You can also create a zip archive including the same files/paths using your favorite Windows Zip tool (7zip, etc.). Make sure you preserve paths relative to the Gecko SDK root when creating the archive!

2. Transfer archive to Raspberry Pi, extract, cd to the new directory, and make.

```
$ mkdir blue_gecko_sdk_v2p12p2
$ tar -xvf ble_2_12_2.tgz -C blue_gecko_sdk_v2p12p2
$ cd blue_gecko_sdk_v2p12p2/app/bluetooth/examples_ncp_host/
$ git clone https://github.com/kryoung-silabs/BLEtest.git
$ cd BLEtest
$ make
```

If using a zip archive with preserved paths relative to the Gecko SDK root, the extraction process is as follows:
```
$ mkdir blue_gecko_sdk_v2p12p2
$ unzip ble_2_12_2.tgz -d blue_gecko_sdk_v2p12p2
```

#### To Run

1. Program your NCP image "ncp-empty-target" into your target. Instructions on how to implement this, both on custom hardware and on Silicon Labs wireless starter kit (WSTK) radio boards are provided in [AN1092: Using the Silicon Labs Bluetooth(R) Stack in Network Co-Processor Mode](https://www.silabs.com/documents/login/application-notes/an1042-bt-ncp-mode.pdf).
2. Connect your Blue Gecko serial NCP device ("ncp-empty-target") to the host (via USB, uart, etc.).
3. Run the application, pointing to the correct serial port. The command line usage is:
```
BLEtest -u <serial port>
```

Arguments:
```
-v Print version number defined in application.
-t <duration in ms>
-m <payload/modulation type, 0:PBRS9, 1:11110000 packet payload, 2:10101010 packet payload, 3:unmodulated carrier>
-p <power level in 0.1dBm steps>
-u <UART port name>
-c <channel, 2402 MHz + 2*channel>
-l <packet length, ignored for unmodulated carrier>
-r DTM receive test. Prints number of received DTM packets.
-x <Set 16-bit crystal tuning value, e.g. 0x0136>
-a <Set 48-bit MAC address, e.g. 01:02:03:04:05:06>
-z Read 16-bit crystal tuning value
-f Read FW revision string from Device Information (GATT)
-e Enter a fast advertisement mode with scan response for TIS/TRP chamber testing
-d Run as a daemon process.
-h <0/1> Disable/Enable hardware flow control (RTS/CTS). Default HW flow control state set by compile time define.
-b <ASCII hex command string> Verify custom BGAPI command.
-y <PHY selection for test packets/waveforms/RX mode, 1:1Mbps, 2:2Mbps, 3:125k LR coded, 4:500k LR coded.>
```

Refer to the [Blue Gecko API documentation](https://docs.silabs.com/bluetooth/latest/) for more details about the various arguments.

Examples:

1. Transmit an unmodulated carrier for 10 seconds on 2402 at 5.0 dBm output power level on the device connected to serial port /dev/ttyAMA0:
```
$ ./exe-host/BLEtest -t 10000 -m 254 -p 50 -u /dev/ttyAMA0 -c 0

------------------------
Waiting for boot pkt...
boot pkt rcvd: gecko_evt_system_boot(1, 0, 2, 755, 0, 1)
MAC address: 00:0B:57:07:6C:04
Outputting modulation type 253 for 10000 ms at 2402 MHz at 5.0 dBm
Test completed!
```

2. Transmit DTM (direct test mode) packets containing a pseudorandom PRBS9 payload of length=25 for 10 seconds on 2404 MHz at 8dBm output power level on device connected to serial port /dev/ttyAMA0
```
$ ./exe-host/BLEtest -t 10000 -m 0 -p 80 -u /dev/ttyAMA0 -c 1 -l 25

------------------------
Waiting for boot pkt...
boot pkt rcvd: gecko_evt_system_boot(1, 0, 2, 755, 0, 1)
MAC address: 00:0B:57:07:6C:04
Outputting modulation type 0 for 10000 ms at 2404 MHz at 8.0 dBm
Test completed!
```

3. Receive DTM (direct test mode) packets for 10 seconds on 2404 MHz on device connected to serial port /dev/ttyAMA0. Note that the printout below shows an example of 100% packet reception rate when running example 2 (TX with PRBS9 DTM length=25) on a separate device.
```
$ ./exe-host/BLEtest -t 20000 -r -u /dev/ttyAMA0 -c 1

------------------------
Waiting for boot pkt...
boot pkt rcvd: gecko_evt_system_boot(1, 0, 2, 755, 0, 1)
MAC address: 00:0B:57:07:6C:04
DTM receive enabled, freq=2404 MHz
DTM receive completed. Number of packets received: 16214
```

4. Program custom MAC address 01:02:03:04:05:06 on device connected to virtual COM port /dev/ttyACM0:
```
$ ./exe-host/BLEtest -u /dev/ttyACM0 -a 01:02:03:04:05:06

------------------------
Waiting for boot pkt...
boot pkt rcvd: gecko_evt_system_boot(1, 0, 2, 755, 0, 1)
MAC address: 00:0B:57:00:E5:01
Writing MAC address: 01:02:03:04:05:06
Rebooting with new MAC address...
boot pkt rcvd: gecko_evt_system_boot(1, 0, 2, 755, 0, 1)
MAC address: 01:02:03:04:05:06
Outputting modulation type 3 for 1000 ms at 2402 MHz at 5.0 dBm
Test completed!
```

5. Write crystal tuning value of 0x0155 to device connected to virtual COM port /dev/ttyACM0 and output test tone of 2402 MHz at 5.0 dBm for 10 seconds to verify frequency using a spectrum analyzer (set the instrument to 500 kHz bandwidth).
```
$ ./exe-host/BLEtest -u /dev/ttyACM0 -x 0x0155 -m 3 -t 10000 -c 0 -p 50

------------------------
Waiting for boot pkt...
boot pkt rcvd: gecko_evt_system_boot(1, 0, 2, 755, 0, 1)
MAC address: 00:0B:57:00:E5:01
Writing ctune value to 0x0155
Rebooting with new ctune value...
boot pkt rcvd: gecko_evt_system_boot(1, 0, 2, 755, 0, 1)
MAC address: 00:0B:57:00:E5:01
Outputting modulation type 3 for 10000 ms at 2402 MHz at 5.0 dBm
Test completed!
```

6. Send custom BGAPI payload "0x06" and print response payload.
```
$ ./exe/BLEtest -u /dev/ttyACM0 -b 06

------------------------
Waiting for boot pkt...

boot pkt rcvd: gecko_evt_system_boot(2, 12, 1, 126, 0x 1070000, 1)
MAC address: 00:0D:6F:20:B2:D6
BGAPI success! Returned payload: BF 8F AF 7F 87 00 37 0F AF 10 10 FF 00 00 00 04 04
```

7. Transmit a PN9 continuously modulated output (100% duty cycle) on logical channel 19 (physical channel 21, 2444 MHz) at 10dBm for 5 seconds using the 2Mbps PHY.
```
$ ./exe/BLEtest -u /dev/ttyACM0 -m 253 -c 21 -p 100 -t 5000 -y 2

------------------------
Waiting for boot pkt...

boot pkt rcvd: gecko_evt_system_boot(2, 12, 1, 126, 0x 1070000, 1)
MAC address: 00:0D:6F:20:B2:D6
Outputting modulation type 253 for 5000 ms at 2402 MHz at 10.0 dBm, phy=0x02
Test completed!
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

This AS-IS example project is licensed under the standard Silicon Labs software license. See the [LICENSE.md](LICENSE.md) file for details
