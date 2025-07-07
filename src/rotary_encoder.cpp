#include "rotary_encoder.h"
#include <Arduino.h>
#include <Elog.h>
#include <logging.h>

// Pin definitions
enum {
ROTARY_BUTTON_PIN = 35,
ROTARY_DT_PIN = 33,
ROTARY_CLK_PIN = 32
};

auto RotaryEncoderManager::getInstance() -> RotaryEncoderManager& {
  static RotaryEncoderManager instance;
  return instance;
}

auto RotaryEncoderManager::init() -> void {
  // Initialize button pin
  pinMode(ROTARY_BUTTON_PIN, INPUT_PULLUP);
  
  // Initialize rotary encoder
  encoder = std::unique_ptr<RotaryEncoder>(new RotaryEncoder(ROTARY_DT_PIN, ROTARY_CLK_PIN, RotaryEncoder::LatchMode::FOUR0));
  
  // Set up interrupts for encoder pins
  attachInterrupt(digitalPinToInterrupt(ROTARY_DT_PIN), checkPositionStatic, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROTARY_CLK_PIN), checkPositionStatic, CHANGE);
  
  // Initialize button state
  currentButtonState = digitalRead(ROTARY_BUTTON_PIN) == LOW;
  lastButtonState = currentButtonState;

  Logger.debug(MAIN_LOG, "Rotary encoder setup complete.");
}

auto RotaryEncoderManager::getRotationDirection() -> RotaryDirection {
  if (encoder == nullptr) { return RotaryDirection::NONE; }

  // NOLINTNEXTLINE(cppcoreguidelines-init-variables) - variable is initialized by method call
  RotaryEncoder::Direction const direction = encoder->getDirection();
  
  switch (direction) {
    case RotaryEncoder::Direction::CLOCKWISE:
      return RotaryDirection::CLOCKWISE;
    case RotaryEncoder::Direction::COUNTERCLOCKWISE:
      return RotaryDirection::COUNTERCLOCKWISE;
    default:
      return RotaryDirection::NONE;
  }
}

auto RotaryEncoderManager::isButtonPressed() -> bool {
  // Read current button state
  currentButtonState = digitalRead(ROTARY_BUTTON_PIN) == LOW;
  
  // Check if button was just pressed (transition from not pressed to pressed)
  bool const wasPressed = !lastButtonState && currentButtonState;
  
  // Update last button state
  lastButtonState = currentButtonState;
  
  return wasPressed;
}

auto RotaryEncoderManager::tick() -> void {
  if (encoder != nullptr) {
    encoder->tick();
  }
}

auto RotaryEncoderManager::checkPosition() -> void {
  if (encoder != nullptr) {
    encoder->tick();
  }
}

auto RotaryEncoderManager::checkPositionStatic() -> void {
  getInstance().checkPosition();
}
