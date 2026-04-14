#include <gtest/gtest.h>
#include <airtree/core/api/AirTreeGenerator.hpp>
#include <airtree/core/common/FPHArray.hpp>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/common/Reconstruct.hpp>
#include <airtree/core/common/SpecialCounts.hpp>
#include <airtree/query/AirTreeQuery_internal.hpp>
#include <airtree/query/bin-boundary/BinBoundary.hpp>
#include <airtree/core/io/AirTreeWriter.hpp>
#include <airtree/core/utils/Utils.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <map>
#include <vector>


using namespace airtree::core::io;
using namespace airtree::core::api;
using namespace airtree::query::bin_boundary;


const std::string dim1_file_path = "../../tests/TestData/dim1_vx.bin";
const std::string dim2_file_path = "../../tests/TestData/dim2_vy.bin";
const std::string result_file_path =
    "../../tests/FnTest_Results/2D_output.json";

// Read the data from the binary file
const std::vector<double> dim1_data = readBinaryFile(dim1_file_path);
// Read the data from the binary file
const std::vector<double> dim2_data = readBinaryFile(dim2_file_path);

TEST(Generate_2d_test_data_splits, Generate_2d_test_data_splits_fntest) {
  GTEST_SKIP();
  // Check that data is non-empty and has the expected structure
  ASSERT_FALSE(dim1_data.empty()) << "Data from dim1.bin file is empty.";
  ASSERT_FALSE(dim2_data.empty()) << "Data from dim2.bin file is empty.";

  // make sure all dims have the same size
  ASSERT_EQ(dim1_data.size(), dim2_data.size())
      << "Data from dim1.bin and dim2.bin files have different sizes.";

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

  AirTreeOptions options;
  options.dimensions = 2;
  options.type = ConfigType::XP;
  // Split each dim's vector into two parts to generate two separate histogram
  // buffers
  std::vector<char> piece1 =
      airtree::core::api::generate(dim1_part1, dim2_part1, options);
  std::cout << "Piece1 size: " << piece1.size() << std::endl;
  std::vector<char> piece2 =
      airtree::core::api::generate(dim1_part2, dim2_part2, options);
  std::cout << "Piece2 size: " << piece2.size() << std::endl;
  std::vector<char> result =
      airtree::core::api::generate(dim1_data, dim2_data, options);
  std::cout << "Result size: " << result.size() << std::endl;
  AirTreeWriter::Write(result, "Original_2DxP.bin");
  AirTreeWriter::Write(piece1, "2DxP_piece1.bin");
  AirTreeWriter::Write(piece2, "2DxP_piece2.bin");

  options.type = ConfigType::XF;
  // Generate the 2DxF result
  piece1 = airtree::core::api::generate(dim1_part1, dim2_part1, options);
  std::cout << "Piece1 size: " << piece1.size() << std::endl;
  piece2 = airtree::core::api::generate(dim1_part2, dim2_part2, options);
  std::cout << "Piece2 size: " << piece2.size() << std::endl;
  result = airtree::core::api::generate(dim1_data, dim2_data, options);
  std::cout << "Result size: " << result.size() << std::endl;
  AirTreeWriter::Write(result, "Original_2DxF.bin");
  AirTreeWriter::Write(piece1, "2DxF_piece1.bin");
  AirTreeWriter::Write(piece2, "2DxF_piece2.bin");
}


TEST(Generate_2d_2x10, Generate_2d_2x10_fntest) {
  // Check that data is non-empty and has the expected structure
  ASSERT_FALSE(dim1_data.empty()) << "Data from dim1.bin file is empty.";
  ASSERT_FALSE(dim2_data.empty()) << "Data from dim2.bin file is empty.";

  AirTreeOptions options;
  options.dimensions = 2;
  options.type = ConfigType::XP;
  std::vector<char> result =
      airtree::core::api::generate(dim1_data, dim2_data, options);

  std::string result_hash = hashBuffer(result);

  std::string expected_hash = readValueFromJson(result_file_path, "2x10");

  EXPECT_EQ(result_hash, expected_hash);
}

