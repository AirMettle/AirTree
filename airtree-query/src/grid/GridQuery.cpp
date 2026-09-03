// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/query/grid/GridQuery.hpp>

#include <airtree/core/AirTreeCore_internal.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

using namespace airtree::core::io;
using namespace airtree::core::common;

namespace airtree::query::grid {

// Per-axis data extent, computed at most once. Held behind a pointer in
// GridQuery so the once_flag does not make GridQuery non-movable.
struct ExtentCache {
  std::once_flag        once;
  std::array<double, 4> min{};
  std::array<double, 4> max{};
  std::array<bool, 4>   has{};
};

namespace {

constexpr int kBitLength = 12;
constexpr int kNumBins = 1 << kBitLength; // 4096 finite bins per axis
constexpr double kPosInf = std::numeric_limits<double>::infinity();
constexpr double kNegInf = -std::numeric_limits<double>::infinity();


struct BinTable {
  std::array<double, kNumBins> value{};
  std::array<double, kNumBins> lower{};
  std::array<double, kNumBins> upper{};
  std::array<bool, kNumBins>   lower_incl{};
  std::array<bool, kNumBins>   upper_incl{};
  std::array<int, kNumBins>    sorted_to_id{}; 
};

BinTable buildBinTable() {
  BinTable t;
  for (int id = 0; id < kNumBins; ++id) {
    t.value[id] = reConstruct<double>(static_cast<unsigned int>(id), kBitLength);
    t.sorted_to_id[id] = id;
  }
  std::sort(t.sorted_to_id.begin(), t.sorted_to_id.end(),
            [&](int a, int b) { return t.value[a] < t.value[b]; });

  for (int pos = 0; pos < kNumBins; ++pos) {
    int id = t.sorted_to_id[pos];
    double v = t.value[id];
    if (v < 0.0) {
      // Negative bin: (prev, value]
      t.lower[id] = (pos > 0) ? t.value[t.sorted_to_id[pos - 1]] : kNegInf;
      t.upper[id] = v;
      t.lower_incl[id] = false;
      t.upper_incl[id] = true;
    } else {
      // Non-negative bin: [value, next)
      t.lower[id] = v;
      t.upper[id] =
          (pos < kNumBins - 1) ? t.value[t.sorted_to_id[pos + 1]] : kPosInf;
      t.lower_incl[id] = true;
      t.upper_incl[id] = false;
    }
  }
  return t;
}

const BinTable &binTable() {
  static const BinTable table = buildBinTable();
  return table;
}

// ---------------------------------------------------------------------------
// Visitor over the deserialized trie nodes. Emits, per populated bin, one
// AxisBin per axis plus the count. Records where any axis is NaN are skipped.
// ---------------------------------------------------------------------------
enum class AxisBinKind { Finite, NegInf, PosInf, Zero };

struct AxisBin {
  AxisBinKind kind = AxisBinKind::Finite;
  uint16_t    internal12 = 0; // valid for Finite
  double      value = 0.0;    // bin value (Finite), 0 (Zero), or +/-inf
};

// One axis's decoded top-level encoding.
struct AxisTle {
  bool         special;
  TLEValueType type;
  uint32_t     prefix; // sign/exponent-sign prefix for finite axes
};

// A whole combined TLE decoded once.
struct TleEntry {
  std::array<AxisTle, 4> axes{};
  int  specialCount = 0;
  bool hasNaN = false;
};

std::vector<TleEntry> buildTleTable(int D) {
  std::vector<TleEntry> table(static_cast<size_t>(1) << (3 * D));
  for (size_t tle = 0; tle < table.size(); ++tle) {
    auto res =
        deconstructTLE(static_cast<uint32_t>(tle), static_cast<uint32_t>(D));
    TleEntry &e = table[tle];
    e.specialCount = static_cast<int>(res.specialCount);
    for (int i = 0; i < D; ++i) {
      const auto &di = res.dimensionInfos[static_cast<size_t>(i)];
      e.axes[static_cast<size_t>(i)] = {di.isSpecial, di.valueType, di.prefix};
      if (di.isSpecial && di.valueType == TLEValueType::NaN) {
        e.hasNaN = true;
      }
    }
  }
  return table;
}

// Every combined TLE decoded once via the canonical deconstructTLE and cached, so
// there is no second decode to drift and the per-node lookup is O(1) with no
// allocation.
const TleEntry &tleEntry(int D, uint32_t tle) {
  if (D == 2) {
    static const std::vector<TleEntry> t = buildTleTable(2);
    return t[tle];
  }
  if (D == 3) {
    static const std::vector<TleEntry> t = buildTleTable(3);
    return t[tle];
  }
  static const std::vector<TleEntry> t = buildTleTable(4);
  return t[tle];
}

template <class Cb>
void emitRecord(int D, const std::array<AxisTle, 4> &dims,
                const uint32_t *chunks, const BinTable &bt, uint64_t count,
                Cb &cb) {
  std::array<AxisBin, 4> axes{};
  int ci = 0;
  for (int i = 0; i < D; ++i) {
    const AxisTle &di = dims[static_cast<size_t>(i)];
    AxisBin &ax = axes[static_cast<size_t>(i)];
    if (!di.special) {
      uint32_t id12 = (di.prefix << 10) | (chunks[ci++] & 0x3FFu);
      ax.kind = AxisBinKind::Finite;
      ax.internal12 = static_cast<uint16_t>(id12);
      ax.value = bt.value[id12];
    } else {
      switch (di.type) {
      case TLEValueType::InfPos:
        ax.kind = AxisBinKind::PosInf;
        ax.value = kPosInf;
        break;
      case TLEValueType::InfNeg:
        ax.kind = AxisBinKind::NegInf;
        ax.value = kNegInf;
        break;
      case TLEValueType::Zero:
      default:
        ax.kind = AxisBinKind::Zero;
        ax.value = 0.0;
        break;
      }
    }
  }
  cb(axes, D, count);
}

// Split a combined chunk (one 10-bit field per finite axis, in dimension order)
// back into per-finite-axis chunks.
void splitChunks(uint64_t combined, int finiteDims, uint32_t *out) {
  switch (finiteDims) {
  case 1:
    out[0] = static_cast<uint32_t>(combined & 0x3FF);
    break;
  case 2: {
    ChunkPair p = reverse_combine_chunks_10b(combined);
    out[0] = p.x;
    out[1] = p.y;
    break;
  }
  case 3: {
    ChunkTriple p = reverse_combine_chunks_10b_3(combined);
    out[0] = p.x;
    out[1] = p.y;
    out[2] = p.z;
    break;
  }
  case 4: {
    ChunkQuad p = reverse_combine_chunks_10b_4(combined);
    out[0] = p.x;
    out[1] = p.y;
    out[2] = p.z;
    out[3] = p.w;
    break;
  }
  default:
    break;
  }
}

template <class Cb>
void visit2D(const TLEoption3_2D *root, const BinTable &bt, Cb &&cb) {
  if (!root) {
    return;
  }
  for (size_t tle = 0; tle < BINS_64; ++tle) {
    if (!root->populated[tle]) {
      continue;
    }
    const TleEntry &e = tleEntry(2, static_cast<uint32_t>(tle));
    if (e.hasNaN) {
      continue;
    }
    int finiteDims = 2 - e.specialCount;
    if (finiteDims == 0) {
      uint32_t c = root->counts[tle];
      if (c > 0) {
        emitRecord(2, e.axes, nullptr, bt, c, cb);
      }
      continue;
    }
    const TrieNode_2D_10 *n0 = root->nodes[tle].get();
    if (!n0) {
      continue;
    }
    if (finiteDims == 1) {
      for (size_t l0 = 0; l0 < BINS_1024; ++l0) {
        uint32_t c = n0->counts[l0];
        if (c == 0) {
          continue;
        }
        uint32_t chunks[1];
        splitChunks(l0, 1, chunks);
        emitRecord(2, e.axes, chunks, bt, c, cb);
      }
    } else {
      for (size_t l0 = 0; l0 < BINS_1024; ++l0) {
        if (!n0->populated[l0]) {
          continue;
        }
        const TrieNode_2D_10_Level1 *n1 = n0->nodes[l0].get();
        if (!n1) {
          continue;
        }
        for (size_t l1 = 0; l1 < BINS_1024; ++l1) {
          uint32_t c = n1->counts[l1];
          if (c == 0) {
            continue;
          }
          uint64_t combined = (static_cast<uint64_t>(l0) << 10) | l1;
          uint32_t chunks[2];
          splitChunks(combined, 2, chunks);
          emitRecord(2, e.axes, chunks, bt, c, cb);
        }
      }
    }
  }
}

template <class Cb>
void visit3D(const TLE_3D_3x10 *root, const BinTable &bt, Cb &&cb) {
  if (!root) {
    return;
  }
  for (size_t tle = 0; tle < BINS_512; ++tle) {
    if (!root->populated[tle]) {
      continue;
    }
    const TleEntry &e = tleEntry(3, static_cast<uint32_t>(tle));
    if (e.hasNaN) {
      continue;
    }
    int finiteDims = 3 - e.specialCount;
    if (finiteDims == 0) {
      uint32_t c = root->counts[tle];
      if (c > 0) {
        emitRecord(3, e.axes, nullptr, bt, c, cb);
      }
      continue;
    }
    const Node3D_3x10_l0 *n0 = root->nodes[tle].get();
    if (!n0) {
      continue;
    }
    if (finiteDims == 1) {
      for (size_t a = 0; a < BINS_1024; ++a) {
        uint32_t c = n0->counts[a];
        if (c == 0) {
          continue;
        }
        uint32_t chunks[1];
        splitChunks(a, 1, chunks);
        emitRecord(3, e.axes, chunks, bt, c, cb);
      }
      continue;
    }
    for (size_t a = 0; a < BINS_1024; ++a) {
      if (!n0->populated[a]) {
        continue;
      }
      const Node3D_3x10_l1 *n1 = n0->nodes[a].get();
      if (!n1) {
        continue;
      }
      if (finiteDims == 2) {
        for (size_t b = 0; b < BINS_1024; ++b) {
          uint32_t c = n1->counts[b];
          if (c == 0) {
            continue;
          }
          uint64_t combined = (static_cast<uint64_t>(a) << 10) | b;
          uint32_t chunks[2];
          splitChunks(combined, 2, chunks);
          emitRecord(3, e.axes, chunks, bt, c, cb);
        }
        continue;
      }
      for (size_t b = 0; b < BINS_1024; ++b) {
        if (!n1->populated[b]) {
          continue;
        }
        const Node3D_3x10_l2 *n2 = n1->nodes[b].get();
        if (!n2) {
          continue;
        }
        for (size_t c2 = 0; c2 < BINS_1024; ++c2) {
          uint32_t c = n2->counts[c2];
          if (c == 0) {
            continue;
          }
          uint64_t combined = (static_cast<uint64_t>(a) << 20) |
                              (static_cast<uint64_t>(b) << 10) | c2;
          uint32_t chunks[3];
          splitChunks(combined, 3, chunks);
          emitRecord(3, e.axes, chunks, bt, c, cb);
        }
      }
    }
  }
}

template <class Cb>
void visit4D(const TLE_4D_4x10 *root, const BinTable &bt, Cb &&cb) {
  if (!root) {
    return;
  }
  for (size_t tle = 0; tle < BINS_4096; ++tle) {
    if (!root->populated[tle]) {
      continue;
    }
    const TleEntry &e = tleEntry(4, static_cast<uint32_t>(tle));
    if (e.hasNaN) {
      continue;
    }
    int finiteDims = 4 - e.specialCount;
    if (finiteDims == 0) {
      uint32_t c = root->counts[tle];
      if (c > 0) {
        emitRecord(4, e.axes, nullptr, bt, c, cb);
      }
      continue;
    }
    const Node4D_4x10_l0 *n0 = root->nodes[tle].get();
    if (!n0) {
      continue;
    }
    auto emit = [&](uint64_t combined, int dims, uint32_t c) {
      if (c == 0) {
        return;
      }
      uint32_t chunks[4];
      splitChunks(combined, dims, chunks);
      emitRecord(4, e.axes, chunks, bt, c, cb);
    };
    n0->forEach([&](size_t a, uint32_t c0, const Node4D_4x10_l1 *n1) {
      if (finiteDims == 1) {
        emit(a, 1, c0);
        return;
      }
      if (!n1) {
        return;
      }
      n1->forEach([&](size_t b, uint32_t c1, const Node4D_4x10_l2 *n2) {
        if (finiteDims == 2) {
          emit((static_cast<uint64_t>(a) << 10) | b, 2, c1);
          return;
        }
        if (!n2) {
          return;
        }
        n2->forEach([&](size_t c3, uint32_t c2, const Node4D_4x10_l3 *n3) {
          if (finiteDims == 3) {
            emit((static_cast<uint64_t>(a) << 20) | (static_cast<uint64_t>(b) << 10) | c3, 3, c2);
            return;
          }
          if (!n3) {
            return;
          }
          n3->forEach([&](size_t d4, uint32_t c) {
            emit((static_cast<uint64_t>(a) << 30) | (static_cast<uint64_t>(b) << 20) |
                     (static_cast<uint64_t>(c3) << 10) | d4, 4, c);
          });
        });
      });
    });
  }
}

// Walk the populated bins of whichever supported trie the buffer holds.
template <class Cb>
void dispatchVisit(int dims, const AirTreeType &node, const BinTable &bt,
                   Cb &&cb) {
  if (dims == 2) {
    visit2D(node.borrow<TLEoption3_2D>(), bt, cb);
  } else if (dims == 3) {
    visit3D(node.borrow<TLE_3D_3x10>(), bt, cb);
  } else if (dims == 4) {
    visit4D(node.borrow<TLE_4D_4x10>(), bt, cb);
  }
}

// ---------------------------------------------------------------------------
// Per-axis partition model.
// ---------------------------------------------------------------------------
struct Extent {
  double min = kPosInf;
  double max = kNegInf;
  bool   has = false;

