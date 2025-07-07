#include "display.h"
#include <Arduino.h>
#include <Elog.h>
#include <logging.h>

const int DISPLAY_WIDTH = 128;
const int HALF_DISPLAY_WIDTH = DISPLAY_WIDTH / 2;
const int DISPLAY_HEIGHT = 64;
const int FONT_HEIGHT = 11;
const int AVG_FONT_WIDTH = 6;
const int BOX_SIZE = 8;

auto Display::getInstance() -> Display& {
  static Display instance;
  return instance;
}

auto Display::init() -> void {
  u8g2.begin();
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay) - U8g2 font data intentionally passed as pointer
  u8g2.setFont(u8g2_font_t0_12b_mf);
  Logger.debug(MAIN_LOG, "Display setup complete.");
}

auto Display::update() -> void {
  DisplayState* currentState = AppState::getInstance().getCurrentState();
  int const currentSubStateIndex = AppState::getInstance().getCurrentSubStateIndex();

  // Update bouncing box position
  boxX += boxDeltaX;
  boxY += boxDeltaY;
  if (boxX <= 0 || boxX >= (DISPLAY_WIDTH - BOX_SIZE)) {
    boxDeltaX = -boxDeltaX;
    boxX += boxDeltaX;
  }
  if (boxY <= (2 * FONT_HEIGHT) + 2 || boxY >= (DISPLAY_HEIGHT - BOX_SIZE)) {
    boxDeltaY = -boxDeltaY;
    boxY += boxDeltaY;
  }

  // Clear the display and render content
  u8g2.clearBuffer();

  // Display current state label
  printCentered(currentState->label.c_str(), FONT_HEIGHT);

  // Display sub-state label if one is selected
  if (currentSubStateIndex != -1) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) - bounds-checked array access
    DisplayState* targetState = &currentState->subStates[currentSubStateIndex];
    printCentered(targetState->label.c_str(), FONT_HEIGHT * 2);
  }

  // Draw the bouncing box
  u8g2.drawBox(boxX, boxY, BOX_SIZE, BOX_SIZE);
  u8g2.sendBuffer();
}

auto Display::setLoadingMessage(const char* line1) -> void {
  u8g2.clearBuffer();
  printCentered(line1, DISPLAY_HEIGHT / 2 - FONT_HEIGHT / 2);
  u8g2.sendBuffer();
}

auto Display::setLoadingMessage(const char* line1, const char* line2) -> void {
  u8g2.clearBuffer();
  printCentered(line1, DISPLAY_HEIGHT / 2 - FONT_HEIGHT / 2);
  printCentered(line2, DISPLAY_HEIGHT / 2 + FONT_HEIGHT / 2);
  u8g2.sendBuffer();
}

auto Display::setLoadingMessage(const char* line1, const char* line2, const char* line3) -> void {
  u8g2.clearBuffer();
  printCentered(line1, DISPLAY_HEIGHT / 2 - FONT_HEIGHT);
  printCentered(line2, DISPLAY_HEIGHT / 2);
  printCentered(line3, DISPLAY_HEIGHT / 2 + FONT_HEIGHT);
  u8g2.sendBuffer();
}

auto Display::printCentered(const char* text, int y) -> void {
  unsigned int const textOffset = (DISPLAY_WIDTH / 2) - (strlen(text) / 2 * AVG_FONT_WIDTH);
  u8g2.setCursor(textOffset, y);
  u8g2.print(text);
}
