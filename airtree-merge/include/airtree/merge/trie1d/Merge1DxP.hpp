// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM1_MERGE1DxP_HPP
#define AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM1_MERGE1DxP_HPP

#include <vector>

#include <airtree/merge/MergeStrategy.hpp>

namespace airtree::merge::dim1 {

class Merge1DxP : public airtree::merge::MergeStrategy {
public:
  // Merges two buffers for the 1DxP configuration.
  std::vector<char> merge(const std::vector<char> &buffer1,
                          const std::vector<char> &buffer2) override;
  std::unique_ptr<TrieNode_20_Level2>
  mergeTrieNode20_Level2(std::unique_ptr<TrieNode_20_Level2> node1,
                         std::unique_ptr<TrieNode_20_Level2> node2);
  std::unique_ptr<TrieNode_20_Level1>
  mergeTrieNode20_Level1(std::unique_ptr<TrieNode_20_Level1> node1,
                         std::unique_ptr<TrieNode_20_Level1> node2);
};

} // namespace airtree::merge::dim1

#endif // AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM1_MERGE1DxP_HPP
