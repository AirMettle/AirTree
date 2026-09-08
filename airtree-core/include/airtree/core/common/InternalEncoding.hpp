// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_COMMON_INTERNAL_ENCODING_HPP
#define AIRTREE_CORE_COMMON_INTERNAL_ENCODING_HPP

#include <airtree/core/common/TLE.hpp>
#include <airtree/core/common/FPHArray.hpp>
#include <airtree/core/common/Conversion.hpp>
#include <airtree/core/Logger.hpp>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <utility>
#ifdef _MSC_VER
#include <intrin.h>
#endif

using namespace airtree::core;


namespace airtree::core::detail {

template <bool LessOrEqual>
inline uint64_t adjustExponent64(uint64_t exponent, unsigned &placed) {
  const int64_t d = static_cast<int64_t>(exponent) - 1023;
  const int64_t m = (LessOrEqual ? d - 1 : d) >> 63;
  placed = static_cast<unsigned>(m & 1);
  return static_cast<uint64_t>(d ^ m) & 0x7FF;
}

inline uint32_t adjustExponent32(uint32_t exponent, unsigned &placed) {
  const int32_t d = static_cast<int32_t>(exponent) - 127;
  const int32_t m = d >> 31;
  placed = static_cast<unsigned>(m & 1);
  return static_cast<uint32_t>(d ^ m) & 0xFF;
}

inline int firstOne64(uint64_t x) {
#ifdef _MSC_VER
  unsigned long index;
  _BitScanReverse64(&index, x);
  return static_cast<int>(index);
#else
  return 63 - __builtin_clzll(x);
#endif
}

inline int firstOne32(uint32_t x) {
#ifdef _MSC_VER
  unsigned long index;
  _BitScanReverse(&index, x);
  return static_cast<int>(index);
#else
  return 31 - __builtin_clz(x);
#endif
}

} // namespace airtree::core::detail

inline unsigned int createInternal8Bit_32(uint32_t fpNumber) {
  unsigned placed_signedExponentBit;
  const uint32_t adjustedExponent =
      detail::adjustExponent32((fpNumber >> 23) & 0xFF, placed_signedExponentBit);
  const uint32_t maskedNumber =
      (fpNumber & 0x007FFFFF) | (adjustedExponent << 23);

  int zeroCount;
  uint32_t precisionBits;
  if (maskedNumber == 0) {
    zeroCount = 7;
    precisionBits = 0;
  } else {
    const int firstOnePos = detail::firstOne32(maskedNumber);
    zeroCount = std::min(29 - firstOnePos, 7);
    precisionBits = (maskedNumber >> std::max(firstOnePos - 5, 18)) & 0x1F;
  }
  return (zeroCount << 5) | precisionBits;
}

inline unsigned int createInternal8Bit(uint64_t fpNumber) {
  unsigned placed_signedExponentBit;
  const uint64_t adjustedExponent =
      detail::adjustExponent64<true>((fpNumber >> 52) & 0x7FF, placed_signedExponentBit);
  const uint64_t maskedNumber =
      (fpNumber & 0x000FFFFFFFFFFFFF) | (adjustedExponent << 52);

  int zeroCount;
  uint64_t precisionBits;
  if (maskedNumber == 0) {
    zeroCount = 7;
    precisionBits = 0;
  } else {
    const int firstOnePos = detail::firstOne64(maskedNumber);
    zeroCount = std::min(std::max(61 - firstOnePos, 0), 7);
    precisionBits = (maskedNumber >> std::max(firstOnePos - 5, 50)) & 0x1F;
  }
  return (zeroCount << 5) | precisionBits;
}

