#include <gtest/gtest.h>
#include <cstdint>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <cstddef>
#include <iostream>
#include <sys/types.h>
#include <tuple>
#include <airtree/query/topk/TopK.hpp>
#include <airtree/core/schema/trie1d/1DxT.hpp>
#include <airtree/core/serdes/trie1d/1DxT.hpp>
#include <airtree/core/utils/TrieManager.hpp>
#include <vector>

using namespace airtree::query::topk;

// Global helper for clean tests
SpecialCounts cleanCountsTopK{0, 0, 0, 0, 0};

class TestTopK : public ::testing::Test {
protected:
  void SetUp() override {}

  void TearDown() override {}
};

// This test case populates values in the positive sign positive exponent
// quadrant of the 1DxT trie and checks if the top K query returns the expected
// result.
TEST_F(TestTopK, TopK_1DxT_POS_POS) {

  TrieManager trieManager;

  // get the root of the trie
  auto &root_13_ref = trieManager.getRoot1DxT();

  // lets populate the +ve sign +ve exponent quadrant
  u_int16_t sign_bit = 0; // +ve sign
  u_int16_t exponent = 0; // +ve exponent
  uint32_t bit_length = 13;

  // Histogram for reference
  auto histogram =
      std::make_shared<airtree::query::meta::Histogram>(bit_length);

  // lets calculate the expected result
  uint32_t total_bins = histogram->getBinCount();
  uint32_t pos_pos_bins =
      total_bins / 4; // 1/4 of the bins are in this quadrant
  uint32_t expected_topK_bins_count = pos_pos_bins * 5 / 100; // 5% of the bins

  for (size_t i = 0; i < histogram->getBinCount(); ++i) {
    if (histogram->getSignBit(i) != sign_bit
        || histogram->getExponentSignBit(i) != exponent) {
      continue; // skip bins that do not match the sign and exponent
    }

    uint32_t internal_rep = histogram->getInternalRepresentation(i);
    uint32_t suffix_8 = (internal_rep >> 5) & 0xFF;
    uint32_t prefix_5 = internal_rep & 0x1F;

    // lets add a single value to the path
    trieManager.insert1DxT(suffix_8, 1, DistributionMethod::SINGLE, prefix_5);
  }

  std::vector<char> buffer =
      trieManager.MockTrieHeader(bit_length, true, cleanCountsTopK);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  uint32_t topK = 5; // Get top 5% bins
  auto topKquery = airtree::query::topk::TopK(buffer);
  auto query_result_vec = topKquery.getTopK(topK);
  auto result_bin = query_result_vec[query_result_vec.size() - 1];
  double lower_bound = result_bin.getLowerBound();
  uint32_t result_internal_rep = result_bin.getCount();
  EXPECT_EQ(expected_topK_bins_count, query_result_vec.size());
}

// This test case populates values in the positive sign negative exponent
// quadrant of the 1DxT trie and checks if the top K query returns the expected
// result.
TEST_F(TestTopK, TopK_1DxT_POS_NEG) {

  TrieManager trieManager;

  // get the root of the trie
  auto &root_13_ref = trieManager.getRoot1DxT();

  // lets populate the +ve sign +ve exponent quadrant
  u_int16_t sign_bit = 0; // +ve sign
  u_int16_t exponent = 0; // +ve exponent
  uint32_t bit_length = 13;

  // Histogram for reference
  auto histogram =
      std::make_shared<airtree::query::meta::Histogram>(bit_length);

  // lets calculate the expected result
  uint32_t total_bins = histogram->getBinCount();
  uint32_t pos_pos_bins =
      total_bins / 4; // 1/4 of the bins are in this quadrant
  uint32_t expected_topK_bins_count = pos_pos_bins * 5 / 100; // 5% of the bins

  for (size_t i = 0; i < histogram->getBinCount(); ++i) {
    if (histogram->getSignBit(i) != sign_bit
        || histogram->getExponentSignBit(i) != exponent) {
      continue; // skip bins that do not match the sign and exponent
    }

    uint32_t internal_rep = histogram->getInternalRepresentation(i);
    uint32_t suffix_8 = (internal_rep >> 5) & 0xFF;
    uint32_t prefix_5 = internal_rep & 0x1F;

    // lets add a single value to the path
    trieManager.insert1DxT(suffix_8, 1, DistributionMethod::SINGLE, prefix_5);
  }

  std::vector<char> buffer =
      trieManager.MockTrieHeader(bit_length, true, cleanCountsTopK);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  uint32_t topK = 5; // Get top 5% bins
  auto topKquery = airtree::query::topk::TopK(buffer);
  auto query_result_vec = topKquery.getTopK(topK);
  auto result_bin = query_result_vec[query_result_vec.size() - 1];
  double lower_bound = result_bin.getLowerBound();
  uint32_t result_internal_rep = result_bin.getCount();
  EXPECT_EQ(expected_topK_bins_count, query_result_vec.size());
}

