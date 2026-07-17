// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <cstring>
#include <airtree/core/common/Reconstruct.hpp>
#include <string>
#include <iostream>

double internalRepToIEEE(uint64_t internalReconstructedBits) {
  double result;
  memcpy(&result, &internalReconstructedBits, sizeof(result));
  return result;
}

float internalRepToIEEE_32(uint32_t internalReconstructedBits) {
  float result;
  memcpy(&result, &internalReconstructedBits, sizeof(result));
  return result;
}

template <typename T> T reConstruct(unsigned int BitRep, int bitLength) {

  std::string binaryRep;

  switch (bitLength) {
  case 12:
    binaryRep = std::bitset<12>(BitRep).to_string();
    break;
  case 13:
    binaryRep = std::bitset<13>(BitRep).to_string();
    break;
  case 16:
    binaryRep = std::bitset<16>(BitRep).to_string();
    break;
  case 20:
    binaryRep = std::bitset<20>(BitRep).to_string();
    break;
  default:
    std::cout << "Invalid bit length" << std::endl;
    return 0;
  }


  char signBit = binaryRep[0];
  char signedExponentBit = binaryRep[1];
  std::string magnitudeBits = binaryRep.substr(2, 4);
  std::string precisionBits = binaryRep.substr(6);

  // std::cout << "=================== REVERSE ENGINEER 12 BIT
  // ===================="
  //      << std::endl;
  // std::cout << "Sign Bit: " << signBit
  //      << ", Signed Exponent Bit: " << signedExponentBit
  //      << ", Magnitude Bits: " << magnitudeBits
  //      << ", Precision Bits: " << precisionBits << std::endl;


  int firstOne = stoi(magnitudeBits, nullptr, 2);
  // cout << "First One: " << firstOne << endl;

  // Define maximum magnitude for saturation check
  const int MAX_MAGNITUDE = 15;

  std::string mod12BitRep;

  if (firstOne == MAX_MAGNITUDE) {
    // SATURATION CASE: Don't insert implicit "1"
    // The precision bits already include the bit at the saturation point (which
    // might be 0)
    mod12BitRep = std::string(1, signBit) + "0" + std::string(firstOne, '0')
                  + precisionBits;
  } else {
    // NORMAL CASE: Insert the implicit "1" that was skipped during encoding
    mod12BitRep = std::string(1, signBit) + "0" + std::string(firstOne, '0')
                  + "1" + precisionBits;
  }

  if (mod12BitRep.length() < 12) {
    mod12BitRep.append(12 - mod12BitRep.length(), '0');
  }

  if constexpr (std::is_same_v<T, double>) {


    // std::cout << "Modified 12 Bit Representation: " << mod12BitRep <<
    // std::endl;

    std::string exponent12Bits = mod12BitRep.substr(1, 11);
    if (exponent12Bits.length() < 11) {
      exponent12Bits.append(11 - exponent12Bits.length(), '0');
    }

    //   std::cout << "Exponent 11 bit (binary): " << exponent12Bits <<
    //   std::endl;
    // std::cout << "Length of Exponent 11 bit: " << exponent12Bits.length() <<
    // std::endl;


    std::string restOfTheBits = mod12BitRep.substr(12);
    int exponentInDec = stoi(exponent12Bits, nullptr, 2);

    // std::cout << "Exponent 11 bit: " << exponentInDec << std::endl;
    // std::cout << "Mantissa: " << restOfTheBits << std::endl;

    int tempExponent =
        (signedExponentBit == '1') ? (exponentInDec + 1) : exponentInDec;
    int absExp = (signedExponentBit == '1') ? (1023 - tempExponent)
                                            : (1023 + tempExponent);

    // Clamp to valid IEEE-754 double exponent range [0, 2046].
    // Exponent 2047 (0x7FF) is reserved for NaN/Inf — underflow past 0
    // means the value is too small to represent even as subnormal.
    absExp = std::clamp(absExp, 0, 2046);

    std::string originalIEEE = std::string(1, signBit)
                               + std::bitset<11>(absExp).to_string()
                               + restOfTheBits;
    // cout << "Original IEEE bits: " << originalIEEE << endl;

    std::string newIEEERep = originalIEEE;
    newIEEERep.append(
        64 - newIEEERep.length(), '0'); // Padding to ensure 64 bits.

    // cout << "Approximated IEEE-754 Representation (padded to 64 bits): "
    //      << newIEEERep << endl;

    // Convert the binary string to a uint64_t representation of IEEE-754
    uint64_t ieeeBinary = stoull(newIEEERep, nullptr, 2);
    return internalRepToIEEE(ieeeBinary);

    // cout << "Approximated Floating-Point Value: " << approxValue << endl;

    // return static_cast<T>(approxValue);

  } else {

    std::string exponent12Bits = mod12BitRep.substr(1, 8);
    if (exponent12Bits.length() < 8) {
      exponent12Bits.append(8 - exponent12Bits.length(), '0');
    }

    //   std::cout << "Exponent 11 bit (binary): " << exponent12Bits <<
    //   std::endl;
    // std::cout << "Length of Exponent 11 bit: " << exponent12Bits.length() <<
    // std::endl;


    std::string restOfTheBits = mod12BitRep.substr(9);
    int exponentInDec = stoi(exponent12Bits, nullptr, 2);

    // std::cout << "Exponent 11 bit: " << exponentInDec << std::endl;
    // std::cout << "Mantissa: " << restOfTheBits << std::endl;

    int tempExponent =
        (signedExponentBit == '1') ? (exponentInDec + 1) : exponentInDec;
    int absExp = (signedExponentBit == '1') ? (127 - tempExponent)
                                            : (127 + tempExponent);

    // Clamp to valid IEEE-754 float exponent range [0, 254].
    // Exponent 255 (0xFF) is reserved for NaN/Inf.
    absExp = std::clamp(absExp, 0, 254);

    std::string originalIEEE = std::string(1, signBit)
                               + std::bitset<8>(absExp).to_string()
                               + restOfTheBits;
    // cout << "Original IEEE bits: " << originalIEEE << endl;

    std::string newIEEERep = originalIEEE;
    newIEEERep.append(
        32 - newIEEERep.length(), '0'); // Padding to ensure 64 bits.

    // cout << "Approximated IEEE-754 Representation (padded to 64 bits): "
    //      << newIEEERep << endl;

    // Convert the binary string to a uint64_t representation of IEEE-754
    uint32_t ieeeBinary = stoull(newIEEERep, nullptr, 2);
    return internalRepToIEEE_32(ieeeBinary);
  }
}


template float reConstruct<float>(unsigned int, int);
template double reConstruct<double>(unsigned int, int);