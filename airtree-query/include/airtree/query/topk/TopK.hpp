// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_TOPK_TOPK_HPP
#define AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_TOPK_TOPK_HPP

#include <airtree/core/common/AirTreeType.hpp>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

#include <airtree/query/meta/Histogram.hpp>

namespace airtree::query::topk {

class TopKResult {
public:
  TopKResult(double lower_bound, double upper_bound, uint32_t count,
             uint32_t internal_rep);

  double getLowerBound() const;
  double getUpperBound() const;
  uint32_t getCount() const;
  uint32_t getInternalRepresentation() const;

  void setLowerBound(double lower_bound);
  void setUpperBound(double upper_bound);
  void setCount(uint32_t count);
  void setInternalRepresentation(uint32_t internal_rep);

private:
  double lower_bound_;
  double upper_bound_;
  uint32_t count_;
  uint32_t internal_rep_;
};

using TopKResultVector = std::vector<TopKResult>;

/**
 * Class to calculate top-k elements from a trie structure where k is the
 * percentage of the total elements.
 * The trie is expected to be serialized in a specific format. Currently
 * supported formats are 1DxT, 1DxF, and 1DxP. The class must be initialized
 * with a valid buffer containing the serialized trie data. Unsupported trie
 * configurations will throw an error.
 */
class TopK {
public:
  TopK(std::vector<char> buffer);

  /**
   * Get the top-k elements from the trie.
   * @param k The percentage of the total elements to retrieve as top-k.
   * @return A vector containing the top-k elements.
   */
  TopKResultVector getTopK(double k);

private:
  template <typename NodeType> TopKResultVector fetchTopK(double k);

  uint16_t dims_;
  uint16_t bit_length_;
  uint64_t bin_count_;
  std::vector<char> buffer_;
  AirTreeType trie_node_;
  std::shared_ptr<airtree::query::meta::Histogram> histogram_;
  airtree::core::common::AirTreeHeader header_;
};

} // namespace airtree::query::topk

#endif // AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_TOPK_TOPK_HPP