TEST(Generate_2d_88, DISABLED_Generate_2d_88_fntest) {
  // Check that data is non-empty and has the expected structure
  ASSERT_FALSE(dim1_data.empty()) << "Data from dim1.bin file is empty.";
  ASSERT_FALSE(dim2_data.empty()) << "Data from dim2.bin file is empty.";

  AirTreeOptions options;
  options.dimensions = 2;
  options.type = ConfigType::XF;
  std::vector<char> result =
      airtree::core::api::generate(dim1_data, dim2_data, options);

  std::string result_hash = hashBuffer(result);

  std::string expected_hash = readValueFromJson(result_file_path, "2d_88");

  EXPECT_EQ(result_hash, expected_hash);
}

// ============================================================================
// Histogram Sanity Tests for 2D
// Verify:
//   1. Total count matches total input row count (multi-D TLE handles
//      mixed special/non-special dimensions, so all rows are filed)
//   2. No NaN/Inf in bin boundaries
//   3. All bins have non-zero count
// ============================================================================

// For multi-dimensional histograms, the TLE handles mixed special/non-special
// dimensions. All input rows are filed — either into the trie (with TLE
// indicating special dimensions) or as fully-special counts. So the total
// histogram count should equal the total number of input rows.

static void verifyHistogramSanity2D(const std::vector<char> &buffer,
                                    const std::vector<double> &dim1,
                                    const std::vector<double> &dim2) {
  BinBoundary query(buffer);
  auto result = query.generateBinBoundaries();
  auto bins = std::get<BinBoundary2DList>(*result.getBoundaries());

  // 1. Total count
  int total_hist = 0;
  for (const auto &bin : bins)
    total_hist += bin.getCount();

  int expected = static_cast<int>(dim1.size());
  EXPECT_EQ(total_hist, expected)
      << "Total histogram count (" << total_hist << ") != input row count ("
      << expected << ")";

  // 2. No NaN/Inf in boundaries, no zero-count bins
  for (size_t i = 0; i < bins.size(); i++) {
    EXPECT_FALSE(std::isnan(bins[i].getLowerBoundX()))
        << "Bin " << i << " x-min is NaN";
    EXPECT_FALSE(std::isnan(bins[i].getUpperBoundX()))
        << "Bin " << i << " x-max is NaN";
    EXPECT_FALSE(std::isnan(bins[i].getLowerBoundY()))
        << "Bin " << i << " y-min is NaN";
    EXPECT_FALSE(std::isnan(bins[i].getUpperBoundY()))
        << "Bin " << i << " y-max is NaN";
    // Lower bounds may be -inf for the most-negative bin (no previous entry).
    // Upper bounds should never be -inf. Neither should be +inf.
    EXPECT_FALSE(std::isinf(bins[i].getLowerBoundX())
                 && bins[i].getLowerBoundX() > 0)
        << "Bin " << i << " x-min is +Inf";
    EXPECT_FALSE(std::isinf(bins[i].getUpperBoundX()))
        << "Bin " << i << " x-max is Inf";
    EXPECT_FALSE(std::isinf(bins[i].getLowerBoundY())
                 && bins[i].getLowerBoundY() > 0)
        << "Bin " << i << " y-min is +Inf";
    EXPECT_FALSE(std::isinf(bins[i].getUpperBoundY()))
        << "Bin " << i << " y-max is Inf";
    EXPECT_GT(bins[i].getCount(), 0u) << "Bin " << i << " has zero count";
  }
}

// ============================================================================
// Bin Count Consistency for 2D (pattern-based)
// Same approach as 1D: re-encode each dimension with createInternal10Bit,
// reconstruct to get the canonical label, match against BinBoundary output.
// ============================================================================

