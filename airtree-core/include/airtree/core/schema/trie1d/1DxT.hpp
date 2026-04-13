#ifndef AIRTREE_CORE_SCHEMA_TRIE1D_1DXT_HPP
#define AIRTREE_CORE_SCHEMA_TRIE1D_1DXT_HPP

#include <airtree/core/common/Bins.hpp>
#include <airtree/core/common/FPHArray.hpp>
#include <airtree/core/common/SpecialCounts.hpp>
#include <memory>
#include <bitset>
#include <vector>


// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
// =====================================================================================================================================================
//
//                                                              13Colonies
//                                                              Config
//
// =====================================================================================================================================================
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────

/**
 * Definitions of constants for 13Colonies Config for the Trie
 *
 * BITS_8: Width of the top level node in the trie.(No of Bits used for
 * indexing) BITS_5: Width of the second level node in the trie.
 *
 * BINS_256: The size of buckets at Level 0. (Total will be 256
 * buckets as 2^8) BINS_32: The size of buckets at Level 1. (Total
 * will be 32 buckets as 2^5)
 */

struct TrieNode_13;        // Forward Declaration
struct TrieNode_13_Level1; // Forward Declaration

/**
 * Structure defining the Trie Node for 13 Colonies.
 *
 * @param populated Bitset to check if the bucket is populated.
 * @param nodes Array of unique pointers to the next level nodes.
 * @param counts Array to store the count of values in the bucket.
 */
struct TrieNode_13 {
  std::bitset<BINS_256> populated;
  std::unique_ptr<TrieNode_13_Level1> nodes[BINS_256];
  uint32_t counts[BINS_256] = {0};

  uint64_t size() const {
    return BINS_256;
  }
};

/**
 * Structure defining the Trie Node for 13 Colonies at Level 1.
 *
 * @param counts Array to store the count of values in the bucket.
 */
struct TrieNode_13_Level1 {
  uint32_t counts[BINS_32] = {0};
};

/**
 * Function to generate the 13Colonies Trie.
 *
 * @param values Array of floating point values.
 * @param values_len Length of the values array.
 * @param default_mode Flag to check if filing is suppose to be in default mode
 * or not
 * @return Trie in the form of a serialized buffer of characters.
 */


/**
 * This function is used to Create root node of the Trie for 13Colonies
 * Configuration.
 *
 * @return The root node of the Trie.
 */
std::unique_ptr<TrieNode_13> CreateParentNode();

[[nodiscard]] std::vector<char> generate_1DxT(const FPHArray &array,
                                              bool default_mode = true);

[[nodiscard]] std::unique_ptr<TrieNode_13>
execCreateAndInsert_TrieNode13(SpecialCounts &specialCounts,
                               uint64_t &curr_trie_size, const FPHArray &array,
                               bool default_mode);

[[nodiscard]] std::vector<char>
execSerialization_TrieNode13(const std::unique_ptr<TrieNode_13> &root,
                             const SpecialCounts &specialCounts,
                             uint64_t trieSize, bool default_mode);


/**
 * This function is used to file and insert a floating-point number into a trie.
 * (13Colonies Configuration)
 *
 * @param node The root node of the Trie.
 * @param fpNumber The floating-point number to file to internal rep and insert.
 * @param default_mode The default mode to use.
 */

void createAndInsertFP(TrieNode_13 *node, uint64_t fpNumber,
                       uint64_t &curr_trie_size, bool default_mode);
void createAndInsertFP_32(TrieNode_13 *node, uint32_t fpNumber,
                          uint64_t &curr_trie_size, bool default_mode);


#endif // AIRTREE_CORE_SCHEMA_TRIE1D_1DXT_HPP
