// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_SERDES_TRIE2D_2DXFSER_HPP
#define AIRTREE_CORE_SERDES_TRIE2D_2DXFSER_HPP


#include <span>
#include <airtree/core/schema/trie2d/2DxF.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>

void serialize_2DxF(const TLETrieNode_2D *node, std::vector<char> &buffer,
                    bool recursive = true);


[[nodiscard]] std::unique_ptr<TLETrieNode_2D>
deserialize_2DxF(std::span<const char> buffer, size_t &offset);

[[nodiscard]] std::unique_ptr<TrieNode_16>
deserialize_2DxF_l0(std::span<const char> buffer, size_t &offset, int level,
                    bool recursive = true);

std::unique_ptr<TrieNode_16_Level1>
deserialize_2DxF_l1(std::span<const char> buffer, size_t &offset,
                    int level [[maybe_unused]]);

[[nodiscard]] std::pair<std::unique_ptr<TLETrieNode_2D>, airtree::core::common::AirTreeHeader>
processBuffer_2DxF(std::span<const char> buffer);

#endif // AIRTREE_CORE_SERDES_TRIE2D_2DXFSER_HPP