// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <gtest/gtest.h>
#include <cstdint>
#include <airtree/query/AirTreeQuery_internal.hpp>
#include <cstddef>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/core/utils/TrieManager.hpp>
#include <sys/types.h>
#include <vector>
#include <random>

using namespace airtree::query::minmax;

// Global helper to ensure tests run without phantom special values
SpecialCounts cleanCounts{0, 0, 0, 0, 0};

class TestMinMax : public ::testing::Test {
protected:
  void SetUp() override {}

  void TearDown() override {}
};


TEST_F(TestMinMax, MaxCount_1DxF) {

  TrieManager trieManager;
  trieManager.insert1DxF(25, 256, DistributionMethod::EVEN);
  trieManager.insert1DxF(25, 1, DistributionMethod::SINGLE, 2);

  auto buffer = trieManager.MockTrieHeader(16, cleanCounts);
  trieManager.serializeTrie<TrieNode_16>(buffer);

  auto expected_result = reConstruct<double>((25 << 8 | 2), 16);

  auto query = airtree::query::minmax::MinMax(buffer);
  auto query_result = query.getMax();
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();
  size_t count = minMaxResult.getBinCount();
  EXPECT_DOUBLE_EQ(lowerBound, expected_result);
  EXPECT_EQ(count, 2);
}

TEST_F(TestMinMax, MinCount_1DxF) {

  TrieManager trieManager;
  trieManager.insert1DxF(15, 512, DistributionMethod::SINGLE, 2);
  trieManager.insert1DxF(25, 256, DistributionMethod::SINGLE, 2);

  auto buffer = trieManager.MockTrieHeader(16, cleanCounts);
  trieManager.serializeTrie<TrieNode_16>(buffer);

  auto expected_result = reConstruct<double>((25 << 8 | 2), 16);

  auto query = airtree::query::minmax::MinMax(buffer);
  auto query_result = query.getMin();
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();
  size_t count = minMaxResult.getBinCount();
  EXPECT_DOUBLE_EQ(lowerBound, expected_result);
  EXPECT_EQ(count, 256);
}

TEST_F(TestMinMax, MaxCount_1DxT) {
  TrieManager trieManager;
  trieManager.insert1DxT(25, 32, DistributionMethod::EVEN);
  trieManager.insert1DxT(25, 1, DistributionMethod::SINGLE, 2);

  auto buffer = trieManager.MockTrieHeader(13, cleanCounts);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto expected_result = reConstruct<double>((25 << 5 | 2), 13);

  airtree::query::minmax::MinMax query(buffer);
  auto query_result = query.getMax();
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();
  size_t count = minMaxResult.getBinCount();
  EXPECT_DOUBLE_EQ(lowerBound, expected_result);
  EXPECT_EQ(count, 2);
}

TEST_F(TestMinMax, MinCount_1DxT) {
  TrieManager trieManager;
  trieManager.insert1DxT(15, 512, DistributionMethod::SINGLE, 2);
  trieManager.insert1DxT(25, 256, DistributionMethod::SINGLE, 2);

  auto buffer = trieManager.MockTrieHeader(13, cleanCounts);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto expected_result = reConstruct<double>((25 << 5 | 2), 13);

  airtree::query::minmax::MinMax query(buffer);
  auto query_result = query.getMin();
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();
  size_t count = minMaxResult.getBinCount();
  EXPECT_DOUBLE_EQ(lowerBound, expected_result);
  EXPECT_EQ(count, 256);
}

TEST_F(TestMinMax, MaxCount_1DxP) {
  TrieManager trieManager;
  trieManager.insert1DxP(25, 15, 64, DistributionMethod::EVEN);
  trieManager.insert1DxP(25, 15, 1, DistributionMethod::SINGLE, 2);

  auto buffer = trieManager.MockTrieHeader(20, cleanCounts);
  trieManager.serializeTrie<TrieNode_20>(buffer);

  auto expected_result = reConstruct<double>((25 << 12 | 15 << 6 | 2), 20);

  airtree::query::minmax::MinMax query(buffer);
  auto query_result = query.getMax();
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();
  size_t count = minMaxResult.getBinCount();
  EXPECT_DOUBLE_EQ(lowerBound, expected_result);
  EXPECT_EQ(count, 2);
}

