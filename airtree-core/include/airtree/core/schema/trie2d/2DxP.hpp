#ifndef AIRTREE_CORE_SCHEMA_TRIE2D_2DXP_HPP
#define AIRTREE_CORE_SCHEMA_TRIE2D_2DXP_HPP

#include <bitset>
#include <airtree/core/common/Bins.hpp>
#include <airtree/core/common/FPHArray.hpp>
#include <airtree/core/common/SpecialCounts.hpp>
#include <airtree/core/api/AirTreeGenerator.hpp>
#include <vector>


struct TrieNode_2D_10_Level1 {
  uint32_t counts[BINS_1024] = {0};
};

struct TrieNode_2D_10 {
  std::bitset<BINS_1024> populated;
  uint32_t counts[BINS_1024] = {0};
  std::unique_ptr<TrieNode_2D_10_Level1> nodes[BINS_1024];
};

struct TLEoption3_2D {
  std::bitset<BINS_64> populated;
  uint32_t counts[BINS_64] = {0};
  std::unique_ptr<TrieNode_2D_10> nodes[BINS_64];
};


/**
* Function to generate 2-D trie from the input values

* @param values1 : Array of input values for the first dimension
* @param values2 : Array of input values for the second dimension
* @param values_len1 : Length of the values1 array
* @param values_len2 : Length of the values2 array
* @param default_mode : Flag to enable the default mode
* @return Trie in serialized buffer of characters.
*/

[[nodiscard]] std::unique_ptr<TLEoption3_2D> execCreateAndInsert_2D_2x10(
    const FPHArray &array1, const FPHArray &array2, uint64_t &curr_trie_size,
    std::unique_ptr<SpecialCounts> &specialCounts, bool default_mode = true);

[[nodiscard]] std::vector<char>
execSerialize_2D_2x10(TLEoption3_2D *root, uint64_t &curr_trie_size,
                      std::unique_ptr<SpecialCounts> &specialCounts,
                      bool default_mode = true);


[[nodiscard]] std::vector<char> generate_2DxP(const FPHArray &array1,
                                              const FPHArray &array2,
                                              bool default_mode = true);
namespace airtree::core::schema::trie2d {

class Generator2DxP : public airtree::core::api::AirTreeGenerator {
public:
  [[nodiscard]] std::vector<char>
  generate(const std::vector<const FPHArray *> &arrays,
           bool default_mode) const override;
};

} // namespace airtree::core::schema::trie2d

#endif // AIRTREE_CORE_SCHEMA_TRIE2D_2DXP_HPP