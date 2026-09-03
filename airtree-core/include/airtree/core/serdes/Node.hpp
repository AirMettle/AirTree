// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)
#ifndef AIRTREE_CORE_SERDES_NODE_HPP
#define AIRTREE_CORE_SERDES_NODE_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

// One trie node on the wire: its populated mask (64 bins per word, bit i = bin i), then the counts of
// the populated bins packed at the minimum width. Every serializer writes nodes through writeNode().

// Mask words from the counts themselves, for nodes that keep no bitset: bin i is populated when its count is non-zero.
inline void maskFromCounts(const uint32_t *counts, std::size_t bins, uint64_t *words) {
  for (std::size_t w = 0; w < (bins + 63) / 64; ++w) {
    const std::size_t base = w * 64;
    const std::size_t end = std::min(bins, base + 64);
    uint64_t m = 0;
    for (std::size_t i = base; i < end; ++i)
      m |= static_cast<uint64_t>(counts[i] != 0) << (i - base);
    words[w] = m;
  }
}

// Appends one node — mask, width byte, packed counts of the set bits — straight into `out`.
void writeNode(const uint64_t *mask, std::size_t bins, const uint32_t *counts,
               std::vector<char> &out);

// Appends the marker that closes a serialized trie (checked by verifyEndOfFileMarker).
void writeEndOfFileMarker(std::vector<char> &out);

#endif // AIRTREE_CORE_SERDES_NODE_HPP
