#ifndef AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM2_MERGE2DxF_HPP
#define AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM2_MERGE2DxF_HPP

#include <vector>

#include <airtree/merge/MergeStrategy.hpp>

namespace airtree::merge::dim2 {

class Merge2DxF : public airtree::merge::MergeStrategy {
public:
  // Merges two buffers for the 2DxF configuration.
  std::vector<char> merge(const std::vector<char> &buffer1,
                          const std::vector<char> &buffer2) override;
};

} // namespace airtree::merge::dim2

#endif // AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM2_MERGE2DxF_HPP
