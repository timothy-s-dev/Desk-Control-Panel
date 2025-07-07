#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <Arduino.h>
#include <RotaryEncoder.h>
#include <memory>

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
enum class RotaryDirection {
  NONE,
  CLOCKWISE,
  COUNTERCLOCKWISE
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
class RotaryEncoderManager {
public:
  static auto getInstance() -> RotaryEncoderManager&;
  
  RotaryEncoderManager(const RotaryEncoderManager&) = delete;
  auto operator=(const RotaryEncoderManager&) -> RotaryEncoderManager& = delete;
  
  auto init() -> void;
  auto getRotationDirection() -> RotaryDirection;
  auto isButtonPressed() -> bool;
  auto tick() -> void;

private:
  RotaryEncoderManager() = default;
  
  std::unique_ptr<RotaryEncoder> encoder = nullptr;
  bool lastButtonState = false;
  bool currentButtonState = false;
  
  auto checkPosition() -> void;
  
  // Static interrupt handler
  static auto checkPositionStatic() -> void;
};

#endif // ROTARY_ENCODER_H
