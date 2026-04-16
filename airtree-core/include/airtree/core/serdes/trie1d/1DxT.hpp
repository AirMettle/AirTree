#ifndef AIRTREE_CORE_SCHEMA_TRIE1D_1DXTSER_HPP
#define AIRTREE_CORE_SCHEMA_TRIE1D_1DXTSER_HPP


#include <airtree/core/schema/trie1d/1DxT.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>


void serialize_1DxT(const TrieNode_13 *node, std::vector<char> &buffer,
                    bool recursive = true);

void serialize_1DxT_l1(const TrieNode_13_Level1 *node,
                       std::vector<char> &buffer);

std::unique_ptr<TrieNode_13> deserialize_1DxT(const std::vector<char> &buffer,
                                              size_t &offset,
                                              bool recursive = true);

std::unique_ptr<TrieNode_13_Level1>
deserialize_1DxT_l1(const std::vector<char> &buffer, size_t &offset);

[[nodiscard]] std::pair<std::unique_ptr<TrieNode_13>, airtree::core::common::AirTreeHeader>
processBuffer_1DxT(const std::vector<char> &buffer);


#endif // AIRTREE_CORE_SCHEMA_TRIE1D_1DXTSER_HPP