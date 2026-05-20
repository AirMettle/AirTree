// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <gtest/gtest.h>

#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/core/utils/TrieManager.hpp>
#include <airtree/query/AirTreeQuery_internal.hpp>

#include <cstdint>
#include <limits>
#include <tuple>
#include <utility>
#include <vector>

using namespace airtree::query::grid;
using airtree::query::meta::Histogram;

namespace {

std::vector<char>
build2D(const std::vector<std::pair<std::size_t, std::size_t>> &cells) {
  Histogram h(12);
  TrieManager tm;
  for (const auto &c : cells) {
    uint32_t rx = static_cast<uint32_t>(h.getInternalRepresentation(c.first));
    uint32_t ry = static_cast<uint32_t>(h.getInternalRepresentation(c.second));
    uint32_t xt = getTLEEncoding((rx >> 10) & 0x3);
    uint32_t yt = getTLEEncoding((ry >> 10) & 0x3);
    uint32_t ctle = (xt << 3) | yt;
    uint64_t rep = combine_chunks_10b(rx & 0x3FF, ry & 0x3FF);
    tm.insert2DxP(ctle, static_cast<uint32_t>(rep), 1);
  }
  std::vector<char> buf = tm.MockTrieHeader2D(6);
  tm.serializeTrie<TLEoption3_2D>(buf);
  return buf;
}

std::vector<char> build3D(
    const std::vector<std::tuple<std::size_t, std::size_t, std::size_t>> &cells) {
  Histogram h(12);
  TrieManager tm;
  for (const auto &c : cells) {
    uint32_t rx = static_cast<uint32_t>(h.getInternalRepresentation(std::get<0>(c)));
    uint32_t ry = static_cast<uint32_t>(h.getInternalRepresentation(std::get<1>(c)));
    uint32_t rz = static_cast<uint32_t>(h.getInternalRepresentation(std::get<2>(c)));
    uint32_t xt = getTLEEncoding((rx >> 10) & 0x3);
    uint32_t yt = getTLEEncoding((ry >> 10) & 0x3);
    uint32_t zt = getTLEEncoding((rz >> 10) & 0x3);
    uint32_t ctle = (xt << 6) | (yt << 3) | zt;
    uint64_t rep = combine_chunks_10b(rx & 0x3FF, ry & 0x3FF, rz & 0x3FF);
    tm.insert3DxP(ctle, rep, 1);
  }
  std::vector<char> buf = tm.MockTrieHeader3D(6);
  tm.serializeTrie<TLE_3D_3x10>(buf);
  return buf;
}

// Like build2D, but with an explicit count per (x bin, y bin).
std::vector<char> build2DCounts(
    const std::vector<std::tuple<std::size_t, std::size_t, uint32_t>> &cells) {
  Histogram h(12);
  TrieManager tm;
  for (const auto &c : cells) {
    uint32_t rx = static_cast<uint32_t>(h.getInternalRepresentation(std::get<0>(c)));
    uint32_t ry = static_cast<uint32_t>(h.getInternalRepresentation(std::get<1>(c)));
    uint32_t xt = getTLEEncoding((rx >> 10) & 0x3);
    uint32_t yt = getTLEEncoding((ry >> 10) & 0x3);
    uint32_t ctle = (xt << 3) | yt;
    uint64_t rep = combine_chunks_10b(rx & 0x3FF, ry & 0x3FF);
    // insert2DxP increments by one per call, so repeat to reach the count.
    for (uint32_t k = 0; k < std::get<2>(c); ++k) {
      tm.insert2DxP(ctle, static_cast<uint32_t>(rep), 1);
    }
  }
  std::vector<char> buf = tm.MockTrieHeader2D(6);
  tm.serializeTrie<TLEoption3_2D>(buf);
  return buf;
}

// Like build3D, but with an explicit count per (x,y,z) bin.
std::vector<char> build3DCounts(
    const std::vector<std::tuple<std::size_t, std::size_t, std::size_t, uint32_t>>
        &cells) {
  Histogram h(12);
  TrieManager tm;
  for (const auto &c : cells) {
    uint32_t rx = static_cast<uint32_t>(h.getInternalRepresentation(std::get<0>(c)));
    uint32_t ry = static_cast<uint32_t>(h.getInternalRepresentation(std::get<1>(c)));
    uint32_t rz = static_cast<uint32_t>(h.getInternalRepresentation(std::get<2>(c)));
    uint32_t xt = getTLEEncoding((rx >> 10) & 0x3);
    uint32_t yt = getTLEEncoding((ry >> 10) & 0x3);
    uint32_t zt = getTLEEncoding((rz >> 10) & 0x3);
    uint32_t ctle = (xt << 6) | (yt << 3) | zt;
    uint64_t rep = combine_chunks_10b(rx & 0x3FF, ry & 0x3FF, rz & 0x3FF);
    // insert3DxP increments by one per call, so repeat to reach the count.
    for (uint32_t k = 0; k < std::get<3>(c); ++k) {
      tm.insert3DxP(ctle, rep, 1);
    }
  }
  std::vector<char> buf = tm.MockTrieHeader3D(6);
  tm.serializeTrie<TLE_3D_3x10>(buf);
  return buf;
}

uint64_t sumCounts(const GridResult &r) {
  uint64_t s = 0;
  for (uint64_t c : r.counts) {
    s += c;
  }
  return s;
}

bool intervalContains(const GridInterval &iv, double v) {
  if (iv.kind != IntervalKind::Finite) {
    return false;
  }
  bool lo_ok = (v > iv.lower) || (v == iv.lower && iv.lower_inclusive);
  bool hi_ok = (v < iv.upper) || (v == iv.upper && iv.upper_inclusive);
  return lo_ok && hi_ok;
}

// Independent lookup: the lowest-index finite partition whose interval contains
// v, or -1 if none.
int findPartition(const std::vector<GridInterval> &intervals, double v) {
  for (std::size_t i = 0; i < intervals.size(); ++i) {
    if (intervalContains(intervals[i], v)) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

int findKind(const std::vector<GridInterval> &intervals, IntervalKind kind) {
  for (std::size_t i = 0; i < intervals.size(); ++i) {
    if (intervals[i].kind == kind) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

} // namespace

class TestGridQuery : public ::testing::Test {};

// Every inserted point lies inside the queried region, so the cell counts must
// sum to the number of points regardless of how the region is partitioned.
TEST_F(TestGridQuery, Conservation2D) {
  const std::size_t B = 3000;
  const std::size_t K = 20;
  std::vector<std::pair<std::size_t, std::size_t>> cells;
  for (std::size_t x = B; x < B + K; ++x) {
    for (std::size_t y = B; y < B + K; ++y) {
      cells.emplace_back(x, y);
    }
  }
  auto buf = build2D(cells);
  GridQuery gq(buf);

  Histogram h(12);
  GridAxisSpec ax{h.getFPNumber(B), h.getFPNumber(B + K - 1), 4,
                  GridScaling::Linear};
  GridAxisSpec ay = ax;
  auto res = gq.getGrid({ax, ay});

  EXPECT_EQ(res.dims, 2);
  EXPECT_EQ(res.counts.size(),
            res.axis_intervals[0].size() * res.axis_intervals[1].size());
  EXPECT_EQ(sumCounts(res), K * K);
}

// Multiplicative scaling over a strictly positive range still conserves counts.
TEST_F(TestGridQuery, MultiplicativeConserves2D) {
  const std::size_t B = 3000;
  const std::size_t K = 20;
  std::vector<std::pair<std::size_t, std::size_t>> cells;
  for (std::size_t x = B; x < B + K; ++x) {
    for (std::size_t y = B; y < B + K; ++y) {
      cells.emplace_back(x, y);
    }
  }
  auto buf = build2D(cells);
  GridQuery gq(buf);

  Histogram h(12);
  ASSERT_GT(h.getFPNumber(B), 0.0); // multiplicative needs a positive interior
  GridAxisSpec ax{h.getFPNumber(B), h.getFPNumber(B + K - 1), 3,
                  GridScaling::Multiplicative};
  GridAxisSpec ay{h.getFPNumber(B), h.getFPNumber(B + K - 1), 3,
                  GridScaling::Linear};
  auto res = gq.getGrid({ax, ay});
  EXPECT_EQ(sumCounts(res), K * K);
}

// A region whose max falls strictly inside a bin produces a high-edge partition,
// and a degenerate (min == max) axis collapses to a single edge bin.
TEST_F(TestGridQuery, EdgePartitions2D) {
  const std::size_t B = 3000;
  std::vector<std::pair<std::size_t, std::size_t>> cells;
  for (std::size_t x = B; x < B + 10; ++x) {
    cells.emplace_back(x, B);
  }
  auto buf = build2D(cells);
  GridQuery gq(buf);

  Histogram h(12);
  GridAxisSpec ax{h.getFPNumber(B), h.getFPNumber(B + 9), 2, GridScaling::Linear};
  GridAxisSpec ay{h.getFPNumber(B), h.getFPNumber(B), 1, GridScaling::Linear};
  auto res = gq.getGrid({ax, ay});

  EXPECT_EQ(sumCounts(res), 10u);
  // y collapses to one bin that straddles the requested point.
  ASSERT_EQ(res.axis_intervals[1].size(), 1u);
  EXPECT_TRUE(res.axis_intervals[1][0].is_edge);
  // x ends in an edge bin (its max sits strictly inside the last bin).
  ASSERT_GE(res.axis_intervals[0].size(), 2u);
  EXPECT_TRUE(res.axis_intervals[0].back().is_edge);
}

TEST_F(TestGridQuery, Conservation3D) {
  const std::size_t B = 3000;
  const std::size_t K = 6;
  std::vector<std::tuple<std::size_t, std::size_t, std::size_t>> cells;
  for (std::size_t x = B; x < B + K; ++x) {
    for (std::size_t y = B; y < B + K; ++y) {
      for (std::size_t z = B; z < B + K; ++z) {
        cells.emplace_back(x, y, z);
      }
    }
  }
  auto buf = build3D(cells);
  GridQuery gq(buf);

  Histogram h(12);
  GridAxisSpec ax{h.getFPNumber(B), h.getFPNumber(B + K - 1), 3,
                  GridScaling::Linear};
  auto res = gq.getGrid({ax, ax, ax});
  EXPECT_EQ(res.dims, 3);
  EXPECT_EQ(sumCounts(res), K * K * K);
}

TEST_F(TestGridQuery, GuardThrows) {
  const std::size_t B = 3000;
  const std::size_t K = 20;
  std::vector<std::pair<std::size_t, std::size_t>> cells;
  for (std::size_t x = B; x < B + K; ++x) {
    for (std::size_t y = B; y < B + K; ++y) {
      cells.emplace_back(x, y);
    }
  }
  auto buf = build2D(cells);
  GridQuery gq(buf);

  Histogram h(12);
  GridAxisSpec ax{h.getFPNumber(B), h.getFPNumber(B + K - 1), 10,
                  GridScaling::Linear};
  EXPECT_THROW(gq.getGrid({ax, ax}, 10), std::runtime_error);
}

TEST_F(TestGridQuery, ValidationThrows) {
  auto buf = build2D({{3000, 3000}});
  GridQuery gq(buf);
  Histogram h(12);
  GridAxisSpec ok{h.getFPNumber(3000), h.getFPNumber(3010), 4,
                  GridScaling::Linear};

  EXPECT_THROW(gq.getGrid({ok}), std::invalid_argument); // wrong axis count
  GridAxisSpec zero_steps{ok.min, ok.max, 0, GridScaling::Linear};
  EXPECT_THROW(gq.getGrid({zero_steps, ok}), std::invalid_argument);
  GridAxisSpec inverted{ok.max, ok.min, 4, GridScaling::Linear};
  EXPECT_THROW(gq.getGrid({inverted, ok}), std::invalid_argument);
  GridAxisSpec neg_mult{-1.0, 5.0, 4, GridScaling::Multiplicative};
  EXPECT_THROW(gq.getGrid({neg_mult, ok}), std::invalid_argument);
}

// Independently bucket the known points into the result's own partitions and
// compare cell-by-cell, exercising the visitor decode, lookup, and indexing.
TEST_F(TestGridQuery, OracleCellCounts2D) {
  const std::size_t B = 2500;
  std::vector<std::tuple<std::size_t, std::size_t, uint32_t>> pts;
  for (std::size_t x = B; x < B + 12; ++x) {
    for (std::size_t y = B; y < B + 12; ++y) {
      pts.emplace_back(x, y, static_cast<uint32_t>(1 + ((x * 7 + y) % 5)));
    }
  }
  auto buf = build2DCounts(pts);
  GridQuery gq(buf);

  Histogram h(12);
  GridAxisSpec ax{h.getFPNumber(B), h.getFPNumber(B + 11), 4,
                  GridScaling::Linear};
  GridAxisSpec ay{h.getFPNumber(B), h.getFPNumber(B + 11), 3,
                  GridScaling::Linear};
  auto res = gq.getGrid({ax, ay});

  std::size_t ny = res.axis_intervals[1].size();
  std::vector<uint64_t> expected(res.counts.size(), 0);
  uint64_t expected_total = 0;
  for (const auto &p : pts) {
    double vx = h.getFPNumber(std::get<0>(p));
    double vy = h.getFPNumber(std::get<1>(p));
    int px = findPartition(res.axis_intervals[0], vx);
    int py = findPartition(res.axis_intervals[1], vy);
    ASSERT_GE(px, 0);
    ASSERT_GE(py, 0);
    expected[static_cast<std::size_t>(px) * ny + static_cast<std::size_t>(py)] +=
        std::get<2>(p);
    expected_total += std::get<2>(p);
  }

  ASSERT_EQ(res.counts.size(), expected.size());
  for (std::size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(res.counts[i], expected[i]) << "cell " << i;
  }
  EXPECT_EQ(sumCounts(res), expected_total);
}

// Per-cell oracle for 3D: exact bins via TrieManager, bucketed independently.
TEST_F(TestGridQuery, OracleCellCounts3D) {
  const std::size_t B = 2500;
  const std::size_t S = 8;
  std::vector<std::tuple<std::size_t, std::size_t, std::size_t, uint32_t>> pts;
  for (std::size_t x = B; x < B + S; ++x) {
    for (std::size_t y = B; y < B + S; ++y) {
      for (std::size_t z = B; z < B + S; ++z) {
        pts.emplace_back(x, y, z, static_cast<uint32_t>(1 + ((x + 2 * y + 3 * z) % 4)));
      }
    }
  }
  auto buf = build3DCounts(pts);
  GridQuery gq(buf);

  Histogram h(12);
  GridAxisSpec ax{h.getFPNumber(B), h.getFPNumber(B + S - 1), 4, GridScaling::Linear};
  GridAxisSpec ay{h.getFPNumber(B), h.getFPNumber(B + S - 1), 3, GridScaling::Linear};
  GridAxisSpec az{h.getFPNumber(B), h.getFPNumber(B + S - 1), 2, GridScaling::Linear};
  auto res = gq.getGrid({ax, ay, az});

  std::size_t ny = res.axis_intervals[1].size();
  std::size_t nz = res.axis_intervals[2].size();
  std::vector<uint64_t> expected(res.counts.size(), 0);
  uint64_t expected_total = 0;
  for (const auto &p : pts) {
    int px = findPartition(res.axis_intervals[0], h.getFPNumber(std::get<0>(p)));
    int py = findPartition(res.axis_intervals[1], h.getFPNumber(std::get<1>(p)));
    int pz = findPartition(res.axis_intervals[2], h.getFPNumber(std::get<2>(p)));
    ASSERT_GE(px, 0);
    ASSERT_GE(py, 0);
    ASSERT_GE(pz, 0);
    std::size_t idx = (static_cast<std::size_t>(px) * ny +
                       static_cast<std::size_t>(py)) * nz +
                      static_cast<std::size_t>(pz);
    expected[idx] += std::get<3>(p);
    expected_total += std::get<3>(p);
  }

  ASSERT_EQ(res.counts.size(), expected.size());
  for (std::size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(res.counts[i], expected[i]) << "cell " << i;
  }
  EXPECT_EQ(sumCounts(res), expected_total);
}

// Per-cell oracle for 4D through the public generator. Each point is bucketed by
// its input value: a value filed into a bin lies within that bin's interval, and
// partitions are unions of whole bins, so value-based lookup matches the bin the
// query counts it in.
TEST_F(TestGridQuery, OracleCellCounts4D) {
  Histogram h(12);
  const std::size_t B = 2600;
  const std::size_t S = 3; // 3 bins per axis

  // Distinct (x,y,z,w) bin tuples with varied counts.
  std::vector<std::tuple<double, double, double, double, uint32_t>> pts;
  std::vector<double> x, y, z, w;
  for (std::size_t i = 0; i < S; ++i) {
    for (std::size_t j = 0; j < S; ++j) {
      for (std::size_t k = 0; k < S; ++k) {
        for (std::size_t l = 0; l < S; ++l) {
          uint32_t cnt = static_cast<uint32_t>(1 + ((i + j + k + l) % 3));
          double vx = h.getFPNumber(B + i);
          double vy = h.getFPNumber(B + j);
          double vz = h.getFPNumber(B + k);
          double vw = h.getFPNumber(B + l);
          pts.emplace_back(vx, vy, vz, vw, cnt);
          for (uint32_t r = 0; r < cnt; ++r) {
            x.push_back(vx);
            y.push_back(vy);
            z.push_back(vz);
            w.push_back(vw);
          }
        }
      }
    }
  }
  auto buf = airtree::core::api::generate(x, y, z, w);
  GridQuery gq(buf);

  const double inf = std::numeric_limits<double>::infinity();
  GridAxisSpec ax{-inf, inf, 2, GridScaling::Linear};
  auto res = gq.getGrid({ax, ax, ax, ax});

  std::size_t n1 = res.axis_intervals[1].size();
  std::size_t n2 = res.axis_intervals[2].size();
  std::size_t n3 = res.axis_intervals[3].size();
  std::vector<uint64_t> expected(res.counts.size(), 0);
  uint64_t expected_total = 0;
  for (const auto &p : pts) {
    int p0 = findPartition(res.axis_intervals[0], std::get<0>(p));
    int p1 = findPartition(res.axis_intervals[1], std::get<1>(p));
    int p2 = findPartition(res.axis_intervals[2], std::get<2>(p));
    int p3 = findPartition(res.axis_intervals[3], std::get<3>(p));
    ASSERT_GE(p0, 0);
    ASSERT_GE(p1, 0);
    ASSERT_GE(p2, 0);
    ASSERT_GE(p3, 0);
    std::size_t idx =
        ((static_cast<std::size_t>(p0) * n1 + static_cast<std::size_t>(p1)) * n2 +
         static_cast<std::size_t>(p2)) *
            n3 +
        static_cast<std::size_t>(p3);
    expected[idx] += std::get<4>(p);
    expected_total += std::get<4>(p);
  }

  ASSERT_EQ(res.counts.size(), expected.size());
  for (std::size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(res.counts[i], expected[i]) << "cell " << i;
  }
  EXPECT_EQ(sumCounts(res), expected_total);
}

// Smoke + conservation for the 4D path, built through the public generator.
TEST_F(TestGridQuery, Conservation4D) {
  std::vector<double> x, y, z, w;
  const int n = 60;
  for (int i = 0; i < n; ++i) {
    x.push_back(1.0 + i);
    y.push_back(2.0 + i * 0.5);
    z.push_back(3.0 + i * 0.25);
    w.push_back(4.0 + i * 2.0);
  }
  auto buf = airtree::core::api::generate(x, y, z, w); // 4D Precise
  GridQuery gq(buf);

  GridAxisSpec ax{-std::numeric_limits<double>::infinity(),
                  std::numeric_limits<double>::infinity(), 3,
                  GridScaling::Linear};
  auto res = gq.getGrid({ax, ax, ax, ax});
  EXPECT_EQ(res.dims, 4);
  EXPECT_EQ(sumCounts(res), static_cast<uint64_t>(n));
}

// Interval bounds are reported correctly: with more steps than bins each interior
// bin is its own partition, so its bounds must equal the raw histogram edges.
TEST_F(TestGridQuery, IntervalBoundsPositive2D) {
  const std::size_t B = 3000;
  const std::size_t M = 5;
  auto buf = build2D({{B, B}});
  GridQuery gq(buf);

  Histogram h(12);
  ASSERT_GT(h.getFPNumber(B), 0.0);
  GridAxisSpec ax{h.getFPNumber(B), h.getFPNumber(B + M), 10000,
                  GridScaling::Linear};
  GridAxisSpec ay{h.getFPNumber(B), h.getFPNumber(B), 1, GridScaling::Linear};
  auto res = gq.getGrid({ax, ay});

  const auto &xs = res.axis_intervals[0];
  ASSERT_EQ(xs.size(), M + 1); // M interior bins + one high edge
  for (std::size_t k = 0; k < M; ++k) {
    EXPECT_EQ(xs[k].lower, h.getFPNumber(B + k));
    EXPECT_EQ(xs[k].upper, h.getFPNumber(B + k + 1));
    EXPECT_TRUE(xs[k].lower_inclusive);
    EXPECT_FALSE(xs[k].upper_inclusive);
    EXPECT_FALSE(xs[k].is_edge);
  }
  EXPECT_TRUE(xs[M].is_edge);
  EXPECT_EQ(xs[M].lower, h.getFPNumber(B + M));
  EXPECT_EQ(xs[M].upper, h.getFPNumber(B + M + 1));
}

// Negative bins use the (prev, recon] convention, so the edge falls on the low
// side and intervals are upper-inclusive. Also a per-cell oracle on negatives.
TEST_F(TestGridQuery, NegativeBinsOracle2D) {
  Histogram h(12);
  const std::size_t B = 400;
  const std::size_t S = 8;
  ASSERT_LT(h.getFPNumber(B), 0.0);
  ASSERT_LT(h.getFPNumber(B + S - 1), 0.0);

  std::vector<std::tuple<std::size_t, std::size_t, uint32_t>> pts;
  for (std::size_t x = B; x < B + S; ++x) {
    for (std::size_t y = B; y < B + S; ++y) {
      pts.emplace_back(x, y, static_cast<uint32_t>(1 + ((x + y) % 3)));
    }
  }
  auto buf = build2DCounts(pts);
  GridQuery gq(buf);

  GridAxisSpec ax{h.getFPNumber(B), h.getFPNumber(B + S - 1), 4,
                  GridScaling::Linear};
  GridAxisSpec ay = ax;
  auto res = gq.getGrid({ax, ay});

  // Edge is on the low side for a negative range; intervals are upper-inclusive.
  ASSERT_FALSE(res.axis_intervals[0].empty());
  EXPECT_TRUE(res.axis_intervals[0].front().is_edge);
  for (const auto &iv : res.axis_intervals[0]) {
    EXPECT_FALSE(iv.lower_inclusive);
    EXPECT_TRUE(iv.upper_inclusive);
  }

  std::size_t ny = res.axis_intervals[1].size();
  std::vector<uint64_t> expected(res.counts.size(), 0);
  for (const auto &p : pts) {
    int px = findPartition(res.axis_intervals[0], h.getFPNumber(std::get<0>(p)));
    int py = findPartition(res.axis_intervals[1], h.getFPNumber(std::get<1>(p)));
    ASSERT_GE(px, 0);
    ASSERT_GE(py, 0);
    expected[static_cast<std::size_t>(px) * ny + static_cast<std::size_t>(py)] +=
        std::get<2>(p);
  }
  for (std::size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(res.counts[i], expected[i]) << "cell " << i;
  }
}

// A pathologically large step count must not overflow or hang: partitions stay
// bounded by the number of bins in range.
TEST_F(TestGridQuery, LargeStepsBounded2D) {
  const std::size_t B = 3000;
  auto buf = build2D({{B, B}});
  GridQuery gq(buf);

  Histogram h(12);
  GridAxisSpec ax{h.getFPNumber(B), h.getFPNumber(B + 5), 4000000000u,
                  GridScaling::Linear};
  GridAxisSpec ay{h.getFPNumber(B), h.getFPNumber(B), 1, GridScaling::Linear};
  auto res = gq.getGrid({ax, ay});
  EXPECT_LE(res.axis_intervals[0].size(), 7u); // ~5 interior bins + edges
}

// +inf / -inf are surfaced as their own entries, zero routes into the finite
// partition containing it, and NaN is excluded from the grid entirely.
TEST_F(TestGridQuery, SpecialValuesInfNaN2D) {
  const double inf = std::numeric_limits<double>::infinity();
  const double nan = std::numeric_limits<double>::quiet_NaN();
  std::vector<double> x, y;
  auto add = [&](double vx, double vy, int n) {
    for (int i = 0; i < n; ++i) {
      x.push_back(vx);
      y.push_back(vy);
    }
  };
  // Finite data straddling zero so a finite partition contains 0.
  int finite = 0;
  for (double vx : {-3.0, -2.0, -1.0, 1.0, 2.0, 3.0}) {
    for (double vy : {10.0, 20.0}) {
      add(vx, vy, 1);
      ++finite;
    }
  }
  add(inf, 10.0, 5);   // +inf x
  add(-inf, 20.0, 3);  // -inf x
  add(0.0, 10.0, 4);   // zero x -> routes into a finite partition
  add(nan, 20.0, 7);   // NaN x -> excluded

  auto buf = airtree::core::api::generate(x, y);
  GridQuery gq(buf);

  GridAxisSpec ax{-inf, inf, 3, GridScaling::Linear};
  GridAxisSpec ay{-inf, inf, 2, GridScaling::Linear};
  auto res = gq.getGrid({ax, ay});

  int pos = findKind(res.axis_intervals[0], IntervalKind::PosInf);
  int neg = findKind(res.axis_intervals[0], IntervalKind::NegInf);
  ASSERT_GE(pos, 0);
  ASSERT_GE(neg, 0);

  std::size_t ny = res.axis_intervals[1].size();
  uint64_t pos_slice = 0, neg_slice = 0;
  for (std::size_t j = 0; j < ny; ++j) {
    pos_slice += res.counts[static_cast<std::size_t>(pos) * ny + j];
    neg_slice += res.counts[static_cast<std::size_t>(neg) * ny + j];
  }
  EXPECT_EQ(pos_slice, 5u);
  EXPECT_EQ(neg_slice, 3u);
  // finite + +inf + -inf + zero counted; NaN excluded.
  EXPECT_EQ(sumCounts(res), static_cast<uint64_t>(finite) + 5u + 3u + 4u);
}

// With a finite positive range, infinities/zero/NaN are all out of scope.
TEST_F(TestGridQuery, SpecialValueExclusion2D) {
  const double inf = std::numeric_limits<double>::infinity();
  const double nan = std::numeric_limits<double>::quiet_NaN();
  std::vector<double> x, y;
  auto add = [&](double vx, double vy, int n) {
    for (int i = 0; i < n; ++i) {
      x.push_back(vx);
      y.push_back(vy);
    }
  };
  int finite = 0;
  for (double vx : {1.0, 2.0, 3.0, 4.0}) {
    for (double vy : {10.0, 20.0}) {
      add(vx, vy, 1);
      ++finite;
    }
  }
  add(inf, 10.0, 5);
  add(-inf, 20.0, 3);
  add(0.0, 10.0, 4);
  add(nan, 20.0, 7);

  auto buf = airtree::core::api::generate(x, y);
  GridQuery gq(buf);

  GridAxisSpec ax{0.5, 5.0, 3, GridScaling::Linear}; // finite, excludes 0 and inf
  GridAxisSpec ay{5.0, 25.0, 2, GridScaling::Linear};
  auto res = gq.getGrid({ax, ay});

  EXPECT_EQ(findKind(res.axis_intervals[0], IntervalKind::PosInf), -1);
  EXPECT_EQ(findKind(res.axis_intervals[0], IntervalKind::NegInf), -1);
  EXPECT_EQ(sumCounts(res), static_cast<uint64_t>(finite)); // specials all dropped
}

// Mixed-special 4D: one interior dimension is always +inf. The remaining three
// finite chunks must be assigned to the correct axes, verified per cell.
TEST_F(TestGridQuery, MixedSpecial4D) {
  Histogram h(12);
  const double inf = std::numeric_limits<double>::infinity();
  const std::size_t B = 2700;
  const std::size_t S = 3;

  std::vector<std::tuple<double, double, double, uint32_t>> pts; // x, z, w, count
  std::vector<double> x, y, z, w;
  for (std::size_t i = 0; i < S; ++i) {
    for (std::size_t k = 0; k < S; ++k) {
      for (std::size_t l = 0; l < S; ++l) {
        uint32_t cnt = static_cast<uint32_t>(1 + ((i + k + l) % 3));
        double vx = h.getFPNumber(B + i);
        double vz = h.getFPNumber(B + k);
        double vw = h.getFPNumber(B + l);
        pts.emplace_back(vx, vz, vw, cnt);
        for (uint32_t r = 0; r < cnt; ++r) {
          x.push_back(vx);
          y.push_back(inf); // dim 1 is always +inf
          z.push_back(vz);
          w.push_back(vw);
        }
      }
    }
  }
  auto buf = airtree::core::api::generate(x, y, z, w);
  GridQuery gq(buf);

  GridAxisSpec ax{-inf, inf, 2, GridScaling::Linear};
  auto res = gq.getGrid({ax, ax, ax, ax});

  int p1 = findKind(res.axis_intervals[1], IntervalKind::PosInf);
  ASSERT_GE(p1, 0);

  std::size_t n1 = res.axis_intervals[1].size();
  std::size_t n2 = res.axis_intervals[2].size();
  std::size_t n3 = res.axis_intervals[3].size();
  std::vector<uint64_t> expected(res.counts.size(), 0);
  for (const auto &p : pts) {
    int p0 = findPartition(res.axis_intervals[0], std::get<0>(p));
    int p2 = findPartition(res.axis_intervals[2], std::get<1>(p));
    int p3 = findPartition(res.axis_intervals[3], std::get<2>(p));
    ASSERT_GE(p0, 0);
    ASSERT_GE(p2, 0);
    ASSERT_GE(p3, 0);
    std::size_t idx =
        ((static_cast<std::size_t>(p0) * n1 + static_cast<std::size_t>(p1)) * n2 +
         static_cast<std::size_t>(p2)) *
            n3 +
        static_cast<std::size_t>(p3);
    expected[idx] += std::get<3>(p);
  }
  for (std::size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(res.counts[i], expected[i]) << "cell " << i;
  }
}

TEST_F(TestGridQuery, UnsupportedBufferThrows) {
  std::vector<double> data;
  for (int i = 1; i <= 100; ++i) {
    data.push_back(static_cast<double>(i));
  }
  auto buf = airtree::core::api::generate(data); // 1D Precise
  GridQuery gq(buf);
  GridAxisSpec ax{1.0, 100.0, 4, GridScaling::Linear};
  EXPECT_THROW(gq.getGrid({ax}), std::runtime_error);
}
