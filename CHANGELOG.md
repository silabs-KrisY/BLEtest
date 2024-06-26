# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.10.1] - 2024-06-10

### Changed
- Modifies README.md to better describe the component selection changes required on the default NCP firmware project in order to use advertising and scanning features.

### Known Issues
- When building your NCP firmware, you must remove the "Legacy Advertising", "Extended Advertising", and "Periodic Advertising" components from the NCP firmware prior to building, otherwise running BLEtest will result in an assert.
- When building your NCP firmware, you must make sure the "Scanner for legacy advertisements" component is present in the NCP firmware.

## [2.10.0] - 2023-03-12
### Added
- Adds support for pushing payload/throughput data from the central to the peripheral when in a connection
- Adds new service / attributes for the purposes of the throughput testing functionality
- Adds the ability to periodically print a report including the channel map and payload throughput (if throughput is running)

### Fixed
- Stops advertising when exiting from advertising mode (and disconnects when connected)
- advscan works when using "Scanner for legacy advertisements" component in NCP firmware

### Changed
- Updated readme doc

### Known Issues
- When building your NCP firmware, you must remove the "Legacy Advertising", "Extended Advertising", and "Periodic Advertising" components from the NCP firmware prior to building, otherwise running BLEtest will result in an assert.
- When building your NCP firmware, you must make sure the "Scanner for legacy advertisements" component is present in the NCP firmware.
- BLEtest hangs when using control-C to exit the peripheral/advertiser side when it's in an active connection with throughput running. You will have to manually kill the process.

## [2.9.0] - 2022-07-14
### Added
- Adds support for enabling coexistence (--coex) and prints coexistence counters on exit. Coexistence support requires GSDK 4.1 or later (see Known Issues).

### Known Issues
- If using Gecko SDK 4.1 or greater, you must remove the "Legacy Advertising", "Extended Advertising", and "Periodic Advertising" components from the NCP firmware prior to building, otherwise running BLEtest will result in an assert.

## [2.8.0] - 2022-06-30
### Fixed
- --power option fixed to accept negative values (i.e. < 0 dBm)

### Added
- Adds support for initiating connection with specified connection interval and power level
- Adds ability to specify advertising period in --adv mode
- Outputs debug information for connection in debug logging mode (-l 4)

### Changed
- Increases default advertising period for --adv from 50ms to 100ms
- Changed minimum power setting to -300 (-30 dBm) for low RF output power testing.
- Tweaked help messages (added more detail to packet options)

### Known Issue
- When used with Bluetooth SDK 4.0 (GSDK 4.1), the "Legacy Advertising" component needs to be removed from the NCP firmware project in order to maintain compatibility with the API currently used in advertising mode. If the "Legacy Advertising" component is present in the NCP firmware, an assert will result (status = 0x000F).

## [2.7.0] - 2022-05-19
### Fixed
- Fixes infinite mode for RX

## [2.6.0] - 2022-05-17
### Added
- Adds support for advertising scan w/ MAC filtering, rssi averaging, and timeout

### Changed
- Changes default time for all modes to "0" (infinite mode). Can override with global #define on DEFAULT_DURATION

## [2.5.0] - 2022-05-11
### Added
- Adds support for "infinite mode" when --time argument is zero. Infinite mode continuously transmits or receives until stopped by control-C.

### Fixed
- Fixes issue with power level parameter for continuously modulated TX by using new API available in Bluetooth SDK v3.3 (note 20 dBm continuously modulated TX not enabled until fix in Bluetooth SDK v3.3.1)
- Fixes help text display issue with "-h"

### Changed
- Enforces NCP compatibility of Bluetooth SDK v3.3.1 or higher

## [2.4.0] - 2022-1-14
### Fixed
- Fixes issue with power level parameter for DTM packet testing

## [2.3.0] - 2022-1-11
### Fixed
- Works around issue with sl_bt_test_dtm_tx_v4() where continuously modulated/unmodulated TX is in deci-dBm while packet TX is in dBm
- Sets power limit with sl_bt_system_set_tx_power() to be able to achieve 10 dBm on DTM packet TX
- Bumped version to 2.3

### Known Issue
- Continuously modulated TX limited to 12.7 dBm. Resolved with new API in GSDK 3.2, will require new BLEtest.

## [2.2.0] - 2021-12-22
### Changed
- Ignores dynamic gatt not supported error if dynamic GATT not implemented on the NCP.
- Bumped version to 2.2 (forgot to bump for 2.1).

## [2.1.0] - 2021-11-09
### Added
- Prints DTM packet count in TX mode

### Changed
- More graceful exit on control-c during DTM test (prints packet counts)
- Minor documentation cleanup edits
- Fixes bug with DTM RX command line parameter (no longer requires argument)

## [2.0.0] - 2021-10-22
### Changed
- Major updates to move to 3.x Bluetooth SDK API and project structure
- Changes in execution parameters to use optparse
- Removed daemon functionality (can easily run the process in the background)
- Major documentation updates

## [1.7.0] - 2020-12-23
### Added
- Ability to set PHY for DTM RX and TX commands.

### Changed
- Updated README.md with new PHY parameter.
- Updated README.md to specify compatibility with Blue Gecko 2.x only.

## [1.6.0] - 2019-09-16
### Added
- Ability to set HW flow control at run time or via build time #define
- Ability to verify custom BGAPI commands
- Defaults to PTA disabled for v2.6 NCPs and later. This prevents PTA defaults in the NCP image from inadvertently affecting test TX operations.
- Added an error response to the DTM TX command to make sure incompatible commands are revealed.
- Added CHANGELOG.md.

### Changed
- Changed default modulation from "3" to "253" to reflect recent SDKs, but this can be changed via build time #define
- Changed makefile to reflect recent SDK path structure
- Fixed up exit() return codes.
- Changed the bootloader version boot print value to hex
- Made major changes to README.md.

### Removed
- Removed range checking on mod_type argument (allows packet_type arguments from recent SDKs to be used).

[1.6.0]: https://github.com/kryoung-silabs/BLEtest/releases/tag/v1.6.0
