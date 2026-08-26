// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/serdes/ND.hpp>
#include <airtree/core/serdes/BooleanArray.hpp>
#include <airtree/core/serdes/Count.hpp>
#include <airtree/core/serdes/Node.hpp>
#include <airtree/core/serdes/trie1d/1DxF.hpp>


void serializeTrieNode_16_ND(const TrieNode_16 *node, std::vector<char> &buffer,
                             bool recursively) {
  // Convert populated bitset to compact BooleanArray
  uint64_t mask[(BINS_256 + 63) / 64];
  maskFromBitset(node->populated, mask);
  writeNode(mask, BINS_256, node->counts, buffer);

  if (!recursively)
    return;

  // Recursively serialize child nodes
  for (size_t i = 0; i < BINS_256; i++) {
    if (node->populated[i] && node->nodes[i]) {
      serialize_1DxF_l1(node->nodes[i].get(), buffer);
    }
  }
}