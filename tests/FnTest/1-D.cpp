#include <airtree/core/api/AirTreeGenerator.hpp>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/common/Reconstruct.hpp>
#include <airtree/query/AirTreeQuery_internal.hpp>
#include <airtree/query/bin-boundary/BinBoundary.hpp>

#include <gtest/gtest.h>
#include <airtree/core/utils/Utils.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <functional>
#include <map>
#include <vector>

using namespace airtree::core::api;
using namespace airtree::core::io;
using namespace airtree::query::bin_boundary;

const std::string dim1_file_path = "../../tests/TestData/dim1_vx.bin";
const std::string result_file_path =
    "../../tests/FnTest_Results/1D_output.json";

// Read the data from the binary file
const std::vector<double> dim1_data = readBinaryFile(dim1_file_path);


TEST(Generate_1d_test_data_splits, Generate_1d_test_data_splits_fntest) {
  GTEST_SKIP();
  // Check that data is non-empty and has the expected structure
  ASSERT_FALSE(dim1_data.empty()) << "Data from dim1.bin file is empty.";


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

  AirTreeOptions options;
  options.dimensions = 1;
  options.type = ConfigType::XP;

  // Split each dim's vector into two parts to generate two separate histogram
  // buffers
  std::vector<char> piece1 = generate(dim1_part1, options);
  std::cout << "Piece1 size: " << piece1.size() << std::endl;
  std::vector<char> piece2 = generate(dim1_part2, options);
  std::cout << "Piece2 size: " << piece2.size() << std::endl;
  std::vector<char> result = generate(dim1_data, options);
  std::cout << "Result size: " << result.size() << std::endl;
  AirTreeWriter::Write(result, "Original_1DxP.bin");
  AirTreeWriter::Write(piece1, "1DxP_piece1.bin");
  AirTreeWriter::Write(piece2, "1DxP_piece2.bin");

  // Generate the 1DxF result
  options.type = ConfigType::XF;
  piece1 = generate(dim1_part1, options);
  std::cout << "Piece1 size: " << piece1.size() << std::endl;
  piece2 = generate(dim1_part2, options);
  std::cout << "Piece2 size: " << piece2.size() << std::endl;
  result = generate(dim1_data, options);
  std::cout << "Result size: " << result.size() << std::endl;
  AirTreeWriter::Write(result, "Original_1DxF.bin");
  AirTreeWriter::Write(piece1, "1DxF_piece1.bin");
  AirTreeWriter::Write(piece2, "1DxF_piece2.bin");

  // 1DxT
  options.type = ConfigType::XT;
  piece1 = generate(dim1_part1, options);
  std::cout << "Piece1 size: " << piece1.size() << std::endl;
  piece2 = generate(dim1_part2, options);
  std::cout << "Piece2 size: " << piece2.size() << std::endl;
  result = generate(dim1_data, options);
  std::cout << "Result size: " << result.size() << std::endl;
  AirTreeWriter::Write(result, "Original_1DxT.bin");
  AirTreeWriter::Write(piece1, "1DxT_piece1.bin");
  AirTreeWriter::Write(piece2, "1DxT_piece2.bin");
}


TEST(Generate_13Colonies, Generate_13Colonies_fntest) {
  // Check that data is non-empty and has the expected structure
  ASSERT_FALSE(dim1_data.empty()) << "Data from binary file is empty.";

  AirTreeOptions options;
  options.dimensions = 1;
  options.type = ConfigType::XT;

  std::vector<char> result = generate(dim1_data, options);

  std::string result_hash = hashBuffer(result);

  std::string expected_hash = readValueFromJson(result_file_path, "13Colonies");

  EXPECT_EQ(result_hash, expected_hash);
}


TEST(Generate_Apollo16, Generate_Apollo16_fntest) {
  // Check that data is non-empty and has the expected structure
  ASSERT_FALSE(dim1_data.empty()) << "Data from binary file is empty.";

  AirTreeOptions options;
  options.dimensions = 1;
  options.type = ConfigType::XF;

  std::vector<char> result = generate(dim1_data, options);

  std::string result_hash = hashBuffer(result);

  std::string expected_hash = readValueFromJson(result_file_path, "Apollo16");

  EXPECT_EQ(result_hash, expected_hash);
}

