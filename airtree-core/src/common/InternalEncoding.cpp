#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/common/Conversion.hpp>
#include <airtree/core/Logger.hpp>

#include <algorithm>
#include <cstring>


using namespace airtree::core;

unsigned int createInternal8Bit_32(uint32_t fpNumber, bool default_mode) {
  uint32_t exponent = (fpNumber >> 23) & 0xFF;
  bool placed_signedExponentBit = (exponent < 127);

  uint32_t adjustedExponent =
      placed_signedExponentBit ? (127 - exponent) : (exponent - 127);
  if (default_mode && placed_signedExponentBit) {
    adjustedExponent = ((adjustedExponent - 1) & 0xFF);
  }

  uint32_t maskedNumber = (fpNumber & 0x007FFFFF) | (adjustedExponent << 23);

  int zeroCount;
  uint64_t precisionBits;
  uint32_t firstOnePos;

  // Handle saturation case: when maskedNumber is 0, BSR has undefined behavior
  if (maskedNumber == 0) {
    zeroCount = 7; // Max for 8-bit representation
    precisionBits = 0;
    firstOnePos = 0;
  } else {
#ifdef x86_ARCH
    __asm__("bsrl %1, %0" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
#elif ARM_ARCH
    __asm__("clz %0, %1" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
    firstOnePos = 31 - firstOnePos;
#endif

    int rawZeros = 31 - static_cast<int>(firstOnePos) - 2;
    zeroCount = std::min(rawZeros, 7);

    if (rawZeros >= 7) {
      precisionBits = ((maskedNumber >> 18) & 0x1F);
    } else {
      precisionBits = ((maskedNumber >> (firstOnePos - 5)) & 0x1F);
    }
  }

  return (zeroCount << 5) | precisionBits;
}

unsigned int createInternal8Bit(uint64_t fpNumber, bool default_mode) {
  uint64_t exponent = (fpNumber >> 52) & 0x7FF;
  bool placed_signedExponentBit = ((exponent <= 1023) ? 1 : 0);

  uint64_t adjustedExponent =
      placed_signedExponentBit ? (1023 - exponent) : (exponent - 1023);
  if (default_mode && placed_signedExponentBit) {
    adjustedExponent = ((adjustedExponent - 1) & 0x7FF);
  }

  // Update fpNumber with the adjusted exponent
  uint64_t maskedNumber =
      (fpNumber & 0x000FFFFFFFFFFFFF) | (adjustedExponent << 52);

  int zeroCount;
  uint64_t precisionBits;
  uint64_t firstOnePos;

  // Handle saturation case: when maskedNumber is 0, BSR has undefined behavior
  if (maskedNumber == 0) {
    zeroCount = 7; // Max for 8-bit representation
    precisionBits = 0;
    firstOnePos = 0; // Not used, but set for consistency
  } else {
#ifdef x86_ARCH
    __asm__("bsrq %1, %0" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
#elif ARM_ARCH
    __asm__("clz %0, %1" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
    firstOnePos = 63 - firstOnePos;
#endif
    int rawZeros = std::max(63 - static_cast<int>(firstOnePos) - 2, 0);
    zeroCount = std::min(rawZeros, 7);

    if (rawZeros >= 7) {
      // Fixed BSR for rawZeros=6: 61-6=55, shift=55-5=50
      precisionBits = ((maskedNumber >> 50) & 0x1F);
    } else {
      precisionBits = ((maskedNumber >> (firstOnePos - 5)) & 0x1F);
    }
  }

  return (zeroCount << 5) | precisionBits;
}

unsigned int createInternal10Bit(uint64_t fpNumber, bool default_mode) {

  uint64_t exponent = (fpNumber >> 52) & 0x7FF;
  bool placed_signedExponentBit = ((exponent < 1023) ? 1 : 0);

  uint64_t adjustedExponent =
      placed_signedExponentBit ? (1023 - exponent) : (exponent - 1023);
  if (default_mode && placed_signedExponentBit) {
    adjustedExponent = ((adjustedExponent - 1) & 0x7FF);
  }

  // Update fpNumber with the adjusted exponent
  uint64_t maskedNumber =
      (fpNumber & 0x000FFFFFFFFFFFFF) | (adjustedExponent << 52);

  int zeroCount;
  uint64_t precisionBits;
  uint64_t firstOnePos;

  // Handle saturation case: when maskedNumber is 0, BSR has undefined behavior
  if (maskedNumber == 0) {
    zeroCount = 15; // Max for 10-bit representation
    precisionBits = 0;
    firstOnePos = 0; // Not used, but set for consistency
  } else {
#ifdef x86_ARCH
    __asm__("bsrq %1, %0" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
#elif ARM_ARCH
    __asm__("clz %0, %1" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
    firstOnePos = 63 - firstOnePos;
#endif
    int rawZeros = std::max(63 - static_cast<int>(firstOnePos) - 2, 0);
    zeroCount = std::min(rawZeros, 15);

    if (rawZeros >= 15) {
      precisionBits = ((maskedNumber >> 41) & 0x3F);
    } else {
      precisionBits = ((maskedNumber >> (firstOnePos - 6)) & 0x3F);
    }
  }

  return (zeroCount << 6) | precisionBits;
}

unsigned int createInternal10Bit_32(uint32_t fpNumber, bool default_mode) {

  uint32_t exponent = (fpNumber >> 23) & 0xFF;
  bool placed_signedExponentBit = (exponent < 127);

  uint32_t adjustedExponent =
      placed_signedExponentBit ? (127 - exponent) : (exponent - 127);
  if (default_mode && placed_signedExponentBit) {
    adjustedExponent = ((adjustedExponent - 1) & 0xFF);
  }

  uint32_t maskedNumber = (fpNumber & 0x007FFFFF) | (adjustedExponent << 23);

  int zeroCount;
  uint32_t precisionBits;
  uint32_t firstOnePos;

  // Handle saturation case: when maskedNumber is 0, BSR has undefined behavior
  if (maskedNumber == 0) {
    zeroCount = 15; // Max for 10-bit representation
    precisionBits = 0;
    firstOnePos = 0; // Not used, but set for consistency
  } else {
#ifdef x86_ARCH
    __asm__("bsrl %1, %0" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
#elif ARM_ARCH
    __asm__("clz %0, %1" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
    firstOnePos = 31 - firstOnePos;
#endif

    int rawZeros = std::max(31 - static_cast<int>(firstOnePos) - 2, 0);
    zeroCount = std::min(rawZeros, 15);

    if (rawZeros >= 15) {
      // Fixed BSR for rawZeros=14: 29-14=15, shift=15-6=9
      precisionBits = ((maskedNumber >> 9) & 0x3F);
    } else {
      precisionBits = ((maskedNumber >> (firstOnePos - 6)) & 0x3F);
    }
  }

  return (zeroCount << 6) | precisionBits;
}


// createInternal16Bit function converts a 64-bit floating-point number to
// 16-bit internal representation for Apollo16 (1DxF) configuration
// Returns a 16-bit value: upper 8 bits (level 0 index) + lower 8 bits (level 1
// index)
unsigned int createInternal16Bit(uint64_t fpNumber, bool default_mode) {
  // Extract sign bit
  bool placed_sign_bit = ((fpNumber >> 63) & 1);

  // Extract and adjust exponent
  uint64_t exponent = (fpNumber >> 52) & 0x7FF;
  bool placed_signedExponentBit = ((exponent < 1023) ? 1 : 0);

  uint64_t adjustedExponent =
      placed_signedExponentBit ? (1023 - exponent) : (exponent - 1023);
  if (default_mode && placed_signedExponentBit) {
    adjustedExponent = ((adjustedExponent - 1) & 0x7FF);
  }

  // Update fpNumber with the adjusted exponent
  uint64_t maskedNumber =
      (fpNumber & 0x000FFFFFFFFFFFFF) | (adjustedExponent << 52);

  int zeroCount;
  uint64_t precisionBits;
  uint64_t firstOnePos;

  // Handle saturation case: when maskedNumber is 0, BSR has undefined behavior
  if (maskedNumber == 0) {
    zeroCount = 15; // Max for 16-bit representation
    precisionBits = 0;
    firstOnePos = 0; // Not used, but set for consistency
  } else {
#ifdef x86_ARCH
    __asm__("bsrq %1, %0" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
#elif ARM_ARCH
    __asm__("clz %0, %1" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
    firstOnePos = 63 - firstOnePos;
#endif

    int rawZeros = 63 - static_cast<int>(firstOnePos) - 2;
    zeroCount = std::min(rawZeros, 15);

    if (rawZeros >= 15) {
      // Fixed BSR for rawZeros=14: 61-14=47, shift=47-10=37
      precisionBits = ((maskedNumber >> 37) & 0x3FF);
    } else {
      precisionBits = ((maskedNumber >> (firstOnePos - 10)) & 0x3FF);
    }
  }

  // Calculate trie indices
  // index8: sign(1) + signedExp(1) + zeroCount(4) + topPrecision(2) = 8 bits
  unsigned int index8 = placed_sign_bit << 7 | placed_signedExponentBit << 6
                        | (zeroCount << 2) | ((precisionBits >> 8) & 0x3);
  // index8_level2: bottomPrecision(8) = 8 bits
  unsigned int index8_level2 = precisionBits & 0xFF;

  // Pack both indices into a single 16-bit value: upper 8 bits + lower 8 bits
  return (index8 << 8) | index8_level2;
}

// createInternal16Bit_32 function converts a 32-bit floating-point number to
// 16-bit internal representation for Apollo16 (1DxF) configuration
// Returns a 16-bit value: upper 8 bits (level 0 index) + lower 8 bits (level 1
// index)
unsigned int createInternal16Bit_32(uint32_t fpNumber, bool default_mode) {
  // Extract sign bit
  bool placed_sign_bit = ((fpNumber >> 31) & 1);

  // Extract and adjust exponent
  uint32_t exponent = (fpNumber >> 23) & 0xFF;
  bool placed_signedExponentBit = (exponent < 127);

  uint32_t adjustedExponent =
      placed_signedExponentBit ? (127 - exponent) : (exponent - 127);
  if (default_mode && placed_signedExponentBit) {
    adjustedExponent = ((adjustedExponent - 1) & 0xFF);
  }

  uint32_t maskedNumber = (fpNumber & 0x007FFFFF) | (adjustedExponent << 23);

  int zeroCount;
  uint32_t precisionBits;
  uint32_t firstOnePos;

  // Handle saturation case: when maskedNumber is 0, BSR has undefined behavior
  if (maskedNumber == 0) {
    zeroCount = 15; // Max for 16-bit representation
    precisionBits = 0;
    firstOnePos = 0; // Not used, but set for consistency
  } else {
#ifdef x86_ARCH
    __asm__("bsrl %1, %0" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
#elif ARM_ARCH
    __asm__("clz %w0, %w1" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
    firstOnePos = 31 - firstOnePos;
#endif

    int rawZeros = 31 - static_cast<int>(firstOnePos) - 2;
    zeroCount = std::min(rawZeros, 15);

    if (rawZeros >= 15) {
      // Fixed BSR for rawZeros=14: 29-14=15, shift=15-10=5
      precisionBits = ((maskedNumber >> 5) & 0x3FF);
    } else {
      precisionBits = ((maskedNumber >> (firstOnePos - 10)) & 0x3FF);
    }
  }

  // Calculate trie indices
  // index8: sign(1) + signedExp(1) + zeroCount(4) + topPrecision(2) = 8 bits
  unsigned int index8 = placed_sign_bit << 7 | placed_signedExponentBit << 6
                        | (zeroCount << 2) | ((precisionBits >> 8) & 0x3);
  // index8_level2: bottomPrecision(8) = 8 bits
  unsigned int index8_level2 = precisionBits & 0xFF;

  // Pack both indices into a single 16-bit value: upper 8 bits + lower 8 bits
  return (index8 << 8) | index8_level2;
}

// createInternal13Bit function converts a 64-bit floating-point number to
// 13-bit internal representation for 13Colonies (1DxT) configuration
// Returns a 13-bit value: upper 8 bits (level 0 index) + lower 5 bits (level 1
// index)
unsigned int createInternal13Bit(uint64_t fpNumber, bool default_mode) {
  // Extract sign bit
  bool placed_sign_bit = ((fpNumber >> 63) & 1);

  // Extract and adjust exponent
  uint64_t exponent = (fpNumber >> 52) & 0x7FF;
  bool placed_signedExponentBit = ((exponent < 1023) ? 1 : 0);

  uint64_t adjustedExponent =
      placed_signedExponentBit ? (1023 - exponent) : (exponent - 1023);
  if (default_mode && placed_signedExponentBit) {
    adjustedExponent = ((adjustedExponent - 1) & 0x7FF);
  }

  // Update fpNumber with the adjusted exponent
  uint64_t maskedNumber =
      (fpNumber & 0x000FFFFFFFFFFFFF) | (adjustedExponent << 52);

  int zeroCount;
  uint64_t precisionBits;
  uint64_t firstOnePos;

  // Handle saturation case: when maskedNumber is 0, BSR has undefined behavior
  if (maskedNumber == 0) {
    // Saturation: all zeros, set zeroCount to max and precisionBits to 0
    zeroCount = 15;
    precisionBits = 0;
    firstOnePos = 0; // Not used, but set for logging
  } else {
    // Normal case: calculate the position of the first significant bit
#ifdef x86_ARCH
    __asm__("bsrq %1, %0" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
#elif ARM_ARCH
    __asm__("clz %0, %1" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
    firstOnePos = 63 - firstOnePos;
#endif

    int rawZeros = 63 - static_cast<int>(firstOnePos) - 2;
    zeroCount = std::min(rawZeros, 15);

    if (rawZeros >= 15) {
      // Fixed BSR for rawZeros=14: 61-14=47, shift=47-7=40
      precisionBits = ((maskedNumber >> 40) & 0x7F);
    } else {
      precisionBits = ((maskedNumber >> (firstOnePos - 7)) & 0x7F);
    }
  }

  // Calculate trie indices
  // index8: sign(1) + signedExp(1) + zeroCount(4) + topPrecision(2) = 8 bits
  unsigned int index8 = placed_sign_bit << 7 | placed_signedExponentBit << 6
                        | (zeroCount << 2) | ((precisionBits >> 5) & 0x3);
  // index5: bottomPrecision(5) = 5 bits
  unsigned int index5 = precisionBits & 0x1F;

  // Pack both indices into a single 13-bit value: upper 8 bits + lower 5 bits
  unsigned int result = (index8 << 5) | index5;

  return result;
}

// createInternal13Bit_32 function converts a 32-bit floating-point number to
// 13-bit internal representation for 13Colonies (1DxT) configuration
// Returns a 13-bit value: upper 8 bits (level 0 index) + lower 5 bits (level 1
// index)
unsigned int createInternal13Bit_32(uint32_t fpNumber, bool default_mode) {
  // Extract sign bit
  bool placed_sign_bit = ((fpNumber >> 31) & 1);

  // Extract and adjust exponent
  uint32_t exponent = (fpNumber >> 23) & 0xFF;
  bool placed_signedExponentBit = (exponent < 127);

  uint32_t adjustedExponent =
      placed_signedExponentBit ? (127 - exponent) : (exponent - 127);
  if (default_mode && placed_signedExponentBit) {
    adjustedExponent = ((adjustedExponent - 1) & 0xFF);
  }

  uint32_t maskedNumber = (fpNumber & 0x007FFFFF) | (adjustedExponent << 23);

  int zeroCount;
  uint32_t precisionBits;
  uint32_t firstOnePos;

  // Handle saturation case: when maskedNumber is 0, BSR has undefined behavior
  if (maskedNumber == 0) {
    zeroCount = 15; // Max for 13-bit representation
    precisionBits = 0;
    firstOnePos = 0; // Not used, but set for consistency
  } else {
#ifdef x86_ARCH
    __asm__("bsrl %1, %0" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
#elif ARM_ARCH
    __asm__("clz %w0, %w1" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
    firstOnePos = 31 - firstOnePos;
#endif

    int rawZeros = 31 - static_cast<int>(firstOnePos) - 2;
    zeroCount = std::min(rawZeros, 15);

    if (rawZeros >= 15) {
      // Fixed BSR for rawZeros=14: 29-14=15, shift=15-7=8
      precisionBits = ((maskedNumber >> 8) & 0x7F);
    } else {
      precisionBits = ((maskedNumber >> (firstOnePos - 7)) & 0x7F);
    }
  }

  // Calculate trie indices
  // index8: sign(1) + signedExp(1) + zeroCount(4) + topPrecision(2) = 8 bits
  unsigned int index8 = placed_sign_bit << 7 | placed_signedExponentBit << 6
                        | (zeroCount << 2) | ((precisionBits >> 5) & 0x3);
  // index5: bottomPrecision(5) = 5 bits
  unsigned int index5 = precisionBits & 0x1F;

  // Pack both indices into a single 13-bit value: upper 8 bits + lower 5 bits
  return (index8 << 5) | index5;
}

// createInternal20Bit function converts a 64-bit floating-point number to
// 20-bit internal representation for Roaring20 (1DxP) configuration
// Returns a 20-bit value: bits 12-19 (level 0), bits 6-11 (level 1), bits 0-5
// (level 2)
unsigned int createInternal20Bit(uint64_t fpNumber, bool default_mode) {
  // Extract sign bit
  bool placed_sign_bit = ((fpNumber >> 63) & 1);

  // Extract and adjust exponent
  uint64_t exponent = (fpNumber >> 52) & 0x7FF;
  bool placed_signedExponentBit = (exponent < 1023);

  uint64_t adjustedExponent =
      placed_signedExponentBit ? (1023 - exponent) : (exponent - 1023);
  if (default_mode && placed_signedExponentBit) {
    adjustedExponent = ((adjustedExponent - 1) & 0x7FF);
  }

  // Update fpNumber with the adjusted exponent
  uint64_t maskedNumber =
      (fpNumber & 0x000FFFFFFFFFFFFF) | (adjustedExponent << 52);

  int zeroCount;
  uint64_t precisionBits;
  uint64_t firstOnePos;

  // Handle saturation case: when maskedNumber is 0, BSR has undefined behavior
  if (maskedNumber == 0) {
    zeroCount = 15; // Max for 20-bit representation
    precisionBits = 0;
    firstOnePos = 0; // Not used, but set for consistency
  } else {
#ifdef x86_ARCH
    __asm__("bsrq %1, %0" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
#elif ARM_ARCH
    __asm__("clz %0, %1" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
    firstOnePos = 63 - firstOnePos;
#endif

    int rawZeros = 63 - static_cast<int>(firstOnePos) - 2;
    zeroCount = std::min(rawZeros, 15);

    if (rawZeros >= 15) {
      // when M is maxed, P starts at fixed position (2^M from MSB).
      // Same extraction point as rawZeros=14 (BSR=47), just without implicit
      // "1". We lose 1 precision bit but only in this case.
      precisionBits = ((maskedNumber >> 33) & 0x3FFF);
    } else {
      precisionBits = ((maskedNumber >> (firstOnePos - 14)) & 0x3FFF);
    }
  }

  // Calculate trie indices
  // index8: sign(1) + signedExp(1) + zeroCount(4) + topPrecision(2) = 8 bits
  unsigned int index8 = placed_sign_bit << 7 | placed_signedExponentBit << 6
                        | (zeroCount << 2) | ((precisionBits >> 12) & 0x3);
  // index6_level1: middlePrecision(6) = 6 bits
  unsigned int index6_level1 = (precisionBits >> 6) & 0x3F;
  // index6_level2: bottomPrecision(6) = 6 bits
  unsigned int index6_level2 = precisionBits & 0x3F;

  // Pack all three indices into a single 20-bit value: 8 bits + 6 bits + 6 bits
  return (index8 << 12) | (index6_level1 << 6) | index6_level2;
}

// createInternal20Bit_32 function converts a 32-bit floating-point number to
// 20-bit internal representation for Roaring20 (1DxP) configuration
// Returns a 20-bit value: bits 12-19 (level 0), bits 6-11 (level 1), bits 0-5
// (level 2)
unsigned int createInternal20Bit_32(uint32_t fpNumber, bool default_mode) {
  // Extract sign bit
  bool placed_sign_bit = ((fpNumber >> 31) & 1);

  // Extract and adjust exponent
  uint32_t exponent = (fpNumber >> 23) & 0xFF;
  bool placed_signedExponentBit = (exponent < 127);

  uint32_t adjustedExponent =
      placed_signedExponentBit ? (127 - exponent) : (exponent - 127);
  if (default_mode && placed_signedExponentBit) {
    adjustedExponent = ((adjustedExponent - 1) & 0xFF);
  }

  uint32_t maskedNumber = (fpNumber & 0x007FFFFF) | (adjustedExponent << 23);

  int zeroCount;
  uint32_t precisionBits;
  uint32_t firstOnePos;

  // Handle saturation case: when maskedNumber is 0, BSR has undefined behavior
  if (maskedNumber == 0) {
    zeroCount = 15; // Max for 20-bit representation
    precisionBits = 0;
    firstOnePos = 0; // Not used, but set for consistency
  } else {
#ifdef x86_ARCH
    __asm__("bsrl %1, %0" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
#elif ARM_ARCH
    __asm__("clz %w0, %w1" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
    firstOnePos = 31 - firstOnePos;
#endif

    int rawZeros = 31 - static_cast<int>(firstOnePos) - 2;
    zeroCount = std::min(rawZeros, 15);

    if (rawZeros >= 15) {
      // when M is maxed, P starts at fixed position (2^M from MSB).
      // For 32-bit: same extraction point as rawZeros=14 (BSR=15), just without
      // implicit "1". Also fixes negative shift UB when firstOnePos < 14.
      precisionBits = ((maskedNumber >> 1) & 0x3FFF);
    } else {
      precisionBits = ((maskedNumber >> (firstOnePos - 14)) & 0x3FFF);
    }
  }

  // Calculate trie indices
  // index8: sign(1) + signedExp(1) + zeroCount(4) + topPrecision(2) = 8 bits
  unsigned int index8 = placed_sign_bit << 7 | placed_signedExponentBit << 6
                        | (zeroCount << 2) | ((precisionBits >> 12) & 0x3);
  // index6_level1: middlePrecision(6) = 6 bits
  unsigned int index6_level1 = (precisionBits >> 6) & 0x3F;
  // index6_level2: bottomPrecision(6) = 6 bits
  unsigned int index6_level2 = precisionBits & 0x3F;

  // Pack all three indices into a single 20-bit value: 8 bits + 6 bits + 6 bits
  return (index8 << 12) | (index6_level1 << 6) | index6_level2;
}