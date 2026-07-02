// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <algorithm>
#include <airtree/core/serdes/Count.hpp>

#include <airtree/core/Logger.hpp>


using namespace airtree::core;


// Helper functions to compute minimum Bits used for count
// serialization/deserialization
int minimumBits(uint32_t maxNumber) {
  if (maxNumber == 0)
    return 1;

  int bits = 0;
  while (maxNumber > 0) {
    maxNumber >>= 1;
    bits++;
  }
  return bits;
}

std::vector<char> serializeCounts(const uint32_t counts[], size_t len) {
  std::vector<char> buffer;
  uint32_t maxCount = *std::max_element(counts, counts + len);
  int minBits = minimumBits(maxCount);
  buffer.push_back(
      static_cast<char>(minBits)); // Store minBits as a single byte

  uint64_t bitBuffer = 0; // Temporary buffer for bits
  int bitCount = 0;       // Counter for bits in bitBuffer

  for (size_t i = 0; i < len; i++) {
    if (counts[i] > 0) {
      bitBuffer |= static_cast<uint64_t>(counts[i])
                   << bitCount; // Shift count left by bitCount into bitBuffer
      bitCount += minBits;      // Increment bitCount

      while (bitCount >= 8) { // While bitBuffer has at least one byte
        buffer.push_back(
            static_cast<char>(bitBuffer & 0xFF)); // Push low 8 bits to buffer
        bitBuffer >>= 8;                          // Remove the stored bits
        bitCount -= 8;                            // Decrement bit count by 8
      }
    }
  }

  if (bitCount > 0) { // Handle remaining bits
    buffer.push_back(static_cast<char>(bitBuffer & 0xFF));
  }

  return buffer;
}

std::vector<uint32_t> deserializeCounts(const std::vector<char> &buffer,
                                        size_t &offset, size_t len) {
  if (offset >= buffer.size()) {
    SPDLOG_LOGGER_ERROR(
        logger(),
        "Attempt to read beyond buffer size at initial offset check.");
    return {};
  }

  int minBits = static_cast<unsigned char>(
      buffer[offset++]); // Read the number of bits used per count

  std::vector<uint32_t> counts(len, 0);
  uint64_t bitBuffer = 0; // Buffer to accumulate bits
  int bitsInBuffer = 0;   // Number of valid bits currently in bitBuffer

  for (size_t i = 0; i < len; i++) {
    uint32_t currentCount = 0;

    // Ensure we have enough bits in bitBuffer
    while (bitsInBuffer < minBits) {
      if (offset >= buffer.size()) {
        SPDLOG_LOGGER_ERROR(
            logger(),
            "Buffer underflow while trying to read new byte at index {}", i);
        return {};
      }
      bitBuffer |= (static_cast<uint64_t>(
                        static_cast<unsigned char>(buffer[offset++]))
                    << bitsInBuffer); // Shift new byte into bitBuffer
      bitsInBuffer += 8;              // We've added 8 new bits to the buffer
    }

    // Extract the count from bitBuffer
    currentCount = static_cast<uint32_t>(
        bitBuffer
        & ((static_cast<uint64_t>(1) << minBits) - 1)); // Mask needed bits
    bitBuffer >>= minBits; // Remove the bits we've just processed
    bitsInBuffer -=
        minBits; // Update the count of valid bits remaining in bitBuffer

    counts[i] = currentCount;
  }

  return counts;
}