// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/merge/AirTreeMerge.hpp>
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <airtree/core/utils/Utils.hpp>
#include <airtree/core/AirTreeCore_internal.hpp>


const std::string dim1_file_path = "../../tests/TestData/dim1_vx.bin";
const std::string dim2_file_path = "../../tests/TestData/dim2_vy.bin";
const std::string dim3_file_path = "../../tests/TestData/dim3_vz.bin";
const std::string dim4_file_path = "../../tests/TestData/dim4_rho.bin";
const std::vector<double> dim1_data = readBinaryFile(dim1_file_path);
const std::vector<double> dim2_data = readBinaryFile(dim2_file_path);
const std::vector<double> dim3_data = readBinaryFile(dim3_file_path);
const std::vector<double> dim4_data = readBinaryFile(dim4_file_path);

using namespace airtree::merge;

TEST(OneDxT_merge, OneDxT) {
  size_t length = dim1_data.size();
  size_t split = length / 2;
  std::vector<char> buffer1, buffer2, result;

  std::vector<double> dim1_part1(dim1_data.begin(), dim1_data.begin() + split);
  std::vector<double> dim1_part2(dim1_data.begin() + split, dim1_data.end());
  FPHArray array0_input1 =
      buildFPHArray(dim1_part1.data(), static_cast<int>(dim1_part1.size()));
  FPHArray array0_input2 =
      buildFPHArray(dim1_part2.data(), static_cast<int>(dim1_part2.size()));
  FPHArray array0_complete =
      buildFPHArray(dim1_data.data(), static_cast<int>(dim1_data.size()));
  buffer1 = generate_1DxT(array0_input1);
  buffer2 = generate_1DxT(array0_input2);
  result = generate_1DxT(array0_complete);
  auto mergedBuffer = mergeAirTree(buffer1, buffer2);
  std::string result_hash = hashBuffer(result);
  std::string merged_hash = hashBuffer(mergedBuffer);
  EXPECT_EQ(result_hash, merged_hash);
}

TEST(OneDxP_merge, OneDxP) {
  size_t length = dim1_data.size();
  size_t split = length / 2;
  std::vector<char> buffer1, buffer2, result;

  std::vector<double> dim1_part1(dim1_data.begin(), dim1_data.begin() + split);
  std::vector<double> dim1_part2(dim1_data.begin() + split, dim1_data.end());
  FPHArray array0_input1 =
      buildFPHArray(dim1_part1.data(), static_cast<int>(dim1_part1.size()));
  FPHArray array0_input2 =
      buildFPHArray(dim1_part2.data(), static_cast<int>(dim1_part2.size()));
  FPHArray array0_complete =
      buildFPHArray(dim1_data.data(), static_cast<int>(dim1_data.size()));
  buffer1 = generate_1DxP(array0_input1);
  buffer2 = generate_1DxP(array0_input2);
  result = generate_1DxP(array0_complete);
  auto mergedBuffer = mergeAirTree(buffer1, buffer2);
  std::string result_hash = hashBuffer(result);
  std::string merged_hash = hashBuffer(mergedBuffer);
  EXPECT_EQ(result_hash, merged_hash);
}

TEST(OneDxF_merge, OneDxF) {
  size_t length = dim1_data.size();
  size_t split = length / 2;
  std::vector<char> buffer1, buffer2, result;

  std::vector<double> dim1_part1(dim1_data.begin(), dim1_data.begin() + split);
  std::vector<double> dim1_part2(dim1_data.begin() + split, dim1_data.end());
  FPHArray array0_input1 =
      buildFPHArray(dim1_part1.data(), static_cast<int>(dim1_part1.size()));
  FPHArray array0_input2 =
      buildFPHArray(dim1_part2.data(), static_cast<int>(dim1_part2.size()));
  FPHArray array0_complete =
      buildFPHArray(dim1_data.data(), static_cast<int>(dim1_data.size()));
  buffer1 = generate_1DxF(array0_input1);
  buffer2 = generate_1DxF(array0_input2);
  result = generate_1DxF(array0_complete);
  auto mergedBuffer = mergeAirTree(buffer1, buffer2);
  std::string result_hash = hashBuffer(result);
  std::string merged_hash = hashBuffer(mergedBuffer);
  EXPECT_EQ(result_hash, merged_hash);
}