TEST(Generate_Roaring20, Generate_Roaring20_fntest) {
  // Check that data is non-empty and has the expected structure
  ASSERT_FALSE(dim1_data.empty()) << "Data from binary file is empty.";

  AirTreeOptions options;
  options.dimensions = 1;
  options.type = ConfigType::XP;

  std::vector<char> result = generate(dim1_data, options);

  std::string result_hash = hashBuffer(result);

  std::string expected_hash = readValueFromJson(result_file_path, "Roaring20");

  EXPECT_EQ(result_hash, expected_hash);
}

// ============================================================================
// Histogram Sanity Tests
// Verify structural correctness of the generated histogram:
//   1. Total count matches non-special input count (no data lost)
//   2. Bins form a non-overlapping partition (no duplicate counting)
//   3. No NaN/Inf in bin boundaries (no reconstruction bugs)
//
// Per-bin count verification is done separately by verifyBinAccuracy1D
// below, which re-encodes each value and matches by pattern.
// Geometric check verifies each value falls in its bin:
//   Positive bins: [lower, upper)
//   Negative bins: (lower, upper]
// ============================================================================

static void verifyHistogramSanity1D(const std::vector<char> &buffer,
                                    const std::vector<double> &raw_data) {
  BinBoundary query(buffer);
  auto result = query.generateBinBoundaries();
  auto bins = std::get<BinBoundary1DList>(*result.getBoundaries());

  // Sort bins by lower bound
  std::sort(bins.begin(), bins.end(),
            [](const BinBoundary1D &a, const BinBoundary1D &b) {
              return a.getLowerBound() < b.getLowerBound();
            });

  // Count non-special values
  int non_special = 0;
  for (double v : raw_data) {
    if (v != 0.0 && !std::isnan(v) && !std::isinf(v))
      non_special++;
  }

  // 1. Total histogram count must equal non-special data count
  int total_hist = 0;
  for (const auto &bin : bins)
    total_hist += bin.getCount();
  EXPECT_EQ(total_hist, non_special)
      << "Total histogram count (" << total_hist
      << ") != non-special data count (" << non_special << ")";

  // 2. Bins must form a non-overlapping partition
  for (size_t i = 1; i < bins.size(); i++) {
    EXPECT_GE(bins[i].getLowerBound(), bins[i - 1].getUpperBound())
        << "Bin " << i << " overlaps with previous";
  }

  // 3. No NaN/Inf in boundaries
  for (size_t i = 0; i < bins.size(); i++) {
    EXPECT_FALSE(std::isnan(bins[i].getLowerBound()))
        << "Bin " << i << " lower is NaN";
    EXPECT_FALSE(std::isnan(bins[i].getUpperBound()))
        << "Bin " << i << " upper is NaN";
    // Lower bound may be -inf for the most-negative bin
    EXPECT_FALSE(std::isinf(bins[i].getLowerBound())
                 && bins[i].getLowerBound() > 0)
        << "Bin " << i << " lower is +Inf";
    EXPECT_FALSE(std::isinf(bins[i].getUpperBound()))
        << "Bin " << i << " upper is Inf";
    EXPECT_GT(bins[i].getCount(), 0u) << "Bin " << i << " has zero count";
  }

  // 4. Every non-special value must geometrically fall in its bin.
  //    Positive bins: [lower, upper)
  //    Negative bins: (lower, upper]
  for (double v : raw_data) {
    if (v == 0.0 || std::isnan(v) || std::isinf(v))
      continue;
    bool found = false;
    for (const auto &bin : bins) {
      double lo = bin.getLowerBound();
      double hi = bin.getUpperBound();
      if (hi <= 0) {
        // Negative bin: (lower, upper]
        if (v > lo && v <= hi) {
          found = true;
          break;
        }
      } else {
        // Positive bin: [lower, upper)
        if (v >= lo && v < hi) {
          found = true;
          break;
        }
      }
    }
    EXPECT_TRUE(found) << "Value " << v << " not in any bin";
  }
}

