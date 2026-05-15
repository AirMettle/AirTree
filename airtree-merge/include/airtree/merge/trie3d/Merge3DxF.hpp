// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM3_MERGE3DxF_HPP
#define AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM3_MERGE3DxF_HPP

#include <vector>

#include <airtree/merge/MergeStrategy.hpp>

namespace airtree::merge::dim3 {

class Merge3DxF : public airtree::merge::MergeStrategy {
public:
  // Merges two buffers for the 3DxF configuration.
  std::vector<char> merge(const std::vector<char> &buffer1,
                          const std::vector<char> &buffer2) override;

  std::unique_ptr<Node3D_888_l0>
  mergeNode3D_888_l0(std::unique_ptr<Node3D_888_l0> node1,
                     std::unique_ptr<Node3D_888_l0> node2);

};

} // namespace airtree::merge::dim3

#endif // AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM3_MERGE3DxF_HPP
