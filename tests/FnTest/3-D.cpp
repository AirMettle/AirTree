#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/core/api/AirTreeGenerator.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/common/Reconstruct.hpp>
#include <airtree/core/common/SpecialCounts.hpp>
#include <airtree/query/AirTreeQuery_internal.hpp>
#include <airtree/query/bin-boundary/BinBoundary.hpp>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <gtest/gtest.h>
#include <limits>
#include <map>
#include <tuple>
#include <vector>
#include <airtree/core/utils/Utils.hpp>

using namespace airtree::core::io;
using namespace airtree::core::api;
using namespace airtree::query::bin_boundary;

const std::string dim1_file_path = "../../tests/TestData/dim1_vx.bin";
const std::string dim2_file_path = "../../tests/TestData/dim2_vy.bin";
const std::string dim3_file_path = "../../tests/TestData/dim3_vz.bin";
const std::string result_file_path =
    "../../tests/FnTest_Results/3D_output.json";

// Read the data from the binary file
const std::vector<double> dim1_data = readBinaryFile(dim1_file_path);
// Read the data from the binary file
const std::vector<double> dim2_data = readBinaryFile(dim2_file_path);
// Read the data from the binary file
const std::vector<double> dim3_data = readBinaryFile(dim3_file_path);

TEST(Generate_3d_test_data_splits, Generate_3d_test_data_splits_fntest) {
  GTEST_SKIP();
  // Check that data is non-empty and has the expected structure
  ASSERT_FALSE(dim1_data.empty()) << "Data from dim1.bin file is empty.";
  ASSERT_FALSE(dim2_data.empty()) << "Data from dim2.bin file is empty.";
  ASSERT_FALSE(dim3_data.empty()) << "Data from dim3.bin file is empty.";

  // make sure all dims have the same size
  ASSERT_EQ(dim1_data.size(), dim2_data.size())
      << "Data from dim1.bin and dim2.bin files have different sizes.";
  ASSERT_EQ(dim1_data.size(), dim3_data.size())
      << "Data from dim1.bin and dim3.bin files have different sizes.";

  size_t length = dim1_data.size();
  size_t split = length / 2;
  // log both length and split to make sure they are divisible by 2. If not then
  // the original will have to be reduced by 1 to make it divisible by 2
  std::cout << "Length: " << length << ", Split: " << split << std::endl;

  // Split input data into two parts
  std::vector<double> dim1_part1(dim1_data.begin(), dim1_data.begin() + split);
  std::cout << "Dim1 Part1 size: " << dim1_part1.size() << std::endl;
  std::vector<double> dim1_part2(dim1_data.begin() + split, dim1_data.end());
  std::cout << "Dim1 Part2 size: " << dim1_part2.size() << std::endl;
  std::vector<double> dim2_part1(dim2_data.begin(), dim2_data.begin() + split);
  std::cout << "Dim2 Part1 size: " << dim2_part1.size() << std::endl;
  std::vector<double> dim2_part2(dim2_data.begin() + split, dim2_data.end());
  std::cout << "Dim2 Part2 size: " << dim2_part2.size() << std::endl;

  std::vector<double> dim3_part1(dim3_data.begin(), dim3_data.begin() + split);
  std::cout << "Dim3 Part1 size: " << dim3_part1.size() << std::endl;
  std::vector<double> dim3_part2(dim3_data.begin() + split, dim3_data.end());
  std::cout << "Dim3 Part2 size: " << dim3_part2.size() << std::endl;

  // Split each dim's vector into two parts to generate two separate histogram
  // buffers
  AirTreeOptions options;
  options.dimensions = 3;
  options.type = ConfigType::XP;
  std::vector<char> piece1 =
      generate(dim1_part1, dim2_part1, dim3_part1, options);
  std::cout << "Piece1 size: " << piece1.size() << std::endl;
  std::vector<char> piece2 =
      generate(dim1_part2, dim2_part2, dim3_part2, options);
  std::cout << "Piece2 size: " << piece2.size() << std::endl;
  std::vector<char> result = generate(dim1_data, dim2_data, dim3_data, options);
  std::cout << "Result size: " << result.size() << std::endl;
  AirTreeWriter::Write(result, "Original_3DxP.bin");
  AirTreeWriter::Write(piece1, "3DxP_piece1.bin");
  AirTreeWriter::Write(piece2, "3DxP_piece2.bin");

  // Generate the 3DxF result
  options.type = ConfigType::XF;
  piece1 = generate(dim1_part1, dim2_part1, dim3_part1, options);
  std::cout << "Piece1 size: " << piece1.size() << std::endl;
  piece2 = generate(dim1_part2, dim2_part2, dim3_part2, options);
  std::cout << "Piece2 size: " << piece2.size() << std::endl;
  result = generate(dim1_data, dim2_data, dim3_data, options);
  std::cout << "Result size: " << result.size() << std::endl;
  AirTreeWriter::Write(result, "Original_3DxF.bin");
  AirTreeWriter::Write(piece1, "3DxF_piece1.bin");
  AirTreeWriter::Write(piece2, "3DxF_piece2.bin");
}

