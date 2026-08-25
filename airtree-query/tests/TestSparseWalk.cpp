// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/query/AirTreeQuery_internal.hpp>
#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <random>
#include <stdexcept>
#include <vector>

using airtree::core::io::AirTreeReader;
using airtree::query::cdf::CDF;
using airtree::query::meta::Histogram;
using airtree::query::minmax::MinMax;
using airtree::query::minmax::MinMaxResultVector;
using airtree::query::percentile::Percentile;
using airtree::query::percentile::PercentileResult;
using airtree::query::topk::TopK;
using airtree::query::topk::TopKResultVector;

namespace {

constexpr double kInf = std::numeric_limits<double>::infinity();
constexpr uint32_t kSpecialRep = std::numeric_limits<uint32_t>::max();

uint64_t bits(double v) {
  uint64_t b;
  std::memcpy(&b, &v, sizeof(b));
  return b;
}

struct Loaded {
  uint16_t bitLength = 0;
  AirTreeType type;
  airtree::core::common::AirTreeHeader header;
  std::shared_ptr<Histogram> histogram;
};

Loaded load(const std::vector<char> &buffer) {
  AirTreeReader reader;
  reader.read(buffer);
  Loaded l;
  l.bitLength = reader.getBitLength();
  l.type = reader.getType();
  l.header = reader.getHeader();
  l.histogram = std::make_shared<Histogram>(l.bitLength);
  return l;
}

template <typename NodeType>
uint32_t refCount(const std::unique_ptr<NodeType> &trie, uint64_t rep) {
  if constexpr (std::is_same_v<NodeType, TrieNode_13>) {
    uint64_t p8 = (rep >> 5) & 0xFF, p5 = rep & 0x1F;
    if (trie->populated.test(p8) && trie->nodes[p8]->counts[p5] > 0) return trie->nodes[p8]->counts[p5];
  } else if constexpr (std::is_same_v<NodeType, TrieNode_16>) {
    uint64_t p8 = (rep >> 8) & 0xFF, s8 = rep & 0xFF;
    if (trie->populated.test(p8) && trie->nodes[p8]->counts[s8] > 0) return trie->nodes[p8]->counts[s8];
  } else {
    uint64_t p8 = (rep >> 12) & 0xFF, m6 = (rep >> 6) & 0x3F, s6 = rep & 0x3F;
    if (trie->populated.test(p8) && trie->nodes[p8]->populated.test(m6)
        && trie->nodes[p8]->nodes[m6]->counts[s6] > 0)
      return trie->nodes[p8]->nodes[m6]->counts[s6];
  }
  return 0;
}

template <typename NodeType> double refPercentile(const Loaded &l, double percentile) {
  const auto &root = l.type.get_ptr<NodeType>();
  const auto &h = l.header;
  uint32_t trie_count = 0;
  for (size_t i = 0; i < root->size(); ++i) trie_count += root->counts[i];
  uint32_t total = trie_count + h.pos_zero_count + h.neg_zero_count + h.pos_inf_count + h.neg_inf_count;
  if (total == 0) return -kInf;
  double rank = (percentile / 100) * total;
  uint32_t cum = h.neg_inf_count;
  if (cum >= rank) return -kInf;
  const auto &bins = l.histogram->getBins();
  const uint64_t n = bins.size();
  bool zeros = false;
  for (size_t i = 0; i < n; ++i) {
    double v = bins[i].first;
    if (!zeros && v >= 0.0) {
      cum += h.neg_zero_count;
      if (cum >= rank) return -0.0;
      cum += h.pos_zero_count;
      if (cum >= rank) return 0.0;
      zeros = true;
    }
    uint32_t c = refCount<NodeType>(root, bins[i].second.getInternalRepresentation());
    cum += c;
    if (cum < rank) continue;
    uint32_t before = cum - c;
    if (i == n - 1) {
      if (rank > cum && h.pos_inf_count > 0) return kInf;
      return v;
    }
    double next = bins[i + 1].first;
    return v + (((rank - before) / c) * (next - v));
  }
  if (!zeros) {
    cum += h.neg_zero_count;
    if (cum >= rank) return -0.0;
    cum += h.pos_zero_count;
    if (cum >= rank) return 0.0;
  }
  cum += h.pos_inf_count;
  if (cum >= rank) return kInf;
  return -kInf;
}

template <typename NodeType> double refCDF(const Loaded &l, double value, bool interpolate) {
  const auto &root = l.type.get_ptr<NodeType>();
  const auto &h = l.header;
  uint32_t total = 0;
  for (size_t i = 0; i < root->size(); ++i) total += root->counts[i];
  total += h.pos_zero_count + h.neg_zero_count + h.pos_inf_count + h.neg_inf_count;
  if (total == 0) return 0.0;
  if (value == kInf) return 1.0;
  double cum = h.neg_inf_count;
  if (value == -kInf) return cum / total;
  const auto &bins = l.histogram->getBins();
  size_t n = bins.size();
  bool negz = false, posz = false;
  bool inc_neg = (value >= 0.0);
  bool inc_pos = (value > 0.0) || (value == 0.0 && !std::signbit(value));
  for (size_t i = 0; i < n; ++i) {
    double v = bins[i].first;
    uint32_t c = refCount<NodeType>(root, bins[i].second.getInternalRepresentation());
    bool ge_neg = (v >= 0.0);
    bool ge_pos = (v > 0.0) || (v == 0.0 && !std::signbit(v));
    if (!negz && ge_neg) { negz = true; if (inc_neg) cum += h.neg_zero_count; }
    if (!posz && ge_pos) { posz = true; if (inc_pos) cum += h.pos_zero_count; }
    double next = (i + 1 < n) ? bins[i + 1].first : kInf;
    if (value < v) break;
    if (value >= next) {
      cum += c;
    } else {
      if (!interpolate) break;
      cum += ((value - v) / (next - v)) * c;
      break;
    }
  }
  if (!negz && inc_neg) cum += h.neg_zero_count;
  if (!posz && inc_pos) cum += h.pos_zero_count;
  return cum / total;
}

void updMin(MinMaxResultVector &r, uint32_t &m, double lo, double hi, uint32_t c) {
  if (c == 0) return;
  if (c < m) { r.clear(); m = c; r.emplace_back(lo, hi, c); }
  else if (c == m) r.emplace_back(lo, hi, c);
}
void updMax(MinMaxResultVector &r, uint32_t &m, double lo, double hi, uint32_t c) {
  if (c == 0) return;
  if (c > m) { r.clear(); m = c; r.emplace_back(lo, hi, c); }
  else if (c == m) r.emplace_back(lo, hi, c);
}

template <typename NodeType, bool Min> MinMaxResultVector refFreq(const Loaded &l) {
  const auto &root = l.type.get_ptr<NodeType>();
  const auto &h = l.header;
  MinMaxResultVector r;
  uint32_t m = Min ? std::numeric_limits<uint32_t>::max() : 0;
  auto upd = [&](double lo, double hi, uint32_t c) { Min ? updMin(r, m, lo, hi, c) : updMax(r, m, lo, hi, c); };
  upd(-kInf, -kInf, h.neg_inf_count);
  upd(-0.0, -0.0, h.neg_zero_count);
  upd(0.0, 0.0, h.pos_zero_count);
  upd(kInf, kInf, h.pos_inf_count);
  const auto &hist = *l.histogram;
  for (size_t i = 0; i < hist.getBinCount(); ++i)
    upd(hist.getBinLowerBound(i), hist.getBinUpperBound(i), refCount<NodeType>(root, hist.getInternalRepresentation(i)));
  return r;
}

template <typename NodeType> MinMaxResultVector refMinValue(const Loaded &l) {
  const auto &root = l.type.get_ptr<NodeType>();
  const auto &h = l.header;
  const auto &hist = *l.histogram;
  MinMaxResultVector r;
  if (h.neg_inf_count > 0) { r.emplace_back(-kInf, -kInf, h.neg_inf_count); return r; }
  bool zeros = false;
  for (size_t i = 0; i < hist.getBinCount(); ++i) {
    double lo = hist.getBinLowerBound(i);
    if (lo >= 0.0 && !zeros) {
      if (h.neg_zero_count > 0) { r.emplace_back(-0.0, -0.0, h.neg_zero_count); return r; }
      if (h.pos_zero_count > 0) { r.emplace_back(0.0, 0.0, h.pos_zero_count); return r; }
      zeros = true;
    }
    uint32_t c = refCount<NodeType>(root, hist.getInternalRepresentation(i));
    if (c > 0) { r.emplace_back(lo, hist.getBinUpperBound(i), c); return r; }
  }
  if (!zeros) {
    if (h.neg_zero_count > 0) { r.emplace_back(-0.0, -0.0, h.neg_zero_count); return r; }
    if (h.pos_zero_count > 0) { r.emplace_back(0.0, 0.0, h.pos_zero_count); return r; }
  }
  if (h.pos_inf_count > 0) r.emplace_back(kInf, kInf, h.pos_inf_count);
  return r;
}

template <typename NodeType> MinMaxResultVector refMaxValue(const Loaded &l) {
  const auto &root = l.type.get_ptr<NodeType>();
  const auto &h = l.header;
  const auto &hist = *l.histogram;
  MinMaxResultVector r;
  if (h.pos_inf_count > 0) { r.emplace_back(kInf, kInf, h.pos_inf_count); return r; }
  bool zeros = false;
  for (size_t i = hist.getBinCount(); i-- > 0;) {
    double lo = hist.getBinLowerBound(i);
    if (lo < 0.0 && !zeros) {
      if (h.pos_zero_count > 0) { r.emplace_back(0.0, 0.0, h.pos_zero_count); return r; }
      if (h.neg_zero_count > 0) { r.emplace_back(-0.0, -0.0, h.neg_zero_count); return r; }
      zeros = true;
    }
    uint32_t c = refCount<NodeType>(root, hist.getInternalRepresentation(i));
    if (c > 0) { r.emplace_back(lo, hist.getBinUpperBound(i), c); return r; }
  }
  if (!zeros) {
    if (h.pos_zero_count > 0) { r.emplace_back(0.0, 0.0, h.pos_zero_count); return r; }
    if (h.neg_zero_count > 0) { r.emplace_back(-0.0, -0.0, h.neg_zero_count); return r; }
  }
  if (h.neg_inf_count > 0) r.emplace_back(-kInf, -kInf, h.neg_inf_count);
  return r;
}

template <typename NodeType> TopKResultVector refTopK(const Loaded &l, double k) {
  const auto &root = l.type.get_ptr<NodeType>();
  const auto &h = l.header;
  const auto &hist = *l.histogram;
  const auto &bins = hist.getBins();
  uint64_t total = h.pos_inf_count + h.neg_inf_count + h.pos_zero_count + h.neg_zero_count;
  for (size_t i = 0; i < bins.size(); ++i) total += refCount<NodeType>(root, bins[i].second.getInternalRepresentation());
  if (total == 0) return {};
  TopKResultVector r;
  uint64_t n = static_cast<uint64_t>(total * (k / 100.0));
  if (n == 0 && k > 0) n = 1;
  uint64_t cum = 0;
  if (h.pos_inf_count > 0) { cum += h.pos_inf_count; r.emplace_back(kInf, kInf, h.pos_inf_count, kSpecialRep); if (cum >= n) return r; }
  bool zeros = false;
  for (size_t idx = bins.size(); idx-- > 0;) {
    double lo = hist.getBinLowerBound(idx);
    if (lo < 0.0 && !zeros) {
      if (h.pos_zero_count > 0) { cum += h.pos_zero_count; r.emplace_back(0.0, 0.0, h.pos_zero_count, kSpecialRep); if (cum >= n) return r; }
      if (h.neg_zero_count > 0) { cum += h.neg_zero_count; r.emplace_back(-0.0, -0.0, h.neg_zero_count, kSpecialRep); if (cum >= n) return r; }
      zeros = true;
    }
    uint64_t rep = hist.getInternalRepresentation(idx);
    uint32_t c = refCount<NodeType>(root, rep);
    if (c == 0) continue;
    cum += c;
    r.emplace_back(lo, hist.getBinUpperBound(idx), c, static_cast<uint32_t>(rep));
    if (cum >= n) return r;
  }
  if (!zeros) {
    if (h.pos_zero_count > 0) { cum += h.pos_zero_count; r.emplace_back(0.0, 0.0, h.pos_zero_count, kSpecialRep); if (cum >= n) return r; }
    if (h.neg_zero_count > 0) { cum += h.neg_zero_count; r.emplace_back(-0.0, -0.0, h.neg_zero_count, kSpecialRep); if (cum >= n) return r; }
  }
  if (h.neg_inf_count > 0) { cum += h.neg_inf_count; r.emplace_back(-kInf, -kInf, h.neg_inf_count, kSpecialRep); }
  return r;
}

template <typename F> auto dispatch(const Loaded &l, F f) {
  switch (l.bitLength) {
  case 13: return f(static_cast<TrieNode_13 *>(nullptr));
  case 16: return f(static_cast<TrieNode_16 *>(nullptr));
  default: return f(static_cast<TrieNode_20 *>(nullptr));
  }
}

void expectSame(const MinMaxResultVector &a, const MinMaxResultVector &b, const char *what) {
  ASSERT_EQ(a.size(), b.size()) << what;
  for (size_t i = 0; i < a.size(); ++i) {
    EXPECT_EQ(bits(a[i].getLowerBound()), bits(b[i].getLowerBound())) << what << " lower " << i;
    EXPECT_EQ(bits(a[i].getUpperBound()), bits(b[i].getUpperBound())) << what << " upper " << i;
    EXPECT_EQ(a[i].getBinCount(), b[i].getBinCount()) << what << " count " << i;
  }
}

void expectSame(const TopKResultVector &a, const TopKResultVector &b, const char *what) {
  ASSERT_EQ(a.size(), b.size()) << what;
  for (size_t i = 0; i < a.size(); ++i) {
    EXPECT_EQ(bits(a[i].getLowerBound()), bits(b[i].getLowerBound())) << what << " lower " << i;
    EXPECT_EQ(bits(a[i].getUpperBound()), bits(b[i].getUpperBound())) << what << " upper " << i;
    EXPECT_EQ(a[i].getCount(), b[i].getCount()) << what << " count " << i;
    EXPECT_EQ(a[i].getInternalRepresentation(), b[i].getInternalRepresentation()) << what << " rep " << i;
  }
}

void compareAll(const std::vector<char> &buffer, const char *label) {
  SCOPED_TRACE(label);
  Loaded l = load(buffer);
  std::vector<char> copy = buffer;
  Percentile percentile(buffer);
  CDF cdf(copy);
  MinMax minmax(buffer);
  TopK topk(buffer);

  for (double p : {0.001, 0.5, 1.0, 5.0, 25.0, 50.0, 75.0, 95.0, 99.0, 99.9, 99.999}) {
    double want = dispatch(l, [&](auto *t) { return refPercentile<std::remove_pointer_t<decltype(t)>>(l, p); });
    EXPECT_EQ(bits(percentile.getPercentile(p)), bits(want)) << "percentile " << p;
  }

  std::vector<double> probes = {-kInf, -1e300, -5.0, -1.0, -0.5, -1e-9, -0.0, 0.0, 1e-9, 0.5, 1.0, 5.0, 1e300, kInf};
  const auto &values = Histogram::sortedValues(l.bitLength);
  for (size_t i : {values.size() / 3, values.size() / 2, values.size() * 2 / 3}) {
    probes.push_back(values[i]);
    probes.push_back(values[i] + (values[i + 1] - values[i]) * 0.37);
  }
  for (double v : probes) {
    for (bool interp : {false, true}) {
      double want = dispatch(l, [&](auto *t) { return refCDF<std::remove_pointer_t<decltype(t)>>(l, v, interp); });
      EXPECT_EQ(bits(cdf.getCDF(v, interp)), bits(want)) << "cdf " << v << " interp " << interp;
    }
  }

  expectSame(minmax.getMin(), dispatch(l, [&](auto *t) { return refFreq<std::remove_pointer_t<decltype(t)>, true>(l); }), "getMin");
  expectSame(minmax.getMax(), dispatch(l, [&](auto *t) { return refFreq<std::remove_pointer_t<decltype(t)>, false>(l); }), "getMax");
  expectSame(minmax.getMinValue(), dispatch(l, [&](auto *t) { return refMinValue<std::remove_pointer_t<decltype(t)>>(l); }), "getMinValue");
  expectSame(minmax.getMaxValue(), dispatch(l, [&](auto *t) { return refMaxValue<std::remove_pointer_t<decltype(t)>>(l); }), "getMaxValue");

  for (double k : {0.01, 1.0, 5.0, 15.0, 50.0, 100.0}) {
    expectSame(topk.getTopK(k), dispatch(l, [&](auto *t) { return refTopK<std::remove_pointer_t<decltype(t)>>(l, k); }), "topk");
  }
}

std::vector<char> build(int bitLength, const std::vector<double> &data) {
  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  switch (bitLength) {
  case 13: return generate_1DxT(array);
  case 16: return generate_1DxF(array);
  default: return generate_1DxP(array);
  }
}

std::vector<std::vector<double>> corpus() {
  std::mt19937_64 rng(20260825);
  std::lognormal_distribution<double> latency(-2.1, 0.5);
  std::normal_distribution<double> gauss(0.0, 1000.0);
  std::uniform_real_distribution<double> uni(-1.0, 1.0);
  std::vector<std::vector<double>> out;
  out.push_back({0.1035});
  out.push_back({0.0});
  out.push_back({-0.0, 0.0, 0.0});
  out.push_back({kInf, -kInf, 1.0});
  out.push_back({-3.0, -2.0, -1.0});
  out.push_back({1e-300, 1e300, -1e-300, -1e300, 1.0, -1.0});
  for (int n : {5, 100, 1500, 20000}) {
    std::vector<double> a, b, c;
    for (int i = 0; i < n; ++i) {
      a.push_back(latency(rng));
      b.push_back(gauss(rng));
      c.push_back(std::round(uni(rng) * 10.0) / 10.0);
    }
    out.push_back(a);
    out.push_back(b);
    out.push_back(c);
    std::vector<double> d = a;
    for (int i = 0; i < 3; ++i) { d.push_back(0.0); d.push_back(-0.0); d.push_back(kInf); d.push_back(-kInf); d.push_back(std::nan("")); }
    out.push_back(d);
  }
  return out;
}

} // namespace

