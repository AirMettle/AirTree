// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_SCHEMA_TRIE2D_2DXPSER_CPP
#define AIRTREE_CORE_SCHEMA_TRIE2D_2DXPSER_CPP


#include <airtree/core/schema/trie2d/2DxP.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>


void serialize_2DxP(const TLEoption3_2D *node, std::vector<char> &buffer,
                    bool recursive = true);

void serialize_2DxP_l0(const TrieNode_2D_10 *node, std::vector<char> &buffer,
                       bool recursive = true);

void serialize_2DxP_l1(const TrieNode_2D_10_Level1 *node,
                       std::vector<char> &buffer);

[[nodiscard]] std::pair<std::unique_ptr<TLEoption3_2D>, airtree::core::common::AirTreeHeader>
processBuffer_2DxP(const std::vector<char> &buffer);

std::unique_ptr<TLEoption3_2D> deserialize_2DxP(std::vector<char> buffer,
                                                size_t &offset);

[[nodiscard]] std::unique_ptr<TrieNode_2D_10>
deserialize_2DxP_l0(const std::vector<char> &buffer, size_t &offset, int level,
                    bool recursive = true);

std::unique_ptr<TrieNode_2D_10_Level1>
deserialize_2DxP_l1(const std::vector<char> &buffer, size_t &offset);

#endif // AIRTREE_CORE_SCHEMA_TRIE2D_2DXPSER_CPP