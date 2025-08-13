#include "ota_manager.h"
#include "display.h"
#include "config.h"
#include <WiFi.h>
#include <ota-github-defaults.h>
#include <OTA-Hub.hpp>

auto OTAManager::getInstance() -> OTAManager& {
  static OTAManager instance;
  return instance;
}

auto OTAManager::init() -> void
{
  wifi_client.setInsecure(); // Disable certificate verification
  OTA::init(wifi_client);
}

auto OTAManager::checkForUpdate() -> void
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Cannot check for updates.");
    Display::getInstance().setLoadingMessage("WiFi not connected");
    delay(2000);
    return;
  }

  Display::getInstance().setLoadingMessage("Checking for", "updates...");
  delay(1000);

  OTA::UpdateObject details = OTA::isUpdateAvailable();
  details.print();
  if (OTA::NO_UPDATE != details.condition)
  {
      if (OTA::performUpdate(&details, true, false) != OTA::SUCCESS)
      {
          Serial.println("Update failed. Continuing...");
          Display::getInstance().setLoadingMessage("Update failed");
          delay(2000);
      }
      else
      {
          Serial.println("Update successful. Restarting...");
          Display::getInstance().setLoadingMessage("Update successful", "Restarting...");
          delay(2000);
          ESP.restart();
      }
  }
  else
  {
      Serial.println("No new update available. Continuing...");
      Display::getInstance().setLoadingMessage("Up to Date");
      delay(2000);
  }
}