class TestSparseWalk : public ::testing::TestWithParam<int> {};

TEST_P(TestSparseWalk, MatchesFullTableWalk) {
  int bitLength = GetParam();
  int i = 0;
  for (const auto &data : corpus()) {
    compareAll(build(bitLength, data), ("corpus " + std::to_string(i++) + " n=" + std::to_string(data.size())).c_str());
  }
}

INSTANTIATE_TEST_SUITE_P(Schemas, TestSparseWalk, ::testing::Values(13, 16, 20));

TEST(HistogramTable, RejectsBitLengthsTheCodecDoesNotHave) {
  for (uint64_t bits : {0u, 8u, 12u, 13u, 16u, 20u, 21u, 24u, 32u}) {
    const bool ok = bits == 12 || bits == 13 || bits == 16 || bits == 20;
    if (ok) {
      EXPECT_NO_THROW(Histogram h(bits));
    } else {
      EXPECT_THROW(Histogram h(bits), std::invalid_argument) << bits;
    }
  }
}

namespace {

// Reference: walk the full table to the crossing bin and apply the export's bounds convention.
template <typename NodeType>
PercentileResult refBounds(const Loaded &l, double percentile) {
  const auto &root = l.type.get_ptr<NodeType>();
  const auto &h = l.header;
  const auto &hist = *l.histogram;
  uint32_t total = 0;
  for (size_t i = 0; i < root->size(); ++i) total += root->counts[i];
  total += h.pos_zero_count + h.neg_zero_count + h.pos_inf_count + h.neg_inf_count;
  auto special = [](double v) { return PercentileResult{v, v, v}; };
  if (total == 0) return special(-kInf);
  const double rank = (percentile / 100) * total;
  uint32_t cum = h.neg_inf_count;
  if (cum >= rank) return special(-kInf);
  const size_t n = hist.getBinCount();
  bool zeros = false;
  for (size_t i = 0; i < n; ++i) {
    const double v = hist.getFPNumber(i);
    if (!zeros && v >= 0.0) {
      cum += h.neg_zero_count; if (cum >= rank) return special(-0.0);
      cum += h.pos_zero_count; if (cum >= rank) return special(0.0);
      zeros = true;
    }
    const uint32_t c = refCount<NodeType>(root, hist.getInternalRepresentation(i));
    cum += c;
    if (cum < rank) continue;
    if (i == n - 1 && rank > cum && h.pos_inf_count > 0) return special(kInf);
    const double lo = v < 0.0 ? (i == 0 ? -kInf : hist.getFPNumber(i - 1)) : v;
    const double hi = i == n - 1 ? kInf : v < 0.0 ? v : hist.getFPNumber(i + 1);
    return {std::nan(""), lo, hi}; // value checked separately against getPercentile
  }
  if (!zeros) {
    cum += h.neg_zero_count; if (cum >= rank) return special(-0.0);
    cum += h.pos_zero_count; if (cum >= rank) return special(0.0);
  }
  cum += h.pos_inf_count;
  if (cum >= rank) return special(kInf);
  return special(-kInf);
}

} // namespace

