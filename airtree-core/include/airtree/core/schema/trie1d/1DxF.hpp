#ifndef AIRTREE_CORE_SCHEMA_TRIE1D_1DXF_HPP
#define AIRTREE_CORE_SCHEMA_TRIE1D_1DXF_HPP


#include <airtree/core/common/Bins.hpp>
#include <airtree/core/common/FPHArray.hpp>
#include <airtree/core/common/SpecialCounts.hpp>
#include <airtree/core/api/AirTreeGenerator.hpp>
#include <memory>
#include <bitset>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
// =====================================================================================================================================================
//
//                                                              Apollo16 Config
//
// =====================================================================================================================================================
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────


/**
 * Definitions of constants for Apollo16 Config for the Trie
 *
 * BITS_8: Width of the top level node in the trie.(No of Bits used for
 * indexing) BITS_5: Width of the second level node in the trie.
 *
 * BINS_256: The size of buckets at Level 0. (Total will be 256
 * buckets as 2^8) BINS_32: The size of buckets at Level 1. (Total
 * will be 256 buckets as 2^8)
 */

struct TrieNode_16_Level1; // Forward Declaration

/**
 * Structure defining the Trie Node for Apollo16 at Level 0.
 *
 * @param populated Bitset to check if the bucket is populated.
 * @param nodes Array of unique pointers to the next level nodes.
 * @param counts Array to store the count of values in the bucket.
 */
struct TrieNode_16 {
  std::bitset<BINS_256> populated;
  std::unique_ptr<TrieNode_16_Level1> nodes[BINS_256];
  uint32_t counts[BINS_256] = {0};

  uint64_t size() const {
    return BINS_256;
  }
};

/**
 * Structure defining the Trie Node for Apollo16 at Level 1.
 *
 * @param counts Array to store the count of values in the bucket.
 */
struct TrieNode_16_Level1 {
  uint32_t counts[BINS_256] = {0}; // 4 * 32 = 128
};


/**
 * Function to generate the Apollo16 Trie.
 *
 * @param values Array of floating point values.
 * @param values_len Length of the values array.
 * @param default_mode Flag to check if filing is suppose to be in default mode
 * or not
 * @return Trie in the form of a serialized buffer of characters.
 */


/**
 * This function is used to Create root node of the Trie for Apollo16
 * Configuration.
 *
 * @return The root node of the Trie.
 */
std::unique_ptr<TrieNode_16> CreateParentNode_16();


/**
 * This function is used to file and insert a floating-point number into a trie.
 * (Apollo16 Configuration)
 *
 * @param node The root node of the Trie.
 * @param fpNumber The floating-point number to file to internal rep and insert.
 * @param default_mode The default mode to use.
 */
void createAndInsertFP16(TrieNode_16 *node, uint64_t fpNumber,
                         uint64_t &curr_trie_size, bool default_mode);
void createAndInsertFP16_32(TrieNode_16 *node, uint32_t fpNumber,
                            uint64_t &curr_trie_size, bool default_mode);

[[nodiscard]] std::vector<char> generate_1DxF(const FPHArray &array,
                                              bool default_mode = true);
[[nodiscard]] std::unique_ptr<TrieNode_16>
execCreateAndInsert_TrieNode16(SpecialCounts &specialCounts,
                               uint64_t &curr_trie_size, const FPHArray &array,
                               bool default_mode);
[[nodiscard]] std::vector<char>
execSerialization_TrieNode16(const std::unique_ptr<TrieNode_16> &root,
                             const SpecialCounts &specialCounts,
                             uint64_t trieSize, bool default_mode);

namespace airtree::core::schema::trie1d {


class Generate1DxF : public airtree::core::api::AirTreeGenerator {
public:
  [[nodiscard]] std::vector<char>
  generate(const std::vector<const FPHArray *> &arrays,
           bool default_mode) const override;
};

} // namespace airtree::core::schema::trie1d


#endif // AIRTREE_CORE_SCHEMA_TRIE1D_1DXF_HPP