TEST_F(TestMinMax, MinCount_1DxP) {
  TrieManager trieManager;
  trieManager.insert1DxP(25, 15, 128, DistributionMethod::SINGLE, 2);
  trieManager.insert1DxP(15, 13, 1, DistributionMethod::SINGLE, 2);

  auto buffer = trieManager.MockTrieHeader(20, cleanCounts);
  trieManager.serializeTrie<TrieNode_20>(buffer);

  auto expected_result = reConstruct<double>((15 << 12 | 13 << 6 | 2), 20);

  airtree::query::minmax::MinMax query(buffer);
  auto query_result = query.getMin();
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();
  size_t count = minMaxResult.getBinCount();
  EXPECT_DOUBLE_EQ(lowerBound, expected_result);
  EXPECT_EQ(count, 1);
}

TEST_F(TestMinMax, MinValue_1DxF) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(16);
  uint32_t expected_smallest_bin = 789;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(
      expected_smallest_bin, histogram->getBinCount() - 1);

  uint32_t expected_internal_rep =
      histogram->getInternalRepresentation(expected_smallest_bin);

  TrieManager trieManager;
  trieManager.insert1DxF((expected_internal_rep >> 8) & 0xFF, 1,
                         DistributionMethod::SINGLE,
                         (expected_internal_rep & 0xFF));

  for (size_t i = 0; i < 10; ++i) {
    uint32_t current_bin = dis(gen);
    uint64_t internal_rep = histogram->getInternalRepresentation(current_bin);
    trieManager.insert1DxF((internal_rep >> 8) & 0xFF, 1,
                           DistributionMethod::SINGLE, (internal_rep & 0xFF));
  }

  auto buffer = trieManager.MockTrieHeader(16, cleanCounts);
  trieManager.serializeTrie<TrieNode_16>(buffer);

  auto expected_result = reConstruct<double>(expected_internal_rep, 16);

  auto query = airtree::query::minmax::MinMax(buffer);
  auto query_result = query.getMinValue();
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();
  size_t count = minMaxResult.getBinCount();
  EXPECT_DOUBLE_EQ(lowerBound, expected_result);
  EXPECT_EQ(count, 1);
}

TEST_F(TestMinMax, MaxValue_1DxF) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(16);
  uint32_t expected_largest_bin = 789;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(
      expected_largest_bin - 11, expected_largest_bin - 1);

  uint32_t expected_internal_rep =
      histogram->getInternalRepresentation(expected_largest_bin);

  TrieManager trieManager;
  trieManager.insert1DxF((expected_internal_rep >> 8) & 0xFF, 1,
                         DistributionMethod::SINGLE,
                         (expected_internal_rep & 0xFF));

  for (size_t i = 0; i < 10; ++i) {
    uint32_t current_bin = dis(gen);
    uint64_t internal_rep = histogram->getInternalRepresentation(current_bin);
    trieManager.insert1DxF((internal_rep >> 8) & 0xFF, 1,
                           DistributionMethod::SINGLE, (internal_rep & 0xFF));
  }

  auto buffer = trieManager.MockTrieHeader(16, cleanCounts);
  trieManager.serializeTrie<TrieNode_16>(buffer);

  auto expected_result = reConstruct<double>(expected_internal_rep, 16);

  auto query = airtree::query::minmax::MinMax(buffer);
  auto query_result = query.getMaxValue();
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();
  size_t count = minMaxResult.getBinCount();
  EXPECT_DOUBLE_EQ(lowerBound, expected_result);
  EXPECT_EQ(count, 1);
}

TEST_F(TestMinMax, MinValue_1DxT) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(13);
  uint32_t expected_smallest_bin = 789;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(
      expected_smallest_bin, histogram->getBinCount() - 1);

  uint32_t expected_internal_rep =
      histogram->getInternalRepresentation(expected_smallest_bin);

  TrieManager trieManager;
  trieManager.insert1DxT((expected_internal_rep >> 5) & 0xFF, 1,
                         DistributionMethod::SINGLE,
                         (expected_internal_rep & 0x1F));

  for (size_t i = 0; i < 10; ++i) {
    uint32_t current_bin = dis(gen);
    uint64_t internal_rep = histogram->getInternalRepresentation(current_bin);
    trieManager.insert1DxT((internal_rep >> 5) & 0xFF, 1,
                           DistributionMethod::SINGLE, (internal_rep & 0x1F));
  }

  auto buffer = trieManager.MockTrieHeader(13, cleanCounts);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto expected_result = reConstruct<double>(expected_internal_rep, 13);

  auto query = airtree::query::minmax::MinMax(buffer);
  auto query_result = query.getMinValue();
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();
  size_t count = minMaxResult.getBinCount();
  EXPECT_DOUBLE_EQ(lowerBound, expected_result);
  EXPECT_EQ(count, 1);
}