TEST(Generate_3d_888, Generate_3d_888_fntest) {
  // Check that data is non-empty and has the expected structure
  ASSERT_FALSE(dim1_data.empty()) << "Data from dim1.bin file is empty.";
  ASSERT_FALSE(dim2_data.empty()) << "Data from dim2.bin file is empty.";
  ASSERT_FALSE(dim3_data.empty()) << "Data from dim3.bin file is empty.";

  AirTreeOptions options;
  options.dimensions = 3;
  options.type = ConfigType::XF;
  std::vector<char> result = generate(dim1_data, dim2_data, dim3_data, options);

  std::string result_hash = hashBuffer(result);

  std::string expected_hash = readValueFromJson(result_file_path, "888");

  EXPECT_EQ(result_hash, expected_hash);
}

// ============================================================================
// Shared helpers for N-D sanity and accuracy tests
// ============================================================================

static double reconFromBounds(double lower, double upper) {
  return (upper <= 0) ? upper : lower;
}

static bool isSpecialValue(double v) {
  uint64_t fp;
  std::memcpy(&fp, &v, sizeof(v));
  SpecialCounts dummy;
  return isSpecialCase(fp, dummy);
}

static unsigned int encode_12bit(double val) {
  uint64_t fp;
  std::memcpy(&fp, &val, sizeof(val));
  TLE tle = setTLEComponents(fp);
  unsigned int internal10 = createInternal10Bit(fp, true);
  unsigned int prefix;
  switch (tle.encoding) {
  case 2:
    prefix = 0b00;
    break;
  case 3:
    prefix = 0b01;
    break;
  case 5:
    prefix = 0b11;
    break;
  case 6:
    prefix = 0b10;
    break;
  default:
    return 0xFFFF;
  }
  return (prefix << 10) | internal10;
}

