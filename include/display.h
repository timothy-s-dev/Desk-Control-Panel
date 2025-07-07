#ifndef DISPLAY_H
#define DISPLAY_H

#include "app_state.h"
#include <Arduino.h>
#include <U8g2lib.h>

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
class Display {
public:
  // Get the singleton instance
  static auto getInstance() -> Display&;
  
  // Delete copy constructor and assignment operator
  Display(const Display&) = delete;
  auto operator=(const Display&) -> Display& = delete;
  
  auto init() -> void;
  auto update() -> void;
  auto setLoadingMessage(const char* line1) -> void;
  auto setLoadingMessage(const char* line1, const char* line2) -> void;
  auto setLoadingMessage(const char* line1, const char* line2, const char* line3) -> void;

private:
  // Private constructor for singleton
  Display() = default;
  
  // Instance variables for bouncing box animation
  int boxX = 0;
  int boxDeltaX = 1;
  int boxY = 24;
  int boxDeltaY = 1;

  // Display instance
  U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2{U8G2_R0, U8X8_PIN_NONE};

  auto printCentered(const char* text, int y) -> void;
};

#endif // DISPLAY_H
