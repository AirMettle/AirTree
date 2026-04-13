#include <gtest/gtest.h>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/utils/Utils.hpp>
using namespace std;

constexpr double tolerance_13 = 5e-07;
constexpr double tolerance_16 = 6e-08;
constexpr double tolerance_20 = 4e-09;

const std::string dim1_file_path = "../../tests/TestData/dim1_vx.bin";
const std::string dim2_file_path = "../../tests/TestData/dim2_vy.bin";
const std::string dim3_file_path = "../../tests/TestData/dim3_vz.bin";
const std::string dim4_file_path = "../../tests/TestData/dim4_rho.bin";
const std::string dim5_file_path = "../../tests/TestData/dim5_e.bin";
const std::string dim6_file_path = "../../tests/TestData/dim6_x.bin";
const std::string dim7_file_path = "../../tests/TestData/dim7_y.bin";
const std::string dim8_file_path = "../../tests/TestData/dim8_z.bin";


const std::string int32_dim1_file_path =
    "../../tests/TestData/int32/dim1_element_id.bin";
const std::string int32_dim2_file_path =
    "../../tests/TestData/int32/dim2_vertex_id.bin";


const std::vector<double> dim1_data = readBinaryFile(dim1_file_path);
const std::vector<double> dim2_data = readBinaryFile(dim2_file_path);
const std::vector<double> dim3_data = readBinaryFile(dim3_file_path);
const std::vector<double> dim4_data = readBinaryFile(dim4_file_path);
const std::vector<double> dim5_data = readBinaryFile(dim5_file_path);
const std::vector<double> dim6_data = readBinaryFile(dim6_file_path);
const std::vector<double> dim7_data = readBinaryFile(dim7_file_path);
const std::vector<double> dim8_data = readBinaryFile(dim8_file_path);

const std::vector<int32_t> int32_dim1_data =
    readBinaryFileInt32(int32_dim1_file_path);
const std::vector<int32_t> int32_dim2_data =
    readBinaryFileInt32(int32_dim2_file_path);

TEST(ReconstructionTestManual, TestCase1) {
  double orignalNumber = 1.113;
  uint64_t fpNumber;
  std::memcpy(&fpNumber, &orignalNumber, sizeof(orignalNumber));

  unsigned int internalRep13 = createInternal13Bit(fpNumber, true);
  double reconstructedNumber13 = reConstruct<double>(internalRep13, 13);

  unsigned int internalRep16 = createInternal16Bit(fpNumber, true);
  double reconstructedNumber16 = reConstruct<double>(internalRep16, 16);

  unsigned int internalRep20 = createInternal20Bit(fpNumber, true);
  double reconstructedNumber20 = reConstruct<double>(internalRep20, 20);


  EXPECT_NEAR(reconstructedNumber13, 1.11279, 1e-5);
  EXPECT_NEAR(reconstructedNumber16, 1.11298, 1e-5);
  EXPECT_NEAR(reconstructedNumber20, 1.113, 1e-5);
}


TEST(ReconstructionTestManual, TestCase2) {
  double orignalNumber = -1.4;
  uint64_t fpNumber;

  std::memcpy(&fpNumber, &orignalNumber, sizeof(orignalNumber));
  unsigned int internalRep13 = createInternal13Bit(fpNumber, true);
  double reconstructedNumber13 = reConstruct<double>(internalRep13, 13);

  unsigned int internalRep16 = createInternal16Bit(fpNumber, true);
  double reconstructedNumber16 = reConstruct<double>(internalRep16, 16);

  unsigned int internalRep20 = createInternal20Bit(fpNumber, true);
  double reconstructedNumber20 = reConstruct<double>(internalRep20, 20);


  EXPECT_NEAR(reconstructedNumber13, -1.39844, 1e-5);
  EXPECT_NEAR(reconstructedNumber16, -1.3999, 1e-5);
  EXPECT_NEAR(reconstructedNumber20, -1.39999, 1e-5);
}


TEST(ReconstructionTestManual, TestCase3) {
  double orignalNumber = -0.00141411414;
  uint64_t fpNumber;

  std::memcpy(&fpNumber, &orignalNumber, sizeof(orignalNumber));
  unsigned int internalRep13 = createInternal13Bit(fpNumber, true);
  double reconstructedNumber13 = reConstruct<double>(internalRep13, 13);

  unsigned int internalRep16 = createInternal16Bit(fpNumber, true);
  double reconstructedNumber16 = reConstruct<double>(internalRep16, 16);

  unsigned int internalRep20 = createInternal20Bit(fpNumber, true);
  double reconstructedNumber20 = reConstruct<double>(internalRep20, 20);


  EXPECT_NEAR(reconstructedNumber13, -0.00140381, 1e-5);
  EXPECT_NEAR(reconstructedNumber16, -0.00141144, 1e-5);
  EXPECT_NEAR(reconstructedNumber20, -0.00141382, 1e-5);
}