static void verifyHistogramSanity3D(const std::vector<char> &buffer,
                                    size_t num_rows) {
  BinBoundary query(buffer);
  auto result = query.generateBinBoundaries();
  auto bins = std::get<BinBoundary3DList>(*result.getBoundaries());

  int total_hist = 0;
  for (const auto &bin : bins)
    total_hist += bin.getCount();
  EXPECT_EQ(total_hist, static_cast<int>(num_rows))
      << "Total histogram count != input row count";

  for (size_t i = 0; i < bins.size(); i++) {
    EXPECT_FALSE(std::isnan(bins[i].getLowerBoundX()))
        << "Bin " << i << " x-min NaN";
    EXPECT_FALSE(std::isnan(bins[i].getUpperBoundX()))
        << "Bin " << i << " x-max NaN";
    EXPECT_FALSE(std::isnan(bins[i].getLowerBoundY()))
        << "Bin " << i << " y-min NaN";
    EXPECT_FALSE(std::isnan(bins[i].getUpperBoundY()))
        << "Bin " << i << " y-max NaN";
    EXPECT_FALSE(std::isnan(bins[i].getLowerBoundZ()))
        << "Bin " << i << " z-min NaN";
    EXPECT_FALSE(std::isnan(bins[i].getUpperBoundZ()))
        << "Bin " << i << " z-max NaN";
    EXPECT_FALSE(std::isinf(bins[i].getLowerBoundX())
                 && bins[i].getLowerBoundX() > 0)
        << "Bin " << i << " x-min +Inf";
    EXPECT_FALSE(std::isinf(bins[i].getUpperBoundX()))
        << "Bin " << i << " x-max Inf";
    EXPECT_FALSE(std::isinf(bins[i].getLowerBoundY())
                 && bins[i].getLowerBoundY() > 0)
        << "Bin " << i << " y-min +Inf";
    EXPECT_FALSE(std::isinf(bins[i].getUpperBoundY()))
        << "Bin " << i << " y-max Inf";
    EXPECT_FALSE(std::isinf(bins[i].getLowerBoundZ())
                 && bins[i].getLowerBoundZ() > 0)
        << "Bin " << i << " z-min +Inf";
    EXPECT_FALSE(std::isinf(bins[i].getUpperBoundZ()))
        << "Bin " << i << " z-max Inf";
    EXPECT_GT(bins[i].getCount(), 0u) << "Bin " << i << " zero count";
  }
}

static void verifyBinAccuracy3DxP(const std::vector<char> &buffer,
                                  const std::vector<double> &d1,
                                  const std::vector<double> &d2,
                                  const std::vector<double> &d3) {
  BinBoundary query(buffer);
  auto result = query.generateBinBoundaries();
  auto bins = std::get<BinBoundary3DList>(*result.getBoundaries());

  using Key = std::tuple<double, double, double>;
  auto hasSpecial = [](double v) {
    return v == 0.0 || std::isnan(v) || std::isinf(v);
  };

  std::map<Key, uint32_t> hist_counts;
  for (const auto &bin : bins) {
    double rx = reconFromBounds(bin.getLowerBoundX(), bin.getUpperBoundX());
    double ry = reconFromBounds(bin.getLowerBoundY(), bin.getUpperBoundY());
    double rz = reconFromBounds(bin.getLowerBoundZ(), bin.getUpperBoundZ());
    if (hasSpecial(rx) || hasSpecial(ry) || hasSpecial(rz))
      continue;
    hist_counts[{rx, ry, rz}] += bin.getCount();
  }

  std::map<Key, int> actual_counts;
  auto enc = [](double v) -> double {
    if (v == 0.0 || std::isnan(v) || std::isinf(v))
      return v;
    return reConstruct<double>(encode_12bit(v), 12);
  };
  for (size_t i = 0; i < d1.size(); i++) {
    if (isSpecialValue(d1[i]) || isSpecialValue(d2[i]) || isSpecialValue(d3[i]))
      continue;
    actual_counts[{enc(d1[i]), enc(d2[i]), enc(d3[i])}]++;
  }

  for (const auto &[key, hc] : hist_counts) {
    int actual = actual_counts.count(key) ? actual_counts[key] : 0;
    EXPECT_EQ(actual, static_cast<int>(hc));
  }
  for (const auto &[key, ac] : actual_counts) {
    EXPECT_TRUE(hist_counts.count(key) > 0)
        << "Row not found in histogram output";
  }
}

// ============================================================================

// --- 3D Sanity + Accuracy Tests ---

TEST(HistogramSanity_3DxP, HistogramSanity_3DxP_fntest) {
  ASSERT_FALSE(dim1_data.empty());
  ASSERT_FALSE(dim2_data.empty());
  ASSERT_FALSE(dim3_data.empty());
  AirTreeOptions options;
  options.dimensions = 3;
  options.type = ConfigType::XP;
  auto buffer = generate(dim1_data, dim2_data, dim3_data, options);
  verifyHistogramSanity3D(buffer, dim1_data.size());
  verifyBinAccuracy3DxP(buffer, dim1_data, dim2_data, dim3_data);
}