TEST_P(TestSparseWalk, PercentileBoundsMatchReference) {
  const int bitLength = GetParam();
  for (const auto &data : corpus()) {
    auto buffer = build(bitLength, data);
    Loaded l = load(buffer);
    Percentile p(buffer);
    for (double q : {0.0, 0.1, 1.0, 25.0, 50.0, 90.0, 99.0, 99.9, 100.0}) {
      const auto got = p.getPercentileWithBounds(q);
      const auto ref = dispatch(l, [&](auto *t) { return refBounds<std::remove_pointer_t<decltype(t)>>(l, q); });
      EXPECT_EQ(bits(got.value), bits(p.getPercentile(q))) << "q=" << q;
      EXPECT_EQ(bits(got.lower_bound), bits(ref.lower_bound)) << "q=" << q << " n=" << data.size();
      EXPECT_EQ(bits(got.upper_bound), bits(ref.upper_bound)) << "q=" << q << " n=" << data.size();
      if (std::isfinite(got.value) && got.value > 0.0) {
        // interpolation places the value inside its bin; it equals the upper edge when the rank
        // lands exactly on the bin's last observation (always at q = 100)
        EXPECT_TRUE(got.lower_bound <= got.value && got.value <= got.upper_bound) << "q=" << q;
      }
    }
  }
}
