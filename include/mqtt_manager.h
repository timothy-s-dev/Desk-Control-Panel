#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiManager.h>

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
class MQTTManager {
public:
  static auto getInstance() -> MQTTManager&;
  
  MQTTManager(const MQTTManager&) = delete;
  auto operator=(const MQTTManager&) -> MQTTManager& = delete;

  auto init() -> void;
  auto update() -> void;
  auto publishButtonState(int button_num, bool pressed) -> void;
  auto publishAction(const String& action) -> void;
  auto subscribeToSignImage() -> void;
  auto subscribeToStatusTopics() -> void;
  auto subscribeToPcMonitoring() -> void;

private:
  MQTTManager() = default;
  
  static const char* mqtt_client_id;
  static const char* mqtt_topic_prefix;
  static const unsigned long MQTT_RECONNECT_INTERVAL;

  String mqtt_server;
  int mqtt_port = 1883;
  String mqtt_username;
  String mqtt_password;
  
  WiFiClient espClient;
  PubSubClient mqtt_client{espClient};
  unsigned long lastMqttReconnectAttempt = 0;
  
  auto setupMQTT() -> void;
  auto mqttReconnect() -> void;
  auto isConnected() -> bool;
  auto publishDiscoveryMessage() -> void;
  auto publishMessage(const char* topic, const char* message) -> void;
  static auto onMqttMessage(char* topic, byte* payload, unsigned int length) -> void;
};

#endif // MQTT_MANAGER_H
