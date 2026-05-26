// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_SERDES_TRIE1D_1DXFSER_HPP
#define AIRTREE_CORE_SERDES_TRIE1D_1DXFSER_HPP


#include <airtree/core/schema/trie1d/1DxF.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>

void serialize_1DxF(const TrieNode_16 *node, std::vector<char> &buffer,
                    bool recursive = true);

void serialize_1DxF_l1(const TrieNode_16_Level1 *node,
                       std::vector<char> &buffer);
std::unique_ptr<TrieNode_16> deserialize_1DxF(const std::vector<char> &buffer,
                                              size_t &offset);

std::unique_ptr<TrieNode_16_Level1>
deserialize_1DxF_l1(const std::vector<char> &buffer, size_t &offset);

[[nodiscard]] std::pair<std::unique_ptr<TrieNode_16>, airtree::core::common::AirTreeHeader>
processBuffer_1DxF(const std::vector<char> &buffer);


#endif // AIRTREE_CORE_SERDES_TRIE1D_1DXFSER_HPP