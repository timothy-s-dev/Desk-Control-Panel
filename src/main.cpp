#include "app_state.h"
#include "display.h"
#include "mqtt_manager.h"
#include "ota_manager.h"
#include "rotary_encoder.h"
#include "sign_state.h"
#include "time_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <Preferences.h>
#include <Elog.h>
#include <logging.h>

// NOLINTNEXTLINE
Preferences preferences;

// NOLINTNEXTLINE(cert-err58-cpp)
const int MAX_MQTT_CONFIG_LENGTH = 40;

// NOLINTNEXTLINE
WiFiManagerParameter mqtt_server("mqtt_server", "MQTT Server", "homeassistant.local", MAX_MQTT_CONFIG_LENGTH);
// NOLINTNEXTLINE
WiFiManagerParameter mqtt_port("mqtt_port", "MQTT Port", "1883", 5);
// NOLINTNEXTLINE
WiFiManagerParameter mqtt_username("mqtt_username", "MQTT Username", "", MAX_MQTT_CONFIG_LENGTH);
// NOLINTNEXTLINE
WiFiManagerParameter mqtt_password("mqtt_password", "MQTT Password", "", MAX_MQTT_CONFIG_LENGTH);

enum {
BUTTON_1_PIN = 13,
BUTTON_2_PIN = 12,
BUTTON_3_PIN = 14,
BUTTON_4_PIN = 27,
BUTTON_5_PIN = 26
};

const int BUTTON_COUNT = 5;
const std::array<int, BUTTON_COUNT> BUTTON_PINS = {
  BUTTON_1_PIN,
  BUTTON_2_PIN,
  BUTTON_3_PIN,
  BUTTON_4_PIN,
  BUTTON_5_PIN
};

const int SERIAL_BAUD_RATE = 115200;

void setup_buttons();
void saveConfigCallback();
void init_wifi();

void setup() {
  setup_buttons();

  Serial.begin(SERIAL_BAUD_RATE);

  Logger.configure(100, true);
  Logger.registerSerial(MAIN_LOG, ELOG_LEVEL_DEBUG, "APP", Serial);

  Logger.debug(MAIN_LOG, "Initializing...");

  Display::getInstance().init();

  init_wifi();
  
  OTAManager::getInstance().init();
  AppState::getInstance().init();
  SignState::getInstance().init();
  TimeManager::getInstance().init();
  MQTTManager::getInstance().init();

  RotaryEncoderManager::getInstance().init();

  Logger.debug(MAIN_LOG, "Setup complete. Starting main loop...");
}

void loop() {
  static std::array<bool, BUTTON_COUNT> button_states = {false, false, false, false, false};
  static long const lastInput = -1;

  RotaryEncoderManager::getInstance().tick();

  // Handle time updates
  TimeManager::getInstance().update();

  // Handle sign state updates
  SignState::getInstance().update();

  // Handle MQTT connection
  MQTTManager::getInstance().update();

  for(int i = 0; i < BUTTON_COUNT; i++) {
    bool const currentState = digitalRead(BUTTON_PINS[i]) == LOW;
    if (currentState != button_states[i]) {
      button_states[i] = currentState;
      MQTTManager::getInstance().publishButtonState(i + 1, currentState);
    }
  }

  if (RotaryEncoderManager::getInstance().isButtonPressed()) {
    AppState::getInstance().onSelect();
  }
  RotaryDirection const rotaryDirection = RotaryEncoderManager::getInstance().getRotationDirection();
  if (rotaryDirection == RotaryDirection::COUNTERCLOCKWISE) {
    AppState::getInstance().onNext();
  } else if (rotaryDirection == RotaryDirection::CLOCKWISE) {
    AppState::getInstance().onPrevious();
  }
  AppState::getInstance().tick();

  Display::getInstance().update();
}

void setup_buttons() {
  for (int const i : BUTTON_PINS) {
    pinMode(i, INPUT_PULLUP);
  }
}

// Callback function to save config when WiFiManager saves parameters
void saveConfigCallback() {
  preferences.begin("mqtt_config", false);
  preferences.putString("server", mqtt_server.getValue());
  preferences.putInt("port", strtol(mqtt_port.getValue(), nullptr, 10)); // NOLINT - base10, safe conversion
  preferences.putString("username", mqtt_username.getValue());
  preferences.putString("password", mqtt_password.getValue());
  preferences.end();
}

void init_wifi()
{
  Display::getInstance().setLoadingMessage("Connecting to WiFi");
  WiFiManager wifiManager;

  // wifiManager.resetSettings();
  
  wifiManager.addParameter(&mqtt_server);
  wifiManager.addParameter(&mqtt_port);
  wifiManager.addParameter(&mqtt_username);
  wifiManager.addParameter(&mqtt_password);
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  bool const res = wifiManager.autoConnect("Desk Control Panel");

  const int RETRY_DELAY_MS = 1000;
  if (!res)
  {
    Logger.debug(MAIN_LOG, "Failed to connect to WiFi. Restarting...");
    Display::getInstance().setLoadingMessage("Failed to connect", "Restarting...");
    delay(RETRY_DELAY_MS);
    wifiManager.resetSettings();
    ESP.restart();
  }
  else
  {
    Logger.debug(MAIN_LOG, "Connected to WiFi.");
  }
}