// Helper: get the recon value from a bin boundary for one dimension.
// Positive: [recon, next) → recon = lower. Negative: (prev, recon] → recon =
// upper.
static double reconFromBounds(double lower, double upper) {
  return (upper <= 0) ? upper : lower;
}

// Check if a value is special (zero, NaN, Inf) — these go to TLE, not encoding
static bool isSpecialValue(double v) {
  uint64_t fp;
  std::memcpy(&fp, &v, sizeof(v));
  SpecialCounts dummy;
  return isSpecialCase(fp, dummy);
}

// Encode a double to 12-bit representation (2-bit TLE prefix + 10-bit
// internal). This matches what BinBoundary uses: bits12 = (prefix << 10) |
// internal10bit.
static unsigned int encode_12bit(double val) {
  uint64_t fp;
  std::memcpy(&fp, &val, sizeof(val));
  TLE tle = setTLEComponents(fp);
  unsigned int internal10 = createInternal10Bit(fp, true);

  // Map TLE to 2-bit prefix (same as getprependbits)
  unsigned int prefix;
  switch (tle.TLE) {
  case 2:
    prefix = 0b00;
    break; // +sign, +exp
  case 3:
    prefix = 0b01;
    break; // +sign, -exp
  case 5:
    prefix = 0b11;
    break; // -sign, -exp
  case 6:
    prefix = 0b10;
    break; // -sign, +exp
  default:
    return 0xFFFF; // special value — shouldn't reach here
  }
  return (prefix << 10) | internal10;
}

static void verifyBinAccuracy2DxP(const std::vector<char> &buffer,
                                  const std::vector<double> &dim1,
                                  const std::vector<double> &dim2) {
  BinBoundary query(buffer);
  auto result = query.generateBinBoundaries();
  auto bins = std::get<BinBoundary2DList>(*result.getBoundaries());

  // Skip bins with special-dimension bounds (NaN/Inf/0) before inserting
  // into the map. NaN violates strict weak ordering and would corrupt the
  // std::map BST if used as a key.
  auto hasSpecial = [](double v) {
    return v == 0.0 || std::isnan(v) || std::isinf(v);
  };

  std::map<std::pair<double, double>, uint32_t> hist_counts;
  for (const auto &bin : bins) {
    double rx = reconFromBounds(bin.getLowerBoundX(), bin.getUpperBoundX());
    double ry = reconFromBounds(bin.getLowerBoundY(), bin.getUpperBoundY());
    if (hasSpecial(rx) || hasSpecial(ry))
      continue;
    hist_counts[{rx, ry}] += bin.getCount();
  }

  // Re-encode each non-special row and count by (recon_x, recon_y)
  std::map<std::pair<double, double>, int> actual_counts;
  for (size_t i = 0; i < dim1.size(); i++) {
    if (isSpecialValue(dim1[i]) || isSpecialValue(dim2[i]))
      continue;
    unsigned int p1 = encode_12bit(dim1[i]);
    unsigned int p2 = encode_12bit(dim2[i]);
    double rx = reConstruct<double>(p1, 12);
    double ry = reConstruct<double>(p2, 12);
    actual_counts[{rx, ry}]++;
  }

  // Compare
  for (const auto &[key, hist_count] : hist_counts) {
    int actual = actual_counts.count(key) ? actual_counts[key] : 0;
    EXPECT_EQ(actual, static_cast<int>(hist_count))
        << "Bin at (" << key.first << ", " << key.second
        << "): actual=" << actual << " hist=" << hist_count;
  }

  for (const auto &[key, count] : actual_counts) {
    EXPECT_TRUE(hist_counts.count(key) > 0)
        << "Row encoded to (" << key.first << ", " << key.second
        << ") which doesn't exist in histogram output";
  }
}

// --- 2D Sanity Tests ---

TEST(HistogramSanity_2DxP, HistogramSanity_2DxP_fntest) {
  ASSERT_FALSE(dim1_data.empty());
  ASSERT_FALSE(dim2_data.empty());
  AirTreeOptions options;
  options.dimensions = 2;
  options.type = ConfigType::XP;
  auto buffer = airtree::core::api::generate(dim1_data, dim2_data, options);
  verifyHistogramSanity2D(buffer, dim1_data, dim2_data);
  verifyBinAccuracy2DxP(buffer, dim1_data, dim2_data);
}

