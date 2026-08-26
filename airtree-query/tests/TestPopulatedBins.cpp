// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/common/FPHArray.hpp>
#include <airtree/core/io/AirTreeReader.hpp>
#include <airtree/core/schema/trie1d/1DxF.hpp>
#include <airtree/core/schema/trie1d/1DxP.hpp>
#include <airtree/core/schema/trie1d/1DxT.hpp>
#include <airtree/core/schema/trie2d/2DxP.hpp>
#include <airtree/query/meta/PopulatedBins.hpp>
#include <gtest/gtest.h>

#include <limits>
#include <random>
#include <vector>

namespace {

using airtree::query::meta::Histogram;
using airtree::query::meta::PopulatedBin;

std::vector<double> sample(size_t n, unsigned seed) {
  std::mt19937_64 g(seed);
  std::lognormal_distribution<double> d(-2.5, 0.8);
  std::vector<double> v(n);
  for (auto &x : v) x = d(g) * (g() % 4 == 0 ? -1.0 : 1.0);
  if (n > 8) {
    const double inf = std::numeric_limits<double>::infinity();
    v[0] = 0.0; v[1] = -0.0; v[2] = inf; v[3] = -inf; v[4] = std::numeric_limits<double>::quiet_NaN(); v[5] = 1.0e9;
  }
  return v;
}

template <typename NodeType>
void expectStreamedEqualsTreeWalk(const std::vector<char> &buffer, uint64_t bits) {
  Histogram h(bits);
  airtree::core::io::AirTreeReader reader;
  reader.read(buffer);
  auto type = reader.getType();
  const auto walked = airtree::query::meta::populatedBins(type.template get_ptr<NodeType>(), h);
  const auto header = airtree::core::common::deserializeHeader(buffer);
  const auto streamed = airtree::query::meta::populatedBins(buffer, header, h);
  ASSERT_EQ(streamed.bins.size(), walked.size());
  uint64_t total = 0;
  for (size_t i = 0; i < walked.size(); ++i) {
    EXPECT_EQ(streamed.bins[i].position, walked[i].position) << i;
    EXPECT_EQ(streamed.bins[i].code, walked[i].code) << i;
    EXPECT_EQ(streamed.bins[i].count, walked[i].count) << i;
    total += walked[i].count;
  }
  EXPECT_EQ(streamed.trieCount, total);
  EXPECT_EQ(streamed.trieCount, header.trie_count);
}

TEST(PopulatedBins, StreamedFromBytesEqualsTheTreeWalk) {
  for (size_t n : {1u, 40u, 1345u, 20000u}) {
    const auto v = sample(n, static_cast<unsigned>(n));
    const FPHArray a = buildFPHArray(v.data(), static_cast<int>(v.size()));
    expectStreamedEqualsTreeWalk<TrieNode_13>(generate_1DxT(a, true), 13);
    expectStreamedEqualsTreeWalk<TrieNode_16>(generate_1DxF(a, true), 16);
    expectStreamedEqualsTreeWalk<TrieNode_20>(generate_1DxP(a, true), 20);
  }
}

TEST(PopulatedBins, RejectsTruncatedAndNonOneDimensionalBuffers) {
  const auto v = sample(500, 7);
  const FPHArray a = buildFPHArray(v.data(), static_cast<int>(v.size()));
  const auto buf = generate_1DxP(a, true);
  const auto header = airtree::core::common::deserializeHeader(buf);
  Histogram h(20);
  const std::vector<char> cut(buf.begin(), buf.begin() + static_cast<long>(buf.size() - 40));
  EXPECT_THROW(airtree::query::meta::populatedBins(cut, header, h), std::runtime_error);
  const auto two = generate_2DxP(a, a, true);
  EXPECT_THROW(airtree::query::meta::populatedBins(two, airtree::core::common::deserializeHeader(two), Histogram(12)), std::runtime_error);
}

} // namespace
