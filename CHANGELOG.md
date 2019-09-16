# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