// --- 2D Saturation Zone ---

// --- Gap 2: Mixed special/non-special row validation ---
// Verify that rows with special values (0, NaN, Inf) in one dimension
// and normal values in the other are properly filed via TLE.
// Appends special rows to the real test data so the trie has proven-working
// normal data, and we verify the addition of specials doesn't break counts.

TEST(MixedSpecialSanity_2D, MixedSpecialSanity_2DxP_fntest) {
  ASSERT_FALSE(dim1_data.empty());
  ASSERT_FALSE(dim2_data.empty());

  std::vector<double> d1(dim1_data), d2(dim2_data);

  double specials[] = {0.0, -0.0, std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity()};
  // Special in dim2
  for (double s : specials) {
    d1.push_back(1.5);
    d2.push_back(s);
  }
  // Special in dim1
  for (double s : specials) {
    d1.push_back(s);
    d2.push_back(2.5);
  }

  AirTreeOptions options;
  options.dimensions = 2;
  options.type = ConfigType::XP;
  auto buffer = airtree::core::api::generate(d1, d2, options);

  // Total count must equal input row count (TLE files all rows)
  BinBoundary query(buffer);
  auto result = query.generateBinBoundaries();
  auto bins = std::get<BinBoundary2DList>(*result.getBoundaries());
  int total = 0;
  for (const auto &bin : bins)
    total += bin.getCount();
  EXPECT_EQ(total, static_cast<int>(d1.size()))
      << "Total histogram count != input row count with mixed specials";

  // Non-special bin accuracy
  verifyBinAccuracy2DxP(buffer, d1, d2);
}

// --- Gap 3: Float32 input for multi-D ---

TEST(Float32Sanity_2D, Float32Sanity_2DxP_fntest) {
  std::vector<float> float_d1 = {1.0f,  1.001f, 0.5f,   0.5001f, 2.0f,  2.001f,
                                 -1.0f, -0.5f,  0.125f, 5.6f,    26.8f, 100.5f};
  std::vector<float> float_d2 = {0.25f,  0.251f,  4.0f,    4.01f,
                                 0.999f, 0.9999f, -1.001f, -0.5001f,
                                 0.126f, 10.0f,   0.001f,  1000.0f};

  ASSERT_EQ(float_d1.size(), float_d2.size());

  AirTreeOptions options;
  options.dimensions = 2;
  options.type = ConfigType::XP;
  auto buffer = airtree::core::api::generate(float_d1, float_d2, options);

  std::vector<double> d1_dbl(float_d1.begin(), float_d1.end());
  std::vector<double> d2_dbl(float_d2.begin(), float_d2.end());
  verifyHistogramSanity2D(buffer, d1_dbl, d2_dbl);
  verifyBinAccuracy2DxP(buffer, d1_dbl, d2_dbl);
}

// --- 2D Saturation Zone ---

TEST(SaturationZoneSanity_2D, SaturationZoneSanity_2DxP_fntest) {
  std::vector<double> data;
  for (int p = -5; p <= 5; p++) {
    double base = std::pow(2.0, p);
    for (double eps : {1e-5, 1e-3, 0.01, 0.015}) {
      data.push_back(base * (1.0 + eps));
      data.push_back(base * (1.0 - eps));
      data.push_back(-base * (1.0 + eps));
      data.push_back(-base * (1.0 - eps));
    }
  }

  // Use the same data for both dimensions
  AirTreeOptions options;
  options.dimensions = 2;
  options.type = ConfigType::XP;
  auto buffer = airtree::core::api::generate(data, data, options);
  verifyHistogramSanity2D(buffer, data, data);
  verifyBinAccuracy2DxP(buffer, data, data);
}
