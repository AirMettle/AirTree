// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <gtest/gtest.h>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/query/AirTreeQuery_internal.hpp>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <airtree/core/utils/TrieManager.hpp>
#include <cstddef>
#include <sys/types.h>
#include <vector>

using namespace airtree::query::percentile;
class TestPercentile : public ::testing::Test {
protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(TestPercentile, TestPercentile1DxT_RankEqualsBinCount) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(13);
  double percentile = 50.0;

  // Calculate expected percentile
  uint64_t cumulative_count = (BINS_256 * BINS_32) / 2;
  double rank = (percentile / 100.0) * (BINS_256 * BINS_32);
  uint64_t mid_idx = (BINS_256 * BINS_32) / 2;
  double min_value = histogram->getBins()[mid_idx].first;
  double max_value = histogram->getBins()[mid_idx + 1].first;
  double expected_percentile =
      min_value + (((rank - cumulative_count) / 1) * (max_value - min_value));

  // Build a test buffer for 1DxF
  TrieManager trieManager;
  // This should add a single value for each bin in the leaf.
  for (size_t l0_idx = 0; l0_idx < BINS_256; l0_idx++) {
    trieManager.insert1DxT(l0_idx, 32, DistributionMethod::EVEN);
  }

  // verify that the trie is populated correctly
  for (size_t idx = 0; idx < BINS_256 * BINS_32; idx++) {
    auto internal_rep = idx;
    uint64_t prefix_8 = (internal_rep >> 5) & 0xFF;
    uint64_t suffix_5 = internal_rep & 0x1F;
    GTEST_EXPECT_TRUE(trieManager.getRoot1DxT().populated.test(prefix_8));
    EXPECT_EQ(trieManager.getRoot1DxT().nodes[prefix_8]->counts[suffix_5], 1);
  }

  SpecialCounts noSpecialValues{
      0, 0, 0, 0, 0}; // No special values for this test
  std::vector<char> buffer =
      trieManager.MockTrieHeader(13, noSpecialValues);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto percentileQuery = std::make_shared<Percentile>(buffer);
  double result = percentileQuery->getPercentile(50);
  EXPECT_EQ(result, expected_percentile);
}

TEST_F(TestPercentile, TestPercentile1DxT_RankLessThanBinCount) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(13);
  double percentile = 50.0;

  // Calculate expected percentile
  uint64_t cumulative_count = (BINS_256 * BINS_32);
  double rank = (percentile / 100.0) * (2 * BINS_256 * BINS_32 + 2);
  uint64_t mid_idx = (BINS_256 * BINS_32) / 2;
  double min_value = histogram->getBins()[mid_idx].first;
  double max_value = histogram->getBins()[mid_idx + 1].first;
  double expected_percentile =
      min_value + (((rank - cumulative_count) / 4) * (max_value - min_value));

  // Build a test buffer for 1DxF
  TrieManager trieManager;

  // This should add 2 values for each bin in the leaf.
  for (size_t l0_idx = 0; l0_idx < BINS_256; l0_idx++) {
    trieManager.insert1DxT(l0_idx, 64, DistributionMethod::EVEN);
  }

  // Add another count to the mid bin to ensure that the rank is less than
  // the bin count.
  // get mid bin index
  unsigned int mid_bin_index = BINS_256 * BINS_32 / 2;
  uint64_t histogram_mid_bin_idx =
      histogram->getInternalRepresentation(mid_bin_index);
  uint64_t prefix__mid_8 = (histogram_mid_bin_idx >> 5) & 0xFF;
  uint64_t suffix__mid_5 = histogram_mid_bin_idx & 0x1F;
  trieManager.insert1DxT(
      prefix__mid_8, 2, DistributionMethod::SINGLE, suffix__mid_5);

  // verify that the trie is populated correctly
  for (size_t idx = 0; idx < BINS_256 * BINS_32; idx++) {
    auto internal_rep = idx;
    uint64_t prefix_8 = (internal_rep >> 5) & 0xFF;
    uint64_t suffix_5 = internal_rep & 0x1F;
    if (prefix_8 == prefix__mid_8 && suffix_5 == suffix__mid_5) {
      GTEST_EXPECT_TRUE(trieManager.getRoot1DxT().populated.test(prefix_8));
      EXPECT_EQ(trieManager.getRoot1DxT().nodes[prefix_8]->counts[suffix_5], 4);
      continue;
    }
    GTEST_EXPECT_TRUE(trieManager.getRoot1DxT().populated.test(prefix_8));
    EXPECT_EQ(trieManager.getRoot1DxT().nodes[prefix_8]->counts[suffix_5], 2);
  }

  SpecialCounts noSpecialValues{
      0, 0, 0, 0, 0}; // No special values for this test
  std::vector<char> buffer =
      trieManager.MockTrieHeader(13, noSpecialValues);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto percentileQuery = std::make_shared<Percentile>(buffer);
  double result = percentileQuery->getPercentile(percentile);
  EXPECT_EQ(result, expected_percentile);
}


