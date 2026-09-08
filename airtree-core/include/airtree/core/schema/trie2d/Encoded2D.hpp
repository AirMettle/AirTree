// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)
#ifndef AIRTREE_CORE_SCHEMA_TRIE2D_ENCODED2D_HPP
#define AIRTREE_CORE_SCHEMA_TRIE2D_ENCODED2D_HPP

// Where one (v1, v2) point lands in a 2D trie: its top-level slot, its packed code (both axes when
// both are finite, the finite one otherwise) and `ndims`, a mask of the finite axes (bit 1 = v1,
// bit 0 = v2; 0 = both special, so no code).
struct Encoded2D {
  unsigned int tle;
  unsigned int combined;
  unsigned int ndims;
};

#endif // AIRTREE_CORE_SCHEMA_TRIE2D_ENCODED2D_HPP