// Test case for exact power of 2: 1.0
// This tests the saturation fix - when maskedNumber=0, should reconstruct
// exactly
TEST(ReconstructionTestManual, TestCase_PowerOfTwo_One) {
  double orignalNumber = 1.0;
  uint64_t fpNumber;

  std::memcpy(&fpNumber, &orignalNumber, sizeof(orignalNumber));
  unsigned int internalRep13 = createInternal13Bit(fpNumber, true);
  double reconstructedNumber13 = reConstruct<double>(internalRep13, 13);

  unsigned int internalRep16 = createInternal16Bit(fpNumber, true);
  double reconstructedNumber16 = reConstruct<double>(internalRep16, 16);

  unsigned int internalRep20 = createInternal20Bit(fpNumber, true);
  double reconstructedNumber20 = reConstruct<double>(internalRep20, 20);

  // Exact powers of 2 should reconstruct exactly with saturation fix
  EXPECT_DOUBLE_EQ(reconstructedNumber13, 1.0);
  EXPECT_DOUBLE_EQ(reconstructedNumber16, 1.0);
  EXPECT_DOUBLE_EQ(reconstructedNumber20, 1.0);
}

// Test case for exact power of 2: 0.5
// This tests the saturation fix - when maskedNumber=0, should reconstruct
// exactly
TEST(ReconstructionTestManual, TestCase_PowerOfTwo_Half) {
  double orignalNumber = 0.5;
  uint64_t fpNumber;

  std::memcpy(&fpNumber, &orignalNumber, sizeof(orignalNumber));
  unsigned int internalRep13 = createInternal13Bit(fpNumber, true);
  double reconstructedNumber13 = reConstruct<double>(internalRep13, 13);

  unsigned int internalRep16 = createInternal16Bit(fpNumber, true);
  double reconstructedNumber16 = reConstruct<double>(internalRep16, 16);

  unsigned int internalRep20 = createInternal20Bit(fpNumber, true);
  double reconstructedNumber20 = reConstruct<double>(internalRep20, 20);

  // Exact powers of 2 should reconstruct exactly with saturation fix
  EXPECT_DOUBLE_EQ(reconstructedNumber13, 0.5);
  EXPECT_DOUBLE_EQ(reconstructedNumber16, 0.5);
  EXPECT_DOUBLE_EQ(reconstructedNumber20, 0.5);
}

// Test case for negative power of 2: -1.0
// This tests the saturation fix with negative values
TEST(ReconstructionTestManual, TestCase_PowerOfTwo_NegativeOne) {
  double orignalNumber = -1.0;
  uint64_t fpNumber;

  std::memcpy(&fpNumber, &orignalNumber, sizeof(orignalNumber));
  unsigned int internalRep13 = createInternal13Bit(fpNumber, true);
  double reconstructedNumber13 = reConstruct<double>(internalRep13, 13);

  unsigned int internalRep16 = createInternal16Bit(fpNumber, true);
  double reconstructedNumber16 = reConstruct<double>(internalRep16, 16);

  unsigned int internalRep20 = createInternal20Bit(fpNumber, true);
  double reconstructedNumber20 = reConstruct<double>(internalRep20, 20);

  // Exact powers of 2 should reconstruct exactly with saturation fix
  EXPECT_DOUBLE_EQ(reconstructedNumber13, -1.0);
  EXPECT_DOUBLE_EQ(reconstructedNumber16, -1.0);
  EXPECT_DOUBLE_EQ(reconstructedNumber20, -1.0);
}

// Test case for negative power of 2: -0.5
// This tests the saturation fix with negative values
TEST(ReconstructionTestManual, TestCase_PowerOfTwo_NegativeHalf) {
  double orignalNumber = -0.5;
  uint64_t fpNumber;

  std::memcpy(&fpNumber, &orignalNumber, sizeof(orignalNumber));
  unsigned int internalRep13 = createInternal13Bit(fpNumber, true);
  double reconstructedNumber13 = reConstruct<double>(internalRep13, 13);

  unsigned int internalRep16 = createInternal16Bit(fpNumber, true);
  double reconstructedNumber16 = reConstruct<double>(internalRep16, 16);

  unsigned int internalRep20 = createInternal20Bit(fpNumber, true);
  double reconstructedNumber20 = reConstruct<double>(internalRep20, 20);

  // Exact powers of 2 should reconstruct exactly with saturation fix
  EXPECT_DOUBLE_EQ(reconstructedNumber13, -0.5);
  EXPECT_DOUBLE_EQ(reconstructedNumber16, -0.5);
  EXPECT_DOUBLE_EQ(reconstructedNumber20, -0.5);
}