inline unsigned int createInternal10Bit(uint64_t fpNumber) {
  unsigned placed_signedExponentBit;
  const uint64_t adjustedExponent = detail::adjustExponent64<false>(
      (fpNumber >> 52) & 0x7FF, placed_signedExponentBit);
  const uint64_t maskedNumber =
      (fpNumber & 0x000FFFFFFFFFFFFF) | (adjustedExponent << 52);

  int zeroCount;
  uint64_t precisionBits;
  if (maskedNumber == 0) {
    zeroCount = 15;
    precisionBits = 0;
  } else {
    const int firstOnePos = detail::firstOne64(maskedNumber);
    const int rawZeros = std::max(61 - firstOnePos, 0);
    if (rawZeros >= 15) {
      zeroCount = 15;
      precisionBits = (maskedNumber >> 41) & 0x3F;
    } else {
      zeroCount = rawZeros;
      precisionBits = (maskedNumber >> (firstOnePos - 6)) & 0x3F;
    }
  }
  return (zeroCount << 6) | precisionBits;
}

inline unsigned int createInternal10Bit_32(uint32_t fpNumber) {
  unsigned placed_signedExponentBit;
  const uint32_t adjustedExponent =
      detail::adjustExponent32((fpNumber >> 23) & 0xFF, placed_signedExponentBit);
  const uint32_t maskedNumber =
      (fpNumber & 0x007FFFFF) | (adjustedExponent << 23);

  int zeroCount;
  uint32_t precisionBits;
  if (maskedNumber == 0) {
    zeroCount = 15;
    precisionBits = 0;
  } else {
    const int firstOnePos = detail::firstOne32(maskedNumber);
    const int rawZeros = std::max(29 - firstOnePos, 0);
    if (rawZeros >= 15) {
      zeroCount = 15;
      precisionBits = (maskedNumber >> 9) & 0x3F;
    } else {
      zeroCount = rawZeros;
      precisionBits = (maskedNumber >> (firstOnePos - 6)) & 0x3F;
    }
  }
  return (zeroCount << 6) | precisionBits;
}

// 16-bit code for 1DxF: upper 8 bits index level 0, lower 8 bits level 1.
inline unsigned int createInternal16Bit(uint64_t fpNumber) {
  const unsigned placed_sign_bit = (fpNumber >> 63) & 1;
  unsigned placed_signedExponentBit;
  const uint64_t adjustedExponent = detail::adjustExponent64<false>(
      (fpNumber >> 52) & 0x7FF, placed_signedExponentBit);
  const uint64_t maskedNumber =
      (fpNumber & 0x000FFFFFFFFFFFFF) | (adjustedExponent << 52);

  int zeroCount;
  uint64_t precisionBits;
  if (maskedNumber == 0) {
    zeroCount = 15;
    precisionBits = 0;
  } else {
    const int firstOnePos = detail::firstOne64(maskedNumber);
    const int rawZeros = 61 - firstOnePos;
    if (rawZeros >= 15) {
      zeroCount = 15;
      precisionBits = (maskedNumber >> 37) & 0x3FF;
    } else {
      zeroCount = rawZeros;
      precisionBits = (maskedNumber >> (firstOnePos - 10)) & 0x3FF;
    }
  }

  const unsigned int index8 = placed_sign_bit << 7
                              | placed_signedExponentBit << 6 | (zeroCount << 2)
                              | ((precisionBits >> 8) & 0x3);
  const unsigned int index8_level2 = precisionBits & 0xFF;
  return (index8 << 8) | index8_level2;
}

inline unsigned int createInternal16Bit_32(uint32_t fpNumber) {
  const unsigned placed_sign_bit = (fpNumber >> 31) & 1;
  unsigned placed_signedExponentBit;
  const uint32_t adjustedExponent =
      detail::adjustExponent32((fpNumber >> 23) & 0xFF, placed_signedExponentBit);
  const uint32_t maskedNumber =
      (fpNumber & 0x007FFFFF) | (adjustedExponent << 23);

  int zeroCount;
  uint32_t precisionBits;
  if (maskedNumber == 0) {
    zeroCount = 15;
    precisionBits = 0;
  } else {
    const int firstOnePos = detail::firstOne32(maskedNumber);
    const int rawZeros = 29 - firstOnePos;
    if (rawZeros >= 15) {
      zeroCount = 15;
      precisionBits = (maskedNumber >> 5) & 0x3FF;
    } else {
      zeroCount = rawZeros;
      precisionBits = (maskedNumber >> (firstOnePos - 10)) & 0x3FF;
    }
  }

  const unsigned int index8 = placed_sign_bit << 7
                              | placed_signedExponentBit << 6 | (zeroCount << 2)
                              | ((precisionBits >> 8) & 0x3);
  const unsigned int index8_level2 = precisionBits & 0xFF;
  return (index8 << 8) | index8_level2;
}

