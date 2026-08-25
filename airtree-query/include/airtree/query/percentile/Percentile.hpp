// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_PERCENTILE_PERCENTILE_HPP
#define AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_PERCENTILE_PERCENTILE_HPP

#include <airtree/query/meta/PopulatedBins.hpp>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

#include <airtree/query/meta/Histogram.hpp>

namespace airtree::query::percentile {

/**
 * Class to calculate percentiles from a trie structure.
 * The trie is expected to be serialized in a specific format. Currently
 * supported formats are 1DxT, 1DxF, and 1DxP. The class must be initialized
 * with a valid buffer containing the serialized trie data. Unsupported trie
 * configurations will throw an error.
 */
// The quantile and the native bin it fell in: [lower, upper) for non-negative bins, (lower, upper]
// for negative ones; lower == upper == value for -inf, ±0 and +inf. The interpolated value can equal
// upper when the rank lands exactly on the bin's last observation (always at percentile 100).
struct PercentileResult {
  double value;
  double lower_bound;
  double upper_bound;
};

class Percentile {
public:
  Percentile(std::vector<char> buffer);
  /**
   * Get the percentile value from the trie.
   * @param percentile The percentile to calculate (0-100).
   * @return The calculated percentile value. Returns -inf if the percentile
   * cannot be calculated. Should not occur unless the trie is empty.
   */
  double getPercentile(double percentile);
  PercentileResult getPercentileWithBounds(double percentile);

private:
  template <typename NodeType> PercentileResult calculatePercentile(double percentile);

  uint16_t dims_;
  uint16_t bit_length_;
  uint64_t bin_count_;
  std::vector<char> buffer_;
  AirTreeType trie_node_;
  std::shared_ptr<airtree::query::meta::Histogram> histogram_;
  airtree::core::common::AirTreeHeader header_;
  std::vector<airtree::query::meta::PopulatedBin> populated_;
  bool populated_ready_ = false;
  template <typename NodeType> const std::vector<airtree::query::meta::PopulatedBin> &populatedBins();
};

} // namespace airtree::query::percentile

#endif // AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_PERCENTILE_PERCENTILE_HPP