// Test case for float (32-bit): 1.0f
// This tests the saturation fix for 32-bit floats
TEST(ReconstructionTestManual, TestCase_PowerOfTwo_Float_One) {
  float orignalNumber = 1.0f;
  uint32_t fpNumber;

  std::memcpy(&fpNumber, &orignalNumber, sizeof(orignalNumber));
  unsigned int internalRep13 = createInternal13Bit_32(fpNumber, true);
  float reconstructedNumber13 = reConstruct<float>(internalRep13, 13);

  unsigned int internalRep16 = createInternal16Bit_32(fpNumber, true);
  float reconstructedNumber16 = reConstruct<float>(internalRep16, 16);

  unsigned int internalRep20 = createInternal20Bit_32(fpNumber, true);
  float reconstructedNumber20 = reConstruct<float>(internalRep20, 20);

  // Exact powers of 2 should reconstruct exactly with saturation fix
  EXPECT_FLOAT_EQ(reconstructedNumber13, 1.0f);
  EXPECT_FLOAT_EQ(reconstructedNumber16, 1.0f);
  EXPECT_FLOAT_EQ(reconstructedNumber20, 1.0f);
}

// Test case for float (32-bit): 0.5f
// This tests the saturation fix for 32-bit floats
TEST(ReconstructionTestManual, TestCase_PowerOfTwo_Float_Half) {
  float orignalNumber = 0.5f;
  uint32_t fpNumber;

  std::memcpy(&fpNumber, &orignalNumber, sizeof(orignalNumber));
  unsigned int internalRep13 = createInternal13Bit_32(fpNumber, true);
  float reconstructedNumber13 = reConstruct<float>(internalRep13, 13);

  unsigned int internalRep16 = createInternal16Bit_32(fpNumber, true);
  float reconstructedNumber16 = reConstruct<float>(internalRep16, 16);

  unsigned int internalRep20 = createInternal20Bit_32(fpNumber, true);
  float reconstructedNumber20 = reConstruct<float>(internalRep20, 20);

  // Exact powers of 2 should reconstruct exactly with saturation fix
  EXPECT_FLOAT_EQ(reconstructedNumber13, 0.5f);
  EXPECT_FLOAT_EQ(reconstructedNumber16, 0.5f);
  EXPECT_FLOAT_EQ(reconstructedNumber20, 0.5f);
}

// Test case for negative float (32-bit): -1.0f
// This tests the saturation fix for negative 32-bit floats
TEST(ReconstructionTestManual, TestCase_PowerOfTwo_Float_NegativeOne) {
  float orignalNumber = -1.0f;
  uint32_t fpNumber;

  std::memcpy(&fpNumber, &orignalNumber, sizeof(orignalNumber));
  unsigned int internalRep13 = createInternal13Bit_32(fpNumber, true);
  float reconstructedNumber13 = reConstruct<float>(internalRep13, 13);

  unsigned int internalRep16 = createInternal16Bit_32(fpNumber, true);
  float reconstructedNumber16 = reConstruct<float>(internalRep16, 16);

  unsigned int internalRep20 = createInternal20Bit_32(fpNumber, true);
  float reconstructedNumber20 = reConstruct<float>(internalRep20, 20);

  // Exact powers of 2 should reconstruct exactly with saturation fix
  EXPECT_FLOAT_EQ(reconstructedNumber13, -1.0f);
  EXPECT_FLOAT_EQ(reconstructedNumber16, -1.0f);
  EXPECT_FLOAT_EQ(reconstructedNumber20, -1.0f);
}

// Test case for negative float (32-bit): -0.5f
// This tests the saturation fix for negative 32-bit floats
TEST(ReconstructionTestManual, TestCase_PowerOfTwo_Float_NegativeHalf) {
  float orignalNumber = -0.5f;
  uint32_t fpNumber;

  std::memcpy(&fpNumber, &orignalNumber, sizeof(orignalNumber));
  unsigned int internalRep13 = createInternal13Bit_32(fpNumber, true);
  float reconstructedNumber13 = reConstruct<float>(internalRep13, 13);

  unsigned int internalRep16 = createInternal16Bit_32(fpNumber, true);
  float reconstructedNumber16 = reConstruct<float>(internalRep16, 16);

  unsigned int internalRep20 = createInternal20Bit_32(fpNumber, true);
  float reconstructedNumber20 = reConstruct<float>(internalRep20, 20);

  // Exact powers of 2 should reconstruct exactly with saturation fix
  EXPECT_FLOAT_EQ(reconstructedNumber13, -0.5f);
  EXPECT_FLOAT_EQ(reconstructedNumber16, -0.5f);
  EXPECT_FLOAT_EQ(reconstructedNumber20, -0.5f);
}

