#include "mqtt_manager.h"
#include "config.h"
#include "sign_state.h"
#include "app_state.h"
#include <Arduino.h>
#include <ctime>
#include <Preferences.h>
#include <Elog.h>
#include <logging.h>
#include <display.h>

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables) - static const class member
const char* MQTTManager::mqtt_client_id = "desk-control-panel";
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables) - static const class member
const char* MQTTManager::mqtt_topic_prefix = "desk-control/";
const unsigned long MQTTManager::MQTT_RECONNECT_INTERVAL = 5000; // 5 seconds
const int DEFAULT_MQTT_PORT = 1883;

auto MQTTManager::getInstance() -> MQTTManager& {
  static MQTTManager instance;
  return instance;
}

auto MQTTManager::init() -> void {
  Logger.debug(MAIN_LOG, "Initializing MQTT manager...");

  Display::getInstance().setLoadingMessage("Connecting to", "Home Assistant");

  Preferences preferences;
  preferences.begin("mqtt_config", true);
  this->mqtt_server = preferences.getString("server", "homeassistant.local");
  this->mqtt_port = preferences.getInt("port", DEFAULT_MQTT_PORT);
  this->mqtt_username = preferences.getString("username", "");
  this->mqtt_password = preferences.getString("password", "");
  preferences.end();

  setupMQTT();
  Logger.debug(MAIN_LOG, "MQTT manager initialized.");
}

auto MQTTManager::update() -> void {
  unsigned long const currentMillis = millis();
  
  if (!mqtt_client.connected()) {
    if (currentMillis - lastMqttReconnectAttempt >= MQTT_RECONNECT_INTERVAL) {
      lastMqttReconnectAttempt = currentMillis;
      mqttReconnect();
    }
  } else {
    mqtt_client.loop();
  }
}

auto MQTTManager::isConnected() -> bool {
  return mqtt_client.connected();
}

auto MQTTManager::publishMessage(const char* topic, const char* message) -> void {
  if (mqtt_client.connected()) {
    String const full_topic = String(mqtt_topic_prefix) + topic;
    mqtt_client.publish(full_topic.c_str(), message);
    Logger.debug(MAIN_LOG, "MQTT: %s -> %s", full_topic.c_str(), message);
  } else {
    Logger.error(MAIN_LOG, "MQTT not connected, failed to send: %s -> %s", topic, message);
  }
}

auto MQTTManager::publishButtonState(int button_num, bool pressed) -> void {
  // Only publish on button press, not release
  if (pressed) {
    // Get current time and format as ISO timestamp
    struct tm timeInfo{};
    if (getLocalTime(&timeInfo)) {
      const int TIMESTAMP_LENGTH = 32;
      std::array<char, TIMESTAMP_LENGTH> timestamp{};
      strftime(timestamp.data(), timestamp.size(), "%Y-%m-%dT%H:%M:%S%z", &timeInfo);
      
      String const topic = "button/" + String(button_num) + "/pressed";
      publishMessage((const char*)topic.c_str(), (const char*)timestamp.data());
    } else {
      // Fallback to millis if time not available
      String const topic = "button/" + String(button_num) + "/pressed";
      String const message = String(millis());
      publishMessage((const char*)topic.c_str(), (const char*)message.c_str());
    }
  }
}

auto MQTTManager::publishAction(const String& action) -> void {
  publishMessage("action", action.c_str());
}


auto MQTTManager::setupMQTT() -> void {
  Logger.debug(MAIN_LOG, "Setting up MQTT connection...");

  mqtt_client.setServer(mqtt_server.c_str(), mqtt_port);
  mqtt_client.setCallback(onMqttMessage);

  // Increase MQTT buffer size to handle larger discovery messages
  mqtt_client.setBufferSize(2048);
  
  // Initial connection attempt
  mqttReconnect();
}

auto MQTTManager::mqttReconnect() -> void {
  if (mqtt_client.connected()) {
    return;
  }

  Logger.debug(MAIN_LOG, "Attempting MQTT connection...");

  Logger.debug(MAIN_LOG, "Server: %s, Port: %d, Username: %s", mqtt_server.c_str(), mqtt_port, mqtt_username.c_str());

  // Set up Last Will and Testament
  String const will_topic = String(mqtt_topic_prefix) + "status";

  if (mqtt_client.connect(mqtt_client_id, mqtt_username.c_str(), mqtt_password.c_str(), will_topic.c_str(), 0, true, "offline")) {
    Logger.debug(MAIN_LOG, "MQTT connected");
    
    // Publish online status
    mqtt_client.publish(will_topic.c_str(), "online", true);
    
    // Publish device info
    String topic = String(mqtt_topic_prefix) + "device_info";
    mqtt_client.publish(topic.c_str(), "desk-control-panel", true);
    
    // Publish firmware version or build info
    topic = String(mqtt_topic_prefix) + "version";
    mqtt_client.publish(topic.c_str(), VERSION, true);
    
    // Publish Home Assistant discovery message
    publishDiscoveryMessage();
    
    // Subscribe to sign image topic
    subscribeToSignImage();
    
    // Subscribe to status topics
    subscribeToStatusTopics();
    
    // Subscribe to PC monitoring topics
    subscribeToPcMonitoring();
    
  } else {
    Logger.error(MAIN_LOG, "MQTT connection failed, rc=%d. Retrying in %lu ms", mqtt_client.state(), MQTT_RECONNECT_INTERVAL);
  }
}

