#include <vector>

#include <airtree/merge/trie2d/Merge2DxP.hpp>
#include <airtree/merge/MergeStrategy.hpp>
#include <airtree/merge/Logger.hpp>

using namespace airtree::merge;
using namespace airtree::merge::dim2;

std::vector<char> Merge2DxP::merge(const std::vector<char> &buffer1,
                                   const std::vector<char> &buffer2) {
  size_t offset1 = 0, offset2 = 0;
  std::vector<char> mergedBuffer;

  // Merge headers.
  trie_header header1 = deserializeTrieHeader(buffer1, offset1);
  trie_header header2 = deserializeTrieHeader(buffer2, offset2);
  trie_header mergedHeader = mergeHeaders(header1, header2);
  serializeTrieHeader(mergedHeader, mergedBuffer);
  SPDLOG_LOGGER_INFO(logger(), "Merged headers successfully");

  // Deserialize the root nodes.
  std::bitset<BINS_64> pop0_1, pop0_2;
  auto mergedRoot = Root_merge<TLEoption3_2D, BINS_64>(
      buffer1, buffer2, offset1, offset2, pop0_1, pop0_2);
  serialize_2DxP(mergedRoot.get(), mergedBuffer, false);
  mergedRoot.reset();

  // Merge level1 children based on the original populated flags.
  std::unique_ptr<TrieNode_2D_10> temp_node_level1;
  std::unique_ptr<TrieNode_2D_10_Level1> temp_node_level2;
  std::bitset<BINS_1024> pop0_level1;
  int nDims = 0;
  for (size_t i = 0; i < BINS_64; i++) {
    // DEBUG_PRINT("Merging 2DxP TLEoption3_2D index " << i);
    nDims = getNumDims2D(i);
    SPDLOG_LOGGER_INFO(logger(), "Number of dimensions: {}", nDims);
    if (nDims == 0) {
      continue;
    }
    if (pop0_1.test(i) && !pop0_2.test(i)) {
      SPDLOG_LOGGER_INFO(logger(), "Only buffer1 has a node at index {}", i);
      temp_node_level1 = deserialize_2DxP_l0(buffer1, offset1, nDims, false);
      serialize_2DxP_l0(temp_node_level1.get(), mergedBuffer, false);
      pop0_level1 = temp_node_level1->populated;
      temp_node_level1.reset();
      if (nDims == 1 || nDims == 2)
        continue;
      for (size_t j = 0; j < BINS_1024; j++) {
        if (pop0_level1.test(j)) {
          temp_node_level2 = deserialize_2DxP_l1(buffer1, offset1);
          serialize_2DxP_l1(temp_node_level2.get(), mergedBuffer);
          temp_node_level2.reset();
        }
      }
    } else if (pop0_2.test(i) && !pop0_1.test(i)) {
      // Similar check for buffer2.
      SPDLOG_LOGGER_INFO(logger(), "Only buffer2 has a node at index {}", i);
      temp_node_level1 = deserialize_2DxP_l0(buffer2, offset2, nDims, false);
      serialize_2DxP_l0(temp_node_level1.get(), mergedBuffer, false);
      pop0_level1 = temp_node_level1->populated;
      temp_node_level1.reset();
      if (nDims == 1 || nDims == 2)
        continue;
      for (size_t j = 0; j < BINS_1024; j++) {
        if (pop0_level1.test(j)) {
          temp_node_level2 = deserialize_2DxP_l1(buffer2, offset2);
          serialize_2DxP_l1(temp_node_level2.get(), mergedBuffer);
          temp_node_level2.reset();
        }
      }
    } else if (pop0_1.test(i) && pop0_2.test(i)) {
      if (offset1 < buffer1.size() && offset2 < buffer2.size()) {
        SPDLOG_LOGGER_INFO(logger(), "Both buffers have a node at index {}", i);
        auto node1_level1 = deserialize_2DxP_l0(buffer1, offset1, nDims, false);
        auto node2_level1 = deserialize_2DxP_l0(buffer2, offset2, nDims, false);
        auto pop1_level1 = node1_level1->populated;
        auto pop2_level1 = node2_level1->populated;
        temp_node_level1 =
            mergeTrieNode2D10(std::move(node1_level1), std::move(node2_level1));
        serialize_2DxP_l0(temp_node_level1.get(), mergedBuffer, false);
        if (nDims == 1 || nDims == 2)
          continue;
        for (size_t j = 0; j < BINS_1024; j++) {
          if (pop1_level1.test(j) && !pop2_level1.test(j)) {
            temp_node_level2 = deserialize_2DxP_l1(buffer1, offset1);
            serialize_2DxP_l1(temp_node_level2.get(), mergedBuffer);
            temp_node_level2.reset();
          } else if (pop2_level1.test(j) && !pop1_level1.test(j)) {
            temp_node_level2 = deserialize_2DxP_l1(buffer2, offset2);
            serialize_2DxP_l1(temp_node_level2.get(), mergedBuffer);
            temp_node_level2.reset();
          } else if (pop1_level1.test(j) && pop2_level1.test(j)) {
            temp_node_level2 = mergeTrieNode2D10_Level1(
                std::move(deserialize_2DxP_l1(buffer1, offset1)),
                std::move(deserialize_2DxP_l1(buffer2, offset2)));
            serialize_2DxP_l1(temp_node_level2.get(), mergedBuffer);
            temp_node_level2.reset();
          }
        }
      }
    }
  }
  add_EOF(mergedBuffer);
  SPDLOG_LOGGER_INFO(
      logger(), "Serialized merged trie (all levels) successfully");
  return mergedBuffer;
}

std::unique_ptr<TrieNode_2D_10>
Merge2DxP::mergeTrieNode2D10(std::unique_ptr<TrieNode_2D_10> node1,
                             std::unique_ptr<TrieNode_2D_10> node2) {
  if (!node1)
    return node2;
  if (!node2)
    return node1;
  for (size_t i = 0; i < BINS_1024; ++i) {
    node1->counts[i] += node2->counts[i];
    if (node2->populated.test(i)) {
      if (!node1->populated.test(i)) {
        node1->populated.set(i);
        node1->nodes[i] = std::move(node2->nodes[i]);
      } else {
        node1->nodes[i] = mergeTrieNode2D10_Level1(
            std::move(node1->nodes[i]), std::move(node2->nodes[i]));
      }
    }
  }
  return node1;
}

std::unique_ptr<TrieNode_2D_10_Level1> Merge2DxP::mergeTrieNode2D10_Level1(
    std::unique_ptr<TrieNode_2D_10_Level1> node1,
    std::unique_ptr<TrieNode_2D_10_Level1> node2) {
  if (!node1)
    return node2;
  if (!node2)
    return node1;
  for (size_t i = 0; i < BINS_1024; ++i) {
    node1->counts[i] += node2->counts[i];
  }
  return node1;
}