// ============================================================================
// Bin Count Consistency Tests (pattern-based)
// Re-encode each value to get its N-bit pattern, reConstruct the pattern to
// get the canonical bin label, then match per-bin counts against BinBoundary
// output. This verifies:
//   - histogram generation and BinBoundary query agree on per-bin counts
//   - counts survive through serialization/deserialization
//   - no values are lost or double-counted
//
// Note: this oracle shares the encoder with the histogram, so it does NOT
// independently verify the encoding scheme against the spec. If encoder and
// oracle share the same defect, these tests still pass. To catch encoding
// bugs, the saturation zone and fuzz tests exercise edge-case inputs that
// would produce detectably wrong counts if the encoder were broken.
// ============================================================================

// encodeFn: encodes a double to the internal N-bit representation
// bitLength: the bit length for reConstruct (13, 16, or 20)
static void verifyBinAccuracy1D(const std::vector<char> &buffer,
                                const std::vector<double> &raw_data,
                                std::function<unsigned int(double)> encodeFn,
                                int bitLength) {
  BinBoundary query(buffer);
  auto result = query.generateBinBoundaries();
  auto bins = std::get<BinBoundary1DList>(*result.getBoundaries());

  // Build a map from reConstruct value → histogram count.
  // Positive bins [recon, next): recon = getLowerBound().
  // Negative bins (prev, recon]: recon = getUpperBound().
  std::map<double, uint32_t> hist_counts;
  for (const auto &bin : bins) {
    double recon =
        (bin.getUpperBound() <= 0) ? bin.getUpperBound() : bin.getLowerBound();
    hist_counts[recon] = bin.getCount();
  }

  // Re-encode each non-special value and count by reConstruct label
  std::map<double, int> actual_counts;
  for (double v : raw_data) {
    if (v == 0.0 || std::isnan(v) || std::isinf(v))
      continue;
    unsigned int pattern = encodeFn(v);
    double bin_label = reConstruct<double>(pattern, bitLength);
    actual_counts[bin_label]++;
  }

  // Compare per-bin counts
  for (const auto &[label, hist_count] : hist_counts) {
    int actual = actual_counts.count(label) ? actual_counts[label] : 0;
    EXPECT_EQ(actual, static_cast<int>(hist_count))
        << "Bin at recon=" << label << ": actual=" << actual
        << " hist=" << hist_count;
  }

  // Check no extra bins in actual that aren't in histogram
  for (const auto &[label, count] : actual_counts) {
    EXPECT_TRUE(hist_counts.count(label) > 0)
        << "Value encoded to bin at recon=" << label
        << " which doesn't exist in histogram output";
  }
}

// Encoder wrappers matching each 1D config's path (Double, default_mode=true)
static unsigned int encode_1DxP(double val) {
  uint64_t fp;
  std::memcpy(&fp, &val, sizeof(val));
  return createInternal20Bit(fp, true);
}
static unsigned int encode_1DxT(double val) {
  uint64_t fp;
  std::memcpy(&fp, &val, sizeof(val));
  return createInternal13Bit(fp, true);
}
static unsigned int encode_1DxF(double val) {
  uint64_t fp;
  std::memcpy(&fp, &val, sizeof(val));
  return createInternal16Bit(fp, true);
}

// --- Test 1: Sanity + accuracy with real data for each 1D config ---

TEST(HistogramSanity_1DxP, HistogramSanity_1DxP_fntest) {
  ASSERT_FALSE(dim1_data.empty());
  AirTreeOptions options;
  options.dimensions = 1;
  options.type = ConfigType::XP;
  auto buffer = generate(dim1_data, options);
  verifyHistogramSanity1D(buffer, dim1_data);
  verifyBinAccuracy1D(buffer, dim1_data, encode_1DxP, 20);
}

TEST(HistogramSanity_1DxT, HistogramSanity_1DxT_fntest) {
  ASSERT_FALSE(dim1_data.empty());
  AirTreeOptions options;
  options.dimensions = 1;
  options.type = ConfigType::XT;
  auto buffer = generate(dim1_data, options);
  verifyHistogramSanity1D(buffer, dim1_data);
  verifyBinAccuracy1D(buffer, dim1_data, encode_1DxT, 13);
}

