// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)
// reConstruct vs the original string implementation (kept below as the oracle), all codes.

#include <airtree/core/common/Reconstruct.hpp>
#include <gtest/gtest.h>

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

namespace {

double oracleToIEEE(uint64_t bits) {
  double r;
  std::memcpy(&r, &bits, sizeof(r));
  return r;
}
float oracleToIEEE_32(uint32_t bits) {
  float r;
  std::memcpy(&r, &bits, sizeof(r));
  return r;
}

template <typename T> T reConstructOracle(unsigned int BitRep, int bitLength) {
  std::string binaryRep;
  switch (bitLength) {
  case 12: binaryRep = std::bitset<12>(BitRep).to_string(); break;
  case 13: binaryRep = std::bitset<13>(BitRep).to_string(); break;
  case 16: binaryRep = std::bitset<16>(BitRep).to_string(); break;
  case 20: binaryRep = std::bitset<20>(BitRep).to_string(); break;
  default: return 0;
  }
  char signBit = binaryRep[0];
  char signedExponentBit = binaryRep[1];
  std::string magnitudeBits = binaryRep.substr(2, 4);
  std::string precisionBits = binaryRep.substr(6);
  int firstOne = std::stoi(magnitudeBits, nullptr, 2);
  const int MAX_MAGNITUDE = 15;
  std::string mod12BitRep;
  if (firstOne == MAX_MAGNITUDE) {
    mod12BitRep = std::string(1, signBit) + "0" + std::string(firstOne, '0') + precisionBits;
  } else {
    mod12BitRep = std::string(1, signBit) + "0" + std::string(firstOne, '0') + "1" + precisionBits;
  }
  if (mod12BitRep.length() < 12) {
    mod12BitRep.append(12 - mod12BitRep.length(), '0');
  }
  if constexpr (std::is_same_v<T, double>) {
    std::string exponent12Bits = mod12BitRep.substr(1, 11);
    if (exponent12Bits.length() < 11) exponent12Bits.append(11 - exponent12Bits.length(), '0');
    std::string restOfTheBits = mod12BitRep.substr(12);
    int exponentInDec = std::stoi(exponent12Bits, nullptr, 2);
    int tempExponent = (signedExponentBit == '1') ? (exponentInDec + 1) : exponentInDec;
    int absExp = (signedExponentBit == '1') ? (1023 - tempExponent) : (1023 + tempExponent);
    absExp = std::clamp(absExp, 0, 2046);
    std::string ieee = std::string(1, signBit) + std::bitset<11>(absExp).to_string() + restOfTheBits;
    ieee.append(64 - ieee.length(), '0');
    return oracleToIEEE(std::stoull(ieee, nullptr, 2));
  } else {
    std::string exponent12Bits = mod12BitRep.substr(1, 8);
    if (exponent12Bits.length() < 8) exponent12Bits.append(8 - exponent12Bits.length(), '0');
    std::string restOfTheBits = mod12BitRep.substr(9);
    int exponentInDec = std::stoi(exponent12Bits, nullptr, 2);
    int tempExponent = (signedExponentBit == '1') ? (exponentInDec + 1) : exponentInDec;
    int absExp = (signedExponentBit == '1') ? (127 - tempExponent) : (127 + tempExponent);
    absExp = std::clamp(absExp, 0, 254);
    std::string ieee = std::string(1, signBit) + std::bitset<8>(absExp).to_string() + restOfTheBits;
    ieee.append(32 - ieee.length(), '0');
    return oracleToIEEE_32(static_cast<uint32_t>(std::stoull(ieee, nullptr, 2)));
  }
}

template <typename T> uint64_t bitsOf(T v) {
  typename std::conditional<sizeof(T) == 8, uint64_t, uint32_t>::type b;
  std::memcpy(&b, &v, sizeof(v));
  return b;
}

template <typename T> void checkAllCodes(int bitLength) {
  const uint64_t n = 1ull << bitLength;
  uint64_t mismatches = 0;
  for (uint64_t code = 0; code < n; ++code) {
    const T got = reConstruct<T>(static_cast<unsigned int>(code), bitLength);
    const T want = reConstructOracle<T>(static_cast<unsigned int>(code), bitLength);
    if (bitsOf(got) != bitsOf(want)) {
      if (mismatches < 10) {
        ADD_FAILURE() << "bitLength=" << bitLength << " code=" << code << " got=" << got
                      << " want=" << want;
      }
      ++mismatches;
    }
  }
  EXPECT_EQ(mismatches, 0u) << "bitLength=" << bitLength;
}

} // namespace

TEST(ReconstructEquivalence, Double12) { checkAllCodes<double>(12); }
TEST(ReconstructEquivalence, Double13) { checkAllCodes<double>(13); }
TEST(ReconstructEquivalence, Double16) { checkAllCodes<double>(16); }
TEST(ReconstructEquivalence, Double20) { checkAllCodes<double>(20); }
TEST(ReconstructEquivalence, Float12) { checkAllCodes<float>(12); }
TEST(ReconstructEquivalence, Float13) { checkAllCodes<float>(13); }
TEST(ReconstructEquivalence, Float16) { checkAllCodes<float>(16); }
TEST(ReconstructEquivalence, Float20) { checkAllCodes<float>(20); }

TEST(ReconstructEquivalence, UnsupportedBitLengthReturnsZero) {
  EXPECT_EQ(reConstruct<double>(5u, 8), 0.0);
  EXPECT_EQ(reConstruct<float>(5u, 24), 0.0f);
}
