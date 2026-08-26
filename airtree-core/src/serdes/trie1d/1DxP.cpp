// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/serdes/trie1d/1DxP.hpp>
#include <airtree/core/common/BooleanArray.hpp>
#include <airtree/core/serdes/BooleanArray.hpp>
#include <airtree/core/serdes/Count.hpp>
#include <airtree/core/serdes/Node.hpp>
#include <airtree/core/serdes/EOF.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/Logger.hpp>

using namespace airtree::core;
using namespace airtree::core::common;

void serialize_1DxP(const TrieNode_20 *node,
                                    std::vector<char> &buffer, bool recursive) {

  // Convert populated bitset to compact BooleanArray
  uint64_t mask[(BINS_256 + 63) / 64];
  maskFromBitset(node->populated, mask);
  writeNode(mask, BINS_256, node->counts, buffer);

  if (!recursive)
    return;

  // Recursively serialize child nodes
  for (size_t i = 0; i < BINS_256; i++) {
    if (node->populated[i] && node->nodes[i]) {
      serialize_1DxP_l1(node->nodes[i].get(), buffer);
    }
  }

  // add end of file marker
  writeEndOfFileMarker(buffer);
}

void serialize_1DxP_l1(const TrieNode_20_Level1 *node,
                                           std::vector<char> &buffer,
                                           bool recursive) {
  // Convert populated bitset to compact BooleanArray
  uint64_t mask[(BINS_64 + 63) / 64];
  maskFromBitset(node->populated, mask);
  writeNode(mask, BINS_64, node->counts, buffer);

  if (!recursive)
    return;

  // Recursively serialize child nodes
  for (size_t i = 0; i < BINS_64; i++) {
    if (node->populated[i] && node->nodes[i]) {
      serialize_1DxP_l2(node->nodes[i].get(), buffer);
    }
  }
}

void serialize_1DxP_l2(const TrieNode_20_Level2 *node,
                                           std::vector<char> &buffer) {
  // Build compact boolean array to represent populated bitset
  uint64_t mask[(BINS_64 + 63) / 64];
  maskFromCounts(node->counts, BINS_64, mask);
  writeNode(mask, BINS_64, node->counts, buffer);
}

std::unique_ptr<TrieNode_20_Level1>
deserialize_1DxP_l1(const std::vector<char> &buffer, size_t &offset,
                    bool recursive) {
  uint64_t mask[(BINS_64 + 63) / 64];
  auto node = std::make_unique<TrieNode_20_Level1>();
  if (!readPopulatedMask(buffer, offset, mask, BINS_64)
      || !deserializeCounts(buffer, offset, mask, BINS_64, node->counts)) {
    return nullptr;
  }
  setPopulated(node->populated, mask);
  if (!recursive)
    return node;
  for (size_t i = 0; i < BINS_64; i++) {
    if (!node->populated[i])
      continue;
    node->nodes[i] = deserialize_1DxP_l2(buffer, offset);
    if (!node->nodes[i]) {
      SPDLOG_LOGGER_ERROR(
          logger(), "Deserialization of child node failed at index {}", i);
      return nullptr;
    }
  }
  return node;
}

std::unique_ptr<TrieNode_20>
deserialize_1DxP(const std::vector<char> &buffer, size_t &offset) {
  uint64_t mask[(BINS_256 + 63) / 64];
  auto node = std::make_unique<TrieNode_20>();
  if (!readPopulatedMask(buffer, offset, mask, BINS_256)
      || !deserializeCounts(buffer, offset, mask, BINS_256, node->counts)) {
    return nullptr;
  }
  setPopulated(node->populated, mask);
  for (size_t i = 0; i < BINS_256; i++) {
    if (!node->populated[i])
      continue;
    node->nodes[i] = deserialize_1DxP_l1(buffer, offset);
    if (!node->nodes[i]) {
      SPDLOG_LOGGER_ERROR(
          logger(), "Deserialization of child node failed at index {}", i);
      return nullptr;
    }
  }
  if (!verifyEndOfFileMarker(buffer, offset)) {
    SPDLOG_LOGGER_ERROR(
        logger(),
        "NODE DESERIALIZATION ERROR : End marker not found at expected "
        "position.");
    return nullptr;
  }
  return node;
}

std::unique_ptr<TrieNode_20_Level2>
deserialize_1DxP_l2(const std::vector<char> &buffer, size_t &offset) {
  uint64_t mask[(BINS_64 + 63) / 64];
  auto node = std::make_unique<TrieNode_20_Level2>();
  if (!readPopulatedMask(buffer, offset, mask, BINS_64)
      || !deserializeCounts(buffer, offset, mask, BINS_64, node->counts)) {
    return nullptr;
  }
  return node;
}

std::pair<std::unique_ptr<TrieNode_20>, airtree::core::common::AirTreeHeader>
processBuffer_1DxP(const std::vector<char> &buffer) {
  auto header = airtree::core::common::deserializeHeader(buffer);
  size_t offset = header.header_length;

  std::unique_ptr<TrieNode_20> root = nullptr;

  if (offset < buffer.size()) {
    root = deserialize_1DxP(buffer, offset);

    if (root.get() == nullptr) {
      SPDLOG_LOGGER_ERROR(
          logger(), "Deserialization resulted in a null root node.");
    }
  } else {
    SPDLOG_LOGGER_ERROR(logger(), "Insufficient buffer size for trie data.");
  }

  return std::make_pair(std::move(root), header);
}