TEST(TwoDxP_merge, TwoDxP) {
  size_t length = dim1_data.size();
  size_t split = length / 2;
  std::vector<char> buffer1, buffer2, result;

  std::vector<double> dim1_part1(dim1_data.begin(), dim1_data.begin() + split);
  std::vector<double> dim1_part2(dim1_data.begin() + split, dim1_data.end());
  std::vector<double> dim2_part1(dim2_data.begin(), dim2_data.begin() + split);
  std::vector<double> dim2_part2(dim2_data.begin() + split, dim2_data.end());
  FPHArray array0_input1 =
      buildFPHArray(dim1_part1.data(), static_cast<int>(dim1_part1.size()));
  FPHArray array1_input1 =
      buildFPHArray(dim2_part1.data(), static_cast<int>(dim2_part1.size()));
  FPHArray array0_input2 =
      buildFPHArray(dim1_part2.data(), static_cast<int>(dim1_part2.size()));
  FPHArray array1_input2 =
      buildFPHArray(dim2_part2.data(), static_cast<int>(dim2_part2.size()));
  FPHArray array0_complete =
      buildFPHArray(dim1_data.data(), static_cast<int>(dim1_data.size()));
  FPHArray array1_complete =
      buildFPHArray(dim2_data.data(), static_cast<int>(dim2_data.size()));
  buffer1 = generate_2DxP(array0_input1, array1_input1);
  buffer2 = generate_2DxP(array0_input2, array1_input2);
  result = generate_2DxP(array0_complete, array1_complete);
  auto mergedBuffer = mergeAirTree(buffer1, buffer2);
  std::string result_hash = hashBuffer(result);
  std::string merged_hash = hashBuffer(mergedBuffer);
  EXPECT_EQ(result_hash, merged_hash);
}

TEST(TwoDxF_merge, TwoDxF) {
  size_t length = dim1_data.size();
  size_t split = length / 2;
  std::vector<char> buffer1, buffer2, result;

  std::vector<double> dim1_part1(dim1_data.begin(), dim1_data.begin() + split);
  std::vector<double> dim1_part2(dim1_data.begin() + split, dim1_data.end());
  std::vector<double> dim2_part1(dim2_data.begin(), dim2_data.begin() + split);
  std::vector<double> dim2_part2(dim2_data.begin() + split, dim2_data.end());
  FPHArray array0_input1 =
      buildFPHArray(dim1_part1.data(), static_cast<int>(dim1_part1.size()));
  FPHArray array1_input1 =
      buildFPHArray(dim2_part1.data(), static_cast<int>(dim2_part1.size()));
  FPHArray array0_input2 =
      buildFPHArray(dim1_part2.data(), static_cast<int>(dim1_part2.size()));
  FPHArray array1_input2 =
      buildFPHArray(dim2_part2.data(), static_cast<int>(dim2_part2.size()));
  FPHArray array0_complete =
      buildFPHArray(dim1_data.data(), static_cast<int>(dim1_data.size()));
  FPHArray array1_complete =
      buildFPHArray(dim2_data.data(), static_cast<int>(dim2_data.size()));
  buffer1 = generate_2DxF(array0_input1, array1_input1);
  buffer2 = generate_2DxF(array0_input2, array1_input2);
  result = generate_2DxF(array0_complete, array1_complete);
  auto mergedBuffer = mergeAirTree(buffer1, buffer2);
  std::string result_hash = hashBuffer(result);
  std::string merged_hash = hashBuffer(mergedBuffer);
  EXPECT_EQ(result_hash, merged_hash);
}

TEST(ThreeDxP_merge, ThreeDxP) {
  size_t length = dim1_data.size();
  size_t split = length / 2;
  std::vector<char> buffer1, buffer2, result;

  std::vector<double> dim1_part1(dim1_data.begin(), dim1_data.begin() + split);
  std::vector<double> dim1_part2(dim1_data.begin() + split, dim1_data.end());
  std::vector<double> dim2_part1(dim2_data.begin(), dim2_data.begin() + split);
  std::vector<double> dim2_part2(dim2_data.begin() + split, dim2_data.end());
  std::vector<double> dim3_part1(dim3_data.begin(), dim3_data.begin() + split);
  std::vector<double> dim3_part2(dim3_data.begin() + split, dim3_data.end());
  FPHArray array0_input1 =
      buildFPHArray(dim1_part1.data(), static_cast<int>(dim1_part1.size()));
  FPHArray array1_input1 =
      buildFPHArray(dim2_part1.data(), static_cast<int>(dim2_part1.size()));
  FPHArray array2_input1 =
      buildFPHArray(dim3_part1.data(), static_cast<int>(dim3_part1.size()));
  FPHArray array0_input2 =
      buildFPHArray(dim1_part2.data(), static_cast<int>(dim1_part2.size()));
  FPHArray array1_input2 =
      buildFPHArray(dim2_part2.data(), static_cast<int>(dim2_part2.size()));
  FPHArray array2_input2 =
      buildFPHArray(dim3_part2.data(), static_cast<int>(dim3_part2.size()));
  FPHArray array0_complete =
      buildFPHArray(dim1_data.data(), static_cast<int>(dim1_data.size()));
  FPHArray array1_complete =
      buildFPHArray(dim2_data.data(), static_cast<int>(dim2_data.size()));
  FPHArray array2_complete =
      buildFPHArray(dim3_data.data(), static_cast<int>(dim3_data.size()));
  buffer1 = generate_3DxP(array0_input1, array1_input1, array2_input1);
  buffer2 = generate_3DxP(array0_input2, array1_input2, array2_input2);
  result = generate_3DxP(array0_complete, array1_complete, array2_complete);
  auto mergedBuffer = mergeAirTree(buffer1, buffer2);
  std::string result_hash = hashBuffer(result);
  std::string merged_hash = hashBuffer(mergedBuffer);
  EXPECT_EQ(result_hash, merged_hash);
}

