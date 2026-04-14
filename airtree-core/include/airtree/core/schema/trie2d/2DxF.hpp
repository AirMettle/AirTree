#ifndef AIRTREE_CORE_SCHEMA_TRIE2D_2DXF_HPP
#define AIRTREE_CORE_SCHEMA_TRIE2D_2DXF_HPP


#include <airtree/core/schema/trie1d/1DxF.hpp>

#include <airtree/core/common/Bins.hpp>
#include <airtree/core/common/FPHArray.hpp>
#include <airtree/core/common/SpecialCounts.hpp>
#include <airtree/core/api/AirTreeGenerator.hpp>

/**
* @brief TLETrieNode : Trie Node for the TLE level of the 2-D trie

* @param populated : Bitset to check if the bucket is populated
* @param TLEcounts : Array to store the counts of the TLEs in the bucket
* @param nodes : Array of shared pointers to the next level of the trie nodes
* @see TriNode_16 definition in FloatingPointTrie/1-D.h
*/
struct TLETrieNode_2D {
  std::bitset<BINS_64> populated;
  uint32_t TLEcounts[BINS_64];
  std::unique_ptr<TrieNode_16> nodes[BINS_64];
};


[[nodiscard]] std::unique_ptr<TLETrieNode_2D> execCreateAndInsert_2D(
    const FPHArray &array1, const FPHArray &array2, uint64_t &curr_trie_size,
    std::unique_ptr<SpecialCounts> &specialCounts, bool default_mode = true);
[[nodiscard]] std::vector<char>
execSerialize_2D(TLETrieNode_2D *root, uint64_t &curr_trie_size,
                 std::unique_ptr<SpecialCounts> &specialCounts,
                 bool default_mode);
[[nodiscard]] std::vector<char> generate_2DxF(const FPHArray &array1,
                                              const FPHArray &array2,
                                              bool default_mode = true);

namespace airtree::core::schema::trie2d {

class Generator2DxF : public airtree::core::api::AirTreeGenerator {
public:
  [[nodiscard]] std::vector<char>
  generate(const std::vector<const FPHArray *> &arrays,
           bool default_mode) const override;
};

} // namespace airtree::core::schema::trie2d


#endif // AIRTREE_CORE_SCHEMA_TRIE2D_2DXF_HPP