TEST(ReconstructionTestForFile, TestCase_vx) {
  SpecialCounts specialCounts;
  for (double originalNumber : dim1_data) {
    uint64_t fpNumber;
    std::memcpy(&fpNumber, &originalNumber, sizeof(originalNumber));


    if (!isSpecialCase(fpNumber, specialCounts)) {
      unsigned int internalRep13 = createInternal13Bit(fpNumber, true);
      double reconstructedNumber13 = reConstruct<double>(internalRep13, 13);

      unsigned int internalRep16 = createInternal16Bit(fpNumber, true);
      double reconstructedNumber16 = reConstruct<double>(internalRep16, 16);

      unsigned int internalRep20 = createInternal20Bit(fpNumber, true);
      double reconstructedNumber20 = reConstruct<double>(internalRep20, 20);

      EXPECT_NEAR(reconstructedNumber13, originalNumber, tolerance_13);


      EXPECT_NEAR(reconstructedNumber16, originalNumber, tolerance_16);

      EXPECT_NEAR(reconstructedNumber20, originalNumber, tolerance_20);
    }
  }
}

TEST(ReconstructionTestForFile, TestCase_vy) {
  SpecialCounts specialCounts;
  for (double originalNumber : dim2_data) {
    uint64_t fpNumber;
    std::memcpy(&fpNumber, &originalNumber, sizeof(originalNumber));
    if (!isSpecialCase(fpNumber, specialCounts)) {
      unsigned int internalRep13 = createInternal13Bit(fpNumber, true);
      double reconstructedNumber13 = reConstruct<double>(internalRep13, 13);

      unsigned int internalRep16 = createInternal16Bit(fpNumber, true);
      double reconstructedNumber16 = reConstruct<double>(internalRep16, 16);

      unsigned int internalRep20 = createInternal20Bit(fpNumber, true);
      double reconstructedNumber20 = reConstruct<double>(internalRep20, 20);

      EXPECT_NEAR(reconstructedNumber13, originalNumber, tolerance_13);


      EXPECT_NEAR(reconstructedNumber16, originalNumber, tolerance_16);

      EXPECT_NEAR(reconstructedNumber20, originalNumber, tolerance_20);
    }
  }
}

TEST(ReconstructionTestForFile, TestCase_vz) {
  SpecialCounts specialCounts;
  for (double originalNumber : dim3_data) {
    uint64_t fpNumber;
    std::memcpy(&fpNumber, &originalNumber, sizeof(originalNumber));
    if (!isSpecialCase(fpNumber, specialCounts)) {
      unsigned int internalRep13 = createInternal13Bit(fpNumber, true);
      double reconstructedNumber13 = reConstruct<double>(internalRep13, 13);

      unsigned int internalRep16 = createInternal16Bit(fpNumber, true);
      double reconstructedNumber16 = reConstruct<double>(internalRep16, 16);

      unsigned int internalRep20 = createInternal20Bit(fpNumber, true);
      double reconstructedNumber20 = reConstruct<double>(internalRep20, 20);

      EXPECT_NEAR(reconstructedNumber13, originalNumber, tolerance_13);


      EXPECT_NEAR(reconstructedNumber16, originalNumber, tolerance_16);

      EXPECT_NEAR(reconstructedNumber20, originalNumber, tolerance_20);
    }
  }
}

TEST(ReconstructionTestManual, TestCaseFloat) {
  float orignalNumber = 1.113;
  uint32_t fpNumber;
  std::memcpy(&fpNumber, &orignalNumber, sizeof(orignalNumber));

  unsigned int internalRep = createInternal13Bit_32(fpNumber, true);
  float reconstructedNumber13 = reConstruct<float>(internalRep, 13);

  internalRep = createInternal16Bit_32(fpNumber, true);
  float reconstructedNumber16 = reConstruct<float>(internalRep, 16);

  internalRep = createInternal20Bit_32(fpNumber, true);
  float reconstructedNumber20 = reConstruct<float>(internalRep, 20);

  double tolerance_13_int32 =
      std::max(orignalNumber * 0.06, 1.0); // 6% for 13-bit, minimum 1
  double tolerance_16_int32 =
      std::max(orignalNumber * 0.008, 0.5); // 0.8% for 16-bit, minimum 0.5
  double tolerance_20_int32 =
      std::max(orignalNumber * 0.0005, 0.1); // 0.05% for 20-bit, minimum 0.1

  EXPECT_NEAR(reconstructedNumber13, 1.11279, tolerance_13_int32);
  EXPECT_NEAR(reconstructedNumber16, 1.11298, tolerance_16_int32);
  EXPECT_NEAR(reconstructedNumber20, 1.113, tolerance_20_int32);
}

