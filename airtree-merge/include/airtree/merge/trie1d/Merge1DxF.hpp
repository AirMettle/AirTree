// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM1_MERGE1DxF_HPP
#define AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM1_MERGE1DxF_HPP

#include <vector>

#include <airtree/merge/MergeStrategy.hpp>

namespace airtree::merge::dim1 {

class Merge1DxF : public airtree::merge::MergeStrategy {
public:
  // Merges two buffers for the 1DxF configuration.
  std::vector<char> merge(const std::vector<char> &buffer1,
                          const std::vector<char> &buffer2) override;
};

} // namespace airtree::merge::dim1

#endif // AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM1_MERGE1DxF_HPP