TEST_F(TestPercentile, TestPercentile1DxF_RankEqualsBinCount) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(16);
  double percentile = 50.0;

  // Calculate expected percentile
  uint64_t cumulative_count = (BINS_256 * BINS_256) / 2;
  double rank = (percentile / 100.0) * (BINS_256 * BINS_256);
  uint64_t mid_idx = (BINS_256 * BINS_256) / 2;
  double min_value = histogram->getBins()[mid_idx].first;
  double max_value = histogram->getBins()[mid_idx + 1].first;
  double expected_percentile =
      min_value + (((rank - cumulative_count) / 1) * (max_value - min_value));

  // Build a test buffer for 1DxF
  TrieManager trieManager;
  // This should add a single value for each bin in the leaf.
  for (size_t l0_idx = 0; l0_idx < BINS_256; l0_idx++) {
    trieManager.insert1DxF(l0_idx, BINS_256, DistributionMethod::EVEN);
  }

  // verify that the trie is populated correctly
  for (size_t idx = 0; idx < BINS_256 * BINS_256; idx++) {
    auto internal_rep = idx;
    uint64_t prefix_8 = (internal_rep >> 8) & 0xFF;
    uint64_t suffix_8 = internal_rep & 0xFF;
    GTEST_EXPECT_TRUE(trieManager.getRoot1DxF().populated.test(prefix_8));
    EXPECT_EQ(trieManager.getRoot1DxF().nodes[prefix_8]->counts[suffix_8], 1);
  }

  std::vector<char> buffer = trieManager.MockTrieHeader(16);
  trieManager.serializeTrie<TrieNode_16>(buffer);

  auto percentileQuery = std::make_shared<Percentile>(buffer);
  double result = percentileQuery->getPercentile(50);
  EXPECT_EQ(result, expected_percentile);
}

TEST_F(TestPercentile, TestPercentile1DxF_RankLessThanBinCount) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(16);
  double percentile = 50.0;

  // Calculate expected percentile
  uint64_t cumulative_count = (BINS_256 * BINS_256);
  double rank = (percentile / 100.0) * (2 * BINS_256 * BINS_256 + 2);
  uint64_t mid_idx = (BINS_256 * BINS_256) / 2;
  double min_value = histogram->getBins()[mid_idx].first;
  double max_value = histogram->getBins()[mid_idx + 1].first;
  double expected_percentile =
      min_value + (((rank - cumulative_count) / 4) * (max_value - min_value));

  // Build a test buffer for 1DxF
  TrieManager trieManager;

  // This should add 2 values for each bin in the leaf.
  for (size_t l0_idx = 0; l0_idx < BINS_256; l0_idx++) {
    trieManager.insert1DxF(l0_idx, BINS_256 * 2, DistributionMethod::EVEN);
  }

  // Add another count to the mid bin to ensure that the rank is less than
  // the bin count.
  // get mid bin index
  unsigned int mid_bin_index = BINS_256 * BINS_256 / 2;
  uint64_t histogram_mid_bin_idx =
      histogram->getInternalRepresentation(mid_bin_index);
  uint64_t prefix__mid_8 = (histogram_mid_bin_idx >> 8) & 0xFF;
  uint64_t suffix__mid_8 = histogram_mid_bin_idx & 0xFF;
  trieManager.insert1DxF(
      prefix__mid_8, 2, DistributionMethod::SINGLE, suffix__mid_8);

  // verify that the trie is populated correctly
  for (size_t idx = 0; idx < BINS_256 * BINS_256; idx++) {
    auto internal_rep = idx;
    uint64_t prefix_8 = (internal_rep >> 8) & 0xFF;
    uint64_t suffix_8 = internal_rep & 0xFF;
    if (prefix_8 == prefix__mid_8 && suffix_8 == suffix__mid_8) {
      GTEST_EXPECT_TRUE(trieManager.getRoot1DxF().populated.test(prefix_8));
      EXPECT_EQ(trieManager.getRoot1DxF().nodes[prefix_8]->counts[suffix_8], 4);
      continue;
    }
    GTEST_EXPECT_TRUE(trieManager.getRoot1DxF().populated.test(prefix_8));
    EXPECT_EQ(trieManager.getRoot1DxF().nodes[prefix_8]->counts[suffix_8], 2);
  }

  SpecialCounts noSpecialValues{
      0, 0, 0, 0, 0}; // No special values for this test
  std::vector<char> buffer =
      trieManager.MockTrieHeader(16, noSpecialValues);
  trieManager.serializeTrie<TrieNode_16>(buffer);

  auto percentileQuery = std::make_shared<Percentile>(buffer);
  double result = percentileQuery->getPercentile(percentile);
  EXPECT_EQ(result, expected_percentile);
}

