#include <Arduino.h>
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <WiFi.h>

// Pin Definitions for ESP32-C3 Supermini
#define LED_BUILTIN 8          // The onboard blue LED is on GPIO8
#define EXTERNAL_LED 2         // External LED on GPIO2
#define EXTERNAL_BUTTON 3      // External Button on GPIO3
#define BUTTON_BOOT 9          // The onboard BOOT button is on GPIO9
#define RESET_WIFI_BUTTON BUTTON_BOOT  // Use BOOT button to reset WiFi config

// WiFi Manager configuration
WiFiManager wm;
bool wifiConnected = false;
unsigned long wifiReconnectAttempt = 0;
unsigned long lastBlink = 0;
int lastButtonState = HIGH;

// Function to reset WiFi settings and restart
void resetWiFiSettings() {
  Serial.println("Resetting WiFi settings...");
  wm.resetSettings();
  delay(1000);
  ESP.restart();
}

// WiFi status LED patterns
void updateStatusLED() {
  static int ledState = LOW;
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  
  if (WiFi.status() == WL_CONNECTED) {
    // Solid ON when connected (with inverted logic for built-in LED)
    digitalWrite(LED_BUILTIN, LOW);  // ON (inverted)
  } else {
    // Fast blinking when not connected
    if (currentMillis - previousMillis >= 200) {
      previousMillis = currentMillis;
      ledState = !ledState;
      digitalWrite(LED_BUILTIN, ledState);  // Will blink (inverted logic)
    }
  }
}

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\nESP32-C3 Supermini WiFi Manager Demo");
  
  // Configure pins
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(EXTERNAL_LED, OUTPUT);
  pinMode(EXTERNAL_BUTTON, INPUT_PULLUP);
  pinMode(BUTTON_BOOT, INPUT_PULLUP);
  
  // Start with LEDs OFF
  digitalWrite(LED_BUILTIN, HIGH);   // OFF (inverted)
  digitalWrite(EXTERNAL_LED, LOW);   // OFF
  
  Serial.println("Starting WiFi Manager...");
  Serial.println("If WiFi doesn't connect in 10 seconds, AP mode will start");
  
  // WiFi Manager configuration
  wm.setConfigPortalTimeout(180);  // AP mode timeout (seconds)
  wm.setConnectTimeout(10);        // Connection timeout
  wm.setWiFiAutoReconnect(true);   // Auto reconnect
  wm.setSaveParamsCallback([](){
    Serial.println("WiFi credentials saved!");
  });
  
  // Set custom AP IP (optional)
  // wm.setAPStaticIPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  
  // Set custom menu items (optional)
  std::vector<const char*> menu = {"wifi", "info", "restart", "exit"};
  wm.setMenu(menu);
  
  // Try to connect to WiFi (will start AP mode if fails)
  bool connected = wm.autoConnect("ESP32-C3-Supermini");  // AP name when not configured
  
  if (connected) {
    Serial.println("WiFi Connected Successfully!");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(EXTERNAL_LED, HIGH);  // Turn on external LED to indicate connection
  } else {
    Serial.println("Failed to connect to WiFi. AP mode is running.");
    Serial.println("Connect to 'ESP32-C3-Supermini' WiFi network to configure");
  }
}

void loop() {
  // Handle WiFi Manager processes
  wm.process();
  
  // Update status LED based on WiFi connection
  updateStatusLED();
  
  // Check for WiFi reset (hold BOOT button for 5 seconds)
  static unsigned long buttonPressTime = 0;
  static bool buttonPressed = false;
  
  if (digitalRead(RESET_WIFI_BUTTON) == LOW) {
    if (!buttonPressed) {
      buttonPressed = true;
      buttonPressTime = millis();
      Serial.println("Button pressed - hold for 5 seconds to reset WiFi");
    } else if (millis() - buttonPressTime > 5000) {
      Serial.println("Button held for 5 seconds - Resetting WiFi...");
      resetWiFiSettings();
    }
  } else {
    if (buttonPressed && (millis() - buttonPressTime < 5000)) {
      Serial.println("Button released - WiFi settings preserved");
    }
    buttonPressed = false;
  }
  
  // Original button toggle functionality
  int currentButtonState = digitalRead(EXTERNAL_BUTTON);
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    delay(50); // Debounce
    digitalWrite(EXTERNAL_LED, !digitalRead(EXTERNAL_LED));
    Serial.println("External button pressed - Toggling external LED");
  }
  lastButtonState = currentButtonState;
  
  // Print WiFi status periodically
  static unsigned long lastStatusPrint = 0;
  if (millis() - lastStatusPrint > 30000) {  // Every 30 seconds
    lastStatusPrint = millis();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("WiFi Connected - IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("WiFi Disconnected - AP mode active");
    }
  }
  
  delay(10);  // Small delay for stability
}