auto MQTTManager::publishDiscoveryMessage() -> void {
  Logger.debug(MAIN_LOG, "Publishing Home Assistant discovery message...");

  // Get device IP for the origin URL
  String const device_ip = WiFi.localIP().toString();
  
  // Create the discovery JSON document with larger buffer
  JsonDocument doc;

  // Device information
  JsonObject dev = doc["dev"].to<JsonObject>();
  JsonArray dev_ids = dev["ids"].to<JsonArray>();
  dev_ids.add(mqtt_client_id);
  dev["name"] = "Desk Control Panel";
  dev["mf"] = "Custom";
  dev["mdl"] = "Desk Control Panel";
  dev["sw"] = VERSION;

  // Origin information
  JsonObject origin = doc["o"].to<JsonObject>();
  origin["name"] = "Desk Control Panel";
  origin["sw"] = VERSION;
  origin["url"] = "http://" + device_ip;
  
  // Components
  JsonObject cmps = doc["cmps"].to<JsonObject>();
  
  // Add button components as timestamp sensors
  const int NUM_BUTTONS = 5;
  for (int i = 1; i <= NUM_BUTTONS; i++) {
    String const component_id = String(mqtt_client_id) + "_button_" + String(i);
    JsonObject button = cmps[component_id].to<JsonObject>();
    button["p"] = "sensor";
    button["unique_id"] = component_id;
    button["name"] = "Button " + String(i);
    button["state_topic"] = String(mqtt_topic_prefix) + "button/" + String(i) + "/pressed";
    button["device_class"] = "timestamp";
    button["icon"] = "mdi:button-pointer";
  }
  
  // Add action sensor component
  String const action_component_id = String(mqtt_client_id) + "_action";
  JsonObject action = cmps[action_component_id].to<JsonObject>();
  action["p"] = "sensor";
  action["unique_id"] = action_component_id;
  action["name"] = "Last Action";
  action["state_topic"] = String(mqtt_topic_prefix) + "action";
  action["icon"] = "mdi:gesture-tap";
  
  // Add status sensor component
  String const status_component_id = String(mqtt_client_id) + "_status";
  JsonObject status = cmps[status_component_id].to<JsonObject>();
  status["p"] = "binary_sensor";
  status["unique_id"] = status_component_id;
  status["name"] = "Status";
  status["state_topic"] = String(mqtt_topic_prefix) + "status";
  status["payload_on"] = "online";
  status["payload_off"] = "offline";
  status["device_class"] = "connectivity";
  
  // QoS
  doc["qos"] = 2;
  
  // Serialize and publish
  String json_string;
  size_t json_size = serializeJson(doc, json_string);

  if (json_size == 0) {
    Logger.error(MAIN_LOG, "Failed to serialize discovery message");
    return;
  }

  Logger.debug(MAIN_LOG, "Discover Message JSON size: %d bytes", json_size);
  Logger.debug(MAIN_LOG, "JSON content preview: %.200s...", json_string.c_str());

  String const discovery_topic = "homeassistant/device/" + String(mqtt_client_id) + "/config";
  
  if (mqtt_client.publish(discovery_topic.c_str(), json_string.c_str(), true)) {
    Logger.debug(MAIN_LOG, "Discovery message published successfully");
    Logger.debug(MAIN_LOG, "Topic: %s", discovery_topic.c_str());
    Logger.debug(MAIN_LOG, "Payload length: %d bytes", json_string.length());
  } else {
    Logger.error(MAIN_LOG, "Failed to publish discovery message, MQTT client state: %d", mqtt_client.state());
  }
}

auto MQTTManager::subscribeToSignImage() -> void {
  const char* topic = "office_sign/image/set";
  if (mqtt_client.subscribe(topic)) {
    Logger.debug(MAIN_LOG, "Subscribed to sign image topic: %s", topic);
  } else {
    Logger.error(MAIN_LOG, "Failed to subscribe to sign image topic: %s", topic);
  }
}

auto MQTTManager::subscribeToStatusTopics() -> void {
  const char* lightTopic = "desk-control/light-status";
  const char* fanTopic = "desk-control/fan-status";
  
  if (mqtt_client.subscribe(lightTopic)) {
    Logger.debug(MAIN_LOG, "Subscribed to light status topic: %s", lightTopic);
  } else {
    Logger.error(MAIN_LOG, "Failed to subscribe to light status topic: %s", lightTopic);
  }
  
  if (mqtt_client.subscribe(fanTopic)) {
    Logger.debug(MAIN_LOG, "Subscribed to fan status topic: %s", fanTopic);
  } else {
    Logger.error(MAIN_LOG, "Failed to subscribe to fan status topic: %s", fanTopic);
  }
}

