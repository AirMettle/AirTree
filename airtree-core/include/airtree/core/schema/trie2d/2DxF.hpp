// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_SCHEMA_TRIE2D_2DXF_HPP
#define AIRTREE_CORE_SCHEMA_TRIE2D_2DXF_HPP


#include <airtree/core/schema/trie1d/1DxF.hpp>
#include <airtree/core/common/Populated.hpp>

#include <airtree/core/common/Bins.hpp>
#include <airtree/core/common/FPHArray.hpp>
#include <airtree/core/common/SpecialCounts.hpp>
#include <airtree/core/api/AirTreeGenerator.hpp>
#include <airtree/core/common/BitCodec.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/schema/trie2d/Encoded2D.hpp>
#include <memory>

/**
* @brief TLETrieNode : Trie Node for the TLE level of the 2-D trie

* @param populated : Bitset to check if the bucket is populated
* @param TLEcounts : Array to store the counts of the TLEs in the bucket
* @param nodes : Array of shared pointers to the next level of the trie nodes
* @see TriNode_16 definition in FloatingPointTrie/1-D.h
*/
struct TLETrieNode_2D {
  PopulatedBins<BINS_64> populated;
  uint32_t TLEcounts[BINS_64];
  std::unique_ptr<TrieNode_16> nodes[BINS_64];
};


[[nodiscard]] std::unique_ptr<TLETrieNode_2D> CreateParentNode_TLE2D88();

// Files one (v1, v2) point: counts its specials into specialCounts and returns where it lands.
template <typename T1, typename T2>
inline Encoded2D encode_2DxF(T1 v1, T2 v2,
                             std::unique_ptr<SpecialCounts> &specialCounts) {
  const std::pair<TLE, unsigned int> in1 = internal_8bit(v1);
  const std::pair<TLE, unsigned int> in2 = internal_8bit(v2);
  const unsigned int special1 = update_special_counts(in1.first, specialCounts);
  const unsigned int special2 = update_special_counts(in2.first, specialCounts);
  Encoded2D e{static_cast<unsigned int>((in1.first.encoding << 3) | in2.first.encoding), 0,
              (~((special1 << 1) | special2)) & 0x3u};
  switch (e.ndims) {
  case 1: e.combined = in2.second; break;
  case 2: e.combined = in1.second; break;
  case 3: e.combined = combine_chunks_8b_temp(in1.second, in2.second); break;
  default: break;
  }
  return e;
}

// Inserts one encoded point (count 1); allocates nodes on first use and grows curr_trie_size.
void insertintoTLETrie_2D_88(TLETrieNode_2D *root, unsigned int combined,
                             unsigned int combinedTLE, int ndims,
                             uint64_t &curr_trie_size);

// One point into the trie: what execCreateAndInsert_2D does per element.
template <typename T1, typename T2>
inline void createAndInsert_2DxF(TLETrieNode_2D *root, T1 v1, T2 v2, uint64_t &curr_trie_size,
                                 std::unique_ptr<SpecialCounts> &specialCounts) {
  const Encoded2D e = encode_2DxF(v1, v2, specialCounts);
  insertintoTLETrie_2D_88(root, e.combined, e.tle, static_cast<int>(e.ndims), curr_trie_size);
}

[[nodiscard]] std::unique_ptr<TLETrieNode_2D> execCreateAndInsert_2D(
    const FPHArray &array1, const FPHArray &array2, uint64_t &curr_trie_size,
    std::unique_ptr<SpecialCounts> &specialCounts);
[[nodiscard]] std::vector<char>
execSerialize_2D(TLETrieNode_2D *root, std::unique_ptr<SpecialCounts> &specialCounts);
[[nodiscard]] std::vector<char> generate_2DxF(const FPHArray &array1,
                                              const FPHArray &array2);

namespace airtree::core::schema::trie2d {

class Generator2DxF : public airtree::core::api::AirTreeGenerator {
public:
  [[nodiscard]] std::vector<char>
  generate(const std::vector<const FPHArray *> &arrays) const override;
};

} // namespace airtree::core::schema::trie2d


#endif // AIRTREE_CORE_SCHEMA_TRIE2D_2DXF_HPP