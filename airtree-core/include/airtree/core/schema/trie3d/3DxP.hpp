#ifndef AIRTREE_CORE_SCHEMA_TRIE3D_3DXP_HPP
#define AIRTREE_CORE_SCHEMA_TRIE3D_3DXP_HPP

#include <bitset>
#include <airtree/core/common/Bins.hpp>
#include <airtree/core/common/FPHArray.hpp>
#include <airtree/core/common/SpecialCounts.hpp>
#include <airtree/core/api/AirTreeGenerator.hpp>
#include <vector>

struct Node3D_3x10_l0;
struct Node3D_3x10_l1;
struct Node3D_3x10_l2;

struct TLE_3D_3x10 {
  std::bitset<BINS_512> populated;
  uint32_t counts[BINS_512] = {0};
  std::unique_ptr<Node3D_3x10_l0> nodes[BINS_512];

  TLE_3D_3x10() {
    populated.reset();
  }
};

struct Node3D_3x10_l0 {
  std::bitset<BINS_1024> populated;
  uint32_t counts[BINS_1024] = {0};
  std::unique_ptr<Node3D_3x10_l1> nodes[BINS_1024];

  Node3D_3x10_l0() {
    populated.reset();
  }
};

struct Node3D_3x10_l1 {
  std::bitset<BINS_1024> populated;
  uint32_t counts[BINS_1024] = {0};
  std::unique_ptr<Node3D_3x10_l2> nodes[BINS_1024];

  Node3D_3x10_l1() {
    populated.reset();
  }
};

struct Node3D_3x10_l2 {
  std::bitset<BINS_1024> populated;
  uint32_t counts[BINS_1024] = {0};

  Node3D_3x10_l2() {
    populated.reset();
  }
};


[[nodiscard]] std::unique_ptr<TLE_3D_3x10>
execCreateAndInsert_3D_3x10(const FPHArray &array1, const FPHArray &array2,
                            const FPHArray &array3, uint64_t &curr_trie_size,
                            std::unique_ptr<SpecialCounts> &specialCounts,
                            bool default_mode = true);
[[nodiscard]] std::vector<char>
execSerialize_3D_3x10(TLE_3D_3x10 *root, uint64_t &curr_trie_size,
                      std::unique_ptr<SpecialCounts> &specialCounts,
                      bool default_mode);
[[nodiscard]] std::vector<char> generate_3DxP(const FPHArray &array1,
                                              const FPHArray &array2,
                                              const FPHArray &array3,
                                              bool default_mode = true);

namespace airtree::core::schema::trie3d {

class Generator3DxP : public airtree::core::api::AirTreeGenerator {
public:
  [[nodiscard]] std::vector<char>
  generate(const std::vector<const FPHArray *> &arrays,
           bool default_mode) const override;
};

} // namespace airtree::core::schema::trie3d

#endif // AIRTREE_CORE_SCHEMA_TRIE3D_3DXP_HPP