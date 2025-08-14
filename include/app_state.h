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
  
  void resetToRoot();
};

#endif // APP_STATE_H
