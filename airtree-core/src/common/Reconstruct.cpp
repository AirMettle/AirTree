// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/common/Reconstruct.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <type_traits>

namespace {

// Inverse of the S|E|M|P encoding in InternalEncoding.cpp
template <typename T> struct IEEELayout;
template <> struct IEEELayout<double> {
  using bits_t = uint64_t;
  static constexpr int exp_bits = 11;
  static constexpr int mant_bits = 52;
  static constexpr int bias = 1023;
  static constexpr int max_exp = 2046; // 2047 is NaN/Inf
};
template <> struct IEEELayout<float> {
  using bits_t = uint32_t;
  static constexpr int exp_bits = 8;
  static constexpr int mant_bits = 23;
  static constexpr int bias = 127;
  static constexpr int max_exp = 254; // 255 is NaN/Inf
};

} // namespace

template <typename T> T reConstruct(unsigned int BitRep, int bitLength) {
  if (bitLength != 12 && bitLength != 13 && bitLength != 16 && bitLength != 20) {
    std::cout << "Invalid bit length" << std::endl;
    return 0;
  }
  using L = IEEELayout<T>;
  using bits_t = typename L::bits_t;

  const int pBits = bitLength - 6;
  const uint64_t sign = (BitRep >> (bitLength - 1)) & 1u;
  const uint64_t expSign = (BitRep >> (bitLength - 2)) & 1u;
  const unsigned magnitude = (BitRep >> pBits) & 0xFu;
  const uint64_t precision = BitRep & ((1ull << pBits) - 1);

  const bool implicitOne = (magnitude != 15);
  const int streamLen = 1 + static_cast<int>(magnitude) + (implicitOne ? 1 : 0) + pBits;
  const uint64_t stream = implicitOne ? ((1ull << pBits) | precision) : precision;

  uint64_t expField;
  uint64_t mantissa;
  if (streamLen >= L::exp_bits) {
    const int rest = streamLen - L::exp_bits;
    expField = stream >> rest;
    mantissa = rest > 0 ? ((stream & ((1ull << rest) - 1)) << (L::mant_bits - rest)) : 0;
  } else {
    expField = stream << (L::exp_bits - streamLen);
    mantissa = 0;
  }

  const int tempExponent = static_cast<int>(expField) + (expSign ? 1 : 0);
  int absExp = expSign ? (L::bias - tempExponent) : (L::bias + tempExponent);
  absExp = std::clamp(absExp, 0, L::max_exp);

  const bits_t ieee = (static_cast<bits_t>(sign) << (L::exp_bits + L::mant_bits))
                      | (static_cast<bits_t>(absExp) << L::mant_bits)
                      | static_cast<bits_t>(mantissa);
  T result;
  std::memcpy(&result, &ieee, sizeof(result));
  return result;
}

template float reConstruct<float>(unsigned int, int);
template double reConstruct<double>(unsigned int, int);
