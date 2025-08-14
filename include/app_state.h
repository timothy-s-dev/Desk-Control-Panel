#ifndef APP_STATE_H
#define APP_STATE_H

#include <Arduino.h>
#include <memory>

// DisplayState structure
struct DisplayState {
  String label;
  DisplayState* subStates;
  int numSubStates;
  String serialAction;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
class AppState {
public:
  // Get the singleton instance
  static AppState& getInstance();
  
  // Delete copy constructor and assignment operator
  AppState(const AppState&) = delete;
  AppState& operator=(const AppState&) = delete;
  
  void init();
  
  auto getRootState() -> DisplayState*;
  auto getCurrentState() -> DisplayState*;
  auto getCurrentSubStateIndex() const -> int;

  auto onSelect() -> void;
  auto onNext() -> void;
  auto onPrevious() -> void;
  auto tick() -> void;

  // Light and fan status
  auto setLightStatus(bool status) -> void;
  auto setFanStatus(bool status) -> void;
  auto getLightStatus() const -> bool;
  auto getFanStatus() const -> bool;

  // PC monitoring
  auto setPcStatus(bool status) -> void;
  auto setCpuTemp(float temp) -> void;
  auto setCpuUsage(float usage) -> void;
  auto setGpuTemp(float temp) -> void;
  auto setGpuUsage(float usage) -> void;
  auto setRamUsage(float usage) -> void;
  auto setGpuMemUsage(float usage) -> void;
  
  auto getPcStatus() const -> bool;
  auto getCpuTemp() const -> float;
  auto getCpuUsage() const -> float;
  auto getGpuTemp() const -> float;
  auto getGpuUsage() const -> float;
  auto getRamUsage() const -> float;
  auto getGpuMemUsage() const -> float;

private:
  // Private constructor for singleton
  AppState() = default;
  
  std::unique_ptr<DisplayState> rootState = nullptr;
  DisplayState* currentState = nullptr;
  int currentSubStateIndex = -1;
  unsigned long lastInput = -1;
  
  // Status variables
  bool lightStatus = false;
  bool fanStatus = false;
  
  // PC monitoring variables
  bool pcStatus = false;
  float cpuTemp = 0.0f;
  float cpuUsage = 0.0f;
  float gpuTemp = 0.0f;
  float gpuUsage = 0.0f;
  float ramUsage = 0.0f;
  float gpuMemUsage = 0.0f;
  
  void resetToRoot();
};

#endif // APP_STATE_H
