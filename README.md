# 🚀 ESP32-C3 Supermini WiFi Manager

![ESP32-C3 Supermini](https://img.shields.io/badge/ESP32--C3-Supermini-blue)
![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32--C3-orange)
![WiFi Manager](https://img.shields.io/badge/WiFiManager-2.0-green)

A comprehensive PlatformIO project for the ESP32-C3 Supermini board featuring WiFi Manager with visual status indicators and easy WiFi reset functionality.

## 📋 Features

- **WiFi Manager** - Easy WiFi configuration through captive portal
- **Visual Status Indicators** - LED shows device state with different blink patterns:
  - Connected: 500ms ON / 500ms OFF
  - Disconnected: 250ms ON / 250ms OFF  
  - **AP Mode: 250ms ON / 750ms OFF** (waiting for configuration)
  - Reset countdown: 100ms rapid blink
- **One-Button WiFi Reset** - Hold BOOT button for 5 seconds to erase credentials and restart
- **GPIO Control** - Example of button-controlled LED
- **Auto-Reconnect** - Automatically reconnects to known WiFi networks

## 🔌 Pin Connections

| Component | GPIO Pin | Notes |
|-----------|----------|-------|
| **Built-in LED** | GPIO8 | Inverted logic (LOW = ON, HIGH = OFF) |
| **External LED** | GPIO2 | Normal logic, used for button demo |
| **External Button** | GPIO3 | Pull-up input, triggers external LED |
| **BOOT Button** | GPIO9 | Built-in button, used for WiFi reset |

## 💡 LED Status Indicators

The built-in LED (GPIO8) shows the device state through different blink patterns:

| State | Blink Pattern | Visual | Description |
|-------|---------------|--------|-------------|
| **Connected** | 500ms ON / 500ms OFF | `█░░░█░░░` | Connected to WiFi network |
| **Disconnected** | 250ms ON / 250ms OFF | `█░█░█░` | No WiFi, not in AP mode |
| **AP Mode** | 250ms ON / 750ms OFF | `█░░░░░░░` | **Waiting for configuration** - Connect to setup WiFi |
| **Reset Countdown** | 100ms ON / 100ms OFF | `████████` | Holding BOOT button to reset |

### Pattern Recognition Guide

- **Slow, equal blink** (█░░░█░░░) = Connected to WiFi ✓
- **Fast, equal blink** (█░█░█░) = Trying to connect, no AP
- **Short blink, long pause** (█░░░░░░░) = **AP Mode active** - Configure me!
- **Rapid continuous blink** (████████) = Reset countdown in progress

## 🎮 How to Use

### First Time Setup

1. **Upload the code** to your ESP32-C3 Supermini
2. **Observe LED pattern** - Should show AP Mode pattern (250ms ON / 750ms OFF)
3. **Connect to the AP** named `ESP32-C3-Supermini` from your phone/computer
4. **Configure WiFi** - A captive portal will open automatically
   - If not, browse to `192.168.4.1`
   - Select your WiFi network
   - Enter password
   - Click "Save"
5. **Device restarts** and connects to your WiFi
   - LED changes to connected pattern (500ms ON / 500ms OFF)

### Normal Operation

- **LED blinks slowly (500ms)** when connected to WiFi
- **External button (GPIO3)** toggles the external LED (GPIO2)
- **Serial monitor** shows IP address and connection status

### Reset WiFi Credentials

Need to connect to a different WiFi network? Hold the BOOT button:

1. **Press and hold** the BOOT button (GPIO9)
2. **Watch the LED** - it will start blinking rapidly (100ms)
3. **Keep holding** - you'll see countdown in serial monitor
4. **After 5 seconds**, device erases credentials and restarts
5. **LED shows AP Mode pattern** (250ms/750ms) - ready for new configuration

## 🔍 Quick Reference

| What You See | What It Means | What To Do |
|--------------|---------------|------------|
| `█░░░█░░░` (slow) | Connected to WiFi | Everything is fine |
| `█░█░█░` (fast equal) | No WiFi, no AP | Check WiFi router |
| `█░░░░░░░` (short blink) | **AP Mode active** | Connect to 'ESP32-C3-Supermini' and configure |
| `████████` (rapid) | Reset in progress | Keep holding button (or release before 5s to cancel) |

## 🛠️ Technical Details

### LED Patterns Configuration
```cpp
#define CONNECTED_BLINK_INTERVAL 500      // 500ms ON / 500ms OFF
#define DISCONNECTED_BLINK_INTERVAL 250   // 250ms ON / 250ms OFF
#define AP_MODE_BLINK_ON 250               // 250ms ON in AP mode
#define AP_MODE_BLINK_OFF 750              // 750ms OFF in AP mode
#define RESET_BLINK_INTERVAL 100          // 100ms during reset