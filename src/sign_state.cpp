#include "sign_state.h"
#include <Arduino.h>
#include <Elog.h>
#include <logging.h>

auto SignState::getInstance() -> SignState& {
  static SignState instance;
  return instance;
}

auto SignState::init() -> void {
  Logger.debug(MAIN_LOG, "Initializing SignState...");
  
  // Initialize with empty image data
  monochromeImageData.clear();
  monochromeImageData.resize((IMAGE_WIDTH * IMAGE_HEIGHT) / 8, 0); // 1 bit per pixel, packed into bytes
  imageDataAvailable = false;
  
  Logger.debug(MAIN_LOG, "SignState initialized.");
}

auto SignState::update() -> void {
  // SignState is event-driven via MQTT, no periodic updates needed
}

auto SignState::onImageReceived(const String& imageData) -> void {
  Logger.debug(MAIN_LOG, "Received new image data, length: %d", imageData.length());
  
  try {
    monochromeImageData = convertToMonochrome(imageData);
    imageDataAvailable = true;
    lastImageUpdate = millis();
    
    Logger.debug(MAIN_LOG, "Image converted to monochrome successfully");
  } catch (const std::exception& e) {
    Logger.error(MAIN_LOG, "Failed to process image data: %s", e.what());
    imageDataAvailable = false;
  }
}

auto SignState::getImageData() const -> const std::vector<uint8_t>& {
  return monochromeImageData;
}

auto SignState::hasImageData() const -> bool {
  return imageDataAvailable;
}

auto SignState::convertToMonochrome(const String& base64Data) -> std::vector<uint8_t> {
  // Decode base64 to get RGB data
  std::vector<uint8_t> rgbData = decodeBase64(base64Data);
  
  // Expected size: 32 * 8 * 3 = 768 bytes (RGB)
  const size_t expectedSize = IMAGE_WIDTH * IMAGE_HEIGHT * 3;
  if (rgbData.size() != expectedSize) {
    Logger.error(MAIN_LOG, "Invalid image data size: %d, expected: %d", rgbData.size(), expectedSize);
    throw std::runtime_error("Invalid image data size");
  }
  
  // Convert to monochrome bitmap (1 bit per pixel)
  std::vector<uint8_t> monoData((IMAGE_WIDTH * IMAGE_HEIGHT) / 8, 0);
  
  for (int y = 0; y < IMAGE_HEIGHT; y++) {
    for (int x = 0; x < IMAGE_WIDTH; x++) {
      int const rgbIndex = (y * IMAGE_WIDTH + x) * 3;
      uint8_t const r = rgbData[rgbIndex];
      uint8_t const g = rgbData[rgbIndex + 1];
      uint8_t const b = rgbData[rgbIndex + 2];
      
      bool const isWhite = rgbToMono(r, g, b);
      
      if (isWhite) {
        // Set the bit for this pixel
        int const bitIndex = y * IMAGE_WIDTH + x;
        int const byteIndex = bitIndex / 8;
        int const bitPosition = bitIndex % 8;
        
        monoData[byteIndex] |= (1 << (7 - bitPosition)); // MSB first
      }
    }
  }
  
  return monoData;
}

auto SignState::decodeBase64(const String& encoded) -> std::vector<uint8_t> {
  // Simple base64 decoder implementation
  const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  
  // Create lookup table
  int decode_table[256];
  for (int i = 0; i < 256; i++) {
    decode_table[i] = -1;
  }
  for (int i = 0; i < 64; i++) {
    decode_table[static_cast<unsigned char>(base64_chars[i])] = i;
  }
  
  size_t const input_len = encoded.length();
  if (input_len % 4 != 0) {
    throw std::runtime_error("Invalid base64 input length");
  }
  
  size_t const output_len = input_len / 4 * 3;
  std::vector<uint8_t> decoded(output_len);
  
  size_t output_pos = 0;
  for (size_t i = 0; i < input_len; i += 4) {
    int a = decode_table[static_cast<unsigned char>(encoded[i])];
    int b = decode_table[static_cast<unsigned char>(encoded[i + 1])];
    int c = decode_table[static_cast<unsigned char>(encoded[i + 2])];
    int d = decode_table[static_cast<unsigned char>(encoded[i + 3])];
    
    if (a == -1 || b == -1) {
      throw std::runtime_error("Invalid base64 character");
    }
    
    decoded[output_pos++] = (a << 2) | (b >> 4);
    
    if (encoded[i + 2] != '=' && c != -1) {
      decoded[output_pos++] = (b << 4) | (c >> 2);
    }
    
    if (encoded[i + 3] != '=' && d != -1) {
      decoded[output_pos++] = (c << 6) | d;
    }
  }
  
  decoded.resize(output_pos);
  return decoded;
}

auto SignState::rgbToMono(uint8_t r, uint8_t g, uint8_t b) -> bool {
  // Convert RGB to grayscale using standard luminance formula
  // Y = 0.299*R + 0.587*G + 0.114*B
  int const grayscale = (299 * r + 587 * g + 114 * b) / 1000;
  
  // Use threshold of 128 (middle value) to determine black/white
  return grayscale > 128;
}
