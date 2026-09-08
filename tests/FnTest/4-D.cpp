// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/core/api/AirTreeGenerator.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/common/Reconstruct.hpp>
#include <airtree/core/common/SpecialCounts.hpp>
#include <airtree/query/AirTreeQuery_internal.hpp>
#include <airtree/query/bin-boundary/BinBoundary.hpp>
#include <gtest/gtest.h>
#include <airtree/core/io/AirTreeWriter.hpp>
#include <airtree/core/utils/Utils.hpp>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <map>
#include <tuple>
#include <vector>

using namespace airtree::core::io;
using namespace airtree::core::api;
using namespace airtree::query::bin_boundary;

const std::string dim1_file_path = "../../tests/TestData/dim1_vx.bin";
const std::string dim2_file_path = "../../tests/TestData/dim2_vy.bin";
const std::string dim3_file_path = "../../tests/TestData/dim3_vz.bin";
const std::string dim4_file_path = "../../tests/TestData/dim4_rho.bin";
const std::string result_file_path =
    "../../tests/FnTest_Results/4D_output.json";

// Read the data from the binary file
const std::vector<double> dim1_data = readBinaryFile(dim1_file_path);
// Read the data from the binary file
const std::vector<double> dim2_data = readBinaryFile(dim2_file_path);
// Read the data from the binary file
const std::vector<double> dim3_data = readBinaryFile(dim3_file_path);
// Read the data from the binary file
const std::vector<double> dim4_data = readBinaryFile(dim4_file_path);


TEST(Generate_4d_4x10, Generate_4d_4x10_fntest) {
  // Check that data is non-empty and has the expected structure
  ASSERT_FALSE(dim1_data.empty()) << "Data from dim1.bin file is empty.";
  ASSERT_FALSE(dim2_data.empty()) << "Data from dim2.bin file is empty.";
  ASSERT_FALSE(dim3_data.empty()) << "Data from dim3.bin file is empty.";
  ASSERT_FALSE(dim4_data.empty()) << "Data from dim4.bin file is empty.";

  AirTreeOptions options;
  options.dimensions = 4;
  options.type = ConfigType::XP;
  std::vector<char> result =
      generate(dim1_data, dim2_data, dim3_data, dim4_data, options);

  std::string result_hash = hashBuffer(result);

  std::string expected_hash = readValueFromJson(result_file_path, "4x10");

  EXPECT_EQ(result_hash, expected_hash);
}