TEST(ReconstructionTestManual, TestCaseInt32) {
  int orignalNumber = 45;
  double floatValue = int32_to_double(orignalNumber);
  uint64_t fpNumber;
  std::memcpy(&fpNumber, &floatValue, sizeof(floatValue));

  unsigned int internalRep = createInternal13Bit(fpNumber, true);
  double reconstructedNumber13 = reConstruct<double>(internalRep, 13);

  internalRep = createInternal16Bit(fpNumber, true);
  double reconstructedNumber16 = reConstruct<double>(internalRep, 16);

  internalRep = createInternal20Bit(fpNumber, true);
  double reconstructedNumber20 = reConstruct<double>(internalRep, 20);

  double tolerance_13_int32 =
      std::max(floatValue * 0.06, 1.0); // 6% for 13-bit, minimum 1
  double tolerance_16_int32 =
      std::max(floatValue * 0.008, 0.5); // 0.8% for 16-bit, minimum 0.5
  double tolerance_20_int32 =
      std::max(floatValue * 0.0005, 0.1); // 0.05% for 20-bit, minimum 0.1


  EXPECT_NEAR(reconstructedNumber13, 45, tolerance_13_int32);
  EXPECT_NEAR(reconstructedNumber16, 45, tolerance_16_int32);
  EXPECT_NEAR(reconstructedNumber20, 45, tolerance_20_int32);
}

TEST(ReconstructionTestManual, TestCaseInt64) {
  long orignalNumber = 1241;
  double floatValue = int64_to_double(orignalNumber);
  uint64_t fpNumber;
  std::memcpy(&fpNumber, &floatValue, sizeof(floatValue));

  unsigned int internalRep = createInternal13Bit(fpNumber, true);
  double reconstructedNumber13 = reConstruct<double>(internalRep, 13);

  internalRep = createInternal16Bit(fpNumber, true);
  double reconstructedNumber16 = reConstruct<double>(internalRep, 16);

  internalRep = createInternal20Bit(fpNumber, true);
  double reconstructedNumber20 = reConstruct<double>(internalRep, 20);

  double tolerance_13_int32 =
      std::max(floatValue * 0.06, 1.0); // 6% for 13-bit, minimum 1
  double tolerance_16_int32 =
      std::max(floatValue * 0.008, 0.5); // 0.8% for 16-bit, minimum 0.5
  double tolerance_20_int32 =
      std::max(floatValue * 0.0005, 0.1); // 0.05% for 20-bit, minimum 0.1

  EXPECT_NEAR(reconstructedNumber13, 1216, tolerance_13_int32);
  EXPECT_NEAR(reconstructedNumber16, 1240, tolerance_16_int32);
  EXPECT_NEAR(reconstructedNumber20, 1241, tolerance_20_int32);
}


TEST(ReconstructionTestForFile, TestCase_int32_element_id) {
  SpecialCounts specialCounts;

  for (int32_t originalNumber : int32_dim1_data) {
    double floatValue = int32_to_double(originalNumber);
    uint64_t fpNumber;
    std::memcpy(&fpNumber, &floatValue, sizeof(floatValue));

    if (!isSpecialCase(fpNumber, specialCounts)) {
      unsigned int internalRep13 = createInternal13Bit(fpNumber, true);
      double reconstructedNumber13 = reConstruct<double>(internalRep13, 13);

      unsigned int internalRep16 = createInternal16Bit(fpNumber, true);
      double reconstructedNumber16 = reConstruct<double>(internalRep16, 16);

      unsigned int internalRep20 = createInternal20Bit(fpNumber, true);
      double reconstructedNumber20 = reConstruct<double>(internalRep20, 20);

      double tolerance_13_int32 = std::max(floatValue * 0.06, 1.0);
      double tolerance_16_int32 = std::max(floatValue * 0.008, 0.5);
      double tolerance_20_int32 = std::max(floatValue * 0.0005, 0.1);

      // Keep the original assertions
      EXPECT_NEAR(reconstructedNumber13, floatValue, tolerance_13_int32);
      EXPECT_NEAR(reconstructedNumber16, floatValue, tolerance_16_int32);
      EXPECT_NEAR(reconstructedNumber20, floatValue, tolerance_20_int32);
    }
  }
}

TEST(ReconstructionTestForFile, TestCase_int32_vertex_id) {
  SpecialCounts specialCounts;

  for (int32_t originalNumber : int32_dim2_data) {
    double floatValue = int32_to_double(originalNumber);

    uint64_t fpNumber;
    std::memcpy(&fpNumber, &floatValue, sizeof(floatValue));

    if (!isSpecialCase(fpNumber, specialCounts)) {
      unsigned int internalRep13 = createInternal13Bit(fpNumber, true);
      double reconstructedNumber13 = reConstruct<double>(internalRep13, 13);

      unsigned int internalRep16 = createInternal16Bit(fpNumber, true);
      double reconstructedNumber16 = reConstruct<double>(internalRep16, 16);

      unsigned int internalRep20 = createInternal20Bit(fpNumber, true);
      double reconstructedNumber20 = reConstruct<double>(internalRep20, 20);


      double tolerance_13_int32 =
          std::max(floatValue * 0.06, 1.0); // 6% for 13-bit, minimum 1
      double tolerance_16_int32 =
          std::max(floatValue * 0.008, 0.5); // 0.8% for 16-bit, minimum 0.5
      double tolerance_20_int32 =
          std::max(floatValue * 0.0005, 0.1); // 0.05% for 20-bit, minimum 0.1

      EXPECT_NEAR(reconstructedNumber13, floatValue, tolerance_13_int32);


      EXPECT_NEAR(reconstructedNumber16, floatValue, tolerance_16_int32);

      EXPECT_NEAR(reconstructedNumber20, floatValue, tolerance_20_int32);
    }
  }
}


