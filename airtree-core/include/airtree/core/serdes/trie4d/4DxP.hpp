// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_SERDES_TRIE4D_4DXPSER_HPP
#define AIRTREE_CORE_SERDES_TRIE4D_4DXPSER_HPP

#include <airtree/core/schema/trie4d/4DxP.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>

void serialize_4DxP(const TLE_4D_4x10 *node, std::vector<char> &buffer,
                    bool recursive = true);
[[nodiscard]] std::unique_ptr<TLE_4D_4x10>
deserialize_4DxP(std::vector<char> buffer, size_t &offset);
[[nodiscard]] std::pair<std::unique_ptr<TLE_4D_4x10>, airtree::core::common::AirTreeHeader>
processBuffer_4DxP(const std::vector<char> &buffer);

void serialize_4DxP_l0(const Node4D_4x10_l0 *node, std::vector<char> &buffer,
                       bool recursive = true);
void serialize_4DxP_l1(const Node4D_4x10_l1 *node, std::vector<char> &buffer,
                       bool recursive = true);
void serialize_4DxP_l2(const Node4D_4x10_l2 *node, std::vector<char> &buffer,
                       bool recursive = true);
void serialize_4DxP_l3(const Node4D_4x10_l3 *node, std::vector<char> &buffer);

[[nodiscard]] std::unique_ptr<Node4D_4x10_l0>
deserialize_4DxP_l0(const std::vector<char> &buffer, size_t &offset, int levels,
                    bool recursive = true);
[[nodiscard]] std::unique_ptr<Node4D_4x10_l1>
deserialize_4DxP_l1(const std::vector<char> &buffer, size_t &offset, int levels,
                    bool recursive = true);
[[nodiscard]] std::unique_ptr<Node4D_4x10_l2>
deserialize_4DxP_l2(const std::vector<char> &buffer, size_t &offset, int levels,
                    bool recursive = true);
[[nodiscard]] std::unique_ptr<Node4D_4x10_l3>
deserialize_4DxP_l3(const std::vector<char> &buffer, size_t &offset,
                    int levels);

#endif // AIRTREE_CORE_SERDES_TRIE4D_4DXPSER_HPP