TEST_F(TestTopK, TopK_TrieNode13_POS_NEG) {
  TrieManager trieManager;
  trieManager.insert1DxT(64, 45, DistributionMethod::SINGLE, 2);
  trieManager.insert1DxT(120, 32, DistributionMethod::SINGLE, 6);
  trieManager.insert1DxT(88, 5, DistributionMethod::SINGLE, 30);

  uint32_t expected_internal_rep = (120 << 5 | 6);

  auto buffer = trieManager.MockTrieHeader(13, true, cleanCountsTopK);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto topK = airtree::query::topk::TopK(buffer);
  auto query_result = topK.getTopK(0.05);
  auto result_bin = query_result[query_result.size() - 1];
  double lower_bound = result_bin.getLowerBound();
  uint32_t result_internal_rep = result_bin.getInternalRepresentation();
  auto expected_result = reConstruct<double>(expected_internal_rep, 13);
  EXPECT_DOUBLE_EQ(expected_result, lower_bound);
  EXPECT_EQ(expected_internal_rep, result_internal_rep);
}

TEST_F(TestTopK, TopK_TrieNode13_NEG_NEG) {
  TrieManager trieManager;
  trieManager.insert1DxT(192, 45, DistributionMethod::SINGLE, 2);
  trieManager.insert1DxT(212, 32, DistributionMethod::SINGLE, 6);
  trieManager.insert1DxT(234, 5, DistributionMethod::SINGLE, 30);

  uint32_t expected_internal_rep = (192 << 5 | 2);

  auto buffer = trieManager.MockTrieHeader(13, true, cleanCountsTopK);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto topK = airtree::query::topk::TopK(buffer);
  auto query_result = topK.getTopK(0.05);
  auto result_bin = query_result[query_result.size() - 1];
  double lower_bound = result_bin.getLowerBound();
  uint32_t result_internal_rep = result_bin.getInternalRepresentation();
  auto expected_result = reConstruct<double>(expected_internal_rep, 13);
  EXPECT_DOUBLE_EQ(expected_result, lower_bound);
  EXPECT_EQ(expected_internal_rep, result_internal_rep);
}

TEST_F(TestTopK, TopK_TrieNode13_NEG_POS) {
  TrieManager trieManager;
  trieManager.insert1DxT(128, 45, DistributionMethod::SINGLE, 2);
  trieManager.insert1DxT(160, 32, DistributionMethod::SINGLE, 6);
  trieManager.insert1DxT(191, 5, DistributionMethod::SINGLE, 30);

  uint32_t expected_internal_rep = (191 << 5 | 30);

  auto buffer = trieManager.MockTrieHeader(13, true, cleanCountsTopK);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto topK = airtree::query::topk::TopK(buffer);
  auto query_result = topK.getTopK(0.05);
  auto result_bin = query_result[query_result.size() - 1];
  double lower_bound = result_bin.getLowerBound();
  uint32_t result_internal_rep = result_bin.getInternalRepresentation();
  auto expected_result = reConstruct<double>(expected_internal_rep, 13);
  EXPECT_DOUBLE_EQ(expected_result, lower_bound);
  EXPECT_EQ(expected_internal_rep, result_internal_rep);
}