TEST_F(TestPercentile, TestPercentile1DxP_RankEqualsBinCount) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(20);
  double percentile = 50.0;

  // Calculate expected percentile
  uint64_t cumulative_count = (BINS_1024 * BINS_1024) / 2;
  double rank = (percentile / 100.0) * (BINS_1024 * BINS_1024);
  uint64_t mid_idx = (BINS_1024 * BINS_1024) / 2;
  double min_value = histogram->getBins()[mid_idx].first;
  double max_value = histogram->getBins()[mid_idx + 1].first;
  double expected_percentile =
      min_value + (((rank - cumulative_count) / 1) * (max_value - min_value));

  // Build a test buffer for 1DxF
  TrieManager trieManager;
  // This should add a single value for each bin in the leaf.
  for (size_t l0_idx = 0; l0_idx < BINS_256; l0_idx++) {
    for (size_t l1_idx = 0; l1_idx < BINS_64; l1_idx++) {
      trieManager.insert1DxP(l0_idx, l1_idx, BINS_64, DistributionMethod::EVEN);
    }
  }

  // verify that the trie is populated correctly
  for (size_t idx = 0; idx < BINS_1024 * BINS_1024; idx++) {
    auto internal_rep = idx;
    uint64_t prefix_8 = (internal_rep >> 12) & 0xFF;
    uint64_t mid_6 = (internal_rep >> 6) & 0x3F;
    uint64_t suffix_6 = internal_rep & 0x3F;
    GTEST_EXPECT_TRUE(trieManager.getRoot1DxP().populated.test(prefix_8));
    GTEST_EXPECT_TRUE(
        trieManager.getRoot1DxP().nodes[prefix_8]->populated.test(mid_6));
    EXPECT_EQ(trieManager.getRoot1DxP()
                  .nodes[prefix_8]
                  ->nodes[mid_6]
                  ->counts[suffix_6],
              1);
  }

  std::vector<char> buffer = trieManager.MockTrieHeader(20);
  trieManager.serializeTrie<TrieNode_20>(buffer);

  auto percentileQuery = std::make_shared<Percentile>(buffer);
  double result = percentileQuery->getPercentile(50);
  EXPECT_EQ(result, expected_percentile);
}

