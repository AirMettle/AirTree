// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_SCHEMA_TRIE2D_2DXP_HPP
#define AIRTREE_CORE_SCHEMA_TRIE2D_2DXP_HPP

#include <bitset>
#include <airtree/core/common/Populated.hpp>
#include <airtree/core/common/Bins.hpp>
#include <airtree/core/common/FPHArray.hpp>
#include <airtree/core/common/SpecialCounts.hpp>
#include <airtree/core/api/AirTreeGenerator.hpp>
#include <airtree/core/common/BitCodec.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/schema/trie2d/Encoded2D.hpp>
#include <memory>
#include <vector>


struct TrieNode_2D_10_Level1 {
  uint32_t counts[BINS_1024] = {0};
};

struct TrieNode_2D_10 {
  PopulatedBins<BINS_1024> populated;
  uint32_t counts[BINS_1024] = {0};
  std::unique_ptr<TrieNode_2D_10_Level1> nodes[BINS_1024];
};

struct TLEoption3_2D {
  PopulatedBins<BINS_64> populated;
  uint32_t counts[BINS_64] = {0};
  std::unique_ptr<TrieNode_2D_10> nodes[BINS_64];
};


/**
* Function to generate 2-D trie from the input values

* @param values1 : Array of input values for the first dimension
* @param values2 : Array of input values for the second dimension
* @param values_len1 : Length of the values1 array
* @param values_len2 : Length of the values2 array
* @return Trie in serialized buffer of characters.
*/

[[nodiscard]] std::unique_ptr<TLEoption3_2D> CreateParent_TLE2D_option3();

// Files one (v1, v2) point: counts its specials into specialCounts and returns where it lands.
template <typename T1, typename T2>
inline Encoded2D encode_2DxP(T1 v1, T2 v2,
                             std::unique_ptr<SpecialCounts> &specialCounts) {
  const std::pair<TLE, unsigned int> in1 = internal_10bit(v1);
  const std::pair<TLE, unsigned int> in2 = internal_10bit(v2);
  const unsigned int special1 = update_special_counts(in1.first, specialCounts);
  const unsigned int special2 = update_special_counts(in2.first, specialCounts);
  Encoded2D e{static_cast<unsigned int>((in1.first.encoding << 3) | in2.first.encoding), 0,
              (~((special1 << 1) | special2)) & 0x3u};
  switch (e.ndims) {
  case 1: e.combined = in2.second; break;
  case 2: e.combined = in1.second; break;
  case 3: e.combined = static_cast<unsigned int>(combine_chunks_10b(in1.second, in2.second)); break;
  default: break;
  }
  return e;
}

// Inserts one encoded point (count 1); allocates nodes on first use and grows curr_trie_size.
void insertintoTLETrie_2D_option3(TLEoption3_2D *root, unsigned int combined,
                                  unsigned int combinedTLE, int ndims,
                                  uint64_t &curr_trie_size);

// One point into the trie: what execCreateAndInsert_2D_2x10 does per element.
template <typename T1, typename T2>
inline void createAndInsert_2DxP(TLEoption3_2D *root, T1 v1, T2 v2, uint64_t &curr_trie_size,
                                 std::unique_ptr<SpecialCounts> &specialCounts) {
  const Encoded2D e = encode_2DxP(v1, v2, specialCounts);
  insertintoTLETrie_2D_option3(root, e.combined, e.tle, static_cast<int>(e.ndims), curr_trie_size);
}

[[nodiscard]] std::unique_ptr<TLEoption3_2D> execCreateAndInsert_2D_2x10(
    const FPHArray &array1, const FPHArray &array2, uint64_t &curr_trie_size,
    std::unique_ptr<SpecialCounts> &specialCounts);

[[nodiscard]] std::vector<char>
execSerialize_2D_2x10(TLEoption3_2D *root, std::unique_ptr<SpecialCounts> &specialCounts);


[[nodiscard]] std::vector<char> generate_2DxP(const FPHArray &array1,
                                              const FPHArray &array2);
namespace airtree::core::schema::trie2d {

class Generator2DxP : public airtree::core::api::AirTreeGenerator {
public:
  [[nodiscard]] std::vector<char>
  generate(const std::vector<const FPHArray *> &arrays) const override;
};

} // namespace airtree::core::schema::trie2d

#endif // AIRTREE_CORE_SCHEMA_TRIE2D_2DXP_HPP