TEST(Generate_4d_4x8, Generate_4d_4x8_fntest) {
  // Check that data is non-empty and has the expected structure
  ASSERT_FALSE(dim1_data.empty()) << "Data from dim1.bin file is empty.";
  ASSERT_FALSE(dim2_data.empty()) << "Data from dim2.bin file is empty.";
  ASSERT_FALSE(dim3_data.empty()) << "Data from dim3.bin file is empty.";
  ASSERT_FALSE(dim4_data.empty()) << "Data from dim4.bin file is empty.";

  AirTreeOptions options;
  options.dimensions = 4;
  options.type = ConfigType::XF;
  std::vector<char> result =
      generate(dim1_data, dim2_data, dim3_data, dim4_data, options);

  std::string result_hash = hashBuffer(result);

  std::string expected_hash = readValueFromJson(result_file_path, "4x8");

  EXPECT_EQ(result_hash, expected_hash);
}
TEST(Generate_special_export_data, Generate_export_data_fntest) {
  GTEST_SKIP();
  // Make up sepcial data for export
  std::vector<double> special_data = {std::numeric_limits<double>::infinity(),
                                      -std::numeric_limits<double>::infinity(),
                                      +0.0, -0.0,
                                      std::numeric_limits<double>::quiet_NaN()};
  std::vector<double> normal_data = {1.0, 2.0, 3.0, 4.0, 5.0};

  AirTreeOptions options;
  options.dimensions = 1;
  options.type = ConfigType::XT;
  std::vector<char> result = generate(special_data, options);
  AirTreeWriter::Write(result, "1DxT_special.bin");

  options.type = ConfigType::XP;
  result = generate(special_data, options);
  AirTreeWriter::Write(result, "1DxP_special.bin");

  options.type = ConfigType::XF;
  result = generate(special_data, options);
  AirTreeWriter::Write(result, "1DxF_special.bin");

  // [special, special]
  options.dimensions = 2;
  options.type = ConfigType::XP;
  std::vector<char> result_2d_ss =
      airtree::core::api::generate(special_data, special_data, options);
  AirTreeWriter::Write(result_2d_ss, "2DxP_special_special.bin");

  // [special, normal]
  std::vector<char> result_2d_sn =
      airtree::core::api::generate(special_data, normal_data, options);
  AirTreeWriter::Write(result_2d_sn, "2DxP_special_normal.bin");

  // [normal, special]
  std::vector<char> result_2d_ns =
      airtree::core::api::generate(normal_data, special_data, options);
  AirTreeWriter::Write(result_2d_ns, "2DxP_normal_special.bin");

  // [special, special, special]
  options.dimensions = 3;
  options.type = ConfigType::XP;
  std::vector<char> result_3d_sss = airtree::core::api::generate(
      special_data, special_data, special_data, options);
  AirTreeWriter::Write(result_3d_sss, "3DxP_special_special_special.bin");

  // [special, special, normal]
  std::vector<char> result_3d_ssn = airtree::core::api::generate(
      special_data, special_data, normal_data, options);
  AirTreeWriter::Write(result_3d_ssn, "3DxP_special_special_normal.bin");

  // [special, normal, special]
  std::vector<char> result_3d_sns = airtree::core::api::generate(
      special_data, normal_data, special_data, options);
  AirTreeWriter::Write(result_3d_sns, "3DxP_special_normal_special.bin");

  // [normal, special, special]
  std::vector<char> result_3d_nss = airtree::core::api::generate(
      normal_data, special_data, special_data, options);
  AirTreeWriter::Write(result_3d_nss, "3DxP_normal_special_special.bin");

  // [normal, normal, special]
  std::vector<char> result_3d_nns = airtree::core::api::generate(
      normal_data, normal_data, special_data, options);
  AirTreeWriter::Write(result_3d_nns, "3DxP_normal_normal_special.bin");

  // [normal, special, normal]
  std::vector<char> result_3d_nsn = airtree::core::api::generate(
      normal_data, special_data, normal_data, options);
  AirTreeWriter::Write(result_3d_nsn, "3DxP_normal_special_normal.bin");

  // [special, normal, normal]
  std::vector<char> result_3d_snn = airtree::core::api::generate(
      special_data, normal_data, normal_data, options);
  AirTreeWriter::Write(result_3d_snn, "3DxP_special_normal_normal.bin");

  // [special, special, special, special]
  options.dimensions = 4;
  options.type = ConfigType::XP;
  std::vector<char> result_4d_ssss = airtree::core::api::generate(
      special_data, special_data, special_data, special_data, options);
  AirTreeWriter::Write(
      result_4d_ssss, "4DxP_special_special_special_special.bin");

  // [special, special, special, normal]
  std::vector<char> result_4d_sssn = airtree::core::api::generate(
      special_data, special_data, special_data, normal_data, options);
  AirTreeWriter::Write(
      result_4d_sssn, "4DxP_special_special_special_normal.bin");

  // [special, special, normal, special]
  std::vector<char> result_4d_ssns = airtree::core::api::generate(
      special_data, special_data, normal_data, special_data, options);
  AirTreeWriter::Write(
      result_4d_ssns, "4DxP_special_special_normal_special.bin");

  // [special, normal, special, special]
  std::vector<char> result_4d_snss = airtree::core::api::generate(
      special_data, normal_data, special_data, special_data, options);
  AirTreeWriter::Write(
      result_4d_snss, "4DxP_special_normal_special_special.bin");

  // [normal, special, special, special]
  std::vector<char> result_4d_nsss = airtree::core::api::generate(
      normal_data, special_data, special_data, special_data, options);
  AirTreeWriter::Write(
      result_4d_nsss, "4DxP_normal_special_special_special.bin");

  // [special, special, normal, normal]
  std::vector<char> result_4d_ssnn = airtree::core::api::generate(
      special_data, special_data, normal_data, normal_data, options);
  AirTreeWriter::Write(
      result_4d_ssnn, "4DxP_special_special_normal_normal.bin");

  // [special, normal, special, normal]
  std::vector<char> result_4d_snsn = airtree::core::api::generate(
      special_data, normal_data, special_data, normal_data, options);
  AirTreeWriter::Write(
      result_4d_snsn, "4DxP_special_normal_special_normal.bin");

  // [special, normal, normal, special]
  std::vector<char> result_4d_snns = airtree::core::api::generate(
      special_data, normal_data, normal_data, special_data, options);
  AirTreeWriter::Write(
      result_4d_snns, "4DxP_special_normal_normal_special.bin");

  // [normal, special, special, normal]
  std::vector<char> result_4d_nssn = airtree::core::api::generate(
      normal_data, special_data, special_data, normal_data, options);
  AirTreeWriter::Write(
      result_4d_nssn, "4DxP_normal_special_special_normal.bin");

  // [normal, special, normal, special]
  std::vector<char> result_4d_nsns = airtree::core::api::generate(
      normal_data, special_data, normal_data, special_data, options);
  AirTreeWriter::Write(
      result_4d_nsns, "4DxP_normal_special_normal_special.bin");

  // [normal, normal, special, special]
  std::vector<char> result_4d_nnss = airtree::core::api::generate(
      normal_data, normal_data, special_data, special_data, options);
  AirTreeWriter::Write(
      result_4d_nnss, "4DxP_normal_normal_special_special.bin");

  // [normal, normal, normal, special]
  std::vector<char> result_4d_nnnn = airtree::core::api::generate(
      normal_data, normal_data, normal_data, special_data, options);
  AirTreeWriter::Write(result_4d_nnnn, "4DxP_normal_normal_normal_special.bin");

  // [normal, normal, special, normal]
  std::vector<char> result_4d_nnsn2 = airtree::core::api::generate(
      normal_data, normal_data, special_data, normal_data, options);
  AirTreeWriter::Write(
      result_4d_nnsn2, "4DxP_normal_normal_special_normal.bin");

  // [normal, special, normal, normal]
  std::vector<char> result_4d_nsnn = airtree::core::api::generate(
      normal_data, special_data, normal_data, normal_data, options);
  AirTreeWriter::Write(result_4d_nsnn, "4DxP_normal_special_normal_normal.bin");

  // [special, normal, normal, normal]
  std::vector<char> result_4d_snnn = airtree::core::api::generate(
      special_data, normal_data, normal_data, normal_data, options);
  AirTreeWriter::Write(result_4d_snnn, "4DxP_special_normal_normal_normal.bin");
}

