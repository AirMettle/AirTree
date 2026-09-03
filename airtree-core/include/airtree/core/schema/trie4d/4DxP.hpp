// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_SCHEMA_TRIE4D_4DXP_HPP
#define AIRTREE_CORE_SCHEMA_TRIE4D_4DXP_HPP

#include <bitset>
#include <airtree/core/common/Populated.hpp>
#include <airtree/core/common/SparseNode.hpp>
#include <airtree/core/common/Bins.hpp>
#include <airtree/core/common/FPHArray.hpp>
#include <airtree/core/common/SpecialCounts.hpp>
#include <airtree/core/api/AirTreeGenerator.hpp>
#include <vector>

struct Node4D_4x10_l0;
struct Node4D_4x10_l1;
struct Node4D_4x10_l2;
struct Node4D_4x10_l3;

struct TLE_4D_4x10 {
  PopulatedBins<BINS_4096> populated;
  uint32_t counts[BINS_4096] = {0};
  std::unique_ptr<Node4D_4x10_l0> nodes[BINS_4096];

  TLE_4D_4x10() {
    populated.reset();
  }
};

struct Node4D_4x10_l3 : SparseLeaf<BINS_1024> {};
struct Node4D_4x10_l2 : SparseNode<BINS_1024, Node4D_4x10_l3> {};
struct Node4D_4x10_l1 : SparseNode<BINS_1024, Node4D_4x10_l2> {};
struct Node4D_4x10_l0 : SparseNode<BINS_1024, Node4D_4x10_l1> {};


std::unique_ptr<TLE_4D_4x10> execCreateAndInsert_4D_4x10(
    const FPHArray &array1, const FPHArray &array2, const FPHArray &array3,
    const FPHArray &array4, uint64_t &curr_trie_size,
    std::unique_ptr<SpecialCounts> &specialCounts);
[[nodiscard]] std::vector<char>
execSerialize_4D_4x10(TLE_4D_4x10 *root, std::unique_ptr<SpecialCounts> &specialCounts);
std::vector<char> generate_4DxP(const FPHArray &array1, const FPHArray &array2,
                                const FPHArray &array3, const FPHArray &array4);


namespace airtree::core::schema::trie4d {

class Generator4DxP : public airtree::core::api::AirTreeGenerator {
public:
  [[nodiscard]] std::vector<char>
  generate(const std::vector<const FPHArray *> &arrays) const override;
};

} // namespace airtree::core::schema::trie4d

#endif // AIRTREE_CORE_SCHEMA_TRIE4D_4DXP_HPP