  void add(double v) {
    has = true;
    min = std::min(min, v);
    max = std::max(max, v);
  }
};

struct AxisModel {
  std::vector<GridInterval> intervals;
  std::array<int, kNumBins>  finite_lut{}; // internal12 -> partition, -1 if outside
  int neg_inf_partition = -1;
  int pos_inf_partition = -1;
  int zero_partition = -1; // partition for Zero records, -1 if dropped
};

struct PartBuild {
  GridInterval iv;
  int pos_lo = -1; // value-sorted bin range covered, -1 for non-finite
  int pos_hi = -1;
};

PartBuild makeFinitePart(const BinTable &bt, int lo, int hi, bool is_edge) {
  int id_lo = bt.sorted_to_id[lo];
  int id_hi = bt.sorted_to_id[hi];
  GridInterval iv;
  iv.kind = IntervalKind::Finite;
  iv.lower = bt.lower[id_lo];
  iv.upper = bt.upper[id_hi];
  iv.lower_inclusive = bt.lower_incl[id_lo];
  iv.upper_inclusive = bt.upper_incl[id_hi];
  iv.is_edge = is_edge;
  return {iv, lo, hi};
}

bool intervalContainsZero(const GridInterval &iv) {
  bool lo_ok = (iv.lower < 0.0) || (iv.lower == 0.0 && iv.lower_inclusive);
  bool hi_ok = (iv.upper > 0.0) || (iv.upper == 0.0 && iv.upper_inclusive);
  return lo_ok && hi_ok;
}

void addInteriorParts(std::vector<PartBuild> &parts, const BinTable &bt, int ilo,
                      int ihi, const GridAxisSpec &spec) {
  double lo_edge = bt.lower[bt.sorted_to_id[ilo]];
  double hi_edge = bt.upper[bt.sorted_to_id[ihi]];

  // A single requested step, a single interior bin, or a degenerate range all
  // collapse to one partition spanning the whole interior.
  if (spec.steps <= 1 || ihi <= ilo || !(hi_edge > lo_edge)) {
    parts.push_back(makeFinitePart(bt, ilo, ihi, false));
    return;
  }

  // Step count as a double so arbitrarily large `steps` never overflows an int
  // or materializes a boundary array; the number of partitions emitted is bounded
  // by the number of interior bins regardless of how large `steps` is.
  const double n = static_cast<double>(spec.steps);
  const bool mult = (spec.scaling == GridScaling::Multiplicative);
  const double denom = mult ? std::log(hi_edge / lo_edge) : (hi_edge - lo_edge);

  // Closed form of "how many step boundaries fall at or below this bin's lower
  // edge", clamped to [0, steps-1]. Assigns each interior bin to a step group;
  // contiguous runs of equal group become partitions, so empty groups simply
  // produce fewer partitions than requested.
  auto groupOf = [&](double le) -> long long {
    double frac = mult ? (std::log(le / lo_edge) / denom) : ((le - lo_edge) / denom);
    double g = std::floor(frac * n);
    if (g < 0.0) {
      g = 0.0;
    } else if (g > n - 1.0) {
      g = n - 1.0;
    }
    return static_cast<long long>(g);
  };

  int run_lo = ilo;
  long long run_group = groupOf(bt.lower[bt.sorted_to_id[ilo]]);
  for (int p = ilo + 1; p <= ihi; ++p) {
    long long g = groupOf(bt.lower[bt.sorted_to_id[p]]);
    if (g != run_group) {
      parts.push_back(makeFinitePart(bt, run_lo, p - 1, false));
      run_lo = p;
      run_group = g;
    }
  }
  parts.push_back(makeFinitePart(bt, run_lo, ihi, false));
}

AxisModel buildAxisModel(const GridAxisSpec &spec, const BinTable &bt,
                         const Extent &ext) {
  AxisModel m;
  m.finite_lut.fill(-1);

  bool neg_inf_end = std::isinf(spec.min) && spec.min < 0.0;
  bool pos_inf_end = std::isinf(spec.max) && spec.max > 0.0;

  // Resolve the finite interior, substituting the data extent for infinite ends.
  double fmin = 0.0;
  double fmax = 0.0;
  bool have_finite = true;
  if (neg_inf_end) {
    if (!ext.has) {
      have_finite = false;
    } else {
      fmin = ext.min;
    }
  } else {
    fmin = spec.min;
  }
  if (pos_inf_end) {
    if (!ext.has) {
      have_finite = false;
    } else {
      fmax = ext.max;
    }
  } else {
    fmax = spec.max;
  }
  if (have_finite && fmin > fmax) {
    have_finite = false;
  }

  if (spec.scaling == GridScaling::Multiplicative) {
    if (!have_finite || !(fmin > 0.0) || !(fmax > fmin)) {
      throw std::invalid_argument(
          "multiplicative scaling requires a strictly positive finite interior "
          "with end greater than start");
    }
  }

  std::vector<PartBuild> parts;

  if (neg_inf_end) {
    GridInterval iv;
    iv.kind = IntervalKind::NegInf;
    iv.lower = kNegInf;
    iv.upper = kNegInf;
    iv.lower_inclusive = true;
    iv.upper_inclusive = true;
    parts.push_back({iv, -1, -1});
  }

  if (have_finite) {
    // Bins whose native interval overlaps the closed range [fmin, fmax].
    int scope_lo = -1;
    int scope_hi = -1;
    for (int pos = 0; pos < kNumBins; ++pos) {
      int id = bt.sorted_to_id[pos];
      double bl = bt.lower[id];
      double bu = bt.upper[id];
      bool right_of_fmin = (bu > fmin) || (bu == fmin && bt.upper_incl[id]);
      bool left_of_fmax = (bl < fmax) || (bl == fmax && bt.lower_incl[id]);
      if (right_of_fmin && left_of_fmax) {
        if (scope_lo < 0) {
          scope_lo = pos;
        }
        scope_hi = pos;
      }
    }

    if (scope_lo >= 0) {
      auto fully = [&](int pos) {
        int id = bt.sorted_to_id[pos];
        return bt.lower[id] >= fmin && bt.upper[id] <= fmax;
      };
      int interior_lo = scope_lo;
      int interior_hi = scope_hi;
      bool low_edge = false;
      bool high_edge = false;
      if (scope_lo == scope_hi) {
        if (!fully(scope_lo)) {
          low_edge = true;
          interior_lo = 1; // empty interior
          interior_hi = 0;
        }
      } else {
        if (!fully(scope_lo)) {
          low_edge = true;
          interior_lo = scope_lo + 1;
        }
        if (!fully(scope_hi)) {
          high_edge = true;
          interior_hi = scope_hi - 1;
        }
      }

      if (low_edge) {
        parts.push_back(makeFinitePart(bt, scope_lo, scope_lo, true));
      }
      if (interior_lo <= interior_hi) {
        addInteriorParts(parts, bt, interior_lo, interior_hi, spec);
      }
      if (high_edge) {
        parts.push_back(makeFinitePart(bt, scope_hi, scope_hi, true));
      }
    }
  }

  if (pos_inf_end) {
    GridInterval iv;
    iv.kind = IntervalKind::PosInf;
    iv.lower = kPosInf;
    iv.upper = kPosInf;
    iv.lower_inclusive = true;
    iv.upper_inclusive = true;
    parts.push_back({iv, -1, -1});
  }

  // Determine where Zero records route, creating a singleton if needed.
  int zero_target = -1;
  bool zero_in_range = have_finite && fmin <= 0.0 && 0.0 <= fmax;
  if (zero_in_range) {
    for (size_t i = 0; i < parts.size(); ++i) {
      if (parts[i].iv.kind == IntervalKind::Finite &&
          parts[i].iv.lower == 0.0) {
        zero_target = static_cast<int>(i);
        break;
      }
    }
    if (zero_target < 0) {
      for (size_t i = 0; i < parts.size(); ++i) {
        if (parts[i].iv.kind == IntervalKind::Finite &&
            intervalContainsZero(parts[i].iv)) {
          zero_target = static_cast<int>(i);
          break;
        }
      }
    }
    if (zero_target < 0) {
      GridInterval iv;
      iv.kind = IntervalKind::Finite;
      iv.lower = 0.0;
      iv.upper = 0.0;
      iv.lower_inclusive = true;
      iv.upper_inclusive = true;
      iv.is_edge = false;
      size_t insert_at = parts.size();
      for (size_t i = 0; i < parts.size(); ++i) {
        if (parts[i].iv.kind == IntervalKind::PosInf) {
          insert_at = i;
          break;
        }
        if (parts[i].iv.kind == IntervalKind::Finite &&
            parts[i].iv.lower >= 0.0) {
          insert_at = i;
          break;
        }
      }
      parts.insert(parts.begin() + static_cast<std::ptrdiff_t>(insert_at),
                   {iv, -1, -1});
      zero_target = static_cast<int>(insert_at);
    }
  }

  // Finalize: assign partition indices and build the lookup tables.
  m.intervals.reserve(parts.size());
  for (size_t i = 0; i < parts.size(); ++i) {
    m.intervals.push_back(parts[i].iv);
    if (parts[i].iv.kind == IntervalKind::NegInf) {
      m.neg_inf_partition = static_cast<int>(i);
    } else if (parts[i].iv.kind == IntervalKind::PosInf) {
      m.pos_inf_partition = static_cast<int>(i);
    }
    if (parts[i].pos_lo >= 0) {
      for (int p = parts[i].pos_lo; p <= parts[i].pos_hi; ++p) {
        m.finite_lut[bt.sorted_to_id[p]] = static_cast<int>(i);
      }
    }
  }
  m.zero_partition = zero_target;
  return m;
}

int partitionIndex(const AxisModel &model, const AxisBin &ax) {
  switch (ax.kind) {
  case AxisBinKind::Finite:
    return model.finite_lut[ax.internal12];
  case AxisBinKind::NegInf:
    return model.neg_inf_partition;
  case AxisBinKind::PosInf:
    return model.pos_inf_partition;
  case AxisBinKind::Zero:
    return model.zero_partition;
  }
  return -1;
}

} // namespace

// ---------------------------------------------------------------------------
// GridResult helpers.
// ---------------------------------------------------------------------------
std::vector<std::size_t> GridResult::shape() const {
  std::vector<std::size_t> s;
  s.reserve(axis_intervals.size());
  for (const auto &axis : axis_intervals) {
    s.push_back(axis.size());
  }
  return s;
}

std::vector<GridCell> GridResult::materializeRows() const {
  std::vector<GridCell> rows;
  rows.reserve(counts.size());
  std::vector<std::size_t> n = shape();
  for (std::size_t idx = 0; idx < counts.size(); ++idx) {
    GridCell cell;
    cell.count = counts[idx];
    cell.bounds.resize(n.size());
    std::size_t rem = idx;
    for (std::size_t d = n.size(); d-- > 0;) {
      std::size_t p = (n[d] == 0) ? 0 : rem % n[d];
      rem = (n[d] == 0) ? 0 : rem / n[d];
      cell.bounds[d] = axis_intervals[d][p];
    }
    rows.push_back(std::move(cell));
  }
  return rows;
}

// ---------------------------------------------------------------------------
// GridQuery.
// ---------------------------------------------------------------------------
GridQuery::GridQuery(std::span<const char> buffer) {
  AirTreeReader reader;
  reader.read(buffer);
  dims_ = reader.getDims();
  bit_length_ = reader.getBitLength();
  trie_node_ = reader.getType();
  header_ = reader.getHeader();
  extent_ = std::make_shared<ExtentCache>();
}

void GridQuery::ensureExtent() const {
  std::call_once(extent_->once, [&]() {
    extent_->min.fill(kPosInf);
    extent_->max.fill(kNegInf);
    extent_->has.fill(false);
    if (!((dims_ == 2 || dims_ == 3 || dims_ == 4) &&
          bit_length_ == kBitLength)) {
      return;
    }
    const BinTable &bt = binTable();
    auto extentCb = [&](const std::array<AxisBin, 4> &ab, int d, uint64_t) {
      for (int i = 0; i < d; ++i) {
        const AxisBin &a = ab[static_cast<size_t>(i)];
        double v;
        if (a.kind == AxisBinKind::Finite) {
          v = a.value;
        } else if (a.kind == AxisBinKind::Zero) {
          v = 0.0;
        } else {
          continue; // infinities do not contribute to the finite extent
        }
        size_t s = static_cast<size_t>(i);
        extent_->has[s] = true;
        extent_->min[s] = std::min(extent_->min[s], v);
        extent_->max[s] = std::max(extent_->max[s], v);
      }
    };
    dispatchVisit(dims_, trie_node_, bt, extentCb);
  });
}

GridResult GridQuery::getGrid(const std::vector<GridAxisSpec> &axes,
                              uint64_t max_cells) const {
  if (!((dims_ == 2 || dims_ == 3 || dims_ == 4) && bit_length_ == kBitLength)) {
    throw std::runtime_error("Unsupported dimensions or bit length.");
  }
  if (static_cast<int>(axes.size()) != dims_) {
    throw std::invalid_argument(
        "number of axes must match the buffer's dimensionality");
  }
  if (max_cells == 0) {
    throw std::invalid_argument("max_cells must be greater than 0");
  }
  bool needs_extent = false;
  for (const auto &a : axes) {
    if (a.steps == 0) {
      throw std::invalid_argument("steps must be greater than 0");
    }
    if (std::isnan(a.min) || std::isnan(a.max)) {
      throw std::invalid_argument("min and max must not be NaN");
    }
    if (a.min > a.max) {
      throw std::invalid_argument("min must be less than or equal to max");
    }
    if (a.scaling == GridScaling::Multiplicative && !std::isinf(a.min) &&
        !std::isinf(a.max)) {
      if (!(a.min > 0.0 && a.max > a.min)) {
        throw std::invalid_argument(
            "multiplicative scaling requires a strictly positive interior with "
            "end greater than start");
      }
    }
    if ((std::isinf(a.min) && a.min < 0.0) ||
        (std::isinf(a.max) && a.max > 0.0)) {
      needs_extent = true;
    }
  }

  // Only resolving an infinite endpoint needs the data extent; finite-only
  // queries never walk the trie for it.
  if (needs_extent) {
    ensureExtent();
  }

  const BinTable &bt = binTable();

  std::vector<AxisModel> models;
  models.reserve(static_cast<size_t>(dims_));
  for (int d = 0; d < dims_; ++d) {
    size_t s = static_cast<size_t>(d);
    const GridAxisSpec &a = axes[s];
    Extent ext;
    // The cache is only read for axes with an infinite endpoint. Those queries
    // ran ensureExtent() above (so the read synchronizes via call_once); a
    // finite axis never touches the cache, and buildAxisModel ignores ext for
    // finite endpoints anyway.
    bool axis_inf = (std::isinf(a.min) && a.min < 0.0) ||
                    (std::isinf(a.max) && a.max > 0.0);
    if (axis_inf) {
      ext.min = extent_->min[s];
      ext.max = extent_->max[s];
      ext.has = extent_->has[s];
    }
    models.push_back(buildAxisModel(a, bt, ext));
  }

  // Project the cell count with overflow-safe multiplication before allocating.
  GridResult result;
  result.dims = static_cast<uint16_t>(dims_);
  result.axis_intervals.resize(static_cast<size_t>(dims_));
  std::vector<uint64_t> radix(static_cast<size_t>(dims_));
  uint64_t cells = 1;
  bool empty = false;
  for (int d = 0; d < dims_; ++d) {
    uint64_t n = models[static_cast<size_t>(d)].intervals.size();
    result.axis_intervals[static_cast<size_t>(d)] =
        models[static_cast<size_t>(d)].intervals;
    radix[static_cast<size_t>(d)] = n;
    if (n == 0) {
      empty = true;
      continue;
    }
    if (cells > max_cells / n) {
      throw std::runtime_error("grid cell count exceeds max_cells");
    }
    cells *= n;
  }

  if (empty) {
    result.counts.clear();
    return result;
  }

  std::vector<uint64_t> counts(static_cast<size_t>(cells), 0);
  auto bucketCb = [&](const std::array<AxisBin, 4> &ab, int d, uint64_t cnt) {
    uint64_t idx = 0;
    for (int i = 0; i < d; ++i) {
      int p = partitionIndex(models[static_cast<size_t>(i)],
                             ab[static_cast<size_t>(i)]);
      if (p < 0) {
        return;
      }
      idx = idx * radix[static_cast<size_t>(i)] + static_cast<uint64_t>(p);
    }
    counts[idx] += cnt;
  };
  dispatchVisit(dims_, trie_node_, bt, bucketCb);

  result.counts = std::move(counts);
  return result;
}

} // namespace airtree::query::grid
