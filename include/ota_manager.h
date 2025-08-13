#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>

class OTAManager {
public:
  static auto getInstance() -> OTAManager&;

  OTAManager(const OTAManager&) = delete;
  auto operator=(const OTAManager&) -> OTAManager& = delete;

  auto init() -> void;
  auto checkForUpdate() -> void;

private:
  OTAManager() = default;
  WiFiClientSecure wifi_client;
};

#endif // OTA_MANAGER_H
