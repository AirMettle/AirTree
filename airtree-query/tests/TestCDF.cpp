// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <gtest/gtest.h>
#include <fstream>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/query/AirTreeQuery_internal.hpp>
#include <airtree/query/Logger.hpp>
#include <cmath>
#include <limits>
#include <ostream>
#include <spdlog/spdlog.h>
#include <airtree/core/utils/TrieManager.hpp>
#include <vector>

using namespace airtree::query::cdf;
using namespace airtree::query::percentile;
using namespace airtree::query;

class TestCDF : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(TestCDF, TestCDF1DxT_SpecialValuesZerosOnly) {

  TrieManager trieManager;

  SpecialCounts noSpecial{0, 0, 10, 10, 0};
  std::vector<char> buffer = trieManager.MockTrieHeader(13, noSpecial);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto cdf = std::make_shared<CDF>(buffer);

  EXPECT_DOUBLE_EQ(cdf->getCDF(0.0, false), 1.0);
}

TEST_F(TestCDF, TestCDF1DxT_SpecialValuesNegInf) {

  TrieManager trieManager;

  SpecialCounts noSpecial{0, 10, 0, 0, 0};
  std::vector<char> buffer = trieManager.MockTrieHeader(13, noSpecial);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto cdf = std::make_shared<CDF>(buffer);

  EXPECT_DOUBLE_EQ(
      cdf->getCDF(-std::numeric_limits<double>::infinity(), false), 1.0);
}

TEST_F(TestCDF, TestCDF1DxT_SpecialValuesPosNegInf) {

  TrieManager trieManager;

  SpecialCounts noSpecial{10, 10, 0, 0, 0};
  std::vector<char> buffer = trieManager.MockTrieHeader(13, noSpecial);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto cdf = std::make_shared<CDF>(buffer);

  EXPECT_DOUBLE_EQ(
      cdf->getCDF(-std::numeric_limits<double>::infinity(), false), 0.5);
}

TEST_F(TestCDF, TestCDF1DxT_SpecialValuesPosInf) {

  TrieManager trieManager;

  SpecialCounts noSpecial{10, 10, 0, 0, 0};
  std::vector<char> buffer = trieManager.MockTrieHeader(13, noSpecial);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto cdf = std::make_shared<CDF>(buffer);

  EXPECT_DOUBLE_EQ(
      cdf->getCDF(std::numeric_limits<double>::infinity(), false), 1.0);
}

TEST_F(TestCDF, TestCDF1DxT_MixedSpecialValues) {
  auto histogram = std::make_shared<airtree::query::meta::Histogram>(13);
  TrieManager trieManager;

  // 5 PosInf, 5 NegInf, 5 PosZero, 5 NegZero = Total Count of 20
  SpecialCounts mixedSpecial{5, 5, 5, 5, 0};
  std::vector<char> buffer = trieManager.MockTrieHeader(13, mixedSpecial);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto cdf = std::make_shared<CDF>(buffer);

  // Negative Infinity only
  EXPECT_DOUBLE_EQ(cdf->getCDF(-std::numeric_limits<double>::infinity(), false),
                   0.25); // 5/20

  // Up to Negative Zero (NegInf + NegZero)
  // Assuming a query of a very slight negative number or -0.0 hits this
  EXPECT_DOUBLE_EQ(cdf->getCDF(-0.0, false), 0.50); // 10/20

  // Up to Positive Zero (NegInf + NegZero + PosZero)
  EXPECT_DOUBLE_EQ(cdf->getCDF(0.0, false), 0.75); // 15/20

  // Positive Infinity (Everything)
  EXPECT_DOUBLE_EQ(cdf->getCDF(std::numeric_limits<double>::infinity(), false),
                   1.0); // 20/20
}

TEST_F(TestCDF, TestCDFEmptyHistogram) {
  TrieManager trieManager;

  SpecialCounts noSpecial{0, 0, 0, 0, 0};
  std::vector<char> buffer = trieManager.MockTrieHeader(13, noSpecial);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto cdf = std::make_shared<CDF>(buffer);

  EXPECT_DOUBLE_EQ(cdf->getCDF(0.0, false), 0.0);
  EXPECT_DOUBLE_EQ(cdf->getCDF(100.0, false), 0.0);
}

TEST_F(TestCDF, TestCDF1DxT_AboveHighestBin) {
  auto histogram = std::make_shared<airtree::query::meta::Histogram>(13);
  TrieManager trieManager;

  // Populate only the lowest possible bin
  trieManager.insert1DxT(0, 1, DistributionMethod::SINGLE, 0);

  SpecialCounts noSpecial{0, 0, 0, 0, 0};
  std::vector<char> buffer = trieManager.MockTrieHeader(13, noSpecial);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  // verify that the trie is populated correctly
  for (size_t idx = 0; idx < BINS_256 * BINS_32; idx++) {
    auto internal_rep = idx;
    uint64_t prefix_8 = (internal_rep >> 5) & 0xFF;
    uint64_t suffix_5 = internal_rep & 0x1F;
    if (prefix_8 == 0 && suffix_5 == 0) {
      GTEST_EXPECT_TRUE(trieManager.getRoot1DxT().populated.test(prefix_8));
      EXPECT_EQ(trieManager.getRoot1DxT().nodes[prefix_8]->counts[suffix_5], 1);
    }
    if (prefix_8 > 0) {
      GTEST_EXPECT_FALSE(trieManager.getRoot1DxT().populated.test(prefix_8));
    }
  }

  auto cdf = std::make_shared<CDF>(buffer);

  // Querying a massive domain value that is well past the first bin
  // It should correctly sum up the single bin and realize it accounts for 100%
  // of the data
  EXPECT_DOUBLE_EQ(
      cdf->getCDF(histogram->getFPNumber((BINS_256 * BINS_32) - 1), false),
      1.0);
}

