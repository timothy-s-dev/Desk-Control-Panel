#include "sign_state.h"
#include <Arduino.h>
#include <Elog.h>
#include <logging.h>
#include <mbedtls/base64.h>

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
  // Decode base64 to get BMP data
  std::vector<uint8_t> bmpData = decodeBase64(base64Data);
  
  Logger.debug(MAIN_LOG, "Decoded BMP data size: %d bytes", bmpData.size());
  
  // Parse BMP to extract RGB data
  std::vector<uint8_t> rgbData = parseBMP(bmpData);
  
  // Expected size: 32 * 8 * 3 = 768 bytes (RGB)
  const size_t expectedSize = IMAGE_WIDTH * IMAGE_HEIGHT * 3;
  if (rgbData.size() != expectedSize) {
    Logger.error(MAIN_LOG, "Invalid RGB data size: %d, expected: %d", rgbData.size(), expectedSize);
    throw std::runtime_error("Invalid RGB data size");
  }
  
  // Convert to monochrome bitmap (1 bit per pixel)
  std::vector<uint8_t> monoData((IMAGE_WIDTH * IMAGE_HEIGHT) / 8, 0);
  
  for (int y = 0; y < IMAGE_HEIGHT; y++) {
    for (int x = 0; x < IMAGE_WIDTH; x++) {
      int const rgbIndex = (y * IMAGE_WIDTH + x) * 3;
      uint8_t const r = rgbData[rgbIndex];
      uint8_t const g = rgbData[rgbIndex + 1];
      uint8_t const b = rgbData[rgbIndex + 2];
      
      bool const isWhite = r + g + b > 5;
      
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
  // Calculate decoded size
  size_t const encodedLen = encoded.length();
  size_t const maxDecodedLen = (encodedLen * 3) / 4 + 4; // Add some padding
  
  std::vector<uint8_t> decoded(maxDecodedLen);
  
  // Use mbedtls base64 decode function
  size_t actualLength = 0;
  int const result = mbedtls_base64_decode(decoded.data(), maxDecodedLen, &actualLength,
                                          reinterpret_cast<const unsigned char*>(encoded.c_str()), encodedLen);
  
  if (result != 0) {
    Logger.error(MAIN_LOG, "Base64 decode failed with error: %d", result);
    throw std::runtime_error("Base64 decode failed");
  }
  
  decoded.resize(actualLength);
  Logger.debug(MAIN_LOG, "Base64 decoded %d bytes from %d encoded bytes", actualLength, encodedLen);
  
  return decoded;
}

auto SignState::parseBMP(const std::vector<uint8_t>& bmpData) -> std::vector<uint8_t> {
  if (bmpData.size() < 54) { // Minimum BMP header size
    throw std::runtime_error("Invalid BMP file: too small");
  }
  
  // Check BMP signature
  if (bmpData[0] != 'B' || bmpData[1] != 'M') {
    throw std::runtime_error("Invalid BMP file: wrong signature");
  }
  
  // Read BMP header fields (little-endian)
  uint32_t const dataOffset = bmpData[10] | (bmpData[11] << 8) | (bmpData[12] << 16) | (bmpData[13] << 24);
  uint32_t const width = bmpData[18] | (bmpData[19] << 8) | (bmpData[20] << 16) | (bmpData[21] << 24);
  uint32_t const height = bmpData[22] | (bmpData[23] << 8) | (bmpData[24] << 16) | (bmpData[25] << 24);
  uint16_t const bitsPerPixel = bmpData[28] | (bmpData[29] << 8);
  
  Logger.debug(MAIN_LOG, "BMP info: %dx%d, %d bpp, data offset: %d", width, height, bitsPerPixel, dataOffset);
  
  // Validate dimensions
  if (width != IMAGE_WIDTH || height != IMAGE_HEIGHT) {
    Logger.error(MAIN_LOG, "Invalid BMP dimensions: %dx%d, expected: %dx%d", width, height, IMAGE_WIDTH, IMAGE_HEIGHT);
    throw std::runtime_error("Invalid BMP dimensions");
  }
  
  // Only support 24-bit BMPs for now
  if (bitsPerPixel != 24) {
    Logger.error(MAIN_LOG, "Unsupported BMP format: %d bpp, expected: 24", bitsPerPixel);
    throw std::runtime_error("Unsupported BMP format");
  }
  
  if (dataOffset >= bmpData.size()) {
    throw std::runtime_error("Invalid BMP file: data offset beyond file size");
  }
  
  // Calculate row padding (BMP rows are padded to 4-byte boundaries)
  int const bytesPerRow = ((width * 3 + 3) / 4) * 4;
  
  std::vector<uint8_t> rgbData(width * height * 3);
  
  // Extract pixel data (BMP is stored bottom-to-top, BGR format)
  for (uint32_t y = 0; y < height; y++) {
    for (uint32_t x = 0; x < width; x++) {
      // BMP is bottom-to-top, so flip Y coordinate
      uint32_t const bmpY = height - 1 - y;
      uint32_t const bmpOffset = dataOffset + bmpY * bytesPerRow + x * 3;
      
      if (bmpOffset + 2 >= bmpData.size()) {
        throw std::runtime_error("Invalid BMP file: pixel data beyond file size");
      }
      
      // BMP uses BGR format, convert to RGB
      uint32_t const rgbIndex = (y * width + x) * 3;
      rgbData[rgbIndex] = bmpData[bmpOffset + 2];     // R
      rgbData[rgbIndex + 1] = bmpData[bmpOffset + 1]; // G
      rgbData[rgbIndex + 2] = bmpData[bmpOffset];     // B
    }
  }
  
  return rgbData;
}
