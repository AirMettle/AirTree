// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_MERGESTRATEGY_HPP
#define AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_MERGESTRATEGY_HPP

#include <cstdint>
#include <vector>
#include <memory>
#include <bitset>
#include <stdexcept>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>

using namespace airtree::core;

namespace airtree::merge {

class MergeStrategy {
public:
  virtual ~MergeStrategy() = default;
  virtual std::vector<char> merge(const std::vector<char> &buf1,
                                  const std::vector<char> &buf2) = 0;

protected:
  airtree::core::common::AirTreeHeader mergeHeaders(airtree::core::common::AirTreeHeader &header1, const airtree::core::common::AirTreeHeader &header2);
  void add_EOF(std::vector<char> &buffer);
  // Merge two Level 1 nodes for TrieNode_16.
  std::unique_ptr<TrieNode_16_Level1>
  mergeTrieNode16Level1(std::unique_ptr<TrieNode_16_Level1> node1,
                        std::unique_ptr<TrieNode_16_Level1> node2);

  std::unique_ptr<TrieNode_16>
  mergeTrieNode16(std::unique_ptr<TrieNode_16> node1,
                  std::unique_ptr<TrieNode_16> node2);

public:
  void saveMergedBinaryFile_buffer(const std::vector<char> &buffer,
                                   const std::string &outputFile);
};

template <typename NodeType, size_t BucketCount>
std::unique_ptr<NodeType>
Root_merge(const std::vector<char> &buffer1, const std::vector<char> &buffer2,
           size_t &offset1, size_t &offset2, std::bitset<BucketCount> &pop1,
           std::bitset<BucketCount> &pop2) {
  auto mergedRoot = std::make_unique<NodeType>();
  uint64_t mask1[(BucketCount + 63) / 64], mask2[(BucketCount + 63) / 64];
  uint32_t counts2[BucketCount] = {0};
  if (!readPopulatedMask(buffer1, offset1, mask1, BucketCount)
      || !deserializeCounts(buffer1, offset1, mask1, BucketCount, mergedRoot->counts)
      || !readPopulatedMask(buffer2, offset2, mask2, BucketCount)
      || !deserializeCounts(buffer2, offset2, mask2, BucketCount, counts2)) {
    throw std::runtime_error("mergeAirTree: truncated root node");
  }
  setPopulated(pop1, mask1);
  setPopulated(pop2, mask2);
  mergedRoot->populated = pop1 | pop2;
  forEachSetBit(mask2, BucketCount, [&](size_t i) { mergedRoot->counts[i] += counts2[i]; });
  return mergedRoot;
}

template <typename NodeType, size_t BucketCount>
std::unique_ptr<NodeType>
Root_merge_TLE(const std::vector<char> &buffer1, const std::vector<char> &buffer2,
           size_t &offset1, size_t &offset2, std::bitset<BucketCount> &pop1,
           std::bitset<BucketCount> &pop2) {
  auto mergedRoot = std::make_unique<NodeType>();
  uint64_t mask1[(BucketCount + 63) / 64], mask2[(BucketCount + 63) / 64];
  uint32_t counts2[BucketCount] = {0};
  if (!readPopulatedMask(buffer1, offset1, mask1, BucketCount)
      || !deserializeCounts(buffer1, offset1, mask1, BucketCount, mergedRoot->TLEcounts)
      || !readPopulatedMask(buffer2, offset2, mask2, BucketCount)
      || !deserializeCounts(buffer2, offset2, mask2, BucketCount, counts2)) {
    throw std::runtime_error("mergeAirTree: truncated root node");
  }
  setPopulated(pop1, mask1);
  setPopulated(pop2, mask2);
  mergedRoot->populated = pop1 | pop2;
  forEachSetBit(mask2, BucketCount, [&](size_t i) { mergedRoot->TLEcounts[i] += counts2[i]; });
  return mergedRoot;
}

} // namespace airtree::merge

#endif // AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_MERGESTRATEGY_HPP
