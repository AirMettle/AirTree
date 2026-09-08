// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/AirTreeCore_internal.hpp>
#include <cstdint>
#include <gtest/gtest.h>
#include <iostream>
#include <cstring>

// THIS TEST CASE IS FOR TESTING 13COLONIES INTERNAL FILING
class CreateAndInsertFPTest : public ::testing::Test {
protected:
  double orignalNumber;
  std::unique_ptr<TrieNode_13> root = CreateParentNode();

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

TEST_F(CreateAndInsertFPTest, NormalOperation_TestCase1) {

  double originalNumber = 1.113;
  uint64_t fpNumber;
  std::memcpy(&fpNumber, &originalNumber, sizeof(originalNumber));

  unsigned int expectedIndex8 =
      0b00110111; // Refer FpInternalManualTest.cpp Test Case 1
  unsigned int expectedIndex5 = 0b00111;

  createAndInsertFP(root.get(), fpNumber);
  rollUpCounts(root.get());

  ASSERT_NE(root->nodes[expectedIndex8], nullptr);
  EXPECT_EQ(root->populated[expectedIndex8], true);
  EXPECT_EQ(root->nodes[expectedIndex8]->counts[expectedIndex5], 1);
}

TEST_F(CreateAndInsertFPTest, NormalOperation_TestCase2) {

  double originalNumber = 85.125;
  uint64_t fpNumber;
  std::memcpy(&fpNumber, &originalNumber, sizeof(originalNumber));

  unsigned int expectedIndex8 =
      0b00011110; // Refer FpInternalManualTest.cpp Test Case 2
  unsigned int expectedIndex5 = 0b01010;

  createAndInsertFP(root.get(), fpNumber);
  rollUpCounts(root.get());

  ASSERT_NE(root->nodes[expectedIndex8], nullptr);
  EXPECT_EQ(root->populated[expectedIndex8], true);
  EXPECT_EQ(root->nodes[expectedIndex8]->counts[expectedIndex5], 1);
}

TEST_F(CreateAndInsertFPTest, NegativeExponent_TestCase1) {

  double originalNumber = 85.125;
  uint64_t fpNumber;
  std::memcpy(&fpNumber, &originalNumber, sizeof(originalNumber));
  fpNumber = 0b0011111111100000000000000000000000000000000000000000000000000000;

  unsigned int expectedIndex8 =
      0b01111100; // Refer FpInternalManualTest.cpp Test Case 2
  unsigned int expectedIndex5 = 0b00000;

  createAndInsertFP(root.get(), fpNumber);
  rollUpCounts(root.get());

  ASSERT_NE(root->nodes[expectedIndex8], nullptr);
  EXPECT_EQ(root->populated[expectedIndex8], true);
  EXPECT_EQ(root->nodes[expectedIndex8]->counts[expectedIndex5], 1);
}

TEST_F(CreateAndInsertFPTest, NegativeExponent_TestCase2) {

  uint64_t fpNumber;

  fpNumber = 0b0010000111000000000000000000000000000000000000000000000000000000;

  unsigned int expectedIndex8 =
      0b01000111; // Refer FpInternalManualTest.cpp Test Case 2
  unsigned int expectedIndex5 = 0b10001;

  createAndInsertFP(root.get(), fpNumber);
  rollUpCounts(root.get());

  ASSERT_NE(root->nodes[expectedIndex8], nullptr);
  EXPECT_EQ(root->populated[expectedIndex8], true);
  EXPECT_EQ(root->nodes[expectedIndex8]->counts[expectedIndex5], 1);
}

TEST_F(CreateAndInsertFPTest, NegativeExponent_TestCase3) {

  double originalNumber = 1.0;
  uint64_t fpNumber;

  setManualExponent(originalNumber, 1); // Orignal Number - 2.22507e-308
  std::memcpy(&fpNumber, &orignalNumber, sizeof(orignalNumber));


  unsigned int expectedIndex8 =
      0b01000011; // Refer FpInternalManualTest.cpp Test Case 2
  unsigned int expectedIndex5 = 0b11111;

  createAndInsertFP(root.get(), fpNumber);
  rollUpCounts(root.get());

  ASSERT_NE(root->nodes[expectedIndex8], nullptr);
  EXPECT_EQ(root->populated[expectedIndex8], true);
  EXPECT_EQ(root->nodes[expectedIndex8]->counts[expectedIndex5], 1);
}


// This test case should not pass as the exponent is set to 1023 indicating a
// special value
TEST_F(CreateAndInsertFPTest, EqualExponent_TestCase3) {

  uint64_t fpNumber = 1.0;

  setManualExponent(fpNumber, 1023); // Orignal Number - 2.22507e-308


  unsigned int expectedIndex8 =
      0b00111100; // Refer FpInternalManualTest.cpp Test Case 2
  unsigned int expectedIndex5 = 0b00000;

  createAndInsertFP(root.get(), fpNumber);
  rollUpCounts(root.get());

  EXPECT_EQ(root->nodes[expectedIndex8]->counts[expectedIndex5], 0);
  EXPECT_EQ(root->populated[expectedIndex8], false);
}

// THIS TEST CASE IS FOR TESTING APOLLO16 INTERNAL FILING

class CreateAndInsertFPTest_Apollo16 : public ::testing::Test {
protected:
  double orignalNumber;
  std::unique_ptr<TrieNode_16> root = CreateParentNode_16();

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

TEST_F(CreateAndInsertFPTest_Apollo16, NormalOperation_TestCase1) {

  double originalNumber = 1.113;
  uint64_t fpNumber;
  std::memcpy(&fpNumber, &originalNumber, sizeof(originalNumber));

  unsigned int expectedIndex8 =
      0b00110111; // Refer FpInternalManualTest.cpp Test Case 1
  unsigned int expectedLast8 = 0b00111011;

  createAndInsertFP16(root.get(), fpNumber);
  rollUpCounts(root.get());

  ASSERT_NE(root->nodes[expectedIndex8], nullptr);
  EXPECT_EQ(root->populated[expectedIndex8], true);
  EXPECT_EQ(root->nodes[expectedIndex8]->counts[expectedLast8], 1);
}

TEST_F(CreateAndInsertFPTest_Apollo16, NormalOperation_TestCase2) {

  double originalNumber = 85.125;
  uint64_t fpNumber;
  std::memcpy(&fpNumber, &originalNumber, sizeof(originalNumber));

  unsigned int expectedIndex8 =
      0b00011110; // Refer FpInternalManualTest.cpp Test Case 2
  unsigned int expectedIndex5 = 0b01010100;

  createAndInsertFP16(root.get(), fpNumber);
  rollUpCounts(root.get());

  ASSERT_NE(root->nodes[expectedIndex8], nullptr);
  EXPECT_EQ(root->populated[expectedIndex8], true);
  EXPECT_EQ(root->nodes[expectedIndex8]->counts[expectedIndex5], 1);
}

TEST_F(CreateAndInsertFPTest_Apollo16, NegativeExponent_TestCase1) {

  double originalNumber = 85.125;
  uint64_t fpNumber;
  std::memcpy(&fpNumber, &originalNumber, sizeof(originalNumber));
  fpNumber = 0b0011111111100000000000000000000000000000000000000000000000000000;

  unsigned int expectedIndex8 =
      0b01111100; // Refer FpInternalManualTest.cpp Test Case 2
  unsigned int expectedIndex5 = 0b00000000;

  createAndInsertFP16(root.get(), fpNumber);
  rollUpCounts(root.get());

  ASSERT_NE(root->nodes[expectedIndex8], nullptr);
  EXPECT_EQ(root->populated[expectedIndex8], true);
  EXPECT_EQ(root->nodes[expectedIndex8]->counts[expectedIndex5], 1);
}

TEST_F(CreateAndInsertFPTest_Apollo16, NegativeExponent_TestCase2) {

  uint64_t fpNumber;

  fpNumber = 0b0010000111000000000000000000000000000000000000000000000000000000;

  unsigned int expectedIndex8 =
      0b01000111; // Refer FpInternalManualTest.cpp Test Case 2
  unsigned int expectedIndex5 = 0b10001000;

  createAndInsertFP16(root.get(), fpNumber);
  rollUpCounts(root.get());

  ASSERT_NE(root->nodes[expectedIndex8], nullptr);
  EXPECT_EQ(root->populated[expectedIndex8], true);
  EXPECT_EQ(root->nodes[expectedIndex8]->counts[expectedIndex5], 1);
}

TEST_F(CreateAndInsertFPTest_Apollo16, NegativeExponent_TestCase3) {

  double originalNumber = 1.0;
  uint64_t fpNumber;

  setManualExponent(originalNumber, 1); // Orignal Number - 2.22507e-308
  std::memcpy(&fpNumber, &orignalNumber, sizeof(orignalNumber));


  unsigned int expectedIndex8 =
      0b01000011; // Refer FpInternalManualTest.cpp Test Case 2
  unsigned int expectedIndex5 = 0b11111010;

  createAndInsertFP16(root.get(), fpNumber);
  rollUpCounts(root.get());

  ASSERT_NE(root->nodes[expectedIndex8], nullptr);
  EXPECT_EQ(root->populated[expectedIndex8], true);
  EXPECT_EQ(root->nodes[expectedIndex8]->counts[expectedIndex5], 1);
}
