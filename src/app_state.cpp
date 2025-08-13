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
           new DisplayState[3]{
               {"Meeting", nullptr, 0, "os-meeting"},
               {"Focus", nullptr, 0, "os-focus"},
               {"Free", nullptr, 0, "os-free"}},
           3, ""},
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
