// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM1_MERGE1DxT_HPP
#define AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM1_MERGE1DxT_HPP

#include <vector>

#include <airtree/merge/MergeStrategy.hpp>
#include <airtree/core/AirTreeCore_internal.hpp>

namespace airtree::merge::dim1 {

class Merge1DxT : public airtree::merge::MergeStrategy {
public:
  // Merges two buffers for the 1DxT configuration.
  std::vector<char> merge(const std::vector<char> &buffer1,
                          const std::vector<char> &buffer2) override;

protected:
  std::unique_ptr<TrieNode_13_Level1>
  mergeTrieNode13Level1(std::unique_ptr<TrieNode_13_Level1> node1,
                        std::unique_ptr<TrieNode_13_Level1> node2);
};

} // namespace airtree::merge::dim1

#endif // AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM1_MERGE1DxT_HPP