TEST_F(TestTopK, TopK_TrieNode13_POS_POS) {
  TrieManager trieManager;
  trieManager.insert1DxT(56, 45, DistributionMethod::SINGLE, 2);
  trieManager.insert1DxT(23, 32, DistributionMethod::SINGLE, 6);
  trieManager.insert1DxT(12, 5, DistributionMethod::SINGLE, 30);

  uint32_t expected_internal_rep = (12 << 5 | 30);

  auto buffer = trieManager.MockTrieHeader(13, true, cleanCountsTopK);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto topK = airtree::query::topk::TopK(buffer);
  auto query_result = topK.getTopK(0.05);
  auto result_bin = query_result[query_result.size() - 1];
  double lower_bound = result_bin.getLowerBound();
  uint32_t result_internal_rep = result_bin.getInternalRepresentation();
  auto expected_result = reConstruct<double>(expected_internal_rep, 13);
  EXPECT_DOUBLE_EQ(expected_result, lower_bound);
  EXPECT_EQ(expected_internal_rep, result_internal_rep);
}

TEST_F(TestTopK, TopK_TrieNode16_POS_NEG) {
  TrieManager trieManager;
  trieManager.insert1DxF(64, 45, DistributionMethod::SINGLE, 2);
  trieManager.insert1DxF(120, 32, DistributionMethod::SINGLE, 6);
  trieManager.insert1DxF(88, 5, DistributionMethod::SINGLE, 30);

  auto buffer = trieManager.MockTrieHeader(16, true, cleanCountsTopK);
  trieManager.serializeTrie<TrieNode_16>(buffer);

  uint32_t expected_internal_rep = (120 << 8 | 6);

  auto topK = airtree::query::topk::TopK(buffer);
  auto query_result = topK.getTopK(0.05);
  auto result_bin = query_result[query_result.size() - 1];
  double lower_bound = result_bin.getLowerBound();
  uint32_t result_internal_rep = result_bin.getInternalRepresentation();
  auto expected_result = reConstruct<double>(expected_internal_rep, 16);
  EXPECT_DOUBLE_EQ(expected_result, lower_bound);
  EXPECT_EQ(expected_internal_rep, result_internal_rep);
}

TEST_F(TestTopK, TopK_TrieNode16_NEG_NEG) {
  TrieManager trieManager;
  trieManager.insert1DxF(192, 45, DistributionMethod::SINGLE, 2);
  trieManager.insert1DxF(212, 32, DistributionMethod::SINGLE, 6);
  trieManager.insert1DxF(234, 5, DistributionMethod::SINGLE, 30);

  auto buffer = trieManager.MockTrieHeader(16, true, cleanCountsTopK);
  trieManager.serializeTrie<TrieNode_16>(buffer);

  uint32_t expected_internal_rep = (192 << 8 | 2);

  auto topK = airtree::query::topk::TopK(buffer);
  auto query_result = topK.getTopK(0.05);
  auto result_bin = query_result[query_result.size() - 1];
  double lower_bound = result_bin.getLowerBound();
  uint32_t result_internal_rep = result_bin.getInternalRepresentation();
  auto expected_result = reConstruct<double>(expected_internal_rep, 16);
  EXPECT_DOUBLE_EQ(expected_result, lower_bound);
  EXPECT_EQ(expected_internal_rep, result_internal_rep);
}