// ============================================================================
// 4D Sanity + Accuracy Tests
// ============================================================================

static void verifyHistogramSanity4D(const std::vector<char> &buffer,
                                    size_t num_rows) {
  BinBoundary query(buffer);
  auto result = query.generateBinBoundaries();
  auto bins = std::get<BinBoundary4DList>(*result.getBoundaries());

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
    EXPECT_FALSE(std::isnan(bins[i].getLowerBoundW()))
        << "Bin " << i << " w-min NaN";
    EXPECT_FALSE(std::isnan(bins[i].getUpperBoundW()))
        << "Bin " << i << " w-max NaN";
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
    EXPECT_FALSE(std::isinf(bins[i].getLowerBoundW())
                 && bins[i].getLowerBoundW() > 0)
        << "Bin " << i << " w-min +Inf";
    EXPECT_FALSE(std::isinf(bins[i].getUpperBoundW()))
        << "Bin " << i << " w-max Inf";
    EXPECT_GT(bins[i].getCount(), 0u) << "Bin " << i << " zero count";
  }
}

static double reconFromBounds(double lower, double upper) {
  return (upper <= 0) ? upper : lower;
}

static bool isSpecialValue(double v) {
  uint64_t fp;
  std::memcpy(&fp, &v, sizeof(v));
  SpecialCounts dummy;
  return isSpecialCase(fp, dummy);
}

static double encAndRecon(double v) {
  uint64_t fp;
  std::memcpy(&fp, &v, sizeof(v));
  TLE tle = setTLEComponents(fp);
  unsigned int i10 = createInternal10Bit(fp);
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
    return v;
  }
  return reConstruct<double>((prefix << 10) | i10, 12);
}

