#include <airtree/core/common/TLE.hpp>
#include <limits>
#include <stdexcept>

TLE setTLEComponents_32(uint32_t fpNumber) {
  TLE tle;
  uint32_t expMask = 0xFF << 23;
  uint32_t signBit = (fpNumber >> 31) & 1;
  uint32_t exponent = (fpNumber >> 23) & 0xFF;
  // mask everything else apart from magnitude and compare
  uint32_t mantissa = fpNumber & 0x7FFFFF;

  // Early return for Zero check
  if (!(fpNumber & ~(1U << 31))) {
    tle.encoding = 4;
    return tle;
  }

  // Early return for Inf or Nan
  if ((fpNumber & expMask) == expMask) {
    tle.encoding = mantissa ? 0 : (signBit ? 7 : 1);
    return tle;
  }

  // Handle normal numbers
  bool isExponentLessThan127 = exponent < 127;
  int index = (signBit << 1) | isExponentLessThan127;
  static const int results[] = {2, 3, 6, 5};
  tle.encoding = results[index];
  return tle;
}

TLE setTLEComponents(uint64_t fpNumber) {
  TLE tle;
  uint64_t expMask = 0x7FFULL << 52;
  uint64_t signBit = (fpNumber >> 63) & 1;
  uint64_t exponent = (fpNumber >> 52) & 0x7FF;
  // mask everything else apart from magnitude and compare
  uint64_t mantissa = fpNumber & 0xFFFFFFFFFFFFF;

  // Early return for Zero check
  if (!(fpNumber & ~(1ULL << 63))) {
    tle.encoding = 4;
    return tle;
  }

  // Early return for Inf or Nan
  if ((fpNumber & expMask) == expMask) {
    tle.encoding = mantissa ? 0 : (signBit ? 7 : 1);
    return tle;
  }

  // Handle normal numbers
  bool isExponentLessThan1023 = exponent < 1023;
  int index = (signBit << 1) | isExponentLessThan1023;
  static const int results[] = {2, 3, 6, 5};
  tle.encoding = results[index];
  return tle;
}


/**
** @brief Individual Top Level Encoding (TLE) bits

** @param TLE 3 bits (0-7) to encode the top level of the 3D trie
** @see options are :          (Prepend bits)
** 1. NaN = 0          000       -
** 2. +ve inf = 1      001       -
** 3. +ve +veExp = 2   010       00
** 4. +ve -veExp = 3   011       01
** 5. 0 = 4            100        -
** 6. -ve -veExp = 5   101       11
** 7. -ve +veExp = 6   110       10
** 8. -ve Inf = 7      111        -
*/
std::bitset<2> getprependbits(std::bitset<3> TLE) {
  std::bitset<2> prependbits;
  switch (TLE.to_ulong()) {
  case 2: // +ve +veExp
    prependbits = 0b00;
    break;
  case 3: // +ve -veExp
    prependbits = 0b01;
    break;
  case 5: // -ve -veExp
    prependbits = 0b11;
    break;
  case 6: // -ve +veExp
    prependbits = 0b10;
    break;
  default:
    throw std::invalid_argument("Invalid TLE value");
  }
  return prependbits;
}

uint32_t getTLEEncoding(uint32_t prefixBits) {
  switch (prefixBits) {
  case 0: // 00 - +ve sign, +ve exponent
    return 2;
  case 1: // 01 - +ve sign, -ve exponent
    return 3;
  case 2: // 10 - -ve sign, +ve exponent
    return 6;
  case 3: // 11 - -ve sign, -ve exponent
    return 5;
  default:
    throw std::invalid_argument("Invalid TLE value");
  }
}

// Merge the below two functions into one
TLEValueType getTLEValueType(uint32_t specialFlag) {
  switch (specialFlag) {
  case 0: // NaN
    return TLEValueType::NaN;
  case 1: // +ve Inf
    return TLEValueType::InfPos;
  case 2: // +ve +veExp // int value = 0
    return TLEValueType::PosSignPosExp;
  case 3: // +ve -veExp // int value = 1
    return TLEValueType::PosSignNegExp;
  case 4: // 0
    return TLEValueType::Zero;
  case 5: // -ve -veExp // int value = 3
    return TLEValueType::NegSignNegExp;
  case 6: // -ve +veExp // int value = 2
    return TLEValueType::NegSignPosExp;
  case 7: // -ve Inf
    return TLEValueType::InfNeg;
  default: // non-special values
    return TLEValueType::Error;
  }
}

uint32_t getPrefixForNonSpecialValueType(TLEValueType nonSpecialValueType) {
  switch (nonSpecialValueType) {
  case TLEValueType::PosSignPosExp:
    return 0;
  case TLEValueType::PosSignNegExp:
    return 1;
  case TLEValueType::NegSignNegExp:
    return 3;
  case TLEValueType::NegSignPosExp:
    return 2;
  default:
    throw std::invalid_argument("Invalid non-special value type");
  }
}

std::pair<double, double> getSpecialValue(TLEValueType type) {
  switch (type) {
  case TLEValueType::NaN:
    return {std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<double>::quiet_NaN()};
  case TLEValueType::InfPos:
    return {std::numeric_limits<double>::infinity(),
            std::numeric_limits<double>::infinity()};
  case TLEValueType::InfNeg:
    return {-std::numeric_limits<double>::infinity(),
            -std::numeric_limits<double>::infinity()};
  case TLEValueType::Zero:
    return {0.0, 0.0};
  default:
    throw std::invalid_argument("Invalid special value type");
  }
}

DimensionInfoResult deconstructTLE(uint32_t TLE, uint32_t numDimensions) {
  DimensionInfoVector dimensionInfos(numDimensions);
  uint32_t specialCount = 0;

  for (uint32_t i = 0; i < numDimensions; i++) {
    DimensionInfo &info = dimensionInfos[i];
    auto dim_i_tle = (TLE >> ((numDimensions - 1 - i) * 3))
                     & 0x7; // Extract TLE for dimension i
    // std::cout << "TLE for dimension " << i << ": " << dim_i_tle << std::endl;
    auto tle_type = getTLEValueType(dim_i_tle);
    switch (tle_type) {
    case TLEValueType::NaN:
    case TLEValueType::InfPos:
    case TLEValueType::InfNeg:
    case TLEValueType::Zero:
      specialCount++;
      info.isSpecial = true;     // Special values
      info.TLE = dim_i_tle;      // Store the TLE value
      info.prefix = 0;           // No prefix for special values
      info.valueType = tle_type; // Store the type of special value
      break;
    case TLEValueType::PosSignPosExp:
    case TLEValueType::PosSignNegExp:
    case TLEValueType::NegSignNegExp:
    case TLEValueType::NegSignPosExp:
      info.isSpecial = false; // Non-special values
      info.TLE = dim_i_tle;   // Store the TLE value
      info.prefix = getPrefixForNonSpecialValueType(
          tle_type);             // Get prefix for non-special
      info.valueType = tle_type; // Store the type of non-special value
      break;
    case TLEValueType::Error:
      throw std::invalid_argument("Invalid TLE value type");
    }
  }

  if (specialCount > numDimensions) {
    throw std::invalid_argument("Special count exceeds number of dimensions");
  }
  if (dimensionInfos.size() != numDimensions) {
    throw std::invalid_argument(
        "Dimension info vec size does not match number of dimensions");
  }
  return DimensionInfoResult{dimensionInfos, specialCount};
}