// Intresting Test Case

TEST(ReconstructionTestForFile, TestCase_rho) {
  GTEST_SKIP();
  SpecialCounts specialCounts;
  for (double originalNumber : dim4_data) {
    uint64_t fpNumber;
    std::memcpy(&fpNumber, &originalNumber, sizeof(originalNumber));
    if (!isSpecialCase(fpNumber, specialCounts)) {
      unsigned int internalRep13 = createInternal13Bit(fpNumber, true);
      double reconstructedNumber13 = reConstruct<double>(internalRep13, 13);

      unsigned int internalRep16 = createInternal16Bit(fpNumber, true);
      double reconstructedNumber16 = reConstruct<double>(internalRep16, 16);

      unsigned int internalRep20 = createInternal20Bit(fpNumber, true);
      double reconstructedNumber20 = reConstruct<double>(internalRep20, 20);

      EXPECT_NEAR(reconstructedNumber13, originalNumber, tolerance_13)
          << "13-bit reconstruction failed for original value: "
          << originalNumber;


      EXPECT_NEAR(reconstructedNumber16, originalNumber, tolerance_16)
          << "16-bit reconstruction failed for original value: "
          << originalNumber;

      EXPECT_NEAR(reconstructedNumber20, originalNumber, tolerance_20)
          << "20-bit reconstruction failed for original value: "
          << originalNumber;
    }
  }
}

// Very Intresting Test Case
TEST(ReconstructionTestForFile, TestCase_e) {
  GTEST_SKIP();
  SpecialCounts specialCounts;
  for (double originalNumber : dim5_data) {
    uint64_t fpNumber;
    std::memcpy(&fpNumber, &originalNumber, sizeof(originalNumber));
    if (!isSpecialCase(fpNumber, specialCounts)) {
      unsigned int internalRep13 = createInternal13Bit(fpNumber, true);
      double reconstructedNumber13 = reConstruct<double>(internalRep13, 13);

      unsigned int internalRep16 = createInternal16Bit(fpNumber, true);
      double reconstructedNumber16 = reConstruct<double>(internalRep16, 16);

      unsigned int internalRep20 = createInternal20Bit(fpNumber, true);
      double reconstructedNumber20 = reConstruct<double>(internalRep20, 20);

      EXPECT_NEAR(reconstructedNumber13, originalNumber, tolerance_13)
          << "13-bit reconstruction failed for original value: "
          << originalNumber;


      EXPECT_NEAR(reconstructedNumber16, originalNumber, tolerance_16)
          << "16-bit reconstruction failed for original value: "
          << originalNumber;

      EXPECT_NEAR(reconstructedNumber20, originalNumber, tolerance_20)
          << "20-bit reconstruction failed for original value: "
          << originalNumber;
    }
  }
}


