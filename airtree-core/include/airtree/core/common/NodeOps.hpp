// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_COMMON_NODEOPS_HPP
#define AIRTREE_CORE_COMMON_NODEOPS_HPP

#include <airtree/core/common/Populated.hpp>
#include <airtree/core/serdes/BooleanArray.hpp>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>

template <std::size_t N>
inline void bumpCount(PopulatedBins<N> &populated, uint32_t (&counts)[N], std::size_t slot) {
  if (counts[slot] == 0)
    populated.set(slot);
  counts[slot]++;
}

template <class Child, std::size_t N>
inline Child *descend(PopulatedBins<N> &populated, std::unique_ptr<Child> (&nodes)[N],
                      std::size_t slot, uint64_t &curr_trie_size) {
  std::unique_ptr<Child> &child = nodes[slot];
  if (!child) {
    child = std::make_unique<Child>();
    populated.set(slot);
    curr_trie_size += sizeof(Child);
  }
  return child.get();
}

// inserts bump only the leaf counter; this fills every parent count from its children
template <class Node> inline uint32_t rollUpNode(Node *node) {
  uint32_t total = 0;
  if constexpr (requires { node->nodes; }) {
    forEachSetBit(node->populated.words, std::size(node->counts), [&](std::size_t i) {
      if (node->nodes[i])
        node->counts[i] = rollUpNode(node->nodes[i].get());
      total += node->counts[i];
    });
  } else if constexpr (requires { node->populated; }) {
    forEachSetBit(node->populated.words, std::size(node->counts),
                  [&](std::size_t i) { total += node->counts[i]; });
  } else {
    for (uint32_t count : node->counts)
      total += count;
  }
  return total;
}

#endif // AIRTREE_CORE_COMMON_NODEOPS_HPP