TEST_F(TestPercentile, TestPercentile1DxP_RankLessThanBinCount) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(20);
  double percentile = 50.0;

  // Calculate expected percentile
  uint64_t cumulative_count = (BINS_1024 * BINS_1024);
  double rank = (percentile / 100.0) * (2 * BINS_1024 * BINS_1024 + 2);
  uint64_t mid_idx = (BINS_1024 * BINS_1024) / 2;
  double min_value = histogram->getBins()[mid_idx].first;
  double max_value = histogram->getBins()[mid_idx + 1].first;
  double expected_percentile =
      min_value + (((rank - cumulative_count) / 4) * (max_value - min_value));

  // Build a test buffer for 1DxF
  TrieManager trieManager;
  // This should add 2 values for each bin in the leaf.
  for (size_t l0_idx = 0; l0_idx < BINS_256; l0_idx++) {
    for (size_t l1_idx = 0; l1_idx < BINS_64; l1_idx++) {
      trieManager.insert1DxP(
          l0_idx, l1_idx, 2 * BINS_64, DistributionMethod::EVEN);
    }
  }

  // Add another count to the mid bin to ensure that the rank is less than
  // the bin count.
  // get mid bin index
  unsigned int mid_bin_index = BINS_1024 * BINS_1024 / 2;
  uint64_t histogram_mid_bin_idx =
      histogram->getInternalRepresentation(mid_bin_index);
  uint64_t prefix__mid_8 = (histogram_mid_bin_idx >> 12) & 0xFF;
  uint64_t mid__mid_6 = (histogram_mid_bin_idx >> 6) & 0x3F;
  uint64_t suffix__mid_6 = histogram_mid_bin_idx & 0x3F;
  trieManager.insert1DxP(
      prefix__mid_8, mid__mid_6, 2, DistributionMethod::SINGLE, suffix__mid_6);

  // verify that the trie is populated correctly
  for (size_t idx = 0; idx < BINS_1024 * BINS_1024; idx++) {
    auto internal_rep = idx;
    uint64_t prefix_8 = (internal_rep >> 12) & 0xFF;
    uint64_t mid_6 = (internal_rep >> 6) & 0x3F;
    uint64_t suffix_6 = internal_rep & 0x3F;
    if (prefix_8 == prefix__mid_8 && mid_6 == mid__mid_6
        && suffix_6 == suffix__mid_6) {
      GTEST_EXPECT_TRUE(trieManager.getRoot1DxP().populated.test(prefix_8));
      GTEST_EXPECT_TRUE(
          trieManager.getRoot1DxP().nodes[prefix_8]->populated.test(mid_6));
      EXPECT_EQ(trieManager.getRoot1DxP()
                    .nodes[prefix_8]
                    ->nodes[mid_6]
                    ->counts[suffix_6],
                4);
      continue;
    }
    GTEST_EXPECT_TRUE(trieManager.getRoot1DxP().populated.test(prefix_8));
    GTEST_EXPECT_TRUE(
        trieManager.getRoot1DxP().nodes[prefix_8]->populated.test(mid_6));
    EXPECT_EQ(trieManager.getRoot1DxP()
                  .nodes[prefix_8]
                  ->nodes[mid_6]
                  ->counts[suffix_6],
              2);
  }

  SpecialCounts noSpecialValues{
      0, 0, 0, 0, 0}; // No special values for this test
  std::vector<char> buffer =
      trieManager.MockTrieHeader(20, noSpecialValues);
  trieManager.serializeTrie<TrieNode_20>(buffer);

  auto percentileQuery = std::make_shared<Percentile>(buffer);
  double result = percentileQuery->getPercentile(50);
  EXPECT_EQ(result, expected_percentile);
}

// Test cases for special values handling

TEST_F(TestPercentile, TestPercentile_ZerosOnly) {
  // Test with only zeros
  std::vector<double> data = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  std::vector<char> buffer = generate_1DxF(array);

  auto percentileQuery = std::make_shared<Percentile>(buffer);

  // 50th percentile should be 0.0
  double result = percentileQuery->getPercentile(50);
  EXPECT_EQ(result, 0.0);

  // 25th percentile should be 0.0
  result = percentileQuery->getPercentile(25);
  EXPECT_EQ(result, 0.0);

  // 75th percentile should be 0.0
  result = percentileQuery->getPercentile(75);
  EXPECT_EQ(result, 0.0);
}

TEST_F(TestPercentile, TestPercentile_MixedZerosAndValues) {
  // Test with zeros and regular values - simulates [0, 0, 0, 0, 1, 1, 1]
  std::vector<double> data = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0};

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  std::vector<char> buffer = generate_1DxF(array);

  auto percentileQuery = std::make_shared<Percentile>(buffer);

  // 50th percentile: rank = 0.5 * 7 = 3.5
  // Sorted: [0, 0, 0, 0, 1, 1, 1]
  // Should be 0.0 (still in the zero range)
  double result = percentileQuery->getPercentile(50);
  EXPECT_EQ(result, 0.0);

  // 60th percentile: rank = 0.6 * 7 = 4.2
  // Should fall between the 4th zero and 5th value (~1.0)
  result = percentileQuery->getPercentile(60);
  EXPECT_DOUBLE_EQ(
      result, 1.0000020345052083); // Interpolated within bin (updated after
                                   // saturation fix)

  // 80th percentile: rank = 0.8 * 7 = 5.6
  // Should be close to 1.0 (within histogram bin boundaries)
  result = percentileQuery->getPercentile(80);
  EXPECT_DOUBLE_EQ(
      result, 1.0000162760416667); // Interpolated within bin (updated after
                                   // saturation fix)
}