TEST_F(TestTopK, TopK_TrieNode16_NEG_POS) {
  TrieManager trieManager;
  trieManager.insert1DxF(128, 45, DistributionMethod::SINGLE, 2);
  trieManager.insert1DxF(160, 32, DistributionMethod::SINGLE, 6);
  trieManager.insert1DxF(191, 5, DistributionMethod::SINGLE, 30);

  auto buffer = trieManager.MockTrieHeader(16, true, cleanCountsTopK);
  trieManager.serializeTrie<TrieNode_16>(buffer);

  uint32_t expected_internal_rep = (191 << 8 | 30);

  auto topK = airtree::query::topk::TopK(buffer);
  auto query_result = topK.getTopK(0.05);
  auto result_bin = query_result[query_result.size() - 1];
  double lower_bound = result_bin.getLowerBound();
  uint32_t result_internal_rep = result_bin.getInternalRepresentation();
  auto expected_result = reConstruct<double>(expected_internal_rep, 16);
  EXPECT_DOUBLE_EQ(expected_result, lower_bound);
  EXPECT_EQ(expected_internal_rep, result_internal_rep);
}

TEST_F(TestTopK, TopK_TrieNode16_POS_POS) {
  TrieManager trieManager;
  trieManager.insert1DxF(56, 45, DistributionMethod::SINGLE, 2);
  trieManager.insert1DxF(23, 32, DistributionMethod::SINGLE, 6);
  trieManager.insert1DxF(12, 5, DistributionMethod::SINGLE, 30);

  auto buffer = trieManager.MockTrieHeader(16, true, cleanCountsTopK);
  trieManager.serializeTrie<TrieNode_16>(buffer);

  uint32_t expected_internal_rep = (12 << 8 | 30);

  auto topK = airtree::query::topk::TopK(buffer);
  auto query_result = topK.getTopK(0.05);
  auto result_bin = query_result[query_result.size() - 1];
  double lower_bound = result_bin.getLowerBound();
  uint32_t result_internal_rep = result_bin.getInternalRepresentation();
  auto expected_result = reConstruct<double>(expected_internal_rep, 16);
  EXPECT_DOUBLE_EQ(expected_result, lower_bound);
  EXPECT_EQ(expected_internal_rep, result_internal_rep);
}

TEST_F(TestTopK, TopK_TrieNode20_POS_NEG) {
  TrieManager trieManager;
  trieManager.insert1DxP(64, 33, 45, DistributionMethod::SINGLE, 2);
  trieManager.insert1DxP(120, 54, 32, DistributionMethod::SINGLE, 6);
  trieManager.insert1DxP(88, 42, 5, DistributionMethod::SINGLE, 30);

  auto buffer = trieManager.MockTrieHeader(20, true, cleanCountsTopK);
  trieManager.serializeTrie<TrieNode_20>(buffer);

  uint32_t expected_internal_rep = (120 << 12 | 54 << 6 | 6);

  auto topK = airtree::query::topk::TopK(buffer);
  auto query_result = topK.getTopK(0.05);
  auto result_bin = query_result[query_result.size() - 1];
  double lower_bound = result_bin.getLowerBound();
  uint32_t result_internal_rep = result_bin.getInternalRepresentation();
  auto expected_result = reConstruct<double>(expected_internal_rep, 20);
  EXPECT_DOUBLE_EQ(expected_result, lower_bound);
  EXPECT_EQ(expected_internal_rep, result_internal_rep);
}

TEST_F(TestTopK, TopK_TrieNode20_NEG_NEG) {
  TrieManager trieManager;
  trieManager.insert1DxP(192, 62, 45, DistributionMethod::SINGLE, 2);
  trieManager.insert1DxP(212, 33, 32, DistributionMethod::SINGLE, 6);
  trieManager.insert1DxP(234, 22, 5, DistributionMethod::SINGLE, 30);

  auto buffer = trieManager.MockTrieHeader(20, true, cleanCountsTopK);
  trieManager.serializeTrie<TrieNode_20>(buffer);

  uint32_t expected_internal_rep = (192 << 12 | 62 << 6 | 2);

  auto topK = airtree::query::topk::TopK(buffer);
  auto query_result = topK.getTopK(0.05);
  auto result_bin = query_result[query_result.size() - 1];
  double lower_bound = result_bin.getLowerBound();
  uint32_t result_internal_rep = result_bin.getInternalRepresentation();
  auto expected_result = reConstruct<double>(expected_internal_rep, 20);
  EXPECT_DOUBLE_EQ(expected_result, lower_bound);
  EXPECT_EQ(expected_internal_rep, result_internal_rep);
}

