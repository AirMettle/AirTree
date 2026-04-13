#ifndef AIRTREE_CORE_COMMON_TRIEHEADER_HPP
#define AIRTREE_CORE_COMMON_TRIEHEADER_HPP

/**
 * Structure defining the header for a trie.
 *
 * @param type_code Identifies the file or data structure type; expected to be
 * "HierFPHG".
 * @param version Version of the trie.
 * @param m_width Width of the magnitude in int.
 * @param precision_bits Number of bits used for precision.
 * @param node_width Width of top level node in the trie.
 * @param type Dimension of the trie. (1-D, 2-D, 3-D, 4-D, etc.)
 * @param mode Checks if the trie is in default mode or not.
 * @param pos_inf_count Count of positive infinity values stored.
 * @param neg_inf_count Count of negative infinity values stored.
 * @param pos_zero_count Count of positive zero values stored.
 * @param neg_zero_count Count of negative zero values stored.
 * @param nan_count Count of NaN (Not a Number) values stored.
 * @param trie_root_ref Reference or pointer to the root node of the trie.
 */
struct trie_header {
  char type_code[9];
  int version = 0;
  int m_width;
  int precision_bits;
  int node_width;
  char type[4];
  char config[4];
  int mode;
  int pos_inf_count = 0;
  int neg_inf_count = 0;
  int pos_zero_count = 0;
  int neg_zero_count = 0;
  int nan_count = 0;
  int trie_root_ref;
};


#endif // AIRTREE_CORE_COMMON_TRIEHEADER_HPP