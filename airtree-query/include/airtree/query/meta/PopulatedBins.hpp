// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_META_POPULATEDBINS_HPP
#define AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_META_POPULATEDBINS_HPP

#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/query/meta/Histogram.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <type_traits>
#include <vector>

namespace airtree::query::meta {

struct PopulatedBin {
  size_t position; // index in the value-sorted bin table
  uint64_t code;
  uint32_t count;
};

// LSD radix sort on position (11-bit digits); positions are < binCount.
inline void sortByPosition(std::vector<PopulatedBin> &bins, uint64_t binCount) {
  constexpr unsigned kDigit = 11;
  constexpr size_t kBuckets = 1u << kDigit;
  std::vector<PopulatedBin> tmp(bins.size());
  std::vector<uint32_t> offsets(kBuckets + 1);
  for (unsigned shift = 0; (binCount - 1) >> shift; shift += kDigit) {
    std::fill(offsets.begin(), offsets.end(), 0);
    for (const auto &b : bins) ++offsets[((b.position >> shift) & (kBuckets - 1)) + 1];
    for (size_t i = 1; i <= kBuckets; ++i) offsets[i] += offsets[i - 1];
    for (const auto &b : bins) tmp[offsets[(b.position >> shift) & (kBuckets - 1)]++] = b;
    bins.swap(tmp);
  }
}

// Populated leaf bins of a 1D trie, sorted by value.
template <typename NodeType>
std::vector<PopulatedBin> populatedBins(const std::unique_ptr<NodeType> &root,
                                        const Histogram &histogram) {
  std::vector<PopulatedBin> out;
  if (!root) {
    return out;
  }
  auto push = [&](uint64_t code, uint32_t count) {
    if (count > 0) {
      out.push_back({histogram.positionOf(code), code, count});
    }
  };
  if constexpr (std::is_same_v<NodeType, TrieNode_13>) {
    for (size_t l0 = 0; l0 < BINS_256; ++l0) {
      if (!root->populated.test(l0) || !root->nodes[l0]) continue;
      for (size_t l1 = 0; l1 < BINS_32; ++l1) push((l0 << 5) | l1, root->nodes[l0]->counts[l1]);
    }
  } else if constexpr (std::is_same_v<NodeType, TrieNode_16>) {
    for (size_t l0 = 0; l0 < BINS_256; ++l0) {
      if (!root->populated.test(l0) || !root->nodes[l0]) continue;
      for (size_t l1 = 0; l1 < BINS_256; ++l1) push((l0 << 8) | l1, root->nodes[l0]->counts[l1]);
    }
  } else if constexpr (std::is_same_v<NodeType, TrieNode_20>) {
    for (size_t l0 = 0; l0 < BINS_256; ++l0) {
      if (!root->populated.test(l0) || !root->nodes[l0]) continue;
      const auto &n1 = root->nodes[l0];
      for (size_t l1 = 0; l1 < BINS_64; ++l1) {
        if (!n1->populated.test(l1) || !n1->nodes[l1]) continue;
        for (size_t l2 = 0; l2 < BINS_64; ++l2)
          push((l0 << 12) | (l1 << 6) | l2, n1->nodes[l1]->counts[l2]);
      }
    }
  } else {
    static_assert(sizeof(NodeType) == 0, "unsupported 1D node type");
  }
  sortByPosition(out, histogram.getBinCount());
  return out;
}

// The populated leaf bins of a serialized 1D trie, read straight from the bytes — no nodes are
// built — sorted by value. trieCount is the sum of the root counts (finite, non-zero observations).
// Throws std::runtime_error on a truncated buffer or a configuration that is not 1D.
struct PopulatedBinSet {
  std::vector<PopulatedBin> bins;
  uint32_t trieCount = 0;
};
PopulatedBinSet populatedBins(std::span<const char> buffer,
                              const airtree::core::common::AirTreeHeader &header,
                              const Histogram &histogram);

} // namespace airtree::query::meta

#endif // AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_META_POPULATEDBINS_HPP
