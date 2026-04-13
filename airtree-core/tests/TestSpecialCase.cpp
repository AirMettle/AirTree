#include <gtest/gtest.h>
#include <iostream>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <limits>
#include <memory>
#include <string>
#include <cstring>

class SpecialCaseTest : public ::testing::Test {
protected:
  SpecialCounts counts;

  void SetUp() override {
    // Initialize counts before each test
    counts.posInfCount = 0;
    counts.negInfCount = 0;
    counts.posZeroCount = 0;
    counts.negZeroCount = 0;
    counts.nanCount = 0;
    counts.posneg = 0;
    counts.pospos = 0;
    counts.negpos = 0;
    counts.negneg = 0;
  }
};

TEST_F(SpecialCaseTest, DetectsPositiveInfinity) {
  double posInf = std::numeric_limits<double>::infinity();
  uint64_t fpNumber;
  std::memcpy(&fpNumber, &posInf, sizeof(posInf));
  std::cout << "fpNumber: " << std::bitset<64>(fpNumber) << std::endl;

  EXPECT_TRUE(isSpecialCase(fpNumber, counts));
  EXPECT_EQ(counts.posInfCount, 1);
  EXPECT_EQ(counts.negInfCount, 0);
  EXPECT_EQ(counts.nanCount, 0);
  EXPECT_EQ(counts.posZeroCount, 0);
  EXPECT_EQ(counts.negZeroCount, 0);
}

TEST_F(SpecialCaseTest, DetectsNegativeInfinity) {
  double negInf = -std::numeric_limits<double>::infinity();
  uint64_t fpNumber;
  std::memcpy(&fpNumber, &negInf, sizeof(negInf));
  std::cout << "fpNumber: " << std::bitset<64>(fpNumber) << std::endl;

  EXPECT_TRUE(isSpecialCase(fpNumber, counts));
  EXPECT_EQ(counts.posInfCount, 0);
  EXPECT_EQ(counts.negInfCount, 1);
  EXPECT_EQ(counts.nanCount, 0);
  EXPECT_EQ(counts.posZeroCount, 0);
  EXPECT_EQ(counts.negZeroCount, 0);
}

TEST_F(SpecialCaseTest, DetectsNaN) {
  double nanValue = std::numeric_limits<double>::quiet_NaN();
  uint64_t fpNumber;
  std::memcpy(&fpNumber, &nanValue, sizeof(nanValue));

  EXPECT_TRUE(isSpecialCase(fpNumber, counts));
  EXPECT_EQ(counts.posInfCount, 0);
  EXPECT_EQ(counts.negInfCount, 0);
  EXPECT_EQ(counts.nanCount, 1);
  EXPECT_EQ(counts.posZeroCount, 0);
  EXPECT_EQ(counts.negZeroCount, 0);
}

TEST_F(SpecialCaseTest, DetectsPositiveZero) {
  double posZero = 0.0;
  uint64_t fpNumber;
  std::memcpy(&fpNumber, &posZero, sizeof(posZero));

  EXPECT_TRUE(isSpecialCase(fpNumber, counts));
  EXPECT_EQ(counts.posInfCount, 0);
  EXPECT_EQ(counts.negInfCount, 0);
  EXPECT_EQ(counts.nanCount, 0);
  EXPECT_EQ(counts.posZeroCount, 1);
  EXPECT_EQ(counts.negZeroCount, 0);
}

TEST_F(SpecialCaseTest, DetectsNegativeZero) {
  double negZero = -0.0;
  uint64_t fpNumber;
  std::memcpy(&fpNumber, &negZero, sizeof(negZero));

  EXPECT_TRUE(isSpecialCase(fpNumber, counts));
  EXPECT_EQ(counts.posInfCount, 0);
  EXPECT_EQ(counts.negInfCount, 0);
  EXPECT_EQ(counts.nanCount, 0);
  EXPECT_EQ(counts.posZeroCount, 0);
  EXPECT_EQ(counts.negZeroCount, 1);
}

TEST_F(SpecialCaseTest, NonSpecialNumberReturnsFalse) {
  double normalNumber = 42.0;
  uint64_t fpNumber;
  std::memcpy(&fpNumber, &normalNumber, sizeof(normalNumber));

  EXPECT_FALSE(isSpecialCase(fpNumber, counts));
  EXPECT_EQ(counts.posInfCount, 0);
  EXPECT_EQ(counts.negInfCount, 0);
  EXPECT_EQ(counts.nanCount, 0);
  EXPECT_EQ(counts.posZeroCount, 0);
  EXPECT_EQ(counts.negZeroCount, 0);
}
