#include <gtest/gtest.h>

#include <airtree/query/meta/Histogram.hpp>

#include <cmath>
#include <cstddef>
#include <limits>

using airtree::query::meta::Histogram;

namespace {

// A bin reported by the Histogram must contain its representative value.
// For positives the representative is the lower edge of the bin's input
// range (zeroing low bits rounds magnitude down); for negatives it is the
// upper edge (rounding magnitude down moves negatives toward zero).
void expect_bracket(const Histogram &h, std::size_t i) {
  double rep = h.getFPNumber(i);
  double lo = h.getBinLowerBound(i);
  double hi = h.getBinUpperBound(i);
  EXPECT_LE(lo, rep) << "bin " << i;
  EXPECT_LE(rep, hi) << "bin " << i;
}

} // namespace

class HistogramBoundary : public ::testing::TestWithParam<int> {};

TEST_P(HistogramBoundary, RepresentativeIsContainedInBin) {
  Histogram h(GetParam());
  const std::size_t n = h.getBins().size();
  ASSERT_GT(n, 0u);
  expect_bracket(h, 0);
  expect_bracket(h, n / 4);
  expect_bracket(h, n / 2);
  expect_bracket(h, n / 2 - 1);
  expect_bracket(h, n - 1);
}

TEST_P(HistogramBoundary, AdjacentBoundsAreContiguous) {
  Histogram h(GetParam());
  const std::size_t n = h.getBins().size();
  ASSERT_GT(n, 1u);
  // Spot-check a handful of adjacent pairs. The negative/positive
  // crossover (around index n/2) leaves a small gap reserved for zero
  // (tracked as a special value), so we exclude it.
  for (std::size_t i : {std::size_t{1}, n / 4, n / 2 + 1, n - 2}) {
    EXPECT_DOUBLE_EQ(h.getBinUpperBound(i), h.getBinLowerBound(i + 1))
        << "bin " << i << " / " << i + 1;
  }
}

TEST_P(HistogramBoundary, BoundsAreOrdered) {
  Histogram h(GetParam());
  const std::size_t n = h.getBins().size();
  for (std::size_t i : {std::size_t{0}, n / 4, n / 2 - 1, n / 2,
                        n / 2 + 1, n - 1}) {
    EXPECT_LE(h.getBinLowerBound(i), h.getBinUpperBound(i)) << "bin " << i;
  }
}

TEST_P(HistogramBoundary, NegativeBinReachesNegativeInfinity) {
  Histogram h(GetParam());
  // Bin 0 holds the most-negative reconstructed value; its lower edge
  // is -inf because nothing precedes it in the sorted order.
  EXPECT_EQ(h.getBinLowerBound(0), -std::numeric_limits<double>::infinity());
}

TEST_P(HistogramBoundary, PositiveBinReachesPositiveInfinity) {
  Histogram h(GetParam());
  const std::size_t n = h.getBins().size();
  EXPECT_EQ(h.getBinUpperBound(n - 1),
            std::numeric_limits<double>::infinity());
}

INSTANTIATE_TEST_SUITE_P(AllBitLengths, HistogramBoundary,
                         ::testing::Values(13, 16, 20));
