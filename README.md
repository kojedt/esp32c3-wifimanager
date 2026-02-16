# ESP32-C3 Supermini WiFi Manager Project

PlatformIO project for ESP32-C3 Supermini with WiFi Manager and GPIO control.

## Features
- WiFi Manager for easy WiFi configuration
- GPIO control (LED on GPIO2, button on GPIO3)
- Onboard LED (GPIO8) status indication
- BOOT button (GPIO9) for WiFi reset

## Pin Connections
| Component | GPIO Pin |
|-----------|----------|
| Built-in LED | 8 (inverted) |
| External LED | 2 |
| External Button | 3 |
| BOOT Button | 9 |

## Requirements
- PlatformIO
- ESP32-C3 Supermini board

## Libraries Used
- WiFiManager by tzapu
- ArduinoJson by bblanchon

## Usage
1. Upload code to ESP32-C3
2. On first boot, connect to "ESP32-C3-Supermini" WiFi AP
3. Configure your WiFi credentials at 192.168.4.1
4. Hold BOOT button for 5 seconds to reset WiFi settings