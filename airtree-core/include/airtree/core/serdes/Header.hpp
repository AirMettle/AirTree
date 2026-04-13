#ifndef AIRTREE_CORE_SERDES_HEADER_SER_HPP
#define AIRTREE_CORE_SERDES_HEADER_SER_HPP

#include <cstddef>
#include <vector>

#include <airtree/core/common/TrieHeader.hpp>


void serializeTrieHeader(const trie_header &header, std::vector<char> &buffer);
trie_header deserializeTrieHeader(const std::vector<char> &buffer,
                                  size_t &offset);

trie_header deserializeTrieHeader(const std::vector<char> &buffer);

#endif // AIRTREE_CORE_SERDES_HEADER_SER_HPP