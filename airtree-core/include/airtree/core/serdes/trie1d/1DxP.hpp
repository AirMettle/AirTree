// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_SCHEMA_TRIE1D_1DXPSER_HPP
#define AIRTREE_CORE_SCHEMA_TRIE1D_1DXPSER_HPP


#include <span>
#include <airtree/core/schema/trie1d/1DxP.hpp>
#include <memory>
#include <vector>
#include <airtree/core/common/AirTreeHeader.hpp>


void serialize_1DxP(const TrieNode_20 *node, std::vector<char> &buffer,
                    bool recursive = true);

void serialize_1DxP_l1(const TrieNode_20_Level1 *node,
                       std::vector<char> &buffer, bool recursive = true);

void serialize_1DxP_l2(const TrieNode_20_Level2 *node,
                       std::vector<char> &buffer);
std::unique_ptr<TrieNode_20> deserialize_1DxP(std::span<const char> buffer,
                                              size_t &offset);

std::unique_ptr<TrieNode_20_Level1>
deserialize_1DxP_l1(std::span<const char> buffer, size_t &offset,
                    bool recursive = true);
std::unique_ptr<TrieNode_20_Level2>
deserialize_1DxP_l2(std::span<const char> buffer, size_t &offset);

[[nodiscard]] std::pair<std::unique_ptr<TrieNode_20>, airtree::core::common::AirTreeHeader>
processBuffer_1DxP(std::span<const char> buffer);


#endif // AIRTREE_CORE_SCHEMA_TRIE1D_1DXPSER_HPP