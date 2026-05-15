// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM2_MERGE2DxP_HPP
#define AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM2_MERGE2DxP_HPP

#include <vector>

#include <airtree/merge/MergeStrategy.hpp>

namespace airtree::merge::dim2 {

class Merge2DxP : public airtree::merge::MergeStrategy {
public:
  // Merges two buffers for the 2DxP configuration.
  std::vector<char> merge(const std::vector<char> &buffer1,
                          const std::vector<char> &buffer2) override;

  std::unique_ptr<TrieNode_2D_10>
  mergeTrieNode2D10(std::unique_ptr<TrieNode_2D_10> node1,
                    std::unique_ptr<TrieNode_2D_10> node2);

  std::unique_ptr<TrieNode_2D_10_Level1>
  mergeTrieNode2D10_Level1(std::unique_ptr<TrieNode_2D_10_Level1> node1,
                           std::unique_ptr<TrieNode_2D_10_Level1> node2);
};

} // namespace airtree::merge::dim2

#endif // AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM2_MERGE2DxP_HPP
