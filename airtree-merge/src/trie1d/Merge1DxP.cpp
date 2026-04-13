#include <vector>

#include <airtree/merge/trie1d/Merge1DxP.hpp>
#include <airtree/merge/MergeStrategy.hpp>
#include <airtree/merge/Logger.hpp>

using namespace airtree::merge;
using namespace airtree::merge::dim1;

std::vector<char> Merge1DxP::merge(const std::vector<char> &buffer1,
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
  std::bitset<BINS_256> pop0_1, pop0_2;
  auto mergedRoot = Root_merge<TrieNode_20, BINS_256>(
      buffer1, buffer2, offset1, offset2, pop0_1, pop0_2);
  serialize_1DxP(mergedRoot.get(), mergedBuffer, false);
  mergedRoot.reset();

  // For each bucket at Level0 where mergedRoot->populated is true,
  // merge Level1 nodes from the original buffers.
  std::unique_ptr<TrieNode_20_Level1> temp_node_level1;
  std::unique_ptr<TrieNode_20_Level2> temp_node_level2;
  for (size_t i = 0; i < BINS_256; i++) {
    if (pop0_1.test(i) && !pop0_2.test(i)) {
      // Only buffer1 has a node at this index.
      temp_node_level1 = deserialize_1DxP_l1(buffer1, offset1, false);
      serialize_1DxP_l1(temp_node_level1.get(), mergedBuffer, false);
      auto pop0_level1 = temp_node_level1->populated;
      temp_node_level1.reset();
      for (size_t j = 0; j < BINS_64; j++) {
        if (pop0_level1.test(j)) {
          SPDLOG_LOGGER_INFO(
              logger(), "Deserializing Trie20 Level2 index {} from buffer1", j);
          temp_node_level2 = deserialize_1DxP_l2(buffer1, offset1);
          serialize_1DxP_l2(temp_node_level2.get(), mergedBuffer);
          temp_node_level2.reset();
        }
      }
    } else if (pop0_2.test(i) && !pop0_1.test(i)) {
      // Only buffer2 has a node at this index.
      temp_node_level1 = deserialize_1DxP_l1(buffer2, offset2, false);
      serialize_1DxP_l1(temp_node_level1.get(), mergedBuffer, false);
      auto pop0_level1 = temp_node_level1->populated;
      temp_node_level1.reset();
      for (size_t j = 0; j < BINS_64; j++) {
        if (pop0_level1.test(j)) {
          SPDLOG_LOGGER_INFO(
              logger(), "Deserializing Trie20 Level2 index {} from buffer2", j);
          temp_node_level2 = deserialize_1DxP_l2(buffer2, offset2);
          serialize_1DxP_l2(temp_node_level2.get(), mergedBuffer);
          temp_node_level2.reset();
        }
      }
    } else if (pop0_1.test(i) && pop0_2.test(i)) {
      // Both buffers have a node; merge them.
      auto node1_level1 = deserialize_1DxP_l1(buffer1, offset1, false);
      auto node2_level1 = deserialize_1DxP_l1(buffer2, offset2, false);
      auto pop1_level1 = node1_level1->populated;
      auto pop2_level1 = node2_level1->populated;
      temp_node_level1 = mergeTrieNode20_Level1(
          std::move(node1_level1), std::move(node2_level1));
      serialize_1DxP_l1(temp_node_level1.get(), mergedBuffer, false);
      for (size_t j = 0; j < BINS_64; j++) {
        if (pop1_level1.test(j) && !pop2_level1.test(j)) {
          temp_node_level2 = deserialize_1DxP_l2(buffer1, offset1);
          serialize_1DxP_l2(temp_node_level2.get(), mergedBuffer);
          temp_node_level2.reset();
        } else if (pop2_level1.test(j) && !pop1_level1.test(j)) {
          temp_node_level2 = deserialize_1DxP_l2(buffer2, offset2);
          serialize_1DxP_l2(temp_node_level2.get(), mergedBuffer);
          temp_node_level2.reset();
        } else if (pop1_level1.test(j) && pop2_level1.test(j)) {
          temp_node_level2 = mergeTrieNode20_Level2(
              std::move(deserialize_1DxP_l2(buffer1, offset1)),
              std::move(deserialize_1DxP_l2(buffer2, offset2)));
          serialize_1DxP_l2(temp_node_level2.get(), mergedBuffer);
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

std::unique_ptr<TrieNode_20_Level2>
Merge1DxP::mergeTrieNode20_Level2(std::unique_ptr<TrieNode_20_Level2> node1,
                                  std::unique_ptr<TrieNode_20_Level2> node2) {
  SPDLOG_LOGGER_INFO(logger(), "Entering mergeTrieNode20_Level2");
  if (!node1)
    return node2;
  if (!node2)
    return node1;
  for (size_t i = 0; i < BINS_64; ++i) {
    SPDLOG_LOGGER_INFO(logger(), "Merging Trie20 Level2 index {}: {} + {}", i,
                       node1->counts[i], node2->counts[i]);
    node1->counts[i] += node2->counts[i];
  }
  return node1;
}

std::unique_ptr<TrieNode_20_Level1>
Merge1DxP::mergeTrieNode20_Level1(std::unique_ptr<TrieNode_20_Level1> node1,
                                  std::unique_ptr<TrieNode_20_Level1> node2) {
  SPDLOG_LOGGER_INFO(logger(), "Entering mergeTrieNode20_Level1");
  if (!node1)
    return node2;
  if (!node2)
    return node1;

  // Merge counts for Level1.
  for (size_t i = 0; i < BINS_64; ++i) {
    SPDLOG_LOGGER_INFO(logger(), "Merging Trie20 Level1 index {}: {} + {}", i,
                       node1->counts[i], node2->counts[i]);
    node1->counts[i] += node2->counts[i];

    // Merge populated flag (do not merge children here).
    if (node1->populated.test(i) || node2->populated.test(i)) {
      node1->populated.set(i);
    }
  }
  return node1;
}
