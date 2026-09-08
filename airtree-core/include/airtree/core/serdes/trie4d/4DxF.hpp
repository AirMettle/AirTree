// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_SERDES_TRIE4D_4DXFSER_HPP
#define AIRTREE_CORE_SERDES_TRIE4D_4DXFSER_HPP


#include <span>
#include <airtree/core/schema/trie4d/4DxF.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>

void serialize_4DxF(const TLE_4D_4x8 *node, std::vector<char> &buffer,
                    bool recursively = true);

void serialize_4DxF_l0(const Node4D_4x8_l0 *node, std::vector<char> &buffer,
                       bool recursively = true);

void serialize_4DxF_l1(const Node4D_4x8_l1 *node, std::vector<char> &buffer,
                       bool recursively = true);

[[nodiscard]] std::unique_ptr<TLE_4D_4x8>
deserialize_4DxF(std::span<const char> buffer, size_t &offset);

[[nodiscard]] std::unique_ptr<Node4D_4x8_l0>
deserialize_4DxF_l0(std::span<const char> buffer, size_t &offset, int level,
                    bool recursively = true);

[[nodiscard]] std::unique_ptr<Node4D_4x8_l1>
deserialize_4DxF_l1(std::span<const char> buffer, size_t &offset, int level,
                    bool recursively = true);

[[nodiscard]] std::unique_ptr<TrieNode_16>
deserialize_4DxF_l2(std::span<const char> buffer, size_t &offset, int level,
                    bool recursively = true);

[[nodiscard]] std::unique_ptr<TrieNode_16_Level1>
deserialize_4DxF_l3(std::span<const char> buffer, size_t &offset, int level);

[[nodiscard]] std::pair<std::unique_ptr<TLE_4D_4x8>, airtree::core::common::AirTreeHeader>
processBuffer_4DxF(std::span<const char> buffer);


#endif // AIRTREE_CORE_SERDES_TRIE4D_4DXFSER_HPP