TEST_F(TestPercentile, TestPercentile_WithInfinities) {
  // Test with infinities
  // Sorted: [-inf, -inf, 1.0, 1.0, 1.0, +inf, +inf]
  std::vector<double> data = {-std::numeric_limits<double>::infinity(),
                              -std::numeric_limits<double>::infinity(),
                              1.0,
                              1.0,
                              1.0,
                              std::numeric_limits<double>::infinity(),
                              std::numeric_limits<double>::infinity()};

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  std::vector<char> buffer = generate_1DxF(array);

  auto percentileQuery = std::make_shared<Percentile>(buffer);

  // 10th percentile: rank = 0.1 * 7 = 0.7, should be -inf
  double result = percentileQuery->getPercentile(10);
  EXPECT_EQ(result, -std::numeric_limits<double>::infinity());

  // 50th percentile: rank = 0.5 * 7 = 3.5, should be in regular value range
  result = percentileQuery->getPercentile(50);
  EXPECT_DOUBLE_EQ(
      result, 1.0000152587890625); // Exact value (updated after saturation fix)

  // 90th percentile: rank = 0.9 * 7 = 6.3, should be +inf
  result = percentileQuery->getPercentile(90);
  EXPECT_EQ(result, std::numeric_limits<double>::infinity());
}

TEST_F(TestPercentile, TestPercentile_WithNaN) {
  // Test that NaN is excluded from percentile calculation
  // Array: [NaN, NaN, NaN, 1.0, 2.0, 3.0, 4.0, 5.0]
  // Only 5 non-NaN values should be counted
  std::vector<double> data = {std::numeric_limits<double>::quiet_NaN(),
                              std::numeric_limits<double>::quiet_NaN(),
                              std::numeric_limits<double>::quiet_NaN(),
                              1.0,
                              2.0,
                              3.0,
                              4.0,
                              5.0};

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  std::vector<char> buffer = generate_1DxF(array);

  auto percentileQuery = std::make_shared<Percentile>(buffer);

  // Total count for percentile: 5 values (NaN excluded)
  // 50th percentile: rank = 0.5 * 5 = 2.5
  // Should be around 3.0
  double result = percentileQuery->getPercentile(50);
  EXPECT_DOUBLE_EQ(result, 3.0009765625); // Exact value
}

TEST_F(TestPercentile, TestPercentile_AllSpecialValues) {
  // Comprehensive test with all types of special values
  // Array includes: NaN, -inf, negative values, -0, +0, positive values, +inf
  std::vector<double> data = {std::numeric_limits<double>::quiet_NaN(),
                              std::numeric_limits<double>::quiet_NaN(),
                              std::numeric_limits<double>::quiet_NaN(),
                              -std::numeric_limits<double>::infinity(),
                              -10.0,
                              -5.0,
                              -0.0,
                              -0.0,
                              0.0,
                              0.0,
                              5.0,
                              10.0,
                              std::numeric_limits<double>::infinity()};

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  std::vector<char> buffer = generate_1DxF(array);

  auto percentileQuery = std::make_shared<Percentile>(buffer);

  // Total: 10 values (excluding 3 NaN)
  // Sorted: [-inf, -10, -5, -0, -0, 0, 0, 5, 10, +inf]

  // 5th percentile: rank = 0.05 * 10 = 0.5, should be -inf
  double result = percentileQuery->getPercentile(5);
  EXPECT_EQ(result, -std::numeric_limits<double>::infinity());

  // 50th percentile: should be around 0
  result = percentileQuery->getPercentile(50);
  EXPECT_EQ(result, -0.0); // Exact value (negative zero)

  // 95th percentile: rank = 0.95 * 10 = 9.5, should be +inf
  result = percentileQuery->getPercentile(95);
  EXPECT_EQ(result, std::numeric_limits<double>::infinity());
}