TEST_F(TestMinMax, MaxValue_1DxT) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(13);
  uint32_t expected_largest_bin = 789;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(
      expected_largest_bin - 11, expected_largest_bin - 1);

  uint32_t expected_internal_rep =
      histogram->getInternalRepresentation(expected_largest_bin);

  TrieManager trieManager;
  trieManager.insert1DxT((expected_internal_rep >> 5) & 0xFF, 1,
                         DistributionMethod::SINGLE,
                         (expected_internal_rep & 0x1F));

  for (size_t i = 0; i < 10; ++i) {
    uint32_t current_bin = dis(gen);
    uint64_t internal_rep = histogram->getInternalRepresentation(current_bin);
    trieManager.insert1DxT((internal_rep >> 5) & 0xFF, 1,
                           DistributionMethod::SINGLE, (internal_rep & 0x1F));
  }

  auto buffer = trieManager.MockTrieHeader(13, cleanCounts);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto expected_result = reConstruct<double>(expected_internal_rep, 13);

  auto query = airtree::query::minmax::MinMax(buffer);
  auto query_result = query.getMaxValue();
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();
  size_t count = minMaxResult.getBinCount();
  EXPECT_DOUBLE_EQ(lowerBound, expected_result);
  EXPECT_EQ(count, 1);
}

TEST_F(TestMinMax, MinValue_1DxP) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(20);
  uint32_t expected_smallest_bin = 789;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(
      expected_smallest_bin, histogram->getBinCount() - 1);

  uint32_t expected_internal_rep =
      histogram->getInternalRepresentation(expected_smallest_bin);

  TrieManager trieManager;
  trieManager.insert1DxP(
      (expected_internal_rep >> 12) & 0xFF, (expected_internal_rep >> 6) & 0x3F,
      1, DistributionMethod::SINGLE, (expected_internal_rep & 0x3F));

  for (size_t i = 0; i < 10; ++i) {
    uint32_t current_bin = dis(gen);
    uint64_t internal_rep = histogram->getInternalRepresentation(current_bin);
    trieManager.insert1DxP((internal_rep >> 12) & 0xFF,
                           (internal_rep >> 6) & 0x3F, 1,
                           DistributionMethod::SINGLE, (internal_rep & 0x3F));
  }

  auto buffer = trieManager.MockTrieHeader(20, cleanCounts);
  trieManager.serializeTrie<TrieNode_20>(buffer);

  auto expected_result = reConstruct<double>(expected_internal_rep, 20);

  auto query = airtree::query::minmax::MinMax(buffer);
  auto query_result = query.getMinValue();
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();
  size_t count = minMaxResult.getBinCount();
  EXPECT_DOUBLE_EQ(lowerBound, expected_result);
  EXPECT_EQ(count, 1);
}

TEST_F(TestMinMax, MaxValue_1DxP) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(20);
  uint32_t expected_largest_bin = 789;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(
      expected_largest_bin - 11, expected_largest_bin - 1);

  uint32_t expected_internal_rep =
      histogram->getInternalRepresentation(expected_largest_bin);

  TrieManager trieManager;
  trieManager.insert1DxP(
      (expected_internal_rep >> 12) & 0xFF, (expected_internal_rep >> 6) & 0x3F,
      1, DistributionMethod::SINGLE, (expected_internal_rep & 0x3F));


  for (size_t i = 0; i < 10; ++i) {
    uint32_t current_bin = dis(gen);
    uint64_t internal_rep = histogram->getInternalRepresentation(current_bin);
    trieManager.insert1DxP((internal_rep >> 12) & 0xFF,
                           (internal_rep >> 6) & 0x3F, 1,
                           DistributionMethod::SINGLE, (internal_rep & 0x3F));
  }

  auto buffer = trieManager.MockTrieHeader(20, cleanCounts);
  trieManager.serializeTrie<TrieNode_20>(buffer);

  auto expected_result = reConstruct<double>(expected_internal_rep, 20);

  auto query = airtree::query::minmax::MinMax(buffer);
  auto query_result = query.getMaxValue();
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();
  size_t count = minMaxResult.getBinCount();
  EXPECT_DOUBLE_EQ(lowerBound, expected_result);
  EXPECT_EQ(count, 1);
}
// ============================================================================
// SPECIAL VALUES TESTS (Infinities and Zeros)
// ============================================================================

