// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/serdes/ND.hpp>
#include <airtree/core/serdes/BooleanArray.hpp>
#include <airtree/core/serdes/Count.hpp>
#include <airtree/core/serdes/Node.hpp>
#include <airtree/core/serdes/trie1d/1DxF.hpp>


void serializeTrieNode_16_ND(const TrieNode_16 *node, std::vector<char> &buffer,
                             bool recursively) {
  writeNode(node->populated.words, BINS_256, node->counts, buffer);

  if (!recursively)
    return;

  forEachSetBit(node->populated.words, BINS_256, [&](size_t i) {
    if (node->nodes[i])
      serialize_1DxF_l1(node->nodes[i].get(), buffer);
  });
}