// To enable the test case , change 0 to 1
#if 0
TEST(ReconstructionTestForFile, DumpErrorsToFileWithSummaryforInt32) {
  SpecialCounts specialCounts;

  // Open files to store absolute differences
  std::ofstream diff13File("diff_13_bits.txt");
  std::ofstream diff16File("diff_16_bits.txt");
  std::ofstream diff20File("diff_20_bits.txt");

  std::ofstream summaryFile("difference_summary.txt"); // Summary file

  // Check if files opened successfully
  ASSERT_TRUE(diff13File.is_open()) << "Failed to open diff_13_bits.txt";
  ASSERT_TRUE(diff16File.is_open()) << "Failed to open diff_16_bits.txt";
  ASSERT_TRUE(diff20File.is_open()) << "Failed to open diff_20_bits.txt";
  ASSERT_TRUE(summaryFile.is_open()) << "Failed to open difference_summary.txt";

  // Variables to track min, max, and total error for each bit level
  double minDiff13 = std::numeric_limits<double>::max();
  double maxDiff13 = std::numeric_limits<double>::lowest();
  double totalDiff13 = 0;
  int32_t maxDiff13_original = 0;  // Store original number for maxDiff13

  double minDiff16 = std::numeric_limits<double>::max();
  double maxDiff16 = std::numeric_limits<double>::lowest();
  double totalDiff16 = 0;
  int32_t maxDiff16_original = 0;  // Store original number for maxDiff16

  double minDiff20 = std::numeric_limits<double>::max();
  double maxDiff20 = std::numeric_limits<double>::lowest();
  double totalDiff20 = 0;
  int32_t maxDiff20_original = 0;  // Store original number for maxDiff20

  size_t count = 0;

  for (int32_t originalNumber : int32_dim2_data) {
    float floatValue = int32_to_float(originalNumber);

    uint32_t fpNumber;
    std::memcpy(&fpNumber, &floatValue, sizeof(floatValue));

    if (!isSpecialCase_32(fpNumber, specialCounts)) {
      unsigned int internalRep13 = createInternal13Bit_32(fpNumber, true);
      float reconstructedNumber13 = reConstruct<float>(internalRep13, 13);

      unsigned int internalRep16 = createInternal16Bit_32(fpNumber, true);
      float reconstructedNumber16 = reConstruct<float>(internalRep16, 16);

      unsigned int internalRep20 = createInternal20Bit_32(fpNumber, true);
      float reconstructedNumber20 = reConstruct<float>(internalRep20, 20);

      // Compute absolute differences
      double diff13 = std::abs(originalNumber - reconstructedNumber13);
      double diff16 = std::abs(originalNumber - reconstructedNumber16);
      double diff20 = std::abs(originalNumber - reconstructedNumber20);

      // Update min, max, and total differences
      minDiff13 = std::min(minDiff13, diff13);
      if (diff13 > maxDiff13) {
        maxDiff13 = diff13;
        maxDiff13_original = originalNumber;  // Store original value for max diff
      }
      totalDiff13 += diff13;

      minDiff16 = std::min(minDiff16, diff16);
      if (diff16 > maxDiff16) {
        maxDiff16 = diff16;
        maxDiff16_original = originalNumber;  // Store original value for max diff
      }
      totalDiff16 += diff16;

      minDiff20 = std::min(minDiff20, diff20);
      if (diff20 > maxDiff20) {
        maxDiff20 = diff20;
        maxDiff20_original = originalNumber;  // Store original value for max diff
      }
      totalDiff20 += diff20;

      // Write absolute differences to corresponding files
      diff13File << diff13 << "\n";
      diff16File << diff16 << "\n";
      diff20File << diff20 << "\n";

      count++;
    }
  }

  // Compute averages
  double avgDiff13 = (count > 0) ? totalDiff13 / count : 0;
  double avgDiff16 = (count > 0) ? totalDiff16 / count : 0;
  double avgDiff20 = (count > 0) ? totalDiff20 / count : 0;

  // Write summary to file
  summaryFile << "Difference Summary:\n";
  summaryFile << "13-bit:\n";
  summaryFile << "  Min: " << minDiff13 << "\n";
  summaryFile << "  Max: " << maxDiff13 << " (Original Value: " << maxDiff13_original << ")\n";
  summaryFile << "  Avg: " << avgDiff13 << "\n";

  summaryFile << "16-bit:\n";
  summaryFile << "  Min: " << minDiff16 << "\n";
  summaryFile << "  Max: " << maxDiff16 << " (Original Value: " << maxDiff16_original << ")\n";
  summaryFile << "  Avg: " << avgDiff16 << "\n";

  summaryFile << "20-bit:\n";
  summaryFile << "  Min: " << minDiff20 << "\n";
  summaryFile << "  Max: " << maxDiff20 << " (Original Value: " << maxDiff20_original << ")\n";
  summaryFile << "  Avg: " << avgDiff20 << "\n";

  // Close files
  diff13File.close();
  diff16File.close();
  diff20File.close();
  summaryFile.close();
}



