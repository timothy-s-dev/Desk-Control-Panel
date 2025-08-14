#include "display.h"
#include "sign_state.h"
#include "app_state.h"
#include <Arduino.h>
#include <Elog.h>
#include <logging.h>

const int DISPLAY_WIDTH = 128;
const int HALF_DISPLAY_WIDTH = DISPLAY_WIDTH / 2;
const int DISPLAY_HEIGHT = 64;
const int FONT_HEIGHT = 11;
const int AVG_FONT_WIDTH = 6;
const int BOX_SIZE = 8;

const byte LIGHT_ICON[] = {
  0b10010001,
  0b01000010,
  0b00011000,
  0b10111100,
  0b00111101,
  0b00011000,
  0b00000000,
  0b00011000
};

const byte FAN_ICON[] = {
  0b00001000,
  0b00001000,
  0b00011000,
  0b11111100,
  0b00111111,
  0b00011000,
  0b00010000,
  0b00010000
};

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
  
  // Render sign image if available
  renderSignImage();
  
  // Render status icons
  renderStatusIcons();
  
  // Render PC monitoring data
  renderPcMonitoring();
  
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

auto Display::renderSignImage() -> void {
  SignState& signState = SignState::getInstance();
  
  if (!signState.hasImageData()) {
    return; // No image data available
  }
  
  const std::vector<uint8_t>& imageData = signState.getImageData();
  
  // Position the 32x8 image in the bottom left corner with 1px border and 2px padding
  const int borderSize = 1;
  const int paddingSize = 2;
  const int totalOffset = borderSize + paddingSize;
  const int offsetX = totalOffset;
  const int offsetY = DISPLAY_HEIGHT - SignState::IMAGE_HEIGHT - totalOffset;
  
  // Draw 1px border around the image (including padding)
  const int borderX = 0;
  const int borderY = DISPLAY_HEIGHT - SignState::IMAGE_HEIGHT - 2 * (paddingSize + borderSize);
  const int borderWidth = SignState::IMAGE_WIDTH + 2 * (paddingSize + borderSize);
  const int borderHeight = SignState::IMAGE_HEIGHT + 2 * (paddingSize + borderSize);
  
  // Draw border frame
  u8g2.drawFrame(borderX, borderY, borderWidth, borderHeight);
  
  // Render the monochrome bitmap
  for (int y = 0; y < SignState::IMAGE_HEIGHT; y++) {
    for (int x = 0; x < SignState::IMAGE_WIDTH; x++) {
      int const bitIndex = y * SignState::IMAGE_WIDTH + x;
      int const byteIndex = bitIndex / 8;
      int const bitPosition = bitIndex % 8;
      
      if (byteIndex < imageData.size()) {
        bool const isPixelSet = (imageData[byteIndex] & (1 << (7 - bitPosition))) != 0;
        
        if (isPixelSet) {
          u8g2.drawPixel(offsetX + x, offsetY + y);
        }
      }
    }
  }
}

auto Display::renderStatusIcons() -> void {
  AppState& appState = AppState::getInstance();
  
  const int iconSize = 8;
  const int borderSize = 1;
  const int paddingSize = 2;
  const int iconWithBorder = iconSize + 2 * (paddingSize + borderSize);
  
  // Calculate position above the sign (sign is at bottom left)
  // Sign area: 36x12 (32+4 for padding + 4 for border)
  const int signAreaWidth = SignState::IMAGE_WIDTH + 2 * (paddingSize + borderSize);
  const int signAreaHeight = SignState::IMAGE_HEIGHT + 2 * (paddingSize + borderSize);
  const int iconsY = DISPLAY_HEIGHT - signAreaHeight - iconWithBorder - 2; // 2px gap between icons and sign
  
  // Light icon position - left edge aligned with left edge of sign
  const int lightX = 0;
  
  // Fan icon position - right edge aligned with right edge of sign
  const int fanX = signAreaWidth - iconWithBorder;
  
  // Always render both frames
  u8g2.drawFrame(lightX, iconsY, iconWithBorder, iconWithBorder);
  u8g2.drawFrame(fanX, iconsY, iconWithBorder, iconWithBorder);
  
  // Render light icon content if light is on
  if (appState.getLightStatus()) {
    renderIconContent(LIGHT_ICON, lightX, iconsY, iconSize, borderSize, paddingSize);
  }
  
  // Render fan icon content if fan is on
  if (appState.getFanStatus()) {
    renderIconContent(FAN_ICON, fanX, iconsY, iconSize, borderSize, paddingSize);
  }
}

