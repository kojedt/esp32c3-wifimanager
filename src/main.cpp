#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFi.h>

// Pin Definitions
#define LED_BUILTIN 8          // Onboard blue LED (inverted logic)
#define EXTERNAL_LED 2         // External LED on GPIO2
#define EXTERNAL_BUTTON 3      // External Button on GPIO3
#define BUTTON_BOOT 9          // Onboard BOOT button (used for WiFi reset)
#define RESET_WIFI_BUTTON BUTTON_BOOT  // Use BOOT button to reset WiFi

// LED Blink Patterns (in milliseconds)
#define CONNECTED_BLINK_INTERVAL 500      // 500ms ON / 500ms OFF when connected
#define DISCONNECTED_BLINK_INTERVAL 250   // 250ms ON / 250ms OFF when disconnected (no AP)
#define AP_MODE_BLINK_ON 250               // 250ms ON in AP mode
#define AP_MODE_BLINK_OFF 750              // 750ms OFF in AP mode
#define RESET_BLINK_INTERVAL 100          // 100ms rapid blink during reset countdown

// WiFi Manager
WiFiManager wm;
unsigned long previousBlink = 0;
bool ledState = LOW;
int lastButtonState = HIGH;
int lastExternalButtonState = HIGH;

// Reset button variables
unsigned long buttonPressTime = 0;
bool buttonPressed = false;
bool resetExecuted = false;
int resetCountdown = 0;

// Track WiFi and AP mode status
bool isAPMode = false;
unsigned long lastAPCheck = 0;

// Function to reset WiFi settings and restart
void resetWiFiSettings() {
  Serial.println("\n⚠️  ===== RESETTING WIFI SETTINGS! ===== ⚠️");
  Serial.println("Erasing saved WiFi credentials...");
  
  // Visual indication of reset
  for(int i = 0; i < 5; i++) {
    digitalWrite(LED_BUILTIN, LOW);  // ON
    digitalWrite(EXTERNAL_LED, HIGH);
    delay(150);
    digitalWrite(LED_BUILTIN, HIGH); // OFF
    digitalWrite(EXTERNAL_LED, LOW);
    delay(150);
  }
  
  // Reset WiFi settings
  wm.resetSettings();
  
  Serial.println("✅ WiFi credentials erased!");
  Serial.println("🔄 Restarting in AP mode for new configuration...");
  delay(2000);
  
  ESP.restart();  // Restart ESP32
}

// Check if we're in AP mode
void checkAPMode() {
  static bool lastAPState = false;
  
  // AP mode is active when WiFi is in AP mode or AP_STA mode
  bool currentAPState = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA);
  
  if (currentAPState != lastAPState) {
    if (currentAPState) {
      Serial.println("📱 AP Mode activated - Connect to 'ESP32-C3-Supermini' to configure");
    } else {
      Serial.println("📡 AP Mode deactivated");
    }
    lastAPState = currentAPState;
  }
  
  isAPMode = currentAPState;
}

// WiFi status LED control with AP mode pattern
void updateStatusLED() {
  unsigned long currentMillis = millis();
  unsigned long blinkOnTime, blinkOffTime;
  
  // Determine blink pattern based on state
  if (buttonPressed && (millis() - buttonPressTime < 5000)) {
    // Rapid blinking during reset countdown
    blinkOnTime = RESET_BLINK_INTERVAL;
    blinkOffTime = RESET_BLINK_INTERVAL;
  } 
  else if (isAPMode) {
    // AP Mode: 250ms ON, 750ms OFF
    blinkOnTime = AP_MODE_BLINK_ON;
    blinkOffTime = AP_MODE_BLINK_OFF;
  }
  else if (WiFi.status() == WL_CONNECTED) {
    // Connected: 500ms ON, 500ms OFF
    blinkOnTime = CONNECTED_BLINK_INTERVAL;
    blinkOffTime = CONNECTED_BLINK_INTERVAL;
  } 
  else {
    // Disconnected (no AP): 250ms ON, 250ms OFF
    blinkOnTime = DISCONNECTED_BLINK_INTERVAL;
    blinkOffTime = DISCONNECTED_BLINK_INTERVAL;
  }
  
  // Handle the blinking with different ON/OFF times
  static enum { LED_OFF, LED_ON } blinkState = LED_OFF;
  static unsigned long stateStartTime = 0;
  
  if (blinkState == LED_OFF) {
    // Currently OFF
    digitalWrite(LED_BUILTIN, HIGH);  // OFF (inverted)
    if (currentMillis - stateStartTime >= blinkOffTime) {
      blinkState = LED_ON;
      stateStartTime = currentMillis;
    }
  } else {
    // Currently ON
    digitalWrite(LED_BUILTIN, LOW);   // ON (inverted)
    if (currentMillis - stateStartTime >= blinkOnTime) {
      blinkState = LED_OFF;
      stateStartTime = currentMillis;
    }
  }
}

