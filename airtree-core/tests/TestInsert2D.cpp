// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)
#include <airtree/core/common/FPHArray.hpp>
#include <airtree/core/schema/trie2d/2DxF.hpp>
#include <airtree/core/schema/trie2d/2DxP.hpp>
#include <gtest/gtest.h>

#include <limits>
#include <random>
#include <vector>

namespace {

std::vector<double> sample(size_t n, unsigned seed) {
  std::mt19937_64 g(seed);
  std::lognormal_distribution<double> d(-2.5, 0.8);
  std::vector<double> v(n);
  for (auto &x : v) x = d(g) * (g() % 5 == 0 ? -1.0 : 1.0);
  const double inf = std::numeric_limits<double>::infinity();
  for (double s : {0.0, -0.0, inf, -inf, std::numeric_limits<double>::quiet_NaN()})
    v[g() % n] = s;
  return v;
}

TEST(Insert2D, PerValueInsertMatchesBatchGenerate) {
  const auto x = sample(5000, 1), y = sample(5000, 2);
  const FPHArray ax = buildFPHArray(x.data(), static_cast<int>(x.size()));
  const FPHArray ay = buildFPHArray(y.data(), static_cast<int>(y.size()));
  {
    auto root = CreateParent_TLE2D_option3();
    uint64_t size = sizeof(TLEoption3_2D);
    auto specials = std::make_unique<SpecialCounts>();
    for (size_t i = 0; i < x.size(); ++i) createAndInsert_2DxP(root.get(), x[i], y[i], size, specials);
    rollUpCounts(root.get());
    EXPECT_EQ(execSerialize_2D_2x10(root.get(), specials), generate_2DxP(ax, ay));
  }
  {
    auto root = CreateParentNode_TLE2D88();
    uint64_t size = sizeof(TLETrieNode_2D);
    auto specials = std::make_unique<SpecialCounts>();
    for (size_t i = 0; i < x.size(); ++i) createAndInsert_2DxF(root.get(), x[i], y[i], size, specials);
    rollUpCounts(root.get());
    EXPECT_EQ(execSerialize_2D(root.get(), specials), generate_2DxF(ax, ay));
  }
}

TEST(Insert2D, EncodeReportsFiniteAxesAndCountsSpecials) {
  auto specials = std::make_unique<SpecialCounts>();
  const double nan = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(encode_2DxP(0.5, 0.25, specials).ndims, 3u);
  EXPECT_EQ(encode_2DxP(nan, 0.25, specials).ndims, 1u);
  EXPECT_EQ(encode_2DxP(0.5, 0.0, specials).ndims, 2u);
  EXPECT_EQ(encode_2DxP(nan, 0.0, specials).ndims, 0u);
  EXPECT_EQ(specials->nanCount, 2);
  EXPECT_EQ(specials->posZeroCount, 2);
}

} // namespace
