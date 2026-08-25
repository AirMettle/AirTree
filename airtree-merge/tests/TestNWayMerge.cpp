// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)
#include <airtree/merge/AirTreeMerge.hpp>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/core/utils/Utils.hpp>
#include <gtest/gtest.h>

#include <airtree/core/common/AirTreeHeader.hpp>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

using airtree::merge::mergeAirTree;
using airtree::merge::mergeAirTrees;

namespace {

using Gen = std::vector<char> (*)(const FPHArray &, bool);

std::vector<char> gen(Gen g, const std::vector<double> &v) {
  FPHArray a = buildFPHArray(v.data(), static_cast<int>(v.size()));
  return g(a, true);
}

std::vector<char> fold(const std::vector<std::vector<char>> &parts) {
  std::vector<char> acc = parts[0];
  for (size_t i = 1; i < parts.size(); ++i)
    acc = mergeAirTree(acc, parts[i]);
  return acc;
}

// Parts with disjoint, overlapping and repeated value ranges so unshared subtrees (verbatim
// copy), shared nodes (re-encode) and width changes all occur.
std::vector<std::vector<double>> makeParts(std::mt19937_64 &rng, size_t nParts) {
  std::vector<std::vector<double>> parts(nParts);
  std::lognormal_distribution<double> lat(0.0, 1.0);
  for (size_t p = 0; p < nParts; ++p) {
    const size_t n = 50 + static_cast<size_t>(rng() % 3000);
    const double scale = (p % 3 == 0) ? 1.0 : (p % 3 == 1) ? 1e3 : 1e-3;
    for (size_t i = 0; i < n; ++i)
      parts[p].push_back(lat(rng) * scale * ((rng() % 7 == 0) ? -1.0 : 1.0));
  }
  parts[0].insert(parts[0].end(), 5000, 42.0); // one heavy bin -> wider counts after merging
  if (nParts > 1)
    parts[1].insert(parts[1].end(), 3000, 42.0);
  parts.back().insert(parts.back().end(), {0.0, -0.0, std::numeric_limits<double>::infinity(),
                                           -std::numeric_limits<double>::infinity(),
                                           std::numeric_limits<double>::quiet_NaN()});
  return parts;
}

void checkSchema(Gen g) {
  std::mt19937_64 rng(2026);
  for (size_t nParts : {2u, 3u, 5u, 9u}) {
    auto values = makeParts(rng, nParts);
    std::vector<std::vector<char>> buffers;
    std::vector<double> all;
    for (const auto &v : values) {
      buffers.push_back(gen(g, v));
      all.insert(all.end(), v.begin(), v.end());
    }
    const auto expected = fold(buffers);
    EXPECT_EQ(mergeAirTrees(buffers), expected) << "N=" << nParts;
    EXPECT_EQ(mergeAirTrees(buffers), gen(g, all)) << "N=" << nParts;
  }
}

TEST(NWayMerge, OneDxT) { checkSchema(&generate_1DxT); }
TEST(NWayMerge, OneDxF) { checkSchema(&generate_1DxF); }
TEST(NWayMerge, OneDxP) { checkSchema(&generate_1DxP); }

TEST(NWayMerge, SingleAndEmpty) {
  auto b = gen(&generate_1DxP, {1.0, 2.0, 3.0});
  EXPECT_EQ(mergeAirTrees({b}), b);
  EXPECT_THROW(mergeAirTrees({}), std::invalid_argument);
}

TEST(NWayMerge, RejectsMixedSchemas) {
  auto p = gen(&generate_1DxP, {1.0, 2.0});
  auto f = gen(&generate_1DxF, {1.0, 2.0});
  EXPECT_THROW(mergeAirTrees({p, f}), std::runtime_error);
}

TEST(NWayMerge, RejectsTruncatedInput) {
  auto a = gen(&generate_1DxP, {1.0, 2.0, 3.0});
  auto b = gen(&generate_1DxP, {4.0, 5.0});
  std::vector<char> cut(b.begin(), b.end() - 6);
  EXPECT_THROW(mergeAirTrees({a, cut}), std::runtime_error);
}

TEST(NWayMerge, TwoDFallsBackToPairwise) {
  std::mt19937_64 rng(7);
  std::vector<std::vector<char>> buffers;
  std::vector<double> x, y;
  for (int p = 0; p < 3; ++p) {
    std::vector<double> px, py;
    for (int i = 0; i < 300; ++i) {
      px.push_back(static_cast<double>(rng() % 1000) / 7.0);
      py.push_back(static_cast<double>(rng() % 1000) / 3.0);
    }
    FPHArray ax = buildFPHArray(px.data(), 300), ay = buildFPHArray(py.data(), 300);
    buffers.push_back(generate_2DxP(ax, ay));
  }
  EXPECT_EQ(mergeAirTrees(buffers), fold(buffers));
}

} // namespace

TEST(NWayMerge, MergedHeaderCarriesTheObservationCount) {
  auto a = gen(&generate_1DxP, {1.0, 2.0, 3.0});
  auto b = gen(&generate_1DxP, {4.0, 5.0});
  using airtree::core::common::deserializeHeader;
  EXPECT_EQ(deserializeHeader(mergeAirTrees({a, b})).trie_count, 5u);
  EXPECT_EQ(deserializeHeader(mergeAirTree(a, b)).trie_count, 5u);
}
