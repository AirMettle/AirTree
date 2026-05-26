// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_GRID_GRIDQUERY_HPP
#define AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_GRID_GRIDQUERY_HPP

#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/common/AirTreeType.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace airtree::query::grid {

// Lazily-computed per-axis data extent cache; defined in the implementation.
struct ExtentCache;

/**
 * How the requested range on an axis is divided into steps.
 *  - Linear:        equal-width steps.
 *  - Multiplicative: geometric (log-spaced) steps. Only valid when the resolved
 *                    finite interior of the axis is strictly positive.
 */
enum class GridScaling { Linear, Multiplicative };

/**
 * Kind of an output partition on an axis. Zero is intentionally absent: a zero
 * value is routed into the finite partition that contains it. NaN is excluded
 * from the grid entirely.
 */
enum class IntervalKind { Finite, NegInf, PosInf };

/**
 * One axis of a grid query: the requested range, the number of partitions to
 * split it into, and the scaling. min may be -inf and max may be +inf to mean
 * "start/end at the data extent" while also surfacing the infinity as its own
 * entry.
 */
struct GridAxisSpec {
  double      min;
  double      max;
  uint32_t    steps;
  GridScaling scaling = GridScaling::Linear;
};

/**
 * One partition on one axis. Bounds use the histogram's native inclusivity:
 * positive bins are lower-inclusive ([lower, upper)), negative bins are
 * upper-inclusive ((lower, upper]). is_edge marks a single bin that straddles
 * the requested min or max; the caller decides how to weigh it.
 */
struct GridInterval {
  IntervalKind kind = IntervalKind::Finite;
  double       lower = 0.0;
  double       upper = 0.0;
  bool         lower_inclusive = true;
  bool         upper_inclusive = false;
  bool         is_edge = false;
};

/**
 * One materialized grid cell: one interval per dimension plus its count.
 */
struct GridCell {
  std::vector<GridInterval> bounds;
  uint64_t                  count = 0;
};

/**
 * Result of a grid query: the per-axis partitions and a flat, row-major
 * (mixed-radix) array of cell counts. counts.size() equals the product of the
 * per-axis partition counts.
 */
struct GridResult {
  uint16_t                               dims = 0;
  std::vector<std::vector<GridInterval>> axis_intervals;
  std::vector<uint64_t>                  counts;

  /// Number of partitions on each axis.
  std::vector<std::size_t> shape() const;

  /// Expand the flat count array into one row per grid cell (including empty
  /// cells). Convenient for printing or feeding a table.
  std::vector<GridCell> materializeRows() const;
};

/**
 * Grid query over a multi-dimensional Precise histogram buffer (2DxP, 3DxP, or
 * 4DxP). Subdivides a requested per-axis range into non-overlapping partitions
 * aligned to internal bin edges and returns a count per grid cell. Other
 * configurations throw.
 */
class GridQuery {
public:
  explicit GridQuery(std::vector<char> buffer);

  /**
   * Run the grid query. axes.size() must equal the buffer's dimensionality.
   * Throws std::invalid_argument on malformed input and std::runtime_error if
   * the buffer is unsupported or the projected cell count exceeds max_cells.
   */
  GridResult getGrid(const std::vector<GridAxisSpec> &axes,
                     uint64_t max_cells = 1000000) const;

private:
  // Computes the per-axis marginal data extent over finite-like values exactly
  // once (only when a query actually has an infinite endpoint) and caches it, so
  // repeated queries never re-walk the trie and finite-only queries never walk it
  // at all.
  void ensureExtent() const;

  int                                 dims_ = 0;
  int                                 bit_length_ = 0;
  airtree::core::common::AirTreeType  trie_node_;
  airtree::core::common::AirTreeHeader header_;

  std::shared_ptr<ExtentCache> extent_;
};

} // namespace airtree::query::grid

#endif // AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_GRID_GRIDQUERY_HPP
