#ifndef SIGN_STATE_H
#define SIGN_STATE_H

#include <Arduino.h>
#include <vector>

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
class SignState {
public:
  // Get the singleton instance
  static auto getInstance() -> SignState&;
  
  // Delete copy constructor and assignment operator
  SignState(const SignState&) = delete;
  auto operator=(const SignState&) -> SignState& = delete;
  
  auto init() -> void;
  auto update() -> void;
  
  // Called when new image data is received via MQTT
  auto onImageReceived(const String& imageData) -> void;
  
  // Get the current monochrome image data (32x8)
  auto getImageData() const -> const std::vector<uint8_t>&;
  
  // Check if image data is available
  auto hasImageData() const -> bool;
  
  // Get image dimensions
  static constexpr int IMAGE_WIDTH = 32;
  static constexpr int IMAGE_HEIGHT = 8;

private:
  // Private constructor for singleton
  SignState() = default;
  
  // Convert RGB image data to monochrome
  auto convertToMonochrome(const String& base64Data) -> std::vector<uint8_t>;
  
  // Decode base64 string
  auto decodeBase64(const String& encoded) -> std::vector<uint8_t>;
  
  // Convert RGB pixel to monochrome (1 bit per pixel)
  auto rgbToMono(uint8_t r, uint8_t g, uint8_t b) -> bool;
  
  std::vector<uint8_t> monochromeImageData;
  bool imageDataAvailable = false;
  unsigned long lastImageUpdate = 0;
};

#endif // SIGN_STATE_H