TEST_F(TestTopK, TopK_TrieNode20_NEG_POS) {
  TrieManager trieManager;
  trieManager.insert1DxP(128, 12, 45, DistributionMethod::SINGLE, 2);
  trieManager.insert1DxP(160, 22, 32, DistributionMethod::SINGLE, 6);
  trieManager.insert1DxP(191, 2, 5, DistributionMethod::SINGLE, 30);

  auto buffer = trieManager.MockTrieHeader(20, true, cleanCountsTopK);
  trieManager.serializeTrie<TrieNode_20>(buffer);

  uint32_t expected_internal_rep = (191 << 12 | 2 << 6 | 30);

  auto topK = airtree::query::topk::TopK(buffer);
  auto query_result = topK.getTopK(0.05);
  auto result_bin = query_result[query_result.size() - 1];
  double lower_bound = result_bin.getLowerBound();
  uint32_t result_internal_rep = result_bin.getInternalRepresentation();
  auto expected_result = reConstruct<double>(expected_internal_rep, 20);
  EXPECT_DOUBLE_EQ(expected_result, lower_bound);
  EXPECT_EQ(expected_internal_rep, result_internal_rep);
}

TEST_F(TestTopK, TopK_TrieNode20_POS_POS) {
  TrieManager trieManager;
  trieManager.insert1DxP(56, 16, 45, DistributionMethod::SINGLE, 2);
  trieManager.insert1DxP(23, 63, 32, DistributionMethod::SINGLE, 6);
  trieManager.insert1DxP(12, 2, 5, DistributionMethod::SINGLE, 30);

  auto buffer = trieManager.MockTrieHeader(20, true, cleanCountsTopK);

  trieManager.serializeTrie<TrieNode_20>(buffer);


  uint32_t expected_internal_rep = (12 << 12 | 2 << 6 | 30);

  auto topK = airtree::query::topk::TopK(buffer);
  auto query_result = topK.getTopK(0.05);
  auto result_bin = query_result[query_result.size() - 1];
  double lower_bound = result_bin.getLowerBound();
  uint32_t result_internal_rep = result_bin.getInternalRepresentation();
  auto expected_result = reConstruct<double>(expected_internal_rep, 20);
  EXPECT_DOUBLE_EQ(expected_result, lower_bound);
  EXPECT_EQ(expected_internal_rep, result_internal_rep);
}

// ============================================================================
// Special Values Tests
// ============================================================================

TEST_F(TestTopK, TopK_OnlyPositiveInfinity) {
  TrieManager trieManager;
  SpecialCounts counts{100, 0, 0, 0, 0};

  auto buffer = trieManager.MockTrieHeader(13, true, counts);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto topK = airtree::query::topk::TopK(buffer);
  auto results = topK.getTopK(100.0);

  ASSERT_EQ(1, results.size());
  EXPECT_DOUBLE_EQ(
      std::numeric_limits<double>::infinity(), results[0].getLowerBound());
  EXPECT_DOUBLE_EQ(
      std::numeric_limits<double>::infinity(), results[0].getUpperBound());
  EXPECT_EQ(100, results[0].getCount());
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(),
            results[0].getInternalRepresentation());
}

TEST_F(TestTopK, TopK_OnlyNegativeInfinity) {
  TrieManager trieManager;
  SpecialCounts counts{0, 100, 0, 0, 0};

  auto buffer = trieManager.MockTrieHeader(13, true, counts);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto topK = airtree::query::topk::TopK(buffer);
  auto results = topK.getTopK(100.0);

  ASSERT_EQ(1, results.size());
  EXPECT_DOUBLE_EQ(
      -std::numeric_limits<double>::infinity(), results[0].getLowerBound());
  EXPECT_DOUBLE_EQ(
      -std::numeric_limits<double>::infinity(), results[0].getUpperBound());
  EXPECT_EQ(100, results[0].getCount());
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(),
            results[0].getInternalRepresentation());
}

