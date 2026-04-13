#ifndef AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM4_MERGE4DxF_HPP
#define AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM4_MERGE4DxF_HPP

#include <vector>

#include <airtree/merge/MergeStrategy.hpp>

namespace airtree::merge::dim4 {

class Merge4DxF : public airtree::merge::MergeStrategy {
public:
  // Merges two buffers for the 4DxF configuration.
  std::vector<char> merge(const std::vector<char> &buffer1,
                          const std::vector<char> &buffer2) override;

  std::unique_ptr<Node4D_4x8_l0>
  mergeNode4D_4x8_l0(std::unique_ptr<Node4D_4x8_l0> node1,
                     std::unique_ptr<Node4D_4x8_l0> node2);

  std::unique_ptr<Node4D_4x8_l1>
  mergeNode4D_4x8_l1(std::unique_ptr<Node4D_4x8_l1> node1,
                     std::unique_ptr<Node4D_4x8_l1> node2);
};

} // namespace airtree::merge::dim4

#endif // AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_DIM4_MERGE4DxF_HPP
