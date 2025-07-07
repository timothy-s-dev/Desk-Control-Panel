#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <sys/time.h>
#include <time.h>

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
class TimeManager {
public:
  static auto getInstance() -> TimeManager&;
  
  TimeManager(const TimeManager&) = delete;
  auto operator=(const TimeManager&) -> TimeManager& = delete;
  
  auto init() -> void;
  auto update() -> void;
  auto getCurrentTimeString() const -> String;
  auto isTimeInitialized() const -> bool;
  auto forceSync() -> void;

private:
  TimeManager() = default;
  
  static const unsigned long NTP_SYNC_INTERVAL;
  static const unsigned long MINUTE_UPDATE_INTERVAL;
  
  unsigned long lastNTPSync = 0;
  unsigned long lastMinuteUpdate = 0;
  struct tm currentTime = {0};
  bool timeInitialized = false;
  
  auto setupNTP() -> void;
  auto syncTimeFromNTP() const -> void;
  auto updateTimeDisplay() -> void;
  auto formatTime(struct tm* timeInfo) const -> String;
};

#endif // TIME_MANAGER_H
