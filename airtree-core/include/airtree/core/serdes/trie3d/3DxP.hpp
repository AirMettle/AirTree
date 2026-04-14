#ifndef AIRTREE_CORE_SERDES_TRIE3D_3DXPSER_HPP
#define AIRTREE_CORE_SERDES_TRIE3D_3DXPSER_HPP

#include <airtree/core/schema/trie3d/3DxP.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>


void serialize_3DxP(const TLE_3D_3x10 *node, std::vector<char> &buffer,
                    bool recursive = true);

void serialize_3DxP_l0(const Node3D_3x10_l0 *node, std::vector<char> &buffer,
                       bool recursive = true);

void serialize_3DxP_l1(const Node3D_3x10_l1 *node, std::vector<char> &buffer,
                       bool recursive = true);

void serialize_3DxP_l2(const Node3D_3x10_l2 *node, std::vector<char> &buffer);

[[nodiscard]] std::pair<std::unique_ptr<TLE_3D_3x10>, airtree::core::common::AirTreeHeader>
processBuffer_3DxP(const std::vector<char> &buffer);

[[nodiscard]] std::unique_ptr<TLE_3D_3x10>
deserialize_3DxP(const std::vector<char> &buffer, size_t &offset);

[[nodiscard]] std::unique_ptr<Node3D_3x10_l0>
deserialize_3DxP_l0(const std::vector<char> &buffer, size_t &offset, int level,
                    bool recursive = true);

[[nodiscard]] std::unique_ptr<Node3D_3x10_l1>
deserialize_3DxP_l1(const std::vector<char> &buffer, size_t &offset, int level,
                    bool recursive = true);

[[nodiscard]] std::unique_ptr<Node3D_3x10_l2>
deserialize_3DxP_l2(const std::vector<char> &buffer, size_t &offset);

#endif // AIRTREE_CORE_SERDES_TRIE3D_3DXPSER_HPP