// Check for WiFi reset button (BOOT button)
void checkResetButton() {
  // Read button state (LOW when pressed due to pull-up)
  int buttonState = digitalRead(RESET_WIFI_BUTTON);
  
  if (buttonState == LOW) {  // Button is pressed
    if (!buttonPressed) {
      // Button just pressed - start timing
      buttonPressed = true;
      buttonPressTime = millis();
      resetExecuted = false;
      resetCountdown = 5;
      Serial.println("\n🔴 BOOT button pressed!");
      Serial.println("Hold for 5 seconds to reset WiFi...");
    } 
    else {
      // Button is being held down
      unsigned long holdTime = millis() - buttonPressTime;
      
      // Calculate seconds remaining
      int secondsRemaining = 5 - (holdTime / 1000);
      
      // Update countdown display every second
      if (secondsRemaining != resetCountdown && secondsRemaining > 0) {
        resetCountdown = secondsRemaining;
        Serial.print("⏱️  Hold for ");
        Serial.print(resetCountdown);
        Serial.println(" more seconds...");
      }
      
      // Check if held for 5 seconds AND reset hasn't been executed yet
      if (holdTime >= 5000 && !resetExecuted) {
        resetExecuted = true;
        Serial.println("\n✅ 5 seconds reached! Resetting WiFi NOW!");
        resetWiFiSettings();
      }
    }
  } 
  else {  // Button is released
    if (buttonPressed) {
      unsigned long holdTime = millis() - buttonPressTime;
      
      if (holdTime < 5000) {
        Serial.print("👆 Button released after ");
        Serial.print(holdTime / 1000.0, 1);
        Serial.println(" seconds - No reset (need 5 seconds)");
      }
      
      // Reset button variables
      buttonPressed = false;
      resetCountdown = 0;
    }
  }
}

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n===================================");
  Serial.println("ESP32-C3 Supermini WiFi Manager");
  Serial.println("===================================");
  Serial.println("\n📋 LED Status Indicators:");
  Serial.println("  - Connected:    500ms ON / 500ms OFF");
  Serial.println("  - Disconnected: 250ms ON / 250ms OFF");
  Serial.println("  - AP Mode:      250ms ON / 750ms OFF  👈 NEW PATTERN");
  Serial.println("  - Reset mode:   100ms rapid blink during countdown");
  Serial.println("\n🔧 WiFi Reset:");
  Serial.println("  - Hold BOOT button (GPIO9) for 5 seconds");
  Serial.println("  - LED will blink rapidly during countdown");
  Serial.println("  - Device will restart in AP mode");
  Serial.println("===================================\n");
  
  // Configure pins
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(EXTERNAL_LED, OUTPUT);
  pinMode(EXTERNAL_BUTTON, INPUT_PULLUP);
  pinMode(BUTTON_BOOT, INPUT_PULLUP);
  
  // Start with LEDs OFF
  digitalWrite(LED_BUILTIN, HIGH);   // OFF
  digitalWrite(EXTERNAL_LED, LOW);   // OFF
  
  // WiFi Manager configuration
  wm.setConfigPortalTimeout(180);     // AP mode timeout
  wm.setConnectTimeout(10);            // Connection timeout
  wm.setWiFiAutoReconnect(true);       // Auto reconnect
  wm.setCleanConnect(true);            // Clean connect
  
  // Set callback for when WiFi connects
  wm.setSaveConfigCallback([](){
    Serial.println("\n✅ WiFi credentials saved successfully!");
  });
  
  // Try to connect to WiFi
  Serial.println("📡 Connecting to WiFi...");
  Serial.println("(If no saved credentials, AP mode will start)\n");
  
  bool connected = wm.autoConnect("ESP32-C3-Supermini");
  
  if (connected) {
    Serial.println("\n✅ WiFi Connected Successfully!");
    Serial.print("   SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("   IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("   RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm\n");
  } else {
    Serial.println("\n⚠️  Failed to connect to WiFi.");
    Serial.println("   AP mode is running at 192.168.4.1");
    Serial.println("   Connect to 'ESP32-C3-Supermini' to configure\n");
  }
}

void loop() {
  // Handle WiFi Manager processes
  wm.process();
  
  // Check if we're in AP mode
  if (millis() - lastAPCheck > 1000) {
    checkAPMode();
    lastAPCheck = millis();
  }
  
  // Update status LED
  updateStatusLED();
  
  // Check for WiFi reset button
  checkResetButton();
  
  // External button toggle functionality
  int currentExternalButton = digitalRead(EXTERNAL_BUTTON);
  if (currentExternalButton == LOW && lastExternalButtonState == HIGH) {
    delay(50); // Debounce
    digitalWrite(EXTERNAL_LED, !digitalRead(EXTERNAL_LED));
    Serial.println("🔘 External button pressed - Toggling external LED");
  }
  lastExternalButtonState = currentExternalButton;
  
  // Print status periodically
  static unsigned long lastStatusPrint = 0;
  if (millis() - lastStatusPrint > 30000) {  // Every 30 seconds
    lastStatusPrint = millis();
    
    if (isAPMode) {
      Serial.println("📱 AP Mode active - Connect to 'ESP32-C3-Supermini' to configure");
    } else if (WiFi.status() == WL_CONNECTED) {
      Serial.print("📊 Connected - IP: ");
      Serial.print(WiFi.localIP());
      Serial.print(" | RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
    }
  }
  
  delay(10);
}