// 13-bit code for 1DxT: upper 8 bits index level 0, lower 5 bits level 1.
inline unsigned int createInternal13Bit(uint64_t fpNumber) {
  const unsigned placed_sign_bit = (fpNumber >> 63) & 1;
  unsigned placed_signedExponentBit;
  const uint64_t adjustedExponent = detail::adjustExponent64<false>(
      (fpNumber >> 52) & 0x7FF, placed_signedExponentBit);
  const uint64_t maskedNumber =
      (fpNumber & 0x000FFFFFFFFFFFFF) | (adjustedExponent << 52);

  int zeroCount;
  uint64_t precisionBits;
  if (maskedNumber == 0) {
    zeroCount = 15;
    precisionBits = 0;
  } else {
    const int firstOnePos = detail::firstOne64(maskedNumber);
    const int rawZeros = 61 - firstOnePos;
    if (rawZeros >= 15) {
      zeroCount = 15;
      precisionBits = (maskedNumber >> 40) & 0x7F;
    } else {
      zeroCount = rawZeros;
      precisionBits = (maskedNumber >> (firstOnePos - 7)) & 0x7F;
    }
  }

  const unsigned int index8 = placed_sign_bit << 7
                              | placed_signedExponentBit << 6 | (zeroCount << 2)
                              | ((precisionBits >> 5) & 0x3);
  const unsigned int index5 = precisionBits & 0x1F;
  return (index8 << 5) | index5;
}

inline unsigned int createInternal13Bit_32(uint32_t fpNumber) {
  const unsigned placed_sign_bit = (fpNumber >> 31) & 1;
  unsigned placed_signedExponentBit;
  const uint32_t adjustedExponent =
      detail::adjustExponent32((fpNumber >> 23) & 0xFF, placed_signedExponentBit);
  const uint32_t maskedNumber =
      (fpNumber & 0x007FFFFF) | (adjustedExponent << 23);

  int zeroCount;
  uint32_t precisionBits;
  if (maskedNumber == 0) {
    zeroCount = 15;
    precisionBits = 0;
  } else {
    const int firstOnePos = detail::firstOne32(maskedNumber);
    const int rawZeros = 29 - firstOnePos;
    if (rawZeros >= 15) {
      zeroCount = 15;
      precisionBits = (maskedNumber >> 8) & 0x7F;
    } else {
      zeroCount = rawZeros;
      precisionBits = (maskedNumber >> (firstOnePos - 7)) & 0x7F;
    }
  }

  const unsigned int index8 = placed_sign_bit << 7
                              | placed_signedExponentBit << 6 | (zeroCount << 2)
                              | ((precisionBits >> 5) & 0x3);
  const unsigned int index5 = precisionBits & 0x1F;
  return (index8 << 5) | index5;
}

// 20-bit code for 1DxP: bits 12-19 index level 0, 6-11 level 1, 0-5 level 2.
inline unsigned int createInternal20Bit(uint64_t fpNumber) {
  const unsigned placed_sign_bit = (fpNumber >> 63) & 1;
  unsigned placed_signedExponentBit;
  const uint64_t adjustedExponent = detail::adjustExponent64<false>(
      (fpNumber >> 52) & 0x7FF, placed_signedExponentBit);
  const uint64_t maskedNumber =
      (fpNumber & 0x000FFFFFFFFFFFFF) | (adjustedExponent << 52);

  int zeroCount;
  uint64_t precisionBits;
  if (maskedNumber == 0) {
    zeroCount = 15;
    precisionBits = 0;
  } else {
    const int firstOnePos = detail::firstOne64(maskedNumber);
    const int rawZeros = 61 - firstOnePos;
    if (rawZeros >= 15) {
      zeroCount = 15;
      precisionBits = (maskedNumber >> 33) & 0x3FFF;
    } else {
      zeroCount = rawZeros;
      precisionBits = (maskedNumber >> (firstOnePos - 14)) & 0x3FFF;
    }
  }

  const unsigned int index8 = placed_sign_bit << 7
                              | placed_signedExponentBit << 6 | (zeroCount << 2)
                              | ((precisionBits >> 12) & 0x3);
  const unsigned int index6_level1 = (precisionBits >> 6) & 0x3F;
  const unsigned int index6_level2 = precisionBits & 0x3F;
  return (index8 << 12) | (index6_level1 << 6) | index6_level2;
}