TEST_F(TestTopK, TopK_OnlyPositiveZero) {
  TrieManager trieManager;
  SpecialCounts counts{0, 0, 50, 0, 0};

  auto buffer = trieManager.MockTrieHeader(13, true, counts);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto topK = airtree::query::topk::TopK(buffer);
  auto results = topK.getTopK(100.0);

  ASSERT_EQ(1, results.size());
  EXPECT_DOUBLE_EQ(0.0, results[0].getLowerBound());
  EXPECT_DOUBLE_EQ(0.0, results[0].getUpperBound());
  EXPECT_EQ(50, results[0].getCount());
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(),
            results[0].getInternalRepresentation());
}

TEST_F(TestTopK, TopK_OnlyNegativeZero) {
  TrieManager trieManager;
  SpecialCounts counts{0, 0, 0, 50, 0};

  auto buffer = trieManager.MockTrieHeader(13, true, counts);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto topK = airtree::query::topk::TopK(buffer);
  auto results = topK.getTopK(100.0);

  ASSERT_EQ(1, results.size());
  EXPECT_DOUBLE_EQ(-0.0, results[0].getLowerBound());
  EXPECT_DOUBLE_EQ(-0.0, results[0].getUpperBound());
  EXPECT_EQ(50, results[0].getCount());
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(),
            results[0].getInternalRepresentation());
}

TEST_F(TestTopK, TopK_BothZeros) {
  TrieManager trieManager;
  SpecialCounts counts{0, 0, 70, 30, 0};

  auto buffer = trieManager.MockTrieHeader(13, true, counts);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto topK = airtree::query::topk::TopK(buffer);
  auto results = topK.getTopK(100.0);

  ASSERT_EQ(2, results.size());
  EXPECT_DOUBLE_EQ(0.0, results[0].getLowerBound());
  EXPECT_EQ(70, results[0].getCount());
  EXPECT_DOUBLE_EQ(-0.0, results[1].getLowerBound());
  EXPECT_EQ(30, results[1].getCount());
}

TEST_F(TestTopK, TopK_AllSpecialValues) {
  TrieManager trieManager;
  SpecialCounts counts{25, 15, 20, 10, 0};

  auto buffer = trieManager.MockTrieHeader(13, true, counts);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto topK = airtree::query::topk::TopK(buffer);
  auto results = topK.getTopK(100.0);

  ASSERT_EQ(4, results.size());
  EXPECT_DOUBLE_EQ(
      std::numeric_limits<double>::infinity(), results[0].getLowerBound());
  EXPECT_EQ(25, results[0].getCount());
  EXPECT_DOUBLE_EQ(0.0, results[1].getLowerBound());
  EXPECT_EQ(20, results[1].getCount());
  EXPECT_DOUBLE_EQ(-0.0, results[2].getLowerBound());
  EXPECT_EQ(10, results[2].getCount());
  EXPECT_DOUBLE_EQ(
      -std::numeric_limits<double>::infinity(), results[3].getLowerBound());
  EXPECT_EQ(15, results[3].getCount());
}

TEST_F(TestTopK, TopK_MixedWithPositiveValues) {
  TrieManager trieManager;

  trieManager.insert1DxT(120, 50, DistributionMethod::SINGLE, 6);
  trieManager.insert1DxT(88, 30, DistributionMethod::SINGLE, 30);

  SpecialCounts counts{100, 0, 10, 0, 0};

  auto buffer = trieManager.MockTrieHeader(13, true, counts);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto topK = airtree::query::topk::TopK(buffer);
  auto results = topK.getTopK(100.0);

  ASSERT_EQ(4, results.size());
  EXPECT_DOUBLE_EQ(
      std::numeric_limits<double>::infinity(), results[0].getLowerBound());
  EXPECT_EQ(100, results[0].getCount());
  EXPECT_EQ(50, results[1].getCount());
  EXPECT_EQ(30, results[2].getCount());
  EXPECT_DOUBLE_EQ(0.0, results[3].getLowerBound());
  EXPECT_EQ(10, results[3].getCount());
}