static void verifyBinAccuracy4DxP(const std::vector<char> &buffer,
                                  const std::vector<double> &d1,
                                  const std::vector<double> &d2,
                                  const std::vector<double> &d3,
                                  const std::vector<double> &d4) {
  BinBoundary query(buffer);
  auto result = query.generateBinBoundaries();
  auto bins = std::get<BinBoundary4DList>(*result.getBoundaries());

  using Key = std::tuple<double, double, double, double>;
  auto hasSpecial = [](double v) {
    return v == 0.0 || std::isnan(v) || std::isinf(v);
  };

  std::map<Key, uint32_t> hist_counts;
  for (const auto &bin : bins) {
    double rx = reconFromBounds(bin.getLowerBoundX(), bin.getUpperBoundX());
    double ry = reconFromBounds(bin.getLowerBoundY(), bin.getUpperBoundY());
    double rz = reconFromBounds(bin.getLowerBoundZ(), bin.getUpperBoundZ());
    double rw = reconFromBounds(bin.getLowerBoundW(), bin.getUpperBoundW());
    if (hasSpecial(rx) || hasSpecial(ry) || hasSpecial(rz) || hasSpecial(rw))
      continue;
    hist_counts[{rx, ry, rz, rw}] += bin.getCount();
  }

  std::map<Key, int> actual_counts;
  for (size_t i = 0; i < d1.size(); i++) {
    if (isSpecialValue(d1[i]) || isSpecialValue(d2[i]) || isSpecialValue(d3[i])
        || isSpecialValue(d4[i]))
      continue;
    actual_counts[{encAndRecon(d1[i]), encAndRecon(d2[i]), encAndRecon(d3[i]),
                   encAndRecon(d4[i])}]++;
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

TEST(HistogramSanity_4DxP, HistogramSanity_4DxP_fntest) {
  ASSERT_FALSE(dim1_data.empty());
  ASSERT_FALSE(dim2_data.empty());
  ASSERT_FALSE(dim3_data.empty());
  ASSERT_FALSE(dim4_data.empty());
  AirTreeOptions options;
  options.dimensions = 4;
  options.type = ConfigType::XP;
  auto buffer = airtree::core::api::generate(
      dim1_data, dim2_data, dim3_data, dim4_data, options);
  verifyHistogramSanity4D(buffer, dim1_data.size());
  verifyBinAccuracy4DxP(buffer, dim1_data, dim2_data, dim3_data, dim4_data);
}

// --- Gap 2: Mixed special/non-special row validation ---

TEST(MixedSpecialSanity_4D, MixedSpecialSanity_4DxP_fntest) {
  std::vector<double> d1, d2, d3, d4;

  // 200 normal rows
  for (int i = 1; i <= 200; i++) {
    d1.push_back(i * 0.37);
    d2.push_back(i * 0.53);
    d3.push_back(i * 0.71);
    d4.push_back(i * 0.19);
  }

  // Append rows with special in each dimension (one at a time)
  double specials[] = {0.0, -0.0, std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity()};
  // Special in dim1
  for (double s : specials) {
    d1.push_back(s);
    d2.push_back(1.5);
    d3.push_back(2.5);
    d4.push_back(3.5);
  }
  // Special in dim2
  for (double s : specials) {
    d1.push_back(1.5);
    d2.push_back(s);
    d3.push_back(2.5);
    d4.push_back(3.5);
  }
  // Special in dim3
  for (double s : specials) {
    d1.push_back(1.5);
    d2.push_back(2.5);
    d3.push_back(s);
    d4.push_back(3.5);
  }
  // Special in dim4
  for (double s : specials) {
    d1.push_back(1.5);
    d2.push_back(2.5);
    d3.push_back(3.5);
    d4.push_back(s);
  }

  AirTreeOptions options;
  options.dimensions = 4;
  options.type = ConfigType::XP;
  auto buffer = airtree::core::api::generate(d1, d2, d3, d4, options);

  BinBoundary query(buffer);
  auto result = query.generateBinBoundaries();
  auto bins = std::get<BinBoundary4DList>(*result.getBoundaries());

  int total = 0;
  for (const auto &bin : bins)
    total += bin.getCount();
  EXPECT_EQ(total, static_cast<int>(d1.size()))
      << "Total histogram count != input row count with mixed specials";

  verifyBinAccuracy4DxP(buffer, d1, d2, d3, d4);
}

// --- Gap 3: Float32 input for multi-D ---

TEST(Float32Sanity_4D, Float32Sanity_4DxP_fntest) {
  std::vector<float> fd1 = {
      1.0f, 1.001f, 0.5f, 2.0f, -1.0f, 5.6f, 0.125f, 100.5f};
  std::vector<float> fd2 = {
      0.25f, 4.0f, 0.999f, 2.001f, -0.5f, 10.0f, 0.126f, 26.8f};
  std::vector<float> fd3 = {
      0.5001f, 4.01f, 0.9999f, 0.251f, -1.001f, 0.001f, 1000.0f, 0.5f};
  std::vector<float> fd4 = {
      3.0f, 0.333f, 7.0f, 0.125f, -2.0f, 50.0f, 0.0625f, 8.8f};

  ASSERT_EQ(fd1.size(), fd2.size());
  ASSERT_EQ(fd1.size(), fd3.size());
  ASSERT_EQ(fd1.size(), fd4.size());

  AirTreeOptions options;
  options.dimensions = 4;
  options.type = ConfigType::XP;
  auto buffer = airtree::core::api::generate(fd1, fd2, fd3, fd4, options);

  std::vector<double> d1(fd1.begin(), fd1.end());
  std::vector<double> d2(fd2.begin(), fd2.end());
  std::vector<double> d3(fd3.begin(), fd3.end());
  std::vector<double> d4(fd4.begin(), fd4.end());
  verifyHistogramSanity4D(buffer, fd1.size());
  verifyBinAccuracy4DxP(buffer, d1, d2, d3, d4);
}

// --- 4D Saturation Zone ---

TEST(SaturationZoneSanity_4D, SaturationZoneSanity_4DxP_fntest) {
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
  options.dimensions = 4;
  options.type = ConfigType::XP;
  auto buffer = airtree::core::api::generate(data, data, data, data, options);
  verifyHistogramSanity4D(buffer, data.size());
  verifyBinAccuracy4DxP(buffer, data, data, data, data);
}
