#include <airtree/query/bin-boundary/BinBoundary.hpp>
#include <gtest/gtest.h>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/query/AirTreeQuery_internal.hpp>

#include <functional>
#include <airtree/core/utils/TrieManager.hpp>
#include <cstddef>
#include <iostream>
#include <sys/types.h>
#include <vector>

using namespace airtree::query::bin_boundary;

class TestBinBoundaries : public ::testing::Test {
protected:
  void SetUp() override {}

  void TearDown() override {}
};

// FIXME: This test is disabled due to the current implementation of the
// TrieManager. TrieManager should work with smart pointers instead of raw
// pointers.
TEST_F(TestBinBoundaries, BuildBinBoundaries1DxF) {

  // Build a mock buffer for 1DxF
  TrieManager trieManager;
  for (size_t l0_idx = 0; l0_idx < BINS_256; l0_idx++) {
    trieManager.insert1DxT(l0_idx, 32, DistributionMethod::SINGLE);
  }

  std::vector<char> buffer = trieManager.MockTrieHeader(13);
  trieManager.serializeTrie<TrieNode_13>(buffer);

  auto binBoundaryQuery = BinBoundary(buffer);
  auto result = binBoundaryQuery.generateBinBoundaries();
  auto bin_boundaries = result.getBoundaries();

  auto bin_boundaries_1d = std::get<BinBoundary1DList>(*bin_boundaries);
  // Print the bin boundaries for debugging
  auto count_sum = 0;
  for (const auto &boundary : bin_boundaries_1d) {
    double lower = boundary.getLowerBound();
    double upper = boundary.getUpperBound();
    uint32_t count = boundary.getCount();
    count_sum += count;
  }

  EXPECT_EQ(bin_boundaries_1d.size(), BINS_256);
  EXPECT_EQ(count_sum, 256 * 32);
}

TEST_F(TestBinBoundaries, DISABLED_Reconstruct_1024_bins) {
  // double lower_bound = std::numeric_limits<double>::infinity();
  // double upper_bound = -std::numeric_limits<double>::infinity();

  std::vector<double> reconstructed_values;
  std::vector<double> sorted_values;

  for (uint32_t l0_idx = 0; l0_idx < BINS_4096; l0_idx++) {
    // Generate 1D boundaries
    double reconstructed_value = reConstruct<double>(l0_idx, 12);
    // std::cout << "For exponent bit: " << i << " Reconstructed Value for bin
    // "
    // << l0_idx << ": " << reconstructed_value << std::endl;
    reconstructed_values.push_back(reconstructed_value);
  }

  // Sort the reconstructed values
  sorted_values = reconstructed_values;
  std::sort(sorted_values.begin(), sorted_values.end(), std::greater<double>());

  for (size_t i = 0; i < 100; i++) {
    std::cout << "At index " << i
              << ", Reconstructed Value: " << reconstructed_values[i]
              << ", Sorted Value: " << sorted_values[i] << std::endl;
  }
}