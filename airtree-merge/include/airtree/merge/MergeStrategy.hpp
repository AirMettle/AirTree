#ifndef AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_MERGESTRATEGY_HPP
#define AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_MERGESTRATEGY_HPP

#include <cstdint>
#include <vector>
#include <memory>
#include <bitset>
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
  std::vector<uint64_t> compact_arr_values1 =
      deserializeCompactBooleanArray(buffer1, offset1, BucketCount / 64);
  std::vector<uint64_t> compact_arr_values2 =
      deserializeCompactBooleanArray(buffer2, offset2, BucketCount / 64);

  BooleanArray compact_array1 = BooleanArray(compact_arr_values1);
  BooleanArray compact_array2 = BooleanArray(compact_arr_values2);

  auto counts1 = deserializeCounts(buffer1, offset1, compact_array1.count());
  auto counts2 = deserializeCounts(buffer2, offset2, compact_array2.count());

  size_t idx1 = 0, idx2 = 0;
  for (size_t i = 0; i < BucketCount; i++) {
    pop1[i] = compact_array1.get(i);
    pop2[i] = compact_array2.get(i);

    if (pop1.test(i) || pop2.test(i)) {
      mergedRoot->populated.set(i);
      if (pop1.test(i) && pop2.test(i)) {
        // If both nodes are populated, merge their counts
        mergedRoot->counts[i] = counts1[idx1] + counts2[idx2];
        idx1++;
        idx2++;
      } else if (pop1.test(i)) {
        // If only node1 is populated
        mergedRoot->counts[i] = counts1[idx1];
        idx1++;
      } else if (pop2.test(i)) {
        // If only node2 is populated
        mergedRoot->counts[i] = counts2[idx2];
        idx2++;
      }
    }
  }
  return mergedRoot;
}

template <typename NodeType, size_t BucketCount>
std::unique_ptr<NodeType> Root_merge_TLE(const std::vector<char> &buffer1,
                                         const std::vector<char> &buffer2,
                                         size_t &offset1, size_t &offset2,
                                         std::bitset<BucketCount> &pop1,
                                         std::bitset<BucketCount> &pop2) {
  auto mergedRoot = std::make_unique<NodeType>();
  std::vector<uint64_t> compact_arr_values1 =
      deserializeCompactBooleanArray(buffer1, offset1, BucketCount / 64);
  std::vector<uint64_t> compact_arr_values2 =
      deserializeCompactBooleanArray(buffer2, offset2, BucketCount / 64);

  BooleanArray compact_array1 = BooleanArray(compact_arr_values1);
  BooleanArray compact_array2 = BooleanArray(compact_arr_values2);

  auto counts1 = deserializeCounts(buffer1, offset1, compact_array1.count());
  auto counts2 = deserializeCounts(buffer2, offset2, compact_array2.count());

  size_t idx1 = 0, idx2 = 0;
  for (size_t i = 0; i < BucketCount; i++) {
    pop1[i] = compact_array1.get(i);
    pop2[i] = compact_array2.get(i);

    if (pop1.test(i) || pop2.test(i)) {
      mergedRoot->populated.set(i);
      if (pop1.test(i) && pop2.test(i)) {
        // If both nodes are populated, merge their counts
        mergedRoot->TLEcounts[i] = counts1[idx1] + counts2[idx2];
        idx1++;
        idx2++;
      } else if (pop1.test(i)) {
        // If only node1 is populated
        mergedRoot->TLEcounts[i] = counts1[idx1];
        idx1++;
      } else if (pop2.test(i)) {
        // If only node2 is populated
        mergedRoot->TLEcounts[i] = counts2[idx2];
        idx2++;
      }
    }
  }
  return mergedRoot;
}

} // namespace airtree::merge

#endif // AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_MERGESTRATEGY_HPP