TEST_F(TestCDF, TestCDF1DxT_InterpolationVsStep) {
  auto histogram = std::make_shared<airtree::query::meta::Histogram>(13);
  TrieManager trieManager;
  auto n_bins = histogram->getBinCount();
  EXPECT_EQ(n_bins, BINS_256 * BINS_32);

  // Populate a few bins
  for (size_t idx = ((n_bins / 2) - 10); idx < ((n_bins / 2) + 10); idx++) {
    auto internal_rep = histogram->getInternalRepresentation(idx);
    uint64_t prefix_8 = (internal_rep >> 5) & 0xFF;
    [[maybe_unused]] uint64_t suffix_5 = internal_rep & 0x1F;
    trieManager.insert1DxT(prefix_8, 32, DistributionMethod::EVEN);
  }

  SpecialCounts noSpecial{0, 0, 0, 0, 0};
  std::vector<char> buffer = trieManager.MockTrieHeader(13, noSpecial);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto cdf = std::make_shared<CDF>(buffer);

  double halfway_value = 4.46e-308;

  // Step function should drop down to the lower bound's cumulative count
  double step_result = cdf->getCDF(halfway_value, false);
  SPDLOG_LOGGER_INFO(airtree::query::logger(), "step_result: {}", step_result);
  // Interpolated function should calculate the fractional count
  double interpolated_result = cdf->getCDF(halfway_value, true);
  SPDLOG_LOGGER_INFO(
      airtree::query::logger(), "interpolated_result: {}", interpolated_result);

  // The interpolated result should logically be higher than the step result
  // for a value halfway through a bin
  EXPECT_GT(interpolated_result, step_result);
}

// TEST_F(TestCDF, TestCDF1DxT_Export) {
//   auto histogram = std::make_shared<airtree::query::meta::Histogram>(13);
//   auto n_bins = histogram->getBinCount();
//   EXPECT_EQ(n_bins, BINS_256 * BINS_32);

//   std::ofstream outFile("cdf_export_test.csv");
//   if (!outFile.is_open()) {
//     SPDLOG_LOGGER_ERROR(airtree::query::logger(), "failed to open file");
//   }

//   TrieManager trieManager;
//   for (size_t idx = ((n_bins / 2) - 10); idx < ((n_bins / 2) + 10); idx++) {
//     SPDLOG_LOGGER_INFO(airtree::query::logger(), "Currently working on index
//     {}", idx); auto internal_rep = histogram->getInternalRepresentation(idx);
//     uint64_t prefix_8 = (internal_rep >> 5) & 0xFF;
//     uint64_t suffix_5 = internal_rep & 0x1F;

//     trieManager.insert1DxT(prefix_8, 1, DistributionMethod::SINGLE,
//     suffix_5);
//     // auto val = reConstruct<double>(internal_rep, 13);
//     auto val = histogram->getFPNumber(idx);
//     SPDLOG_LOGGER_INFO(
//         airtree::query::logger(),
//         "Inserted value: {} at index: {} (prefix_8: {}, suffix_5: {})", val,
//         idx, prefix_8, suffix_5);
//   }

//   SpecialCounts noSpecial{0, 0, 0, 0, 0};
//   std::vector<char> buffer = trieManager.MockTrieHeader(13, noSpecial);
//   trieManager.serializeTrie<TrieNode_13>(buffer);
//   airtree::xport::exportAirTree(
//       buffer, "exported_buffer.csv", airtree::xport::ExportFormat::CSV);

//   outFile << "idx, lower_bound, upper_bound, internal_rep, fp_number, "
//              "prefix_8, suffix_5"
//           << std::endl;
//   for (size_t i = 0; i < n_bins; ++i) {
//     auto internal_rep = histogram->getInternalRepresentation(i);
//     auto fp_number = histogram->getFPNumber(i);
//     auto lower = histogram->getBinLowerBound(i);
//     auto upper = histogram->getBinUpperBound(i);
//     auto prefix_8 = (internal_rep >> 5) & 0xFF;
//     auto suffix_5 = internal_rep & 0x1F;
//     outFile << i << ", " << lower << ", " << upper << ", " << internal_rep
//             << ", " << fp_number << ", " << prefix_8 << ", " << suffix_5
//             << std::endl;
//   }
// }

// TEST_F(TestCDF, TestCDF_InversionConsistency) {
//   std::vector<double> data = {
//       1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
//   FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
//   std::vector<char> buffer = generate_1DxF(array);

//   auto percentileObj = std::make_shared<Percentile>(buffer);
//   auto cdfObj = std::make_shared<CDF>(buffer);

//   for (double p : {10.0, 25.0, 50.0, 75.0, 90.0}) {
//     double value_from_percentile = percentileObj->getPercentile(p);
//     double cdf_at_that_value =
//         cdfObj->getCDF(value_from_percentile, true); // linear mode

//     EXPECT_NEAR(cdf_at_that_value, p / 100.0,
//                 0.02); // small tolerance due to interpolation
//   }
// }