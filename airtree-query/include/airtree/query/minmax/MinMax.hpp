// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_MINMAX_MINMAX_HPP
#define AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_MINMAX_MINMAX_HPP

#include <airtree/query/meta/PopulatedBins.hpp>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

#include <airtree/query/meta/Histogram.hpp>

namespace airtree::query::minmax {


class MinMaxResult {
public:
  MinMaxResult(double lowerBound, double upperBound, uint32_t bin_count);

  double getLowerBound() const;
  double getUpperBound() const;
  uint32_t getBinCount() const;

  void setLowerBound(double lowerBound);
  void setUpperBound(double upperBound);
  void setBinCount(uint32_t bin_count);

private:
  double lowerBound_;
  double upperBound_;
  uint32_t bin_count_;
};

using MinMaxResultVector = std::vector<MinMaxResult>;

/**
 * Class to calculate min and max values from a trie structure.
 * The trie is expected to be serialized in a specific format. Currently
 * supported formats are 1DxT, 1DxF, and 1DxP. The class must be initialized
 * with a valid buffer containing the serialized trie data. Unsupported trie
 * configurations will throw an error.
 */
class MinMax {
public:
  MinMax(std::vector<char> buffer);
  /**
   * Get the min value from the trie.
   * @return The calculated min value. Returns -inf if the min cannot be
   * calculated. Should not occur unless the trie is empty.
   */
  MinMaxResultVector getMin();

  /**
   * Get the max value from the trie.
   * @return The calculated max value. Returns inf if the max cannot be
   * calculated. Should not occur unless the trie is empty.
   */
  MinMaxResultVector getMax();

  /**
   * Get the smallest value that is set in the histogram.
   * @return The calculated min value. Returns -inf if the min cannot be
   * calculated. Should not occur unless the histogram is empty.
   */
  MinMaxResultVector getMinValue();

  /**
   * Get the largest value that is set in the histogram.
   * @return The calculated max value. Returns inf if the max cannot be
   * calculated. Should not occur unless the histogram is empty.
   */
  MinMaxResultVector getMaxValue();

private:
  template <typename NodeType> MinMaxResultVector calculateMin();
  template <typename NodeType> MinMaxResultVector calculateMax();

  template <typename NodeType> MinMaxResultVector calculateMinValue();
  template <typename NodeType> MinMaxResultVector calculateMaxValue();

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

} // namespace airtree::query::minmax

#endif // AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_MINMAX_MINMAX_HPP
