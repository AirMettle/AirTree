#ifndef AIRTREE_CORE_COMMON_TLE_HPP
#define AIRTREE_CORE_COMMON_TLE_HPP


#include <bitset>
#include <cstdint>
#include <vector>

/**
** @brief Individual Top Level Encoding (TLE) bits

** @param TLE 3 bits (0-7) to encode the top level of the 3D trie
** @see options are :
** 1. NaN = 0
** 2. +ve inf = 1
** 3. +ve +veExp = 2
** 4. +ve -veExp = 3
** 5. 0 = 4
** 6. -ve -veExp = 5
** 7. -ve +veExp = 6
** 8. -ve Inf = 7
*/
struct TLE {
  uint8_t TLE : 3;
};

TLE setTLEComponents(uint64_t fpNumber);
TLE setTLEComponents_32(uint32_t fpNumber);
[[maybe_unused]] TLE setTLEComponents_streamlined(uint64_t fpNumber);
[[maybe_unused]] TLE setTLEComponents_streamlined_v1(uint64_t fpNumber);

std::bitset<2> getprependbits(std::bitset<3> TLE);
uint32_t getTLEEncoding(uint32_t prefixBits);

enum class TLEValueType {
  NaN,
  InfPos,
  InfNeg,
  Zero,
  PosSignPosExp,
  PosSignNegExp,
  NegSignPosExp,
  NegSignNegExp,
  Error // To indicate an error state
};

struct DimensionInfo {
  bool isSpecial;  // Flag to indicate if the value is special
  uint32_t TLE;    // The bits that encode this dimension type in top level
  uint32_t prefix; // Prefix for non-special values
  TLEValueType valueType; // Type of special value
};

using DimensionInfoVector = std::vector<DimensionInfo>;

struct DimensionInfoResult {
  DimensionInfoVector dimensionInfos;
  uint32_t specialCount; // Count of special dimensions
};

[[nodiscard]] DimensionInfoResult deconstructTLE(uint32_t TLE,
                                                 uint32_t numDimensions);

TLEValueType getTLEValueType(uint32_t specialFlag);
std::pair<double, double> getSpecialValue(TLEValueType type);


#endif // AIRTREE_CORE_COMMON_TLE_HPP