TEST(ReconstructionTestForFile, DumpErrorsToFileWithSummary) {
  SpecialCounts specialCounts;

  // Open files to store absolute differences
  std::ofstream diff13File("diff_13_bits.txt");
  std::ofstream diff16File("diff_16_bits.txt");
  std::ofstream diff20File("diff_20_bits.txt");

  std::ofstream summaryFile("difference_summary.txt"); // Summary file

  // Check if files opened successfully
  ASSERT_TRUE(diff13File.is_open()) << "Failed to open diff_13_bits.txt";
  ASSERT_TRUE(diff16File.is_open()) << "Failed to open diff_16_bits.txt";
  ASSERT_TRUE(diff20File.is_open()) << "Failed to open diff_20_bits.txt";
  ASSERT_TRUE(summaryFile.is_open()) << "Failed to open difference_summary.txt";

  // Variables to track min, max, and total error for each bit level
  double minDiff13 = std::numeric_limits<double>::max();
  double maxDiff13 = std::numeric_limits<double>::lowest();
  double totalDiff13 = 0;

  double minDiff16 = std::numeric_limits<double>::max();
  double maxDiff16 = std::numeric_limits<double>::lowest();
  double totalDiff16 = 0;

  double minDiff20 = std::numeric_limits<double>::max();
  double maxDiff20 = std::numeric_limits<double>::lowest();
  double totalDiff20 = 0;

  size_t count = 0;

  for (double originalNumber : dim4_data) {
    uint64_t fpNumber;
    std::memcpy(&fpNumber, &originalNumber, sizeof(originalNumber));

    if (!isSpecialCase(fpNumber, specialCounts)) {

      unsigned int internalRep13 = createInternal13Bit(fpNumber, true);
      double reconstructedNumber13 = reConstruct<double>(internalRep13, 13);

      unsigned int internalRep16 = createInternal16Bit(fpNumber, true);
      double reconstructedNumber16 = reConstruct<double>(internalRep16, 16);
    
      unsigned int internalRep20 = createInternal20Bit(fpNumber, true);
      double reconstructedNumber20 = reConstruct<double>(internalRep20, 20);

      // Compute absolute differences
      double diff13 = std::abs(originalNumber - reconstructedNumber13);
      double diff16 = std::abs(originalNumber - reconstructedNumber16);
      double diff20 = std::abs(originalNumber - reconstructedNumber20);

      // Update min, max, and total differences
      minDiff13 = std::min(minDiff13, diff13);
      maxDiff13 = std::max(maxDiff13, diff13);
      totalDiff13 += diff13;

      minDiff16 = std::min(minDiff16, diff16);
      maxDiff16 = std::max(maxDiff16, diff16);
      totalDiff16 += diff16;

      minDiff20 = std::min(minDiff20, diff20);
      maxDiff20 = std::max(maxDiff20, diff20);
      totalDiff20 += diff20;

      // Write absolute differences to corresponding files
      diff13File << diff13 << "\n";
      diff16File << diff16 << "\n";
      diff20File << diff20 << "\n";

      count++;
    }
  }

  // Compute averages
  double avgDiff13 = (count > 0) ? totalDiff13 / count : 0;
  double avgDiff16 = (count > 0) ? totalDiff16 / count : 0;
  double avgDiff20 = (count > 0) ? totalDiff20 / count : 0;

  // Write summary to file
  summaryFile << "Difference Summary:\n";
  summaryFile << "13-bit:\n";
  summaryFile << "  Min: " << minDiff13 << "\n";
  summaryFile << "  Max: " << maxDiff13 << "\n";
  summaryFile << "  Avg: " << avgDiff13 << "\n";

  summaryFile << "16-bit:\n";
  summaryFile << "  Min: " << minDiff16 << "\n";
  summaryFile << "  Max: " << maxDiff16 << "\n";
  summaryFile << "  Avg: " << avgDiff16 << "\n";

  summaryFile << "20-bit:\n";
  summaryFile << "  Min: " << minDiff20 << "\n";
  summaryFile << "  Max: " << maxDiff20 << "\n";
  summaryFile << "  Avg: " << avgDiff20 << "\n";

  // Close files
  diff13File.close();
  diff16File.close();
  diff20File.close();
  summaryFile.close();
}

TEST(ReconstructionTest, FileBasedTest) {
  SpecialCounts specialCounts;
  // Open files to store original and reconstructed values
  std::ofstream originalFile("original_vals.txt");
  std::ofstream reconstructed13File("reconstructed_13_vals.txt");
  std::ofstream reconstructed16File("reconstructed_16_vals.txt");
  std::ofstream reconstructed20File("reconstructed_20_vals.txt");

  // Check if files opened successfully
  ASSERT_TRUE(originalFile.is_open()) << "Failed to open original_vals.txt";
  ASSERT_TRUE(reconstructed13File.is_open())
      << "Failed to open reconstructed_13_vals.txt";
  ASSERT_TRUE(reconstructed16File.is_open())
      << "Failed to open reconstructed_16_vals.txt";
  ASSERT_TRUE(reconstructed20File.is_open())
      << "Failed to open reconstructed_20_vals.txt";

  for (double originalNumber : dim4_data) {
    uint64_t fpNumber;
    std::memcpy(&fpNumber, &originalNumber, sizeof(originalNumber));

    if (!isSpecialCase(fpNumber, specialCounts)) {

      unsigned int internalRep13 = createInternal13Bit(fpNumber, true);
      double reconstructedNumber13 = reConstruct<double>(internalRep13, 13);

      unsigned int internalRep16 = createInternal16Bit(fpNumber, true);
      double reconstructedNumber16 = reConstruct<double>(internalRep16, 16);
    
      unsigned int internalRep20 = createInternal20Bit(fpNumber, true);
      double reconstructedNumber20 = reConstruct<double>(internalRep20, 20);

      // Write values to corresponding files
      originalFile << originalNumber << "\n";
      reconstructed13File << reconstructedNumber13 << "\n";
      reconstructed16File << reconstructedNumber16 << "\n";
      reconstructed20File << reconstructedNumber20 << "\n";
    }
  }

  // Close files
  originalFile.close();
  reconstructed13File.close();
  reconstructed16File.close();
  reconstructed20File.close();
}
#endif