auto MQTTManager::subscribeToPcMonitoring() -> void {
  const char* pcStatusTopic = "homeassistant/sensor/pc_status_monitor_status/status";
  const char* cpuTempTopic = "homeassistant/sensor/pc_status_monitor_cpu_temp_avg/state";
  const char* cpuUsageTopic = "homeassistant/sensor/pc_status_monitor_cpu_usage_avg/state";
  const char* gpuTempTopic = "homeassistant/sensor/pc_status_monitor_gpu_temp/state";
  const char* gpuUsageTopic = "homeassistant/sensor/pc_status_monitor_gpu_util/state";
  const char* ramUsageTopic = "homeassistant/sensor/pc_status_monitor_ram_usage/state";
  const char* gpuMemTopic = "homeassistant/sensor/pc_status_monitor_gpu_mem_util/state";
  
  const char* topics[] = {pcStatusTopic, cpuTempTopic, cpuUsageTopic, gpuTempTopic, 
                         gpuUsageTopic, ramUsageTopic, gpuMemTopic};
  
  for (const char* topic : topics) {
    if (mqtt_client.subscribe(topic)) {
      Logger.debug(MAIN_LOG, "Subscribed to PC monitoring topic: %s", topic);
    } else {
      Logger.error(MAIN_LOG, "Failed to subscribe to PC monitoring topic: %s", topic);
    }
  }
}

auto MQTTManager::onMqttMessage(char* topic, byte* payload, unsigned int length) -> void {
  Logger.debug(MAIN_LOG, "MQTT message received on topic: %s, length: %d", topic, length);
  
  // Convert payload to string
  String message;
  message.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) {
    message += static_cast<char>(payload[i]);
  }
  
  // Check if this is a sign image update
  if (strcmp(topic, "office_sign/image/set") == 0) {
    Logger.debug(MAIN_LOG, "Processing sign image update");
    SignState::getInstance().onImageReceived(message);
  }
  // Check if this is a light status update
  else if (strcmp(topic, "desk-control/light-status") == 0) {
    Logger.debug(MAIN_LOG, "Processing light status update: %s", message.c_str());
    bool const lightOn = (message == "on");
    AppState::getInstance().setLightStatus(lightOn);
  }
  // Check if this is a fan status update
  else if (strcmp(topic, "desk-control/fan-status") == 0) {
    Logger.debug(MAIN_LOG, "Processing fan status update: %s", message.c_str());
    bool const fanOn = (message == "on");
    AppState::getInstance().setFanStatus(fanOn);
  }
  // Check if this is a PC status update
  else if (strcmp(topic, "homeassistant/sensor/pc_status_monitor_status/status") == 0) {
    Logger.debug(MAIN_LOG, "Processing PC status update: %s", message.c_str());
    bool const pcOn = (message == "ON");
    AppState::getInstance().setPcStatus(pcOn);
  }
  // Check if this is a CPU temperature update
  else if (strcmp(topic, "homeassistant/sensor/pc_status_monitor_cpu_temp_avg/state") == 0) {
    Logger.debug(MAIN_LOG, "Processing CPU temp update: %s", message.c_str());
    float const temp = message.toFloat();
    AppState::getInstance().setCpuTemp(temp);
  }
  // Check if this is a CPU usage update
  else if (strcmp(topic, "homeassistant/sensor/pc_status_monitor_cpu_usage_avg/state") == 0) {
    Logger.debug(MAIN_LOG, "Processing CPU usage update: %s", message.c_str());
    float const usage = message.toFloat();
    AppState::getInstance().setCpuUsage(usage);
  }
  // Check if this is a GPU temperature update
  else if (strcmp(topic, "homeassistant/sensor/pc_status_monitor_gpu_temp/state") == 0) {
    Logger.debug(MAIN_LOG, "Processing GPU temp update: %s", message.c_str());
    float const temp = message.toFloat();
    AppState::getInstance().setGpuTemp(temp);
  }
  // Check if this is a GPU usage update
  else if (strcmp(topic, "homeassistant/sensor/pc_status_monitor_gpu_util/state") == 0) {
    Logger.debug(MAIN_LOG, "Processing GPU usage update: %s", message.c_str());
    float const usage = message.toFloat();
    AppState::getInstance().setGpuUsage(usage);
  }
  // Check if this is a RAM usage update
  else if (strcmp(topic, "homeassistant/sensor/pc_status_monitor_ram_usage/state") == 0) {
    Logger.debug(MAIN_LOG, "Processing RAM usage update: %s", message.c_str());
    float const usage = message.toFloat();
    AppState::getInstance().setRamUsage(usage);
  }
  // Check if this is a GPU memory usage update
  else if (strcmp(topic, "homeassistant/sensor/pc_status_monitor_gpu_mem_util/state") == 0) {
    Logger.debug(MAIN_LOG, "Processing GPU mem usage update: %s", message.c_str());
    float const usage = message.toFloat();
    AppState::getInstance().setGpuMemUsage(usage);
  }
}