TEST(HistogramSanity_1DxF, HistogramSanity_1DxF_fntest) {
  ASSERT_FALSE(dim1_data.empty());
  AirTreeOptions options;
  options.dimensions = 1;
  options.type = ConfigType::XF;
  auto buffer = generate(dim1_data, options);
  verifyHistogramSanity1D(buffer, dim1_data);
  verifyBinAccuracy1D(buffer, dim1_data, encode_1DxF, 16);
}

// --- Test 2: Saturation zone — values near powers of 2 ---

TEST(SaturationZoneSanity, SaturationZoneSanity_1DxP_fntest) {
  std::vector<double> data;
  for (int p = -10; p <= 10; p++) {
    double base = std::pow(2.0, p);
    for (double eps : {1e-7, 1e-5, 1e-3, 0.005, 0.01, 0.015}) {
      data.push_back(base * (1.0 + eps));
      data.push_back(base * (1.0 - eps));
      data.push_back(-base * (1.0 + eps));
      data.push_back(-base * (1.0 - eps));
    }
  }

  AirTreeOptions options;
  options.dimensions = 1;
  options.type = ConfigType::XP;
  auto buffer = generate(data, options);
  verifyHistogramSanity1D(buffer, data);
  verifyBinAccuracy1D(buffer, data, encode_1DxP, 20);
}

TEST(SaturationZoneSanity, SaturationZoneSanity_1DxT_fntest) {
  std::vector<double> data;
  for (int p = -10; p <= 10; p++) {
    double base = std::pow(2.0, p);
    for (double eps : {1e-7, 1e-5, 1e-3, 0.005, 0.01, 0.015}) {
      data.push_back(base * (1.0 + eps));
      data.push_back(base * (1.0 - eps));
      data.push_back(-base * (1.0 + eps));
      data.push_back(-base * (1.0 - eps));
    }
  }

  AirTreeOptions options;
  options.dimensions = 1;
  options.type = ConfigType::XT;
  auto buffer = generate(data, options);
  verifyHistogramSanity1D(buffer, data);
  verifyBinAccuracy1D(buffer, data, encode_1DxT, 13);
}

TEST(SaturationZoneSanity, SaturationZoneSanity_1DxF_fntest) {
  std::vector<double> data;
  for (int p = -10; p <= 10; p++) {
    double base = std::pow(2.0, p);
    for (double eps : {1e-7, 1e-5, 1e-3, 0.005, 0.01, 0.015}) {
      data.push_back(base * (1.0 + eps));
      data.push_back(base * (1.0 - eps));
      data.push_back(-base * (1.0 + eps));
      data.push_back(-base * (1.0 - eps));
    }
  }

  AirTreeOptions options;
  options.dimensions = 1;
  options.type = ConfigType::XF;
  auto buffer = generate(data, options);
  verifyHistogramSanity1D(buffer, data);
  verifyBinAccuracy1D(buffer, data, encode_1DxF, 16);
}

// --- Test 4: Float32 input pipeline ---

TEST(Float32Sanity, Float32Sanity_1DxP_fntest) {
  std::vector<float> float_data = {
      1.0f,   1.001f,   1.00001f, 1.01f,   1.1f,   0.5f,    0.5001f, 0.50001f,
      0.999f, 0.9999f,  2.0f,     2.001f,  4.0f,   4.01f,   0.125f,  0.126f,
      0.25f,  0.251f,   5.6f,     26.8f,   100.5f, 2500.3f, -1.0f,   -1.001f,
      -0.5f,  -0.5001f, 0.001f,   0.0001f, 10.0f,  1000.0f,
  };

  AirTreeOptions options;
  options.dimensions = 1;
  options.type = ConfigType::XP;
  auto buffer = generate(float_data, options);

  // Convert to double for verification (matching the encoder's path)
  std::vector<double> as_double(float_data.begin(), float_data.end());
  verifyHistogramSanity1D(buffer, as_double);
  verifyBinAccuracy1D(buffer, as_double, encode_1DxP, 20);
}

// --- Test 5: Export semantic correctness ---
// This is now covered by verifyBinAccuracy1D (checks NaN, Inf, overlap,
// counts). The BinAccuracy tests above exercise it for all configs.
