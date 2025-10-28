# ESP32 OTA Update System

This repository hosts firmware updates for ESP32 devices with OTA (Over-The-Air) update capability.

## Repository Structure

- `firmware.bin` - Compiled firmware binary for OTA updates
- `version.json` - Version information and metadata
- `src/` - Source code for the firmware
- `README.md` - This file

## How to Update Firmware

1. Compile new firmware in Arduino IDE
2. Upload the new `firmware.bin` to this repository
3. Update `version.json` with new version number
4. ESP32 devices will automatically detect and can install the update

## Version History

- v1.0.0 - Initial release with OTA functionality
