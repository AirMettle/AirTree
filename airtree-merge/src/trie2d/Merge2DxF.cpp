#include <vector>

#include <airtree/merge/trie2d/Merge2DxF.hpp>
#include <airtree/merge/MergeStrategy.hpp>
#include <airtree/merge/Logger.hpp>

using namespace airtree::merge;
using namespace airtree::merge::dim2;

std::vector<char> Merge2DxF::merge(const std::vector<char> &buffer1,
                                   const std::vector<char> &buffer2) {
  size_t offset1 = 0, offset2 = 0;
  std::vector<char> mergedBuffer;

  // Merge headers.
  trie_header header1 = deserializeTrieHeader(buffer1, offset1);
  trie_header header2 = deserializeTrieHeader(buffer2, offset2);
  trie_header mergedHeader = mergeHeaders(header1, header2);
  serializeTrieHeader(mergedHeader, mergedBuffer);

  // Deserialize the root nodes.
  std::bitset<BINS_64> pop0_1, pop0_2;
  auto mergedRoot = Root_merge_TLE<TLETrieNode_2D, BINS_64>(
      buffer1, buffer2, offset1, offset2, pop0_1, pop0_2);

  serialize_2DxF(mergedRoot.get(), mergedBuffer, false);
  mergedRoot.reset();
  int level = 0;
  int nDims = 0;
  for (size_t i = 0; i < BINS_64; i++) {
    nDims = getNumDims2D(i);
    if (nDims == 0)
      continue;
    if (nDims == 1)
      level = 1;
    else
      level = nDims - 1;
    if (pop0_1.test(i) && !pop0_2.test(i)) {
      auto temp_node_level1 =
          deserialize_2DxF_l0(buffer1, offset1, level, false);
      serializeTrieNode_16_ND(temp_node_level1.get(), mergedBuffer);
      auto pop0_level1 = temp_node_level1->populated;
      temp_node_level1.reset();
      if (level == 1)
        continue;
      for (size_t j = 0; j < BINS_256; j++) {
        if (pop0_level1.test(j)) {
          auto temp_node_level2 = deserialize_2DxF_l1(buffer1, offset1, level);
          serialize_1DxF_l1(temp_node_level2.get(), mergedBuffer);
          temp_node_level2.reset();
          break;
        }
      }
    } else if (pop0_2.test(i) && !pop0_1.test(i)) {
      auto temp_node_level1 =
          deserialize_2DxF_l0(buffer2, offset2, level, false);
      serializeTrieNode_16_ND(temp_node_level1.get(), mergedBuffer);
      auto pop0_level1 = temp_node_level1->populated;
      temp_node_level1.reset();
      if (level == 1)
        continue;
      for (size_t j = 0; j < BINS_256; j++) {
        if (pop0_level1.test(j)) {
          auto temp_node_level2 = deserialize_2DxF_l1(buffer2, offset2, level);
          serialize_1DxF_l1(temp_node_level2.get(), mergedBuffer);
          temp_node_level2.reset();
        }
      }
    } else if (pop0_1.test(i) && pop0_2.test(i)) {
      auto node1_level1 = deserialize_2DxF_l0(buffer1, offset1, level, false);
      auto node2_level1 = deserialize_2DxF_l0(buffer2, offset2, level, false);
      auto pop1_level1 = node1_level1->populated;
      auto pop2_level1 = node2_level1->populated;
      auto temp_node_level1 =
          mergeTrieNode16(std::move(node1_level1), std::move(node2_level1));
      serializeTrieNode_16_ND(temp_node_level1.get(), mergedBuffer);
      if (level == 1)
        continue;
      for (size_t j = 0; j < BINS_256; j++) {
        if (pop1_level1.test(j) && !pop2_level1.test(j)) {
          auto temp_node_level2 = deserialize_2DxF_l1(buffer1, offset1, level);
          serialize_1DxF_l1(temp_node_level2.get(), mergedBuffer);
          temp_node_level2.reset();
        } else if (pop2_level1.test(j) && !pop1_level1.test(j)) {
          auto temp_node_level2 = deserialize_2DxF_l1(buffer2, offset2, level);
          serialize_1DxF_l1(temp_node_level2.get(), mergedBuffer);
          temp_node_level2.reset();
        } else if (pop1_level1.test(j) && pop2_level1.test(j)) {
          auto temp_node_level2 = mergeTrieNode16Level1(
              std::move(deserialize_2DxF_l1(buffer1, offset1, level)),
              std::move(deserialize_2DxF_l1(buffer2, offset2, level)));
          serialize_1DxF_l1(temp_node_level2.get(), mergedBuffer);
          temp_node_level2.reset();
        }
      }
    }
  }
  add_EOF(mergedBuffer);
  SPDLOG_LOGGER_INFO(
      logger(), "Serialized merged trie (all levels) successfully");
  return mergedBuffer;
}
