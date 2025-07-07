#include "app_state.h"
#include "time_manager.h"
#include <Arduino.h>
#include <Elog.h>
#include <logging.h>
#include <display.h>

// Define static member variables (constants only)
const unsigned long TimeManager::NTP_SYNC_INTERVAL = 3600000; // 1 hour in milliseconds
const unsigned long TimeManager::MINUTE_UPDATE_INTERVAL = 60000; // 1 minute in milliseconds
const long SECONDS_PER_HOUR = 3600;
const long TZ_OFFSET = -5;

auto TimeManager::getInstance() -> TimeManager& {
  static TimeManager instance;
  return instance;
}

auto TimeManager::init() -> void {
  Logger.debug(MAIN_LOG, "Initializing time manager...");

  Display::getInstance().setLoadingMessage("Syncing Time");

  // Give WiFi a moment to stabilize
  const int WIFI_DELAY_MS = 1000;
  delay(WIFI_DELAY_MS);
  
  setupNTP();
  Logger.debug(MAIN_LOG, "Time manager initialized.");
}

auto TimeManager::update() -> void {
  unsigned long const currentMillis = millis();
  
  // Sync with NTP server every hour
  if (currentMillis - lastNTPSync >= NTP_SYNC_INTERVAL) {
    syncTimeFromNTP();
    lastNTPSync = currentMillis;
  }
  
  // Update time display every minute
  if (currentMillis - lastMinuteUpdate >= MINUTE_UPDATE_INTERVAL) {
    updateTimeDisplay();
    lastMinuteUpdate = currentMillis;
  }
}

auto TimeManager::getCurrentTimeString() const -> String {
  if (!timeInitialized) {
    return "Time Error";
  }
  
  struct tm timeInfo{};
  if (getLocalTime(&timeInfo)) {
    return formatTime(&timeInfo);
  }     return "Time Error";
 
}

auto TimeManager::isTimeInitialized() const -> bool {
  return timeInitialized;
}

auto TimeManager::forceSync() -> void {
  syncTimeFromNTP();
  updateTimeDisplay();
}

auto TimeManager::setupNTP() -> void {
  Logger.debug(MAIN_LOG, "Setting up NTP time synchronization...");

  configTime(TZ_OFFSET * SECONDS_PER_HOUR, SECONDS_PER_HOUR, "pool.ntp.org", "time.nist.gov");

  Logger.debug(MAIN_LOG, "Waiting for NTP time sync");
  time_t now = 0;
  int retry = 0;
  const int max_retries = 20;

  const int RETRY_DELAY_MS = 500;
  const int LOADED_TIME = 8 * SECONDS_PER_HOUR * 2; // 16 hours in seconds
  while (now < LOADED_TIME && retry < max_retries) {
    Logger.debug(MAIN_LOG, "Waiting for NTP time sync...");
    delay(RETRY_DELAY_MS);
    time(&now); //NOLINT(cert-err33-c) No need to check return, loop handles it
    retry++;
  }
  
  if (retry >= max_retries) {
    Logger.error(MAIN_LOG, "Failed to get time from NTP server.");
    DisplayState* rootState = AppState::getInstance().getRootState();
    if (rootState != nullptr) {
      rootState->label = "Time Error";
    }
    return;
  }

  Logger.debug(MAIN_LOG, "Time synchronized. Current time: %s", ctime(&now));
  timeInitialized = true;
  
  updateTimeDisplay();
  
  lastNTPSync = millis();
  lastMinuteUpdate = millis();
}

auto TimeManager::syncTimeFromNTP() const -> void {
  Logger.debug(MAIN_LOG, "Syncing time with NTP server...");

  // Force NTP update with timezone configuration
  configTime(TZ_OFFSET * SECONDS_PER_HOUR, SECONDS_PER_HOUR, "pool.ntp.org", "time.nist.gov");

  // No point waiting for the sync, the update will happen next time the clock updates anyways
}

auto TimeManager::updateTimeDisplay() -> void {
  if (!timeInitialized) {
    return;
  }
  
  // Get current time from ESP32 RTC
  struct tm timeInfo{};
  if (getLocalTime(&timeInfo)) {
    currentTime = timeInfo;
    String const timeString = formatTime(&timeInfo);
    DisplayState* rootState = AppState::getInstance().getRootState();
    if (rootState != nullptr) {
      rootState->label = timeString;
    }
    Logger.debug(MAIN_LOG, "Time updated: %s", timeString.c_str());
  } else {
    Logger.error(MAIN_LOG, "Failed to get local time!");
  }
}

const int MAX_TIME_STRING_LENGTH = 20;
auto TimeManager::formatTime(struct tm* timeInfo) const -> String {
  std::array<char, MAX_TIME_STRING_LENGTH> buffer;
  strftime(buffer.data(), buffer.size(), "%a %m/%d %I:%M %p", timeInfo);
  return {buffer.data()};
}