TEST(ThreeDxF_merge, ThreeDxF) {
  size_t length = dim1_data.size();
  size_t split = length / 2;
  std::vector<char> buffer1, buffer2, result;

  // Split each dimension's data into two parts.
  std::vector<double> dim1_part1(dim1_data.begin(), dim1_data.begin() + split);
  std::vector<double> dim1_part2(dim1_data.begin() + split, dim1_data.end());
  std::vector<double> dim2_part1(dim2_data.begin(), dim2_data.begin() + split);
  std::vector<double> dim2_part2(dim2_data.begin() + split, dim2_data.end());
  std::vector<double> dim3_part1(dim3_data.begin(), dim3_data.begin() + split);
  std::vector<double> dim3_part2(dim3_data.begin() + split, dim3_data.end());

  // Build arrays for the first and second halves.
  FPHArray array0_input1 =
      buildFPHArray(dim1_part1.data(), static_cast<int>(dim1_part1.size()));
  FPHArray array1_input1 =
      buildFPHArray(dim2_part1.data(), static_cast<int>(dim2_part1.size()));
  FPHArray array2_input1 =
      buildFPHArray(dim3_part1.data(), static_cast<int>(dim3_part1.size()));
  FPHArray array0_input2 =
      buildFPHArray(dim1_part2.data(), static_cast<int>(dim1_part2.size()));
  FPHArray array1_input2 =
      buildFPHArray(dim2_part2.data(), static_cast<int>(dim2_part2.size()));
  FPHArray array2_input2 =
      buildFPHArray(dim3_part2.data(), static_cast<int>(dim3_part2.size()));

  // Build arrays for the complete data.
  FPHArray array0_complete =
      buildFPHArray(dim1_data.data(), static_cast<int>(dim1_data.size()));
  FPHArray array1_complete =
      buildFPHArray(dim2_data.data(), static_cast<int>(dim2_data.size()));
  FPHArray array2_complete =
      buildFPHArray(dim3_data.data(), static_cast<int>(dim3_data.size()));

  // Generate buffers for each half and for the complete data.
  buffer1 = generate_3DxF(array0_input1, array1_input1, array2_input1);
  buffer2 = generate_3DxF(array0_input2, array1_input2, array2_input2);
  result = generate_3DxF(array0_complete, array1_complete, array2_complete);
  auto mergedBuffer = mergeAirTree(buffer1, buffer2);
  std::string result_hash = hashBuffer(result);
  std::string merged_hash = hashBuffer(mergedBuffer);
  EXPECT_EQ(result_hash, merged_hash);
}