auto Display::renderIconContent(const byte* iconData, int x, int y, int iconSize, int borderSize, int paddingSize) -> void {
  // Calculate icon position with border and padding
  const int iconX = x + borderSize + paddingSize;
  const int iconY = y + borderSize + paddingSize;
  
  // Render the 8x8 icon
  for (int row = 0; row < iconSize; row++) {
    byte rowData = iconData[row];
    for (int col = 0; col < iconSize; col++) {
      if (rowData & (1 << (7 - col))) { // MSB first
        u8g2.drawPixel(iconX + col, iconY + row);
      }
    }
  }
}

auto Display::renderPcMonitoring() -> void {
  AppState& appState = AppState::getInstance();
  
  // Only display if PC is on
  if (!appState.getPcStatus()) {
    return;
  }
  
  // Layout calculations
  const int iconSize = 8;
  const int borderSize = 1;
  const int paddingSize = 2;
  const int iconWithBorder = iconSize + 2 * (paddingSize + borderSize);
  const int signAreaWidth = SignState::IMAGE_WIDTH + 2 * (paddingSize + borderSize);
  const int signAreaHeight = SignState::IMAGE_HEIGHT + 2 * (paddingSize + borderSize);
  
  // Position to the right of the sign area with some margin
  const int rightMargin = 0; // Margin from right edge of display
  const int leftMargin = 8; // Margin from sign area
  const int availableWidth = DISPLAY_WIDTH - signAreaWidth - leftMargin - rightMargin;
  
  // Switch to a smaller font, not a ton of room to work with
  u8g2.setFont(u8g2_font_6x10_tf);
  
  // Line height for this font
  const int lineHeight = 10;
  const int columnWidth = availableWidth / 3; // Divide available space into 3 columns
  
  // Start position - align with top of status icons area
  const int statusIconsY = DISPLAY_HEIGHT - signAreaHeight - iconWithBorder - 2;
  const int startY = statusIconsY + lineHeight - 1; // Account for font baseline
  
  // Column positions - right-aligned within available space
  const int startX = signAreaWidth + leftMargin;
  const int cpuX = startX;
  const int gpuX = startX + columnWidth;
  const int ramX = startX + columnWidth * 2;
  
  // Row 1: Labels
  u8g2.setCursor(cpuX, startY);
  u8g2.print("CPU");
  u8g2.setCursor(gpuX, startY);
  u8g2.print("GPU");
  u8g2.setCursor(ramX, startY);
  u8g2.print("RAM");
  
  // Row 2: Usage percentages
  char text[16];
  snprintf(text, sizeof(text), "%.0f%%", appState.getCpuUsage());
  u8g2.setCursor(cpuX, startY + lineHeight);
  u8g2.print(text);
  
  snprintf(text, sizeof(text), "%.0f%%", appState.getGpuUsage());
  u8g2.setCursor(gpuX, startY + lineHeight);
  u8g2.print(text);
  
  snprintf(text, sizeof(text), "%.0f%%", appState.getRamUsage());
  u8g2.setCursor(ramX, startY + lineHeight);
  u8g2.print(text);
  
  // Row 3: Temperatures/VRAM
  snprintf(text, sizeof(text), "%.0fC", appState.getCpuTemp());
  u8g2.setCursor(cpuX, startY + lineHeight * 2);
  u8g2.print(text);
  
  snprintf(text, sizeof(text), "%.0fC", appState.getGpuTemp());
  u8g2.setCursor(gpuX, startY + lineHeight * 2);
  u8g2.print(text);
  
  snprintf(text, sizeof(text), "%.0f%%", appState.getGpuMemUsage());
  u8g2.setCursor(ramX, startY + lineHeight * 2);
  u8g2.print(text);
  
  // Restore original font
  u8g2.setFont(u8g2_font_t0_12b_mf);
}
