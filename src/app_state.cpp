#include "app_state.h"
#include "mqtt_manager.h"
#include "display.h"
#include "ota_manager.h"
#include <Arduino.h>

const int TIMEOUT_MS = 3000;

auto AppState::getInstance() -> AppState& {
  static AppState instance;
  return instance;
}

void AppState::init() {
  rootState = std::unique_ptr<DisplayState>(new DisplayState{
      "Idle",
      new DisplayState[2]{
          {"Office Sign",
           new DisplayState[5]{
               {"Work", nullptr, 0, "os-work"},
               {"Meeting", nullptr, 0, "os-meeting"},
               {"Focus", nullptr, 0, "os-focus"},
               {"Gaming", nullptr, 0, "os-play"},
               {"Free", nullptr, 0, "os-free"}},
           5, ""},
          {"Update", nullptr, 0, "update"}},
      2, ""});
  currentState = rootState.get();
}

auto AppState::getRootState() -> DisplayState* {
  return rootState.get();
}

auto AppState::getCurrentState() -> DisplayState* {
  return currentState;
}

auto AppState::getCurrentSubStateIndex() const -> int {
  return currentSubStateIndex;
}

void AppState::resetToRoot() {
  currentState = rootState.get();
  currentSubStateIndex = -1;
}

void AppState::onSelect() {
  lastInput = millis();
  if (currentSubStateIndex == -1) {
      currentSubStateIndex = 0;
    } else {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) - bounds-checked array access
      DisplayState* targetState = &currentState->subStates[currentSubStateIndex];
      if (targetState->numSubStates > 0) {
        currentState = targetState;
        currentSubStateIndex = 0;
      } else if (!targetState->serialAction.isEmpty()) {
        if (targetState->serialAction == "update") {
          OTAManager::getInstance().checkForUpdate();
        } else {
        MQTTManager::getInstance().publishAction(targetState->serialAction);
        resetToRoot();
        lastInput = -1;
        }
      }
    }
}

void AppState::onNext() {
  lastInput = millis();
  if (currentSubStateIndex == -1) {
    currentSubStateIndex = 0;
  } else {
    currentSubStateIndex++;
    if (currentSubStateIndex >= currentState->numSubStates) {
      currentSubStateIndex = 0;
    }
    currentSubStateIndex = currentSubStateIndex;
  }
}

void AppState::onPrevious() {
  lastInput = millis();
  if (currentSubStateIndex == -1) {
    currentSubStateIndex = 0;
  } else {
    currentSubStateIndex--;
    if (currentSubStateIndex < 0) {
      currentSubStateIndex = currentState->numSubStates - 1;
    }
    currentSubStateIndex = currentSubStateIndex;
  }
}

void AppState::tick() {
  unsigned long const time = millis();
  if (lastInput != -1 && time - lastInput > TIMEOUT_MS) {
    resetToRoot();
    lastInput = -1;
  }
}

auto AppState::setLightStatus(bool status) -> void {
  lightStatus = status;
}

auto AppState::setFanStatus(bool status) -> void {
  fanStatus = status;
}

auto AppState::getLightStatus() const -> bool {
  return lightStatus;
}

auto AppState::getFanStatus() const -> bool {
  return fanStatus;
}

auto AppState::setPcStatus(bool status) -> void {
  pcStatus = status;
}

auto AppState::setCpuTemp(float temp) -> void {
  cpuTemp = temp;
}

auto AppState::setCpuUsage(float usage) -> void {
  cpuUsage = usage;
}

auto AppState::setGpuTemp(float temp) -> void {
  gpuTemp = temp;
}

auto AppState::setGpuUsage(float usage) -> void {
  gpuUsage = usage;
}

auto AppState::setRamUsage(float usage) -> void {
  ramUsage = usage;
}

auto AppState::setGpuMemUsage(float usage) -> void {
  gpuMemUsage = usage;
}

auto AppState::getPcStatus() const -> bool {
  return pcStatus;
}

auto AppState::getCpuTemp() const -> float {
  return cpuTemp;
}

auto AppState::getCpuUsage() const -> float {
  return cpuUsage;
}

auto AppState::getGpuTemp() const -> float {
  return gpuTemp;
}

auto AppState::getGpuUsage() const -> float {
  return gpuUsage;
}

auto AppState::getRamUsage() const -> float {
  return ramUsage;
}

auto AppState::getGpuMemUsage() const -> float {
  return gpuMemUsage;
}
