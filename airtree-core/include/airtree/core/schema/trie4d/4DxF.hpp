// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_SCHEMA_TRIE4D_4DXF_HPP
#define AIRTREE_CORE_SCHEMA_TRIE4D_4DXF_HPP

#include <bitset>
#include <airtree/core/common/Bins.hpp>
#include <airtree/core/common/FPHArray.hpp>
#include <airtree/core/common/SpecialCounts.hpp>
#include <airtree/core/api/AirTreeGenerator.hpp>
#include <airtree/core/schema/trie1d/1DxF.hpp>
#include <vector>

struct Node4D_4x8_l0;
struct Node4D_4x8_l1;

struct TLE_4D_4x8 {
  std::bitset<BINS_4096> populated;
  uint32_t counts[BINS_4096] = {0};
  std::unique_ptr<Node4D_4x8_l0> nodes[BINS_4096];

  TLE_4D_4x8() {
    populated.reset();
  }
};

struct Node4D_4x8_l0 {
  std::bitset<BINS_256> populated;
  uint32_t counts[BINS_256] = {0};
  std::unique_ptr<Node4D_4x8_l1> nodes[BINS_256];

  Node4D_4x8_l0() {
    populated.reset();
  }
};

struct Node4D_4x8_l1 {
  std::bitset<BINS_256> populated;
  uint32_t counts[BINS_256] = {0};
  std::unique_ptr<TrieNode_16> nodes[BINS_256];

  Node4D_4x8_l1() {
    populated.reset();
  }
};

[[nodiscard]] std::unique_ptr<TLE_4D_4x8> execCreateAndInsert_4D_4x8(
    const FPHArray &array1, const FPHArray &array2, const FPHArray &array3,
    const FPHArray &array4, uint64_t &curr_trie_size,
    std::unique_ptr<SpecialCounts> &specialCounts, bool default_mode = true);
[[nodiscard]] std::vector<char>
execSerialize_4D_4x8(TLE_4D_4x8 *root, uint64_t &curr_trie_size,
                     std::unique_ptr<SpecialCounts> &specialCounts,
                     bool default_mode);
[[nodiscard]] std::vector<char> generate_4DxF(const FPHArray &array1,
                                              const FPHArray &array2,
                                              const FPHArray &array3,
                                              const FPHArray &array4,
                                              bool default_mode = true);

namespace airtree::core::schema::trie4d {

class Generator4DxF : public airtree::core::api::AirTreeGenerator {
public:
  [[nodiscard]] std::vector<char>
  generate(const std::vector<const FPHArray *> &arrays,
           bool default_mode) const override;
};

} // namespace airtree::core::schema::trie4d

#endif // AIRTREE_CORE_SCHEMA_TRIE4D_4DXF_HPP