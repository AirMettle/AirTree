// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/AirTreeCore_internal.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <iostream>
#include <sys/types.h>
#include <tuple>
#include <vector>

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
class TestUtilities : public ::testing::Test {
protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(TestUtilities, TestDeconstructTLE2D_1Special) {
  // Example TLE value and number of dimensions

  uint32_t TLE = 0b010111;
  auto [dimInfoVec, specialCount] = deconstructTLE(TLE, 2);

  EXPECT_EQ(specialCount, 1); // Expecting 1 special value

  EXPECT_EQ(dimInfoVec[0].isSpecial, false);
  EXPECT_EQ(dimInfoVec[0].TLE, 2);
  EXPECT_EQ(dimInfoVec[0].prefix, 0);
  EXPECT_EQ(dimInfoVec[0].valueType, TLEValueType::PosSignPosExp);

  EXPECT_EQ(dimInfoVec[1].isSpecial, true);
  EXPECT_EQ(dimInfoVec[1].TLE, 7);
  EXPECT_EQ(dimInfoVec[1].prefix, 0);
  EXPECT_EQ(dimInfoVec[1].valueType, TLEValueType::InfNeg);
}

TEST_F(TestUtilities, TestDeconstructTLE2D_2Special) {
  // Example TLE value and number of dimensions

  uint32_t TLE = 0b000100;
  auto [dimInfoVec, specialCount] = deconstructTLE(TLE, 2);

  EXPECT_EQ(specialCount, 2); // Expecting 2 special values

  EXPECT_EQ(dimInfoVec[0].isSpecial, true);
  EXPECT_EQ(dimInfoVec[0].TLE, 0);
  EXPECT_EQ(dimInfoVec[0].prefix, 0);
  EXPECT_EQ(dimInfoVec[0].valueType, TLEValueType::NaN);

  EXPECT_EQ(dimInfoVec[1].isSpecial, true);
  EXPECT_EQ(dimInfoVec[1].TLE, 4);
  EXPECT_EQ(dimInfoVec[1].prefix, 0);
  EXPECT_EQ(dimInfoVec[1].valueType, TLEValueType::Zero);
}

TEST_F(TestUtilities, TestDeconstructTLE3D_1Special) {
  // Example TLE value and number of dimensions

  uint32_t TLE = 0b011000110;
  auto [dimInfoVec, specialCount] = deconstructTLE(TLE, 3);

  EXPECT_EQ(specialCount, 1); // Expecting one special value

  EXPECT_EQ(dimInfoVec[0].isSpecial, false);
  EXPECT_EQ(dimInfoVec[0].TLE, 3);
  EXPECT_EQ(dimInfoVec[0].prefix, 1);
  EXPECT_EQ(dimInfoVec[0].valueType, TLEValueType::PosSignNegExp);

  EXPECT_EQ(dimInfoVec[1].isSpecial, true);
  EXPECT_EQ(dimInfoVec[1].TLE, 0);
  EXPECT_EQ(dimInfoVec[1].prefix, 0);
  EXPECT_EQ(dimInfoVec[1].valueType, TLEValueType::NaN);


  EXPECT_EQ(dimInfoVec[2].isSpecial, false);
  EXPECT_EQ(dimInfoVec[2].TLE, 6);
  EXPECT_EQ(dimInfoVec[2].prefix, 2);
  EXPECT_EQ(dimInfoVec[2].valueType, TLEValueType::NegSignPosExp);
}

TEST_F(TestUtilities, TestDeconstructTLE3D_2Special) {
  // Example TLE value and number of dimensions

  uint32_t TLE = 0b000101100;
  auto [dimInfoVec, specialCount] = deconstructTLE(TLE, 3);

  EXPECT_EQ(specialCount, 2); // Expecting 2 special dimensions

  EXPECT_EQ(dimInfoVec[0].isSpecial, true);
  EXPECT_EQ(dimInfoVec[0].TLE, 0);
  EXPECT_EQ(dimInfoVec[0].prefix, 0);
  EXPECT_EQ(dimInfoVec[0].valueType, TLEValueType::NaN);

  EXPECT_EQ(dimInfoVec[1].isSpecial, false);
  EXPECT_EQ(dimInfoVec[1].TLE, 5);
  EXPECT_EQ(dimInfoVec[1].prefix, 3);
  EXPECT_EQ(dimInfoVec[1].valueType, TLEValueType::NegSignNegExp);


  EXPECT_EQ(dimInfoVec[2].isSpecial, true);
  EXPECT_EQ(dimInfoVec[2].TLE, 4);
  EXPECT_EQ(dimInfoVec[2].prefix, 0);
  EXPECT_EQ(dimInfoVec[2].valueType, TLEValueType::Zero);
}

TEST_F(TestUtilities, TestDeconstructTLE3D_3Special) {
  // Example TLE value and number of dimensions

  uint32_t TLE = 0b000100111;
  auto [dimInfoVec, specialCount] = deconstructTLE(TLE, 3);

  EXPECT_EQ(specialCount, 3); // Expecting 3 special values

  EXPECT_EQ(dimInfoVec[0].isSpecial, true);
  EXPECT_EQ(dimInfoVec[0].TLE, 0);
  EXPECT_EQ(dimInfoVec[0].prefix, 0);
  EXPECT_EQ(dimInfoVec[0].valueType, TLEValueType::NaN);

  EXPECT_EQ(dimInfoVec[1].isSpecial, true);
  EXPECT_EQ(dimInfoVec[1].TLE, 4);
  EXPECT_EQ(dimInfoVec[1].prefix, 0);
  EXPECT_EQ(dimInfoVec[1].valueType, TLEValueType::Zero);


  EXPECT_EQ(dimInfoVec[2].isSpecial, true);
  EXPECT_EQ(dimInfoVec[2].TLE, 7);
  EXPECT_EQ(dimInfoVec[2].prefix, 0);
  EXPECT_EQ(dimInfoVec[2].valueType, TLEValueType::InfNeg);
}

TEST_F(TestUtilities, TestDeconstructTLE4D_1Special) {
  // Example TLE value and number of dimensions

  uint32_t TLE = 0b010100011101;
  auto [dimInfoVec, specialCount] = deconstructTLE(TLE, 4);

  EXPECT_EQ(specialCount, 1); // Expecting 1 special value

  EXPECT_EQ(dimInfoVec[0].isSpecial, false);
  EXPECT_EQ(dimInfoVec[0].TLE, 2);
  EXPECT_EQ(dimInfoVec[0].prefix, 0);
  EXPECT_EQ(dimInfoVec[0].valueType, TLEValueType::PosSignPosExp);

  EXPECT_EQ(dimInfoVec[1].isSpecial, true);
  EXPECT_EQ(dimInfoVec[1].TLE, 4);
  EXPECT_EQ(dimInfoVec[1].prefix, 0);
  EXPECT_EQ(dimInfoVec[1].valueType, TLEValueType::Zero);

  EXPECT_EQ(dimInfoVec[2].isSpecial, false);
  EXPECT_EQ(dimInfoVec[2].TLE, 3);
  EXPECT_EQ(dimInfoVec[2].prefix, 1);
  EXPECT_EQ(dimInfoVec[2].valueType, TLEValueType::PosSignNegExp);

  EXPECT_EQ(dimInfoVec[3].isSpecial, false);
  EXPECT_EQ(dimInfoVec[3].TLE, 5);
  EXPECT_EQ(dimInfoVec[3].prefix, 3);
  EXPECT_EQ(dimInfoVec[3].valueType, TLEValueType::NegSignNegExp);
}

TEST_F(TestUtilities, TestDeconstructTLE4D_2Special) {
  // Example TLE value and number of dimensions

  uint32_t TLE = 0b000101110111;
  auto [dimInfoVec, specialCount] = deconstructTLE(TLE, 4);

  EXPECT_EQ(specialCount, 2); // Expecting 2 special values

  EXPECT_EQ(dimInfoVec[0].isSpecial, true);
  EXPECT_EQ(dimInfoVec[0].TLE, 0);
  EXPECT_EQ(dimInfoVec[0].prefix, 0);
  EXPECT_EQ(dimInfoVec[0].valueType, TLEValueType::NaN);

  EXPECT_EQ(dimInfoVec[1].isSpecial, false);
  EXPECT_EQ(dimInfoVec[1].TLE, 5);
  EXPECT_EQ(dimInfoVec[1].prefix, 3);
  EXPECT_EQ(dimInfoVec[1].valueType, TLEValueType::NegSignNegExp);


  EXPECT_EQ(dimInfoVec[2].isSpecial, false);
  EXPECT_EQ(dimInfoVec[2].TLE, 6);
  EXPECT_EQ(dimInfoVec[2].prefix, 2);
  EXPECT_EQ(dimInfoVec[2].valueType, TLEValueType::NegSignPosExp);

  EXPECT_EQ(dimInfoVec[3].isSpecial, true);
  EXPECT_EQ(dimInfoVec[3].TLE, 7);
  EXPECT_EQ(dimInfoVec[3].prefix, 0);
  EXPECT_EQ(dimInfoVec[3].valueType, TLEValueType::InfNeg);
}

TEST_F(TestUtilities, TestDeconstructTLE4D_3Special) {
  // Example TLE value and number of dimensions

  uint32_t TLE = 0b010100111111;
  auto [dimInfoVec, specialCount] = deconstructTLE(TLE, 4);

  EXPECT_EQ(specialCount, 3);

  EXPECT_EQ(dimInfoVec[0].isSpecial, false);
  EXPECT_EQ(dimInfoVec[0].TLE, 2);
  EXPECT_EQ(dimInfoVec[0].prefix, 0);
  EXPECT_EQ(dimInfoVec[0].valueType, TLEValueType::PosSignPosExp);

  EXPECT_EQ(dimInfoVec[1].isSpecial, true);
  EXPECT_EQ(dimInfoVec[1].TLE, 4);
  EXPECT_EQ(dimInfoVec[1].prefix, 0);
  EXPECT_EQ(dimInfoVec[1].valueType, TLEValueType::Zero);


  EXPECT_EQ(dimInfoVec[2].isSpecial, true);
  EXPECT_EQ(dimInfoVec[2].TLE, 7);
  EXPECT_EQ(dimInfoVec[2].prefix, 0);
  EXPECT_EQ(dimInfoVec[2].valueType, TLEValueType::InfNeg);

  EXPECT_EQ(dimInfoVec[3].isSpecial, true);
  EXPECT_EQ(dimInfoVec[3].TLE, 7);
  EXPECT_EQ(dimInfoVec[3].prefix, 0);
  EXPECT_EQ(dimInfoVec[3].valueType, TLEValueType::InfNeg);
}

TEST_F(TestUtilities, TestDeconstructTLE4D_4Special) {
  // Example TLE value and number of dimensions

  uint32_t TLE = 0b000100111100;
  auto [dimInfoVec, specialCount] = deconstructTLE(TLE, 4);

  EXPECT_EQ(specialCount, 4); // Expecting 4 special values

  EXPECT_EQ(dimInfoVec[0].isSpecial, true);
  EXPECT_EQ(dimInfoVec[0].TLE, 0);
  EXPECT_EQ(dimInfoVec[0].prefix, 0);
  EXPECT_EQ(dimInfoVec[0].valueType, TLEValueType::NaN);

  EXPECT_EQ(dimInfoVec[1].isSpecial, true);
  EXPECT_EQ(dimInfoVec[1].TLE, 4);
  EXPECT_EQ(dimInfoVec[1].prefix, 0);
  EXPECT_EQ(dimInfoVec[1].valueType, TLEValueType::Zero);


  EXPECT_EQ(dimInfoVec[2].isSpecial, true);
  EXPECT_EQ(dimInfoVec[2].TLE, 7);
  EXPECT_EQ(dimInfoVec[2].prefix, 0);
  EXPECT_EQ(dimInfoVec[2].valueType, TLEValueType::InfNeg);

  EXPECT_EQ(dimInfoVec[3].isSpecial, true);
  EXPECT_EQ(dimInfoVec[3].TLE, 4);
  EXPECT_EQ(dimInfoVec[3].prefix, 0);
  EXPECT_EQ(dimInfoVec[3].valueType, TLEValueType::Zero);
}
