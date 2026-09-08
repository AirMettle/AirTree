// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <iostream>
#include <sys/types.h>
#include <airtree/core/AirTreeCore_internal.hpp>
#ifdef _MSC_VER
#include <intrin.h>
#endif

class BitManipulationTest : public ::testing::Test {
protected:
  double orignalNumber;
  uint64_t fpNumber;
  bool placed_sign_bit;
  bool placed_signedExponentBit;
  uint64_t exponent;
  uint64_t adjustedExponent;
  uint64_t maskedNumber;
  uint64_t firstOnePos;
  int zeroCount;
  uint64_t precisionBits;

  void runBitManipulation() {
    std::memcpy(&fpNumber, &orignalNumber, sizeof(orignalNumber));
    // Extract and calculate necessary bits directly from the floating-point
    // number
    placed_sign_bit = ((fpNumber >> 63) & 1);
    exponent = (fpNumber >> 52) & 0x7FF;
    placed_signedExponentBit = ((exponent < 1023) ? 1 : 0);
    adjustedExponent =
        placed_signedExponentBit ? (1023 - exponent) : (exponent - 1023);


    if (placed_signedExponentBit) {
      adjustedExponent = ((adjustedExponent - 1) & 0x7FF);
    }

    // Update fpNumber with the adjusted exponent
    maskedNumber = (fpNumber & 0x000FFFFFFFFFFFFF) | (adjustedExponent << 52);

    // Calculate the position of the first significant bit using assembly or
    // intrinsic
#ifdef _MSC_VER
    unsigned long index;
    _BitScanReverse64(&index, maskedNumber);
    firstOnePos = index;
#elif defined(x86_ARCH)
    __asm__("bsrq %1, %0" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
#elif defined(ARM_ARCH)
    __asm__("clz %0, %1" : "=r"(firstOnePos) : "r"(maskedNumber) : "cc");
    firstOnePos = 63 - firstOnePos;
#endif
    if (firstOnePos > 63) {
      firstOnePos = 0;
    }
    zeroCount = std::min(63 - static_cast<int>(firstOnePos) - 2, 15);
    precisionBits = ((maskedNumber >> (firstOnePos - 6)) & 0x3F);
  }

  void setManualExponent(double number, uint64_t manualExponent) {
    uint64_t fpTemp;
    std::memcpy(&fpTemp, &number, sizeof(number));

    // Clear the existing exponent bits
    fpTemp &= ~(0x7FFULL << 52);

    // Set the manual exponent
    fpTemp |= (manualExponent << 52);

    // Update the original number with the new exponent
    std::memcpy(&orignalNumber, &fpTemp, sizeof(fpTemp));
  }
};

TEST_F(BitManipulationTest, NormalOperation_TestCase1) {
  orignalNumber = 1.113;
  // fpNumber = 0x3FF1CED916872B02; // decimal - 1.113

  runBitManipulation();
  EXPECT_EQ(fpNumber,
            0b0011111111110001110011101101100100010110100001110010101100000010);
  EXPECT_EQ(placed_sign_bit, 0);
  EXPECT_EQ(placed_signedExponentBit, 0);
  EXPECT_EQ(exponent, 1023);
  EXPECT_EQ(adjustedExponent, 0);
  EXPECT_EQ(maskedNumber,
            0b0000000000000001110011101101100100010110100001110010101100000010);
  EXPECT_EQ(firstOnePos, 48);
  EXPECT_EQ(zeroCount, 13);
  EXPECT_EQ(precisionBits, 0b110011);
}

TEST_F(BitManipulationTest, NormalOperation_TestCase2) {
  orignalNumber = 85.125;


  runBitManipulation();
  EXPECT_EQ(
      fpNumber,
      0b0100000001010101010010000000000000000000000000000000000000000000); // decimal
                                                                           // - 85.125
  EXPECT_EQ(placed_sign_bit, 0);
  EXPECT_EQ(exponent, 1029);
  EXPECT_EQ(placed_signedExponentBit, 0);
  EXPECT_EQ(adjustedExponent, 6);
  EXPECT_EQ(maskedNumber,
            0b0000000001100101010010000000000000000000000000000000000000000000);
  EXPECT_EQ(firstOnePos, 54);
  EXPECT_EQ(zeroCount, 7);
  EXPECT_EQ(precisionBits, 0b100101);
}

// Manual Exponent Test Cases
TEST_F(BitManipulationTest, NegativeExponet_TestCase1) {
  setManualExponent(1.0, 1022); // Manually set exponent to 1022

  runBitManipulation();
  EXPECT_EQ(fpNumber,
            0b0011111111100000000000000000000000000000000000000000000000000000);
  EXPECT_EQ(placed_sign_bit, 0);
  EXPECT_EQ(exponent, 1022);
  EXPECT_EQ(placed_signedExponentBit, 1);
  EXPECT_EQ(adjustedExponent, 0);
  EXPECT_EQ(maskedNumber, 0);
  EXPECT_EQ(firstOnePos, 0);
  EXPECT_EQ(zeroCount, 15);
  EXPECT_EQ(precisionBits, 0);
}

TEST_F(BitManipulationTest, NegativeExponet_TestCase2) {
  setManualExponent(1.0, 540); // Manually set exponent to 1021

  runBitManipulation();

  EXPECT_EQ(fpNumber,
            0b0010000111000000000000000000000000000000000000000000000000000000);
  EXPECT_EQ(placed_sign_bit, 0);
  EXPECT_EQ(exponent, 540);
  EXPECT_EQ(placed_signedExponentBit, 1);
  EXPECT_EQ(adjustedExponent, 482);
  EXPECT_EQ(maskedNumber,
            0b0001111000100000000000000000000000000000000000000000000000000000);
  EXPECT_EQ(firstOnePos, 60);
  EXPECT_EQ(zeroCount, 1);
  EXPECT_EQ(precisionBits, 0b111000);
}

TEST_F(BitManipulationTest, NegativeExponet_TestCase3) {
  setManualExponent(1.0, 1); // Orignal Number - 2.22507e-308

  runBitManipulation();

  EXPECT_EQ(placed_sign_bit, 0);
  EXPECT_EQ(exponent, 1);
  EXPECT_EQ(placed_signedExponentBit, 1);
  EXPECT_EQ(adjustedExponent, 1021);
  EXPECT_EQ(maskedNumber,
            0b0011111111010000000000000000000000000000000000000000000000000000);
  EXPECT_EQ(firstOnePos, 61);
  EXPECT_EQ(zeroCount, 0);
  EXPECT_EQ(precisionBits, 0b111111);
}

TEST_F(BitManipulationTest, EqualExponent_TestCase) {
  setManualExponent(1.0, 1023); // Orignal Number - 1

  runBitManipulation();

  EXPECT_EQ(placed_sign_bit, 0);
  EXPECT_EQ(exponent, 1023);
  EXPECT_EQ(placed_signedExponentBit, 0);
  EXPECT_EQ(adjustedExponent, 0);
  EXPECT_EQ(maskedNumber, 0);
  EXPECT_EQ(firstOnePos, 0);
  EXPECT_EQ(zeroCount, 15);
  EXPECT_EQ(precisionBits, 0);
}

TEST_F(BitManipulationTest, PositiveExponet_TestCase1) {
  setManualExponent(1.0, 1024); // Orignal Number - 2

  runBitManipulation();

  EXPECT_EQ(placed_sign_bit, 0);
  EXPECT_EQ(exponent, 1024);
  EXPECT_EQ(placed_signedExponentBit, 0);
  EXPECT_EQ(adjustedExponent, 1);
  EXPECT_EQ(maskedNumber,
            0b0000000000010000000000000000000000000000000000000000000000000000);
  EXPECT_EQ(firstOnePos, 52);
  EXPECT_EQ(zeroCount, 9);
  EXPECT_EQ(precisionBits, 0b000000);
}

TEST_F(BitManipulationTest, PositiveExponet_TestCase2) {
  setManualExponent(1.0, 1099); // Orignal Number - 7.55579e+22

  runBitManipulation();

  EXPECT_EQ(placed_sign_bit, 0);
  EXPECT_EQ(exponent, 1099);
  EXPECT_EQ(placed_signedExponentBit, 0);
  EXPECT_EQ(adjustedExponent, 76);
  EXPECT_EQ(maskedNumber,
            0b0000010011000000000000000000000000000000000000000000000000000000);
  EXPECT_EQ(firstOnePos, 58);
  EXPECT_EQ(zeroCount, 3);
  EXPECT_EQ(precisionBits, 0b001100);
}

TEST_F(BitManipulationTest, NegativeNumber_TestCase1) {
  orignalNumber = -1.4;


  runBitManipulation();

  EXPECT_EQ(placed_sign_bit, 1);
  EXPECT_EQ(exponent, 1023);
  EXPECT_EQ(placed_signedExponentBit, 0);
  EXPECT_EQ(adjustedExponent, 0);
  EXPECT_EQ(maskedNumber,
            0b0000000000000110011001100110011001100110011001100110011001100110);
  EXPECT_EQ(firstOnePos, 50);
  EXPECT_EQ(zeroCount, 11);
  EXPECT_EQ(precisionBits, 0b100110);
}

TEST_F(BitManipulationTest, NegativeNumber_TestCase2) {
  orignalNumber = -0.0001321;


  runBitManipulation();

  EXPECT_EQ(placed_sign_bit, 1);
  EXPECT_EQ(exponent, 1010);
  EXPECT_EQ(placed_signedExponentBit, 1);
  EXPECT_EQ(adjustedExponent, 12);
  EXPECT_EQ(maskedNumber,
            0b0000000011000001010100001000101001011100000011101111010010010000);
  EXPECT_EQ(firstOnePos, 55);
  EXPECT_EQ(zeroCount, 6);
  EXPECT_EQ(precisionBits, 0b100000);
}

TEST_F(BitManipulationTest, NegativeNumber_TestCase3) {
  orignalNumber = -15515.15151;


  runBitManipulation();

  EXPECT_EQ(placed_sign_bit, 1);
  EXPECT_EQ(exponent, 1036);
  EXPECT_EQ(placed_signedExponentBit, 0);
  EXPECT_EQ(adjustedExponent, 13);
  EXPECT_EQ(maskedNumber,
            0b0000000011011110010011011001001101100100101011011111111110000010);
  EXPECT_EQ(firstOnePos, 55);
  EXPECT_EQ(zeroCount, 6);
  EXPECT_EQ(precisionBits, 0b101111);
}