TEST(FourDxP_merge, FourDxP) {
  size_t length = dim1_data.size();
  size_t split = length / 2;
  std::vector<char> buffer1, buffer2, result;

  // Split data for each of the four dimensions.
  std::vector<double> dim1_part1(dim1_data.begin(), dim1_data.begin() + split);
  std::vector<double> dim1_part2(dim1_data.begin() + split, dim1_data.end());
  std::vector<double> dim2_part1(dim2_data.begin(), dim2_data.begin() + split);
  std::vector<double> dim2_part2(dim2_data.begin() + split, dim2_data.end());
  std::vector<double> dim3_part1(dim3_data.begin(), dim3_data.begin() + split);
  std::vector<double> dim3_part2(dim3_data.begin() + split, dim3_data.end());
  std::vector<double> dim4_part1(dim4_data.begin(), dim4_data.begin() + split);
  std::vector<double> dim4_part2(dim4_data.begin() + split, dim4_data.end());

  // Build FPHArrays for each half.
  FPHArray array0_input1 =
      buildFPHArray(dim1_part1.data(), static_cast<int>(dim1_part1.size()));
  FPHArray array1_input1 =
      buildFPHArray(dim2_part1.data(), static_cast<int>(dim2_part1.size()));
  FPHArray array2_input1 =
      buildFPHArray(dim3_part1.data(), static_cast<int>(dim3_part1.size()));
  FPHArray array3_input1 =
      buildFPHArray(dim4_part1.data(), static_cast<int>(dim4_part1.size()));
  FPHArray array0_input2 =
      buildFPHArray(dim1_part2.data(), static_cast<int>(dim1_part2.size()));
  FPHArray array1_input2 =
      buildFPHArray(dim2_part2.data(), static_cast<int>(dim2_part2.size()));
  FPHArray array2_input2 =
      buildFPHArray(dim3_part2.data(), static_cast<int>(dim3_part2.size()));
  FPHArray array3_input2 =
      buildFPHArray(dim4_part2.data(), static_cast<int>(dim4_part2.size()));

  // Build FPHArrays for the complete data.
  FPHArray array0_complete =
      buildFPHArray(dim1_data.data(), static_cast<int>(dim1_data.size()));
  FPHArray array1_complete =
      buildFPHArray(dim2_data.data(), static_cast<int>(dim2_data.size()));
  FPHArray array2_complete =
      buildFPHArray(dim3_data.data(), static_cast<int>(dim3_data.size()));
  FPHArray array3_complete =
      buildFPHArray(dim4_data.data(), static_cast<int>(dim4_data.size()));

  // Generate and merge buffers.
  buffer1 =
      generate_4DxP(array0_input1, array1_input1, array2_input1, array3_input1);
  buffer2 =
      generate_4DxP(array0_input2, array1_input2, array2_input2, array3_input2);
  result = generate_4DxP(
      array0_complete, array1_complete, array2_complete, array3_complete);
  auto mergedBuffer = mergeAirTree(buffer1, buffer2);
  std::string result_hash = hashBuffer(result);
  std::string merged_hash = hashBuffer(mergedBuffer);
  EXPECT_EQ(result_hash, merged_hash);
}

TEST(FourDxF_merge, FourDxF) {
  size_t length = dim1_data.size();
  size_t split = length / 2;
  std::vector<char> buffer1, buffer2, result;

  // Split data for each dimension.
  std::vector<double> dim1_part1(dim1_data.begin(), dim1_data.begin() + split);
  std::vector<double> dim1_part2(dim1_data.begin() + split, dim1_data.end());
  std::vector<double> dim2_part1(dim2_data.begin(), dim2_data.begin() + split);
  std::vector<double> dim2_part2(dim2_data.begin() + split, dim2_data.end());
  std::vector<double> dim3_part1(dim3_data.begin(), dim3_data.begin() + split);
  std::vector<double> dim3_part2(dim3_data.begin() + split, dim3_data.end());
  std::vector<double> dim4_part1(dim4_data.begin(), dim4_data.begin() + split);
  std::vector<double> dim4_part2(dim4_data.begin() + split, dim4_data.end());

  // Build FPHArrays for each half.
  FPHArray array0_input1 =
      buildFPHArray(dim1_part1.data(), static_cast<int>(dim1_part1.size()));
  FPHArray array1_input1 =
      buildFPHArray(dim2_part1.data(), static_cast<int>(dim2_part1.size()));
  FPHArray array2_input1 =
      buildFPHArray(dim3_part1.data(), static_cast<int>(dim3_part1.size()));
  FPHArray array3_input1 =
      buildFPHArray(dim4_part1.data(), static_cast<int>(dim4_part1.size()));
  FPHArray array0_input2 =
      buildFPHArray(dim1_part2.data(), static_cast<int>(dim1_part2.size()));
  FPHArray array1_input2 =
      buildFPHArray(dim2_part2.data(), static_cast<int>(dim2_part2.size()));
  FPHArray array2_input2 =
      buildFPHArray(dim3_part2.data(), static_cast<int>(dim3_part2.size()));
  FPHArray array3_input2 =
      buildFPHArray(dim4_part2.data(), static_cast<int>(dim4_part2.size()));

  // Build FPHArrays for the complete data.
  FPHArray array0_complete =
      buildFPHArray(dim1_data.data(), static_cast<int>(dim1_data.size()));
  FPHArray array1_complete =
      buildFPHArray(dim2_data.data(), static_cast<int>(dim2_data.size()));
  FPHArray array2_complete =
      buildFPHArray(dim3_data.data(), static_cast<int>(dim3_data.size()));
  FPHArray array3_complete =
      buildFPHArray(dim4_data.data(), static_cast<int>(dim4_data.size()));

  // Generate and merge buffers.
  buffer1 =
      generate_4DxF(array0_input1, array1_input1, array2_input1, array3_input1);
  buffer2 =
      generate_4DxF(array0_input2, array1_input2, array2_input2, array3_input2);
  result = generate_4DxF(
      array0_complete, array1_complete, array2_complete, array3_complete);
  auto mergedBuffer = mergeAirTree(buffer1, buffer2);

  std::string result_hash = hashBuffer(result);
  std::string merged_hash = hashBuffer(mergedBuffer);
  EXPECT_EQ(result_hash, merged_hash);
}
