// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <vector>

#include <airtree/merge/trie1d/Merge1DxT.hpp>
#include <airtree/merge/Logger.hpp>

using namespace airtree::merge;
using namespace airtree::merge::dim1;
using namespace airtree::core;
using namespace airtree::core::common;

std::vector<char> Merge1DxT::merge(const std::vector<char> &buffer1,
                                   const std::vector<char> &buffer2) {
  auto header1 = airtree::core::common::deserializeHeader(buffer1);
  auto header2 = airtree::core::common::deserializeHeader(buffer2);
  size_t offset1 = header1.header_length;
  size_t offset2 = header2.header_length;
  auto mergedHeader = mergeHeaders(header1, header2);
  std::vector<char> mergedBuffer;
  airtree::core::common::serializeHeader(mergedHeader, mergedBuffer);
  size_t header_end = mergedBuffer.size();
  SPDLOG_LOGGER_INFO(airtree::merge::logger(), "Merged headers successfully");

  // Deserialize the root nodes.
  std::bitset<BINS_256> pop1, pop2;
  auto mergedRoot = Root_merge<TrieNode_13, BINS_256>(
      buffer1, buffer2, offset1, offset2, pop1, pop2);
  serialize_1DxT(mergedRoot.get(), mergedBuffer, false);
  SPDLOG_LOGGER_INFO(airtree::merge::logger(), "Serialized merged trie (root) successfully");
  mergedRoot.reset();

  // Merge the level-1 children based on original populated flags.
  std::unique_ptr<TrieNode_13_Level1> temp_node;
  for (size_t i = 0; i < BINS_256; i++) {
    if (pop1.test(i) && !pop2.test(i)) {
      if (offset1 < buffer1.size()) {
        temp_node = deserialize_1DxT_l1(buffer1, offset1);
        serialize_1DxT_l1(temp_node.get(), mergedBuffer);
        temp_node.reset();
      }
    } else if (pop2.test(i) && !pop1.test(i)) {
      if (offset2 < buffer2.size()) {
        temp_node = deserialize_1DxT_l1(buffer2, offset2);
        serialize_1DxT_l1(temp_node.get(), mergedBuffer);
        temp_node.reset();
      }
    } else if (pop1.test(i) && pop2.test(i)) { // both have a level1 node
      if (offset1 < buffer1.size() && offset2 < buffer2.size()) {
        temp_node = mergeTrieNode13Level1(
            deserialize_1DxT_l1(buffer1, offset1),
            deserialize_1DxT_l1(buffer2, offset2));
        serialize_1DxT_l1(temp_node.get(), mergedBuffer);
        temp_node.reset();
      }
    }
  }

  // Add EOF marker and serialize the merged trie.
  add_EOF(mergedBuffer);
  airtree::core::common::finalizeHeader(mergedBuffer, mergedBuffer.size() - header_end);
  SPDLOG_LOGGER_INFO(
      airtree::merge::logger(), "Serialized merged trie (root and children) successfully");
  return mergedBuffer;
}

// Merge two Level 1 nodes for TrieNode_13.
std::unique_ptr<TrieNode_13_Level1>
Merge1DxT::mergeTrieNode13Level1(std::unique_ptr<TrieNode_13_Level1> node1,
                                 std::unique_ptr<TrieNode_13_Level1> node2) {
  SPDLOG_LOGGER_INFO(airtree::merge::logger(), "Entering mergeTrieNode13Level1");
  if (!node1) {
    SPDLOG_LOGGER_INFO(airtree::merge::logger(), "node1 is nullptr, returning node2");
    return node2;
  }
  if (!node2) {
    SPDLOG_LOGGER_INFO(airtree::merge::logger(), "node2 is nullptr, returning node1");
    return node1;
  }
  for (size_t i = 0; i < BINS_32; ++i) {
    SPDLOG_LOGGER_INFO(airtree::merge::logger(), "Merging Trie13 Level1 index {}: {} + {}", i,
                       node1->counts[i], node2->counts[i]);
    node1->counts[i] += node2->counts[i];
  }
  return node1;
}

// Merge two Level 0 nodes for TrieNode_13.
[[deprecated("This function may not be used in the current merging process.")]]
std::unique_ptr<TrieNode_13>
mergeTrieNode13(std::unique_ptr<TrieNode_13> node1,
                std::unique_ptr<TrieNode_13> node2) {
  SPDLOG_LOGGER_INFO(airtree::merge::logger(), "Entering mergeTrieNode13");
  if (!node1) {
    SPDLOG_LOGGER_INFO(airtree::merge::logger(), "node1 is nullptr, returning node2");
    return node2;
  }
  if (!node2) {
    SPDLOG_LOGGER_INFO(airtree::merge::logger(), "node2 is nullptr, returning node1");
    return node1;
  }

  for (size_t i = 0; i < BINS_256; ++i) {
    SPDLOG_LOGGER_INFO(airtree::merge::logger(), "Merging Trie13 Level0 index {}", i);
    // Merge the counts at this index.
    node1->counts[i] += node2->counts[i];

    // Set the populated flag if either node has it.
    if (node1->populated.test(i) || node2->populated.test(i)) {
      node1->populated.set(i);
    }
    // Do not merge the child pointers here.
  }
  return node1;
}
