// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)
#ifndef AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_NWAYMERGE_HPP
#define AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_NWAYMERGE_HPP

#include <airtree/core/common/AirTreeHeader.hpp>
#include <vector>

namespace airtree::merge {

// Bins per trie level (root first) when `config` is merged by streaming; empty otherwise.
std::vector<size_t> streamingLevels(airtree::core::common::ConfigWire config);

// Single-pass merge of buffers whose headers are already parsed and config-checked.
std::vector<char> mergeStreaming(const std::vector<std::vector<char>> &buffers,
                                 const std::vector<airtree::core::common::AirTreeHeader> &headers,
                                 const std::vector<size_t> &levels);

} // namespace airtree::merge

#endif // AIRTREE_MERGE_INCLUDE_AIRTREE_MERGE_NWAYMERGE_HPP