TEST_F(TestTopK, TopK_MixedWithNegativeValues) {
  TrieManager trieManager;

  trieManager.insert1DxT(64, 40, DistributionMethod::SINGLE, 2);
  trieManager.insert1DxT(32, 20, DistributionMethod::SINGLE, 15);

  SpecialCounts counts{0, 80, 0, 5, 0};

  auto buffer = trieManager.MockTrieHeader(13, true, counts);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto topK = airtree::query::topk::TopK(buffer);
  auto results = topK.getTopK(100.0);

  ASSERT_EQ(4, results.size());
  EXPECT_EQ(20, results[0].getCount());
  EXPECT_EQ(40, results[1].getCount());
  EXPECT_DOUBLE_EQ(-0.0, results[2].getLowerBound());
  EXPECT_EQ(5, results[2].getCount());
  EXPECT_DOUBLE_EQ(
      -std::numeric_limits<double>::infinity(), results[3].getLowerBound());
  EXPECT_EQ(80, results[3].getCount());
}

TEST_F(TestTopK, TopK_PartialK_WithSpecialValues) {
  TrieManager trieManager;

  trieManager.insert1DxT(120, 50, DistributionMethod::SINGLE, 6);
  trieManager.insert1DxT(88, 30, DistributionMethod::SINGLE, 30);
  trieManager.insert1DxT(64, 20, DistributionMethod::SINGLE, 2);

  SpecialCounts counts{100, 15, 5, 10, 0};

  auto buffer = trieManager.MockTrieHeader(13, true, counts);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto topK = airtree::query::topk::TopK(buffer);

  auto results_50 = topK.getTopK(50.0);
  EXPECT_EQ(2, results_50.size());
  EXPECT_DOUBLE_EQ(
      std::numeric_limits<double>::infinity(), results_50[0].getLowerBound());
  EXPECT_EQ(100, results_50[0].getCount());
  EXPECT_EQ(50, results_50[1].getCount());

  auto results_10 = topK.getTopK(10.0);
  EXPECT_EQ(1, results_10.size());
  EXPECT_DOUBLE_EQ(
      std::numeric_limits<double>::infinity(), results_10[0].getLowerBound());
  EXPECT_EQ(100, results_10[0].getCount());
}

TEST_F(TestTopK, TopK_InfinityDominatesSmallK) {
  TrieManager trieManager;

  trieManager.insert1DxT(120, 10, DistributionMethod::SINGLE, 6);
  trieManager.insert1DxT(88, 10, DistributionMethod::SINGLE, 30);

  SpecialCounts counts{1000, 0, 0, 0, 0};

  auto buffer = trieManager.MockTrieHeader(13, true, counts);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto topK = airtree::query::topk::TopK(buffer);
  auto results = topK.getTopK(5.0);

  ASSERT_EQ(1, results.size());
  EXPECT_DOUBLE_EQ(
      std::numeric_limits<double>::infinity(), results[0].getLowerBound());
  EXPECT_EQ(1000, results[0].getCount());
}

TEST_F(TestTopK, TopK_ZeroTransition) {
  TrieManager trieManager;

  trieManager.insert1DxT(200, 30, DistributionMethod::SINGLE, 10);
  trieManager.insert1DxT(50, 25, DistributionMethod::SINGLE, 5);

  SpecialCounts counts{0, 0, 35, 40, 0};

  auto buffer = trieManager.MockTrieHeader(13, true, counts);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto topK = airtree::query::topk::TopK(buffer);
  auto results = topK.getTopK(100.0);

  ASSERT_EQ(4, results.size());
  EXPECT_EQ(25, results[0].getCount());
  EXPECT_DOUBLE_EQ(0.0, results[1].getLowerBound());
  EXPECT_EQ(35, results[1].getCount());
  EXPECT_DOUBLE_EQ(-0.0, results[2].getLowerBound());
  EXPECT_EQ(40, results[2].getCount());
  EXPECT_EQ(30, results[3].getCount());
}