inline unsigned int createInternal20Bit_32(uint32_t fpNumber) {
  const unsigned placed_sign_bit = (fpNumber >> 31) & 1;
  unsigned placed_signedExponentBit;
  const uint32_t adjustedExponent =
      detail::adjustExponent32((fpNumber >> 23) & 0xFF, placed_signedExponentBit);
  const uint32_t maskedNumber =
      (fpNumber & 0x007FFFFF) | (adjustedExponent << 23);

  int zeroCount;
  uint32_t precisionBits;
  if (maskedNumber == 0) {
    zeroCount = 15;
    precisionBits = 0;
  } else {
    const int firstOnePos = detail::firstOne32(maskedNumber);
    const int rawZeros = 29 - firstOnePos;
    if (rawZeros >= 15) {
      zeroCount = 15;
      precisionBits = (maskedNumber >> 1) & 0x3FFF;
    } else {
      zeroCount = rawZeros;
      precisionBits = (maskedNumber >> (firstOnePos - 14)) & 0x3FFF;
    }
  }

  const unsigned int index8 = placed_sign_bit << 7
                              | placed_signedExponentBit << 6 | (zeroCount << 2)
                              | ((precisionBits >> 12) & 0x3);
  const unsigned int index6_level1 = (precisionBits >> 6) & 0x3F;
  const unsigned int index6_level2 = precisionBits & 0x3F;
  return (index8 << 12) | (index6_level1 << 6) | index6_level2;
}


// internal_8bit function files the input value into the internal 8-bit
// representation and returns the TLE and the internal 8-bit representation
template <typename T>
inline std::pair<TLE, unsigned int> internal_8bit(T value) {

  double doubleValue;
  if constexpr (std::is_same_v<T, double>) {
    doubleValue = value;
  } else if constexpr (std::is_same_v<T, float>) {
    doubleValue = static_cast<double>(value);
  } else if constexpr (std::is_same_v<T, int32_t>) {
    doubleValue = int32_to_double(value);
  } else if constexpr (std::is_same_v<T, int64_t>) {
    doubleValue = int64_to_double(value);
  } else {
    static_assert(sizeof(T) == 0, "Unsupported type for internal_8bit");
  }

  uint64_t fpNumber;
  std::memcpy(&fpNumber, &doubleValue, sizeof(doubleValue));

  TLE tle = setTLEComponents(fpNumber);
  unsigned int result = createInternal8Bit(fpNumber);
  return std::make_pair(tle, result);
}

// internal_10bit function files the input value into the internal 10-bit
// representation and returns the TLE and the internal 10-bit representation
template <typename T>
inline std::pair<TLE, unsigned int> internal_10bit(T value) {

  double doubleValue;
  if constexpr (std::is_same_v<T, double>) {
    doubleValue = value;
  } else if constexpr (std::is_same_v<T, float>) {
    doubleValue = static_cast<double>(value);
  } else if constexpr (std::is_same_v<T, int32_t>) {
    doubleValue = int32_to_double(value);
  } else if constexpr (std::is_same_v<T, int64_t>) {
    doubleValue = int64_to_double(value);
  } else {
    static_assert(sizeof(T) == 0, "Unsupported type for internal_10bit");
  }

  uint64_t fpNumber;
  std::memcpy(&fpNumber, &doubleValue, sizeof(doubleValue));

  TLE tle = setTLEComponents(fpNumber);
  unsigned int result = createInternal10Bit(fpNumber);
  return std::make_pair(tle, result);
}

#endif // AIRTREE_CORE_COMMON_INTERNAL_ENCODING_HPP