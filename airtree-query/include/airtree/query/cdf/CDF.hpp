// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_CDF__CDF_HPP
#define AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_CDF__CDF_HPP

#include <airtree/query/meta/PopulatedBins.hpp>
#include <cstdint>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/query/meta/Histogram.hpp>
#include <vector>

namespace airtree::query::cdf {

class CDF {
public:
  CDF(std::vector<char> &buffer);

  // Returns the cumulative distribution function (CDF) value for the histogram
  [[nodiscard]] double getCDF(double value, bool interpolate = false);

private:
  template <typename NodeType>
  double calculateCDF(double value, bool interpolate);

  uint16_t dims_;
  uint16_t bit_length_;
  uint64_t bin_count_;
  AirTreeType trie_node_;
  std::shared_ptr<airtree::query::meta::Histogram> histogram_;
  airtree::core::common::AirTreeHeader header_;
  std::vector<airtree::query::meta::PopulatedBin> populated_;
  bool populated_ready_ = false;
  template <typename NodeType> const std::vector<airtree::query::meta::PopulatedBin> &populatedBins();
};

} // namespace airtree::query::cdf

#endif // AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_CDF__CDF_HPP
