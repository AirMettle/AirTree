#ifndef AIRTREE_CORE_COMMON_INTERNAL_ENCODING_HPP
#define AIRTREE_CORE_COMMON_INTERNAL_ENCODING_HPP

#include <airtree/core/common/TLE.hpp>
#include <airtree/core/common/FPHArray.hpp>
#include <airtree/core/common/Conversion.hpp>
#include <airtree/core/Logger.hpp>
#include <cstdint>
#include <cstring>
#include <utility>

using namespace airtree::core;


unsigned int createInternal8Bit(uint64_t fpNumber, bool default_mode);
unsigned int createInternal8Bit_32(uint32_t fpNumber, bool default_mode);
unsigned int createInternal10Bit(uint64_t fpNumber, bool default_mode);
unsigned int createInternal10Bit_32(uint32_t fpNumber, bool default_mode);
unsigned int createInternal13Bit(uint64_t fpNumber, bool default_mode);
unsigned int createInternal13Bit_32(uint32_t fpNumber, bool default_mode);
unsigned int createInternal16Bit(uint64_t fpNumber, bool default_mode);
unsigned int createInternal16Bit_32(uint32_t fpNumber, bool default_mode);
unsigned int createInternal20Bit(uint64_t fpNumber, bool default_mode);
unsigned int createInternal20Bit_32(uint32_t fpNumber, bool default_mode);


// internal_8bit function files the input value into the internal 8-bit
// representation and returns the TLE and the internal 8-bit representation
template <typename T>
inline std::pair<TLE, unsigned int> internal_8bit(T value,
                                                  const bool &default_mode) {

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
  unsigned int result = createInternal8Bit(fpNumber, default_mode);
  return std::make_pair(tle, result);
}

// internal_10bit function files the input value into the internal 10-bit
// representation and returns the TLE and the internal 10-bit representation
template <typename T>
inline std::pair<TLE, unsigned int> internal_10bit(T value,
                                                   const bool &default_mode) {

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
  unsigned int result = createInternal10Bit(fpNumber, default_mode);
  return std::make_pair(tle, result);
}

#endif // AIRTREE_CORE_COMMON_INTERNAL_ENCODING_HPP