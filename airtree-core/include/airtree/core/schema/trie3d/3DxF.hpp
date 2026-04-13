#ifndef AIRTREE_CORE_SCHEMA_TRIE3D_3DXF_HPP
#define AIRTREE_CORE_SCHEMA_TRIE3D_3DXF_HPP


#include <airtree/core/schema/trie1d/1DxF.hpp>


#include <airtree/core/common/Bins.hpp>
#include <airtree/core/common/FPHArray.hpp>
#include <airtree/core/common/SpecialCounts.hpp>


struct Node3D_888_l0;

struct TLE_3D_888 {
  std::bitset<BINS_512> populated;
  uint32_t counts[BINS_512] = {0};
  std::unique_ptr<Node3D_888_l0> nodes[BINS_512];

  TLE_3D_888() {
    populated.reset();
  }
};

struct Node3D_888_l0 {
  std::bitset<BINS_256> populated;
  uint32_t counts[BINS_256] = {0};
  std::unique_ptr<TrieNode_16> nodes[BINS_256];

  Node3D_888_l0() {
    populated.reset();
  }
};


[[nodiscard]] std::unique_ptr<TLE_3D_888>
execCreateAndInsert_3D_888(const FPHArray &array1, const FPHArray &array2,
                           const FPHArray &array3, uint64_t &curr_trie_size,
                           std::unique_ptr<SpecialCounts> &specialCounts,
                           bool default_mode = true);
[[nodiscard]] std::vector<char>
execSerialize_3D_888(TLE_3D_888 *root, uint64_t &curr_trie_size,
                     std::unique_ptr<SpecialCounts> &specialCounts,
                     bool default_mode);
[[nodiscard]] std::vector<char> generate_3DxF(const FPHArray &array1,
                                              const FPHArray &array2,
                                              const FPHArray &array3,
                                              bool default_mode = true);

#endif // AIRTREE_CORE_SCHEMA_TRIE3D_3DXF_HPP