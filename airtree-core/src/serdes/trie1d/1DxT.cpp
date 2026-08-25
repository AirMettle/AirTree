// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <cstddef>
#include <airtree/core/serdes/trie1d/1DxT.hpp>
#include <airtree/core/common/BooleanArray.hpp>
#include <airtree/core/serdes/BooleanArray.hpp>
#include <airtree/core/serdes/Count.hpp>
#include <airtree/core/serdes/EOF.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/Logger.hpp>

using namespace airtree::core;
using namespace airtree::core::common;


void serialize_1DxT(const TrieNode_13 *node,
                                     std::vector<char> &buffer,
                                     bool recursive) {
  // Convert populated bitset to compact BooleanArray
  BooleanArray compact_array = BooleanArray(BINS_256 / 64);
  for (size_t i = 0; i < BINS_256; i++) {
    if (node->populated[i]) {
      compact_array.set(i, true);
    }
  }
  // Store compact array to buffer
  auto compact_array_buffer = serializeCompactBooleanArray(compact_array);
  buffer.insert(
      buffer.end(), compact_array_buffer.begin(), compact_array_buffer.end());

  // Store count for buckets with populated bit set
  auto counts_buffer = serializeCounts(node->counts, BINS_256);
  buffer.insert(buffer.end(), counts_buffer.begin(), counts_buffer.end());

  if (!recursive)
    return;


  // Recursively serialize child nodes
  for (size_t i = 0; i < BINS_256; i++) {
    if (node->populated[i] && node->nodes[i]) {
      serialize_1DxT_l1(node->nodes[i].get(), buffer);
    }
  }

  // Add end of file marker
  int32_t endOfFileMarker = -1;
  auto marker_bytes = reinterpret_cast<const char *>(&endOfFileMarker);
  buffer.insert(
      buffer.end(), marker_bytes, marker_bytes + sizeof(endOfFileMarker));
}

void serialize_1DxT_l1(const TrieNode_13_Level1 *node,
                                            std::vector<char> &buffer) {
  // Build compact boolean array to represent populated bitset
  BooleanArray compact_array = BooleanArray(1);
  for (size_t i = 0; i < BINS_32; i++) {
    if (node->counts[i] > 0) {
      compact_array.set(i, true);
    }
  }
  // Store compact array to buffer
  auto compact_array_buffer = serializeCompactBooleanArray(compact_array);
  buffer.insert(
      buffer.end(), compact_array_buffer.begin(), compact_array_buffer.end());

  // Store counts for buckets with count > 0
  auto counts_buffer = serializeCounts(node->counts, BINS_32);
  buffer.insert(buffer.end(), counts_buffer.begin(), counts_buffer.end());
}


std::unique_ptr<TrieNode_13_Level1>
deserialize_1DxT_l1(const std::vector<char> &buffer, size_t &offset) {
  uint64_t mask[(BINS_32 + 63) / 64];
  auto node = std::make_unique<TrieNode_13_Level1>();
  if (!readPopulatedMask(buffer, offset, mask, BINS_32)
      || !deserializeCounts(buffer, offset, mask, BINS_32, node->counts)) {
    return nullptr;
  }
  return node;
}

std::unique_ptr<TrieNode_13>
deserialize_1DxT(const std::vector<char> &buffer, size_t &offset,
                 bool recursive) {
  uint64_t mask[(BINS_256 + 63) / 64];
  auto node = std::make_unique<TrieNode_13>();
  if (!readPopulatedMask(buffer, offset, mask, BINS_256)
      || !deserializeCounts(buffer, offset, mask, BINS_256, node->counts)) {
    return nullptr;
  }
  setPopulated(node->populated, mask);
  if (!recursive)
    return node;
  for (size_t i = 0; i < BINS_256; i++) {
    if (!node->populated[i])
      continue;
    node->nodes[i] = deserialize_1DxT_l1(buffer, offset);
    if (!node->nodes[i]) {
      SPDLOG_LOGGER_ERROR(
          logger(), "Deserialization of child node failed at index {}", i);
      return nullptr;
    }
  }
  if (!verifyEndOfFileMarker(buffer, offset)) {
    return nullptr;
  }
  return node;
}

std::pair<std::unique_ptr<TrieNode_13>, airtree::core::common::AirTreeHeader>
processBuffer_1DxT(const std::vector<char> &buffer) {
  auto header = airtree::core::common::deserializeHeader(buffer);
  size_t offset = header.header_length;

  std::unique_ptr<TrieNode_13> root = nullptr;

  if (offset < buffer.size()) {
    root = deserialize_1DxT(buffer, offset);

    if (root.get() == nullptr) {
      SPDLOG_LOGGER_ERROR(
          logger(), "Deserialization resulted in a null root node.");
    }
  } else {
    SPDLOG_LOGGER_ERROR(logger(), "Insufficient buffer size for trie data.");
  }

  return std::make_pair(std::move(root), header);
}
