#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFi.h>

// Pin Definitions
#define BOARD_LED 8          // Onboard blue LED (inverted logic)
#define EXTERNAL_LED 2         // External LED on GPIO2
#define EXTERNAL_BUTTON 3      // External Button on GPIO3
#define BUTTON_BOOT 9          // Onboard BOOT button (used for WiFi reset)
#define RESET_WIFI_BUTTON BUTTON_BOOT  // Use BOOT button to reset WiFi

// LED Blink Patterns (in milliseconds)
#define CONNECTED_BLINK_INTERVAL 1000      // 1000ms ON / 1000ms OFF when connected
#define DISCONNECTED_BLINK_INTERVAL 500   // 500ms ON / 500ms OFF when disconnected (no AP)
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
bool setupComplete = false;
bool wifiManagerStarted = false;

// LED blink state variables
unsigned long lastLedUpdate = 0;
bool ledPhysicalState = false;
unsigned long currentOnTime = 250;
unsigned long currentOffTime = 250;

// Forward declarations
void updateLED();
void checkResetButton();

// Callback functions
void apModeCallback(WiFiManager* myWiFiManager) {
  Serial.println("📱 AP Mode activated - Connect to 'ESP32-C3-Supermini'");
  Serial.println("📱 Configuration URL: http://192.168.4.1");
  isAPMode = true;
}

void saveConfigCallback() {
  Serial.println("✅ WiFi credentials saved! Restarting...");
  delay(1000);
  ESP.restart();
}

