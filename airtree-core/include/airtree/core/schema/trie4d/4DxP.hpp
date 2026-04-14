#ifndef AIRTREE_CORE_SCHEMA_TRIE4D_4DXP_HPP
#define AIRTREE_CORE_SCHEMA_TRIE4D_4DXP_HPP

#include <bitset>
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
  std::bitset<BINS_4096> populated;
  uint32_t counts[BINS_4096] = {0};
  std::unique_ptr<Node4D_4x10_l0> nodes[BINS_4096];

  TLE_4D_4x10() {
    populated.reset();
  }
};

struct Node4D_4x10_l0 {
  std::bitset<BINS_1024> populated;
  uint32_t counts[BINS_1024] = {0};
  std::unique_ptr<Node4D_4x10_l1> nodes[BINS_1024];

  Node4D_4x10_l0() {
    populated.reset();
  }
};

struct Node4D_4x10_l1 {
  std::bitset<BINS_1024> populated;
  uint32_t counts[BINS_1024] = {0};
  std::unique_ptr<Node4D_4x10_l2> nodes[BINS_1024];

  Node4D_4x10_l1() {
    populated.reset();
  }
};

struct Node4D_4x10_l2 {
  std::bitset<BINS_1024> populated;
  uint32_t counts[BINS_1024] = {0};
  std::unique_ptr<Node4D_4x10_l3> nodes[BINS_1024];

  Node4D_4x10_l2() {
    populated.reset();
  }
};

struct Node4D_4x10_l3 {
  std::bitset<BINS_1024> populated;
  uint32_t counts[BINS_1024] = {0};

  Node4D_4x10_l3() {
    populated.reset();
  }
};


std::unique_ptr<TLE_4D_4x10> execCreateAndInsert_4D_4x10(
    const FPHArray &array1, const FPHArray &array2, const FPHArray &array3,
    const FPHArray &array4, uint64_t &curr_trie_size,
    std::unique_ptr<SpecialCounts> &specialCounts, bool default_mode = true);
[[nodiscard]] std::vector<char>
execSerialize_4D_4x10(TLE_4D_4x10 *root, uint64_t &curr_trie_size,
                      std::unique_ptr<SpecialCounts> &specialCounts,
                      bool default_mode);
std::vector<char> generate_4DxP(const FPHArray &array1, const FPHArray &array2,
                                const FPHArray &array3, const FPHArray &array4,
                                bool default_mode = true);


namespace airtree::core::schema::trie4d {

class Generator4DxP : public airtree::core::api::AirTreeGenerator {
public:
  [[nodiscard]] std::vector<char>
  generate(const std::vector<const FPHArray *> &arrays,
           bool default_mode) const override;
};

} // namespace airtree::core::schema::trie4d

#endif // AIRTREE_CORE_SCHEMA_TRIE4D_4DXP_HPP