// --- Gap 2: Mixed special/non-special row validation ---

TEST(MixedSpecialSanity_3D, MixedSpecialSanity_3DxP_fntest) {
  std::vector<double> d1, d2, d3;

  // 200 normal rows
  for (int i = 1; i <= 200; i++) {
    d1.push_back(i * 0.37);
    d2.push_back(i * 0.53);
    d3.push_back(i * 0.71);
  }

  // Append rows with special in each dimension (one at a time)
  // This exercises specialCount=1 for every dimension position,
  // including NaN which would corrupt std::map if not filtered.
  double specials[] = {0.0, -0.0, std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity()};
  // Special in dim1
  for (double s : specials) {
    d1.push_back(s);
    d2.push_back(1.5);
    d3.push_back(2.5);
  }
  // Special in dim2
  for (double s : specials) {
    d1.push_back(1.5);
    d2.push_back(s);
    d3.push_back(2.5);
  }
  // Special in dim3
  for (double s : specials) {
    d1.push_back(1.5);
    d2.push_back(2.5);
    d3.push_back(s);
  }

  AirTreeOptions options;
  options.dimensions = 3;
  options.type = ConfigType::XP;
  auto buffer = generate(d1, d2, d3, options);

  BinBoundary query(buffer);
  auto result = query.generateBinBoundaries();
  auto bins = std::get<BinBoundary3DList>(*result.getBoundaries());

  int total = 0;
  for (const auto &bin : bins)
    total += bin.getCount();
  EXPECT_EQ(total, static_cast<int>(d1.size()))
      << "Total histogram count != input row count with mixed specials";

  verifyBinAccuracy3DxP(buffer, d1, d2, d3);
}

// --- Gap 3: Float32 input for multi-D ---

TEST(Float32Sanity_3D, Float32Sanity_3DxP_fntest) {
  std::vector<float> fd1 = {
      1.0f, 1.001f, 0.5f, 2.0f, -1.0f, 5.6f, 0.125f, 100.5f};
  std::vector<float> fd2 = {
      0.25f, 4.0f, 0.999f, 2.001f, -0.5f, 10.0f, 0.126f, 26.8f};
  std::vector<float> fd3 = {
      0.5001f, 4.01f, 0.9999f, 0.251f, -1.001f, 0.001f, 1000.0f, 0.5f};

  ASSERT_EQ(fd1.size(), fd2.size());
  ASSERT_EQ(fd1.size(), fd3.size());

  AirTreeOptions options;
  options.dimensions = 3;
  options.type = ConfigType::XP;
  auto buffer = generate(fd1, fd2, fd3, options);

  std::vector<double> d1(fd1.begin(), fd1.end());
  std::vector<double> d2(fd2.begin(), fd2.end());
  std::vector<double> d3(fd3.begin(), fd3.end());
  verifyHistogramSanity3D(buffer, fd1.size());
  verifyBinAccuracy3DxP(buffer, d1, d2, d3);
}

// --- 3D Saturation Zone ---

TEST(SaturationZoneSanity_3D, SaturationZoneSanity_3DxP_fntest) {
  std::vector<double> data;
  for (int p = -3; p <= 3; p++) {
    double base = std::pow(2.0, p);
    for (double eps : {1e-5, 1e-3, 0.01}) {
      data.push_back(base * (1.0 + eps));
      data.push_back(base * (1.0 - eps));
      data.push_back(-base * (1.0 + eps));
      data.push_back(-base * (1.0 - eps));
    }
  }

  AirTreeOptions options;
  options.dimensions = 3;
  options.type = ConfigType::XP;
  auto buffer = generate(data, data, data, options);
  verifyHistogramSanity3D(buffer, data.size());
  verifyBinAccuracy3DxP(buffer, data, data, data);
}
