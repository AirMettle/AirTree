#ifndef AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_CDF__CDF_HPP
#define AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_CDF__CDF_HPP

#include <cstdint>
#include <airtree/core/AirTreeCore_internal.hpp>
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
  std::vector<char> buffer_;
  AirTreeType trie_node_;
  std::shared_ptr<airtree::query::meta::Histogram> histogram_;
  trie_header header_;
};

} // namespace airtree::query::cdf

#endif // AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_CDF__CDF_HPP
