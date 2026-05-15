// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/serdes/ND.hpp>
#include <airtree/core/serdes/BooleanArray.hpp>
#include <airtree/core/serdes/Count.hpp>
#include <airtree/core/serdes/trie1d/1DxF.hpp>


void serializeTrieNode_16_ND(const TrieNode_16 *node, std::vector<char> &buffer,
                             bool recursively) {
  // Convert populated bitset to compact BooleanArray
  BooleanArray compact_array = BooleanArray(BINS_256 / 64);
  for (size_t i = 0; i < BINS_256; i++) {
    if (node->populated[i]) {
      compact_array.set(i, true);
    }
  }

  // Store compact array to buffer
  auto compact_array_buffer = serializeCompactBooleanArray(compact_array);
  buffer.insert(
      buffer.end(), compact_array_buffer.begin(), compact_array_buffer.end());

  // Store counts
  auto counts_buffer = serializeCounts(node->counts, BINS_256);
  buffer.insert(buffer.end(), counts_buffer.begin(), counts_buffer.end());

  if (!recursively)
    return;

  // Recursively serialize child nodes
  for (size_t i = 0; i < BINS_256; i++) {
    if (node->populated[i] && node->nodes[i]) {
      serialize_1DxF_l1(node->nodes[i].get(), buffer);
    }
  }
}