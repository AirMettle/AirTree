#ifndef AIRTREE_CORE_SERDES_TRIE3D_3DXFSER_HPP
#define AIRTREE_CORE_SERDES_TRIE3D_3DXFSER_HPP

#include <airtree/core/schema/trie3d/3DxF.hpp>
#include <airtree/core/serdes/Header.hpp>

void serialize_3DxF(const TLE_3D_888 *node, std::vector<char> &buffer,
                    bool recursive = true);

void serialize_3DxF_l0(const Node3D_888_l0 *node, std::vector<char> &buffer,
                       bool recursive = true);

[[nodiscard]] std::pair<std::unique_ptr<TLE_3D_888>, trie_header>
processBuffer_3DxF(const std::vector<char> &buffer);

[[nodiscard]] std::unique_ptr<TLE_3D_888>
deserialize_3DxF(const std::vector<char> &buffer, size_t &offset);

[[nodiscard]] std::unique_ptr<Node3D_888_l0>
deserialize_3DxF_l0(const std::vector<char> &buffer, size_t &offset, int level,
                    bool recursive = true);

[[nodiscard]] std::unique_ptr<TrieNode_16>
deserialize_3DxF_l1(const std::vector<char> &buffer, size_t &offset, int level,
                    bool recursive = true);

[[nodiscard]] std::unique_ptr<TrieNode_16_Level1>
deserialize_3DxF_l2(const std::vector<char> &buffer, size_t &offset,
                    int level [[maybe_unused]]);

#endif // AIRTREE_CORE_SERDES_TRIE3D_3DXFSER_HPP