TEST_F(TestMinMax, MinValue_WithNegativeInfinity) {
  // Test: getMinValue should return -inf when present
  std::vector<double> data = {-std::numeric_limits<double>::infinity(),
                              -std::numeric_limits<double>::infinity(), 1.0,
                              2.0, 3.0};

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  std::vector<char> buffer = generate_1DxF(array);

  auto query = airtree::query::minmax::MinMax(buffer);
  auto query_result = query.getMinValue();

  ASSERT_FALSE(query_result.empty());
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();
  size_t count = minMaxResult.getBinCount();

  EXPECT_EQ(lowerBound, -std::numeric_limits<double>::infinity());
  EXPECT_EQ(count, 2);
}

TEST_F(TestMinMax, MaxValue_WithPositiveInfinity) {
  // Test: getMaxValue should return +inf when present
  std::vector<double> data = {1.0,
                              2.0,
                              3.0,
                              std::numeric_limits<double>::infinity(),
                              std::numeric_limits<double>::infinity(),
                              std::numeric_limits<double>::infinity()};

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  std::vector<char> buffer = generate_1DxF(array);

  auto query = airtree::query::minmax::MinMax(buffer);
  auto query_result = query.getMaxValue();

  ASSERT_FALSE(query_result.empty());
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();
  size_t count = minMaxResult.getBinCount();

  EXPECT_EQ(lowerBound, std::numeric_limits<double>::infinity());
  EXPECT_EQ(count, 3);
}

TEST_F(TestMinMax, MinValue_WithZeros) {
  // Test: getMinValue with zeros between negative and positive values
  // Should return negative zero as the minimum
  std::vector<double> data = {-0.0, -0.0, // Negative zeros
                              0.0,  0.0,  // Positive zeros
                              1.0,  2.0};

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  std::vector<char> buffer = generate_1DxF(array);

  auto query = airtree::query::minmax::MinMax(buffer);
  auto query_result = query.getMinValue();

  ASSERT_FALSE(query_result.empty());
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();
  size_t count = minMaxResult.getBinCount();

  EXPECT_EQ(lowerBound, -0.0);
  EXPECT_EQ(count, 2);
}

TEST_F(TestMinMax, MaxValue_WithZeros) {
  // Test: getMaxValue with zeros between negative and positive values
  // Should return positive zero as the maximum
  std::vector<double> data = {
      -2.0, -1.0, -0.0, -0.0, // Negative zeros
      0.0,  0.0               // Positive zeros
  };

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  std::vector<char> buffer = generate_1DxF(array);

  auto query = airtree::query::minmax::MinMax(buffer);
  auto query_result = query.getMaxValue();

  ASSERT_FALSE(query_result.empty());
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();
  size_t count = minMaxResult.getBinCount();

  EXPECT_EQ(lowerBound, 0.0);
  EXPECT_EQ(count, 2);
}

TEST_F(TestMinMax, MinValue_ZerosOnly) {
  // Test: Dataset with only zeros - should return negative zero
  std::vector<double> data = {-0.0, -0.0, -0.0, 0.0, 0.0};

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  std::vector<char> buffer = generate_1DxF(array);

  auto query = airtree::query::minmax::MinMax(buffer);
  auto query_result = query.getMinValue();

  ASSERT_FALSE(query_result.empty());
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();

  EXPECT_EQ(lowerBound, -0.0);
}

TEST_F(TestMinMax, MaxValue_ZerosOnly) {
  // Test: Dataset with only zeros - should return positive zero
  std::vector<double> data = {-0.0, -0.0, 0.0, 0.0, 0.0};

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  std::vector<char> buffer = generate_1DxF(array);

  auto query = airtree::query::minmax::MinMax(buffer);
  auto query_result = query.getMaxValue();

  ASSERT_FALSE(query_result.empty());
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();

  EXPECT_EQ(lowerBound, 0.0);
}

TEST_F(TestMinMax, MinValue_MixedNegativesAndZeros) {
  // Test: Negatives, then zeros - should return smallest negative value
  std::vector<double> data = {-10.0, -5.0, -1.0, -0.0, 0.0};

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  std::vector<char> buffer = generate_1DxF(array);

  auto query = airtree::query::minmax::MinMax(buffer);
  auto query_result = query.getMinValue();

  ASSERT_FALSE(query_result.empty());
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();

  // Should be -10.0 (or close to it based on histogram binning)
  EXPECT_LT(lowerBound, -5.0);
}

TEST_F(TestMinMax, MaxValue_MixedPositivesAndZeros) {
  // Test: Zeros, then positives - should return largest positive value
  std::vector<double> data = {-0.0, 0.0, 1.0, 5.0, 10.0};

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  std::vector<char> buffer = generate_1DxF(array);

  auto query = airtree::query::minmax::MinMax(buffer);
  auto query_result = query.getMaxValue();

  ASSERT_FALSE(query_result.empty());
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();

  // Should be 10.0 (or close to it based on histogram binning)
  EXPECT_GT(lowerBound, 5.0);
}

// ============================================================================
// FREQUENCY TESTS (getMin/getMax with special values)
// ============================================================================

TEST_F(TestMinMax, MaxFrequency_InfinityIsMostCommon) {
  // Test: +inf is the most frequent value
  std::vector<double> data = {
      1.0,                                     // 1 occurrence
      2.0,                                     // 1 occurrence
      std::numeric_limits<double>::infinity(), // 10 occurrences
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::infinity()};

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  std::vector<char> buffer = generate_1DxF(array);

  auto query = airtree::query::minmax::MinMax(buffer);
  auto query_result = query.getMax(); // Max FREQUENCY (mode)

  ASSERT_FALSE(query_result.empty());
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();
  size_t count = minMaxResult.getBinCount();

  EXPECT_EQ(lowerBound, std::numeric_limits<double>::infinity());
  EXPECT_EQ(count, 10);
}

TEST_F(TestMinMax, MinFrequency_WithSpecialValues) {
  // Test: Find rarest value when special values are present
  std::vector<double> data = {
      -std::numeric_limits<double>::infinity(),
      -std::numeric_limits<double>::infinity(),
      -std::numeric_limits<double>::infinity(), // 3 occurrences
      -0.0,                                     // 1 occurrence (rarest)
      0.0,
      0.0, // 2 occurrences
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::infinity() // 4 occurrences
  };

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  std::vector<char> buffer = generate_1DxF(array);

  auto query = airtree::query::minmax::MinMax(buffer);
  auto query_result = query.getMin(); // Min FREQUENCY (rarest)

  ASSERT_FALSE(query_result.empty());
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();
  size_t count = minMaxResult.getBinCount();

  EXPECT_EQ(lowerBound, -0.0);
  EXPECT_EQ(count, 1);
}

TEST_F(TestMinMax, MaxFrequency_ZerosAreMostCommon) {
  // Test: Zeros are the most frequent value
  std::vector<double> data = {
      -10.0,                               // 1 occurrence
      0.0,   0.0, 0.0, 0.0, 0.0, 0.0, 0.0, // 7 occurrences (most common)
      10.0                                 // 1 occurrence
  };

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  std::vector<char> buffer = generate_1DxF(array);

  auto query = airtree::query::minmax::MinMax(buffer);
  auto query_result = query.getMax(); // Max FREQUENCY (mode)

  ASSERT_FALSE(query_result.empty());
  auto minMaxResult = query_result[0];
  double lowerBound = minMaxResult.getLowerBound();
  size_t count = minMaxResult.getBinCount();

  EXPECT_EQ(lowerBound, 0.0);
  EXPECT_EQ(count, 7);
}

TEST_F(TestMinMax, AllSpecialValues_ComprehensiveTest) {
  // Test all special value types together
  std::vector<double> data = {
      -std::numeric_limits<double>::infinity(),
      -std::numeric_limits<double>::infinity(), // 2 -inf
      -10.0,                                    // 1 negative
      -0.0,                                     // 1 -zero
      0.0,                                      // 1 +zero
      10.0,                                     // 1 positive
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::infinity() // 3 +inf
  };

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  std::vector<char> buffer = generate_1DxF(array);

  auto query = airtree::query::minmax::MinMax(buffer);

  // Test getMinValue - should be -inf
  auto min_value_result = query.getMinValue();
  ASSERT_FALSE(min_value_result.empty());
  EXPECT_EQ(min_value_result[0].getLowerBound(),
            -std::numeric_limits<double>::infinity());

  // Test getMaxValue - should be +inf
  auto max_value_result = query.getMaxValue();
  ASSERT_FALSE(max_value_result.empty());
  EXPECT_EQ(max_value_result[0].getLowerBound(),
            std::numeric_limits<double>::infinity());

  // Test getMax (frequency) - should be +inf (count=3)
  auto max_freq_result = query.getMax();
  ASSERT_FALSE(max_freq_result.empty());
  EXPECT_EQ(max_freq_result[0].getLowerBound(),
            std::numeric_limits<double>::infinity());
  EXPECT_EQ(max_freq_result[0].getBinCount(), 3);
}