// Non-blocking LED update function - MUST be called very frequently
void updateLED() {
  unsigned long currentMillis = millis();
  
  // Update pattern based on current state (every time, not just when changing)
  if (buttonPressed && (millis() - buttonPressTime < 5000)) {
    // Reset countdown pattern
    currentOnTime = RESET_BLINK_INTERVAL;
    currentOffTime = RESET_BLINK_INTERVAL;
  }
  else if (isAPMode) {
    // AP Mode pattern: 250ms ON, 750ms OFF
    currentOnTime = AP_MODE_BLINK_ON;
    currentOffTime = AP_MODE_BLINK_OFF;
  }
  else if (WiFi.status() == WL_CONNECTED) {
    // Connected pattern: 1000ms ON, 1000ms OFF
    currentOnTime = CONNECTED_BLINK_INTERVAL;
    currentOffTime = CONNECTED_BLINK_INTERVAL;
  }
  else {
    // Disconnected pattern: 500ms ON, 500ms OFF
    currentOnTime = DISCONNECTED_BLINK_INTERVAL;
    currentOffTime = DISCONNECTED_BLINK_INTERVAL;
  }
  
  // Handle the blinking - this runs EVERY time updateLED is called
  if (ledPhysicalState) {
    // LED is currently ON
    digitalWrite(BOARD_LED, LOW);  // ON (inverted)
    if (currentMillis - lastLedUpdate >= currentOnTime) {
      ledPhysicalState = false;
      lastLedUpdate = currentMillis;
    }
  } else {
    // LED is currently OFF
    digitalWrite(BOARD_LED, HIGH);  // OFF (inverted)
    if (currentMillis - lastLedUpdate >= currentOffTime) {
      ledPhysicalState = true;
      lastLedUpdate = currentMillis;
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
  Serial.println("  - Connected:    1000ms ON / 1000ms OFF");
  Serial.println("  - Disconnected: 500ms ON / 500ms OFF");
  Serial.println("  - AP Mode:      250ms ON / 750ms OFF");
  Serial.println("  - Reset mode:   100ms rapid blink");
  Serial.println("\n🔧 WiFi Reset: Hold BOOT button for 5 seconds");
  Serial.println("===================================\n");
  
  // Configure pins
  pinMode(BOARD_LED, OUTPUT);
  pinMode(EXTERNAL_LED, OUTPUT);
  pinMode(EXTERNAL_BUTTON, INPUT_PULLUP);
  pinMode(BUTTON_BOOT, INPUT_PULLUP);
  
  // Start with LEDs OFF
  digitalWrite(BOARD_LED, HIGH);   // OFF
  digitalWrite(EXTERNAL_LED, LOW);  // OFF
  
  // Initialize LED timing
  lastLedUpdate = millis();
  ledPhysicalState = false;
  
  // Set WiFiManager callbacks
  wm.setAPCallback(apModeCallback);
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setConnectTimeout(10);
  wm.setConfigPortalTimeout(180);
  wm.setWiFiAutoReconnect(true);
  
  // Disable blocking behavior
  wm.setConfigPortalBlocking(false);  // THIS IS KEY - non-blocking mode
  
  Serial.println("📡 Starting WiFi connection in NON-BLOCKING mode...");
  
  // Start WiFi manager in non-blocking mode
  wm.autoConnect("ESP32-C3-Supermini");
  
  // Immediately check if we're in AP mode
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    isAPMode = true;
    Serial.println("📱 ESP32 is in AP Mode (non-blocking)");
  }
  
  setupComplete = true;
}

void loop() {
  // CRITICAL: Update LED every single loop iteration
  updateLED();
  
  // Handle WiFi Manager in non-blocking mode
  wm.process();  // This must be called regularly
  
  // Check if WiFi status has changed
  static wl_status_t lastStatus = WL_IDLE_STATUS;
  wl_status_t currentStatus = WiFi.status();
  
  if (currentStatus != lastStatus) {
    if (currentStatus == WL_CONNECTED) {
      Serial.println("\n✅ WiFi Connected Successfully!");
      Serial.print("   SSID: ");
      Serial.println(WiFi.SSID());
      Serial.print("   IP Address: ");
      Serial.println(WiFi.localIP());
      isAPMode = false;
    }
    lastStatus = currentStatus;
  }
  
  // Check AP mode status
  static unsigned long lastAPCheck = 0;
  if (millis() - lastAPCheck > 500) {
    bool apActive = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA);
    if (apActive != isAPMode) {
      isAPMode = apActive;
      if (isAPMode) {
        Serial.println("📱 AP Mode active - Connect to 'ESP32-C3-Supermini' (192.168.4.1)");
      }
    }
    lastAPCheck = millis();
  }
  
  // Check for WiFi reset button
  checkResetButton();
  
  // External button toggle
  int currentExternalButton = digitalRead(EXTERNAL_BUTTON);
  if (currentExternalButton == LOW && lastExternalButtonState == HIGH) {
    delay(50);
    digitalWrite(EXTERNAL_LED, !digitalRead(EXTERNAL_LED));
    Serial.println("🔘 External button pressed");
  }
  lastExternalButtonState = currentExternalButton;
  
  // Status print every 30 seconds
  static unsigned long lastStatusPrint = 0;
  if (millis() - lastStatusPrint > 30000) {
    lastStatusPrint = millis();
    if (isAPMode) {
      Serial.println("📱 AP Mode active - Connect to 'ESP32-C3-Supermini' (192.168.4.1)");
    } else if (WiFi.status() == WL_CONNECTED) {
      Serial.print("📊 Connected - IP: ");
      Serial.print(WiFi.localIP());
      Serial.print(" | RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
    }
  }
  
  // Small delay - but not too long or LED will flicker
  delay(5);
}

void checkResetButton() {
  int buttonState = digitalRead(RESET_WIFI_BUTTON);
  
  if (buttonState == LOW) {
    if (!buttonPressed) {
      buttonPressed = true;
      buttonPressTime = millis();
      resetExecuted = false;
      resetCountdown = 5;
      Serial.println("\n🔴 BOOT button pressed!");
      Serial.println("Hold for 5 seconds to reset WiFi...");
    } 
    else {
      unsigned long holdTime = millis() - buttonPressTime;
      int secondsRemaining = 5 - (holdTime / 1000);
      
      if (secondsRemaining != resetCountdown && secondsRemaining > 0 && secondsRemaining <= 5) {
        resetCountdown = secondsRemaining;
        Serial.print("⏱️  Hold for ");
        Serial.print(resetCountdown);
        Serial.println(" more seconds...");
      }
      
      if (holdTime >= 5000 && !resetExecuted) {
        resetExecuted = true;
        Serial.println("\n✅ 5 seconds reached! Resetting WiFi NOW!");
        
        // Visual indication
        for(int i = 0; i < 3; i++) {
          digitalWrite(BOARD_LED, LOW);
          digitalWrite(EXTERNAL_LED, HIGH);
          delay(100);
          digitalWrite(BOARD_LED, HIGH);
          digitalWrite(EXTERNAL_LED, LOW);
          delay(100);
        }
        
        wm.resetSettings();
        delay(1000);
        ESP.restart();
      }
    }
  } 
  else {
    if (buttonPressed) {
      unsigned long holdTime = millis() - buttonPressTime;
      if (holdTime < 5000) {
        Serial.print("👆 Button released after ");
        Serial.print(holdTime / 1000.0, 1);
        Serial.println(" seconds - No reset");
      }
      buttonPressed = false;
      resetCountdown = 0;
    }
  }
}