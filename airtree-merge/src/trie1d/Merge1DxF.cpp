// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <vector>

#include <airtree/merge/trie1d/Merge1DxF.hpp>
#include <airtree/merge/MergeStrategy.hpp>
#include <airtree/merge/Logger.hpp>

using namespace airtree::merge;
using namespace airtree::merge::dim1;

std::vector<char> Merge1DxF::merge(const std::vector<char> &buffer1,
                                   const std::vector<char> &buffer2) {
  auto header1 = airtree::core::common::deserializeHeader(buffer1);
  auto header2 = airtree::core::common::deserializeHeader(buffer2);
  size_t offset1 = header1.header_length;
  size_t offset2 = header2.header_length;
  auto mergedHeader = mergeHeaders(header1, header2);
  std::vector<char> mergedBuffer;
  airtree::core::common::serializeHeader(mergedHeader, mergedBuffer);
  size_t header_end = mergedBuffer.size();
  SPDLOG_LOGGER_DEBUG(logger(), "Merged headers successfully");

  // Deserialize the root nodes.
  PopulatedBins<BINS_256> pop1, pop2;
  auto mergedRoot = Root_merge<TrieNode_16, BINS_256>(
      buffer1, buffer2, offset1, offset2, pop1, pop2);
  serialize_1DxF(mergedRoot.get(), mergedBuffer, false);
  mergedRoot.reset();


  SPDLOG_LOGGER_DEBUG(logger(), "Merged root nodes successfully");
  // Merge level1 children based on the original populated flags.
  std::unique_ptr<TrieNode_16_Level1> temp_node;
  for (size_t i = 0; i < BINS_256; i++) {
    SPDLOG_LOGGER_TRACE(logger(), "Merging Trie16 Level1 index {}", i);
    if (pop1.test(i) && !pop2.test(i)) {
      if (offset1 < buffer1.size()) {
        temp_node = deserialize_1DxF_l1(buffer1, offset1);
        serialize_1DxF_l1(temp_node.get(), mergedBuffer);
        temp_node.reset();
      }
    } else if (pop2.test(i) && !pop1.test(i)) {
      if (offset2 < buffer2.size()) {
        temp_node = deserialize_1DxF_l1(buffer2, offset2);
        serialize_1DxF_l1(temp_node.get(), mergedBuffer);
        temp_node.reset();
      }
    } else if (pop1.test(i)
               && pop2.test(i)) { // Both buffers have a level1 node at index i.
      if (offset1 < buffer1.size() && offset2 < buffer2.size()) {
        temp_node = mergeTrieNode16Level1(
            deserialize_1DxF_l1(buffer1, offset1),
            deserialize_1DxF_l1(buffer2, offset2));
        serialize_1DxF_l1(temp_node.get(), mergedBuffer);
        temp_node.reset();
      }
    }
  }
  SPDLOG_LOGGER_DEBUG(
      logger(), "Serialized merged trie (root and children) successfully");
  add_EOF(mergedBuffer);
  airtree::core::common::finalizeHeader(mergedBuffer, mergedBuffer.size() - header_end);
  return mergedBuffer;
}
