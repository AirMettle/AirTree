// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/serdes/trie4d/4DxF.hpp>
#include <airtree/core/serdes/BooleanArray.hpp>
#include <airtree/core/serdes/Count.hpp>
#include <airtree/core/serdes/Node.hpp>
#include <airtree/core/serdes/ND.hpp>
#include <airtree/core/serdes/EOF.hpp>
#include <airtree/core/common/NDims.hpp>

#include <airtree/core/Logger.hpp>

using namespace airtree::core;
using namespace airtree::core::common;

std::pair<std::unique_ptr<TLE_4D_4x8>, airtree::core::common::AirTreeHeader>
processBuffer_4DxF(const std::vector<char> &buffer) {

  auto header = airtree::core::common::deserializeHeader(buffer);
  size_t offset = header.header_length;

  std::unique_ptr<TLE_4D_4x8> node = nullptr;

  if (offset < buffer.size()) {
    node = deserialize_4DxF(buffer, offset);
  } else {
    SPDLOG_LOGGER_ERROR(
        logger(), "Deserialization failed. Offset is out of bounds.");
  }


  return std::make_pair(std::move(node), header);
}


void serialize_4DxF(const TLE_4D_4x8 *node, std::vector<char> &buffer,
                    bool recursively) {
  uint64_t mask[(BINS_4096 + 63) / 64];
  maskFromBitset(node->populated, mask);
  writeNode(mask, BINS_4096, node->counts, buffer);

  if (!recursively)
    return;

  for (size_t i = 0; i < BINS_4096; i++) {
    if (node->populated[i] && node->nodes[i]) {
      serialize_4DxF_l0(node->nodes[i].get(), buffer);
    }
  }

  // add end of file marker
  writeEndOfFileMarker(buffer);
}

void serialize_4DxF_l0(const Node4D_4x8_l0 *node, std::vector<char> &buffer,
                       bool recursively) {
  uint64_t mask[(BINS_256 + 63) / 64];
  maskFromBitset(node->populated, mask);
  writeNode(mask, BINS_256, node->counts, buffer);

  if (!recursively)
    return;

  for (size_t i = 0; i < BINS_256; i++) {
    if (node->populated[i] && node->nodes[i]) {
      serialize_4DxF_l1(node->nodes[i].get(), buffer);
    }
  }
}

void serialize_4DxF_l1(const Node4D_4x8_l1 *node, std::vector<char> &buffer,
                       bool recursively) {
  uint64_t mask[(BINS_256 + 63) / 64];
  maskFromBitset(node->populated, mask);
  writeNode(mask, BINS_256, node->counts, buffer);

  if (!recursively)
    return;

  for (size_t i = 0; i < BINS_256; i++) {
    if (node->populated[i] && node->nodes[i]) {
      serializeTrieNode_16_ND(node->nodes[i].get(), buffer);
    }
  }
}

std::unique_ptr<TLE_4D_4x8> deserialize_4DxF(const std::vector<char> &buffer,
                                             size_t &offset) {
  auto node = std::make_unique<TLE_4D_4x8>();
  uint64_t mask[(BINS_4096 + 63) / 64];
  if (!readPopulatedMask(buffer, offset, mask, BINS_4096)
      || !deserializeCounts(buffer, offset, mask, BINS_4096, node->counts)) {
    return nullptr;
  }
  setPopulated(node->populated, mask);

  // Recursively deserialize child nodes
  for (size_t i = 0; i < BINS_4096; i++) {
    if (node->populated[i]) {
      size_t nDims = getNumDims4D(i);
      switch (nDims) {
      case 0:
        continue;
      case 1:
      case 2:
      case 4:
      case 8:
        node->nodes[i] = deserialize_4DxF_l0(buffer, offset, 1);
        break;
      case 3:
      case 5:
      case 6:
      case 9:
      case 10:
      case 12:
        node->nodes[i] = deserialize_4DxF_l0(buffer, offset, 2);
        break;
      case 7:
      case 11:
      case 13:
      case 14:
        node->nodes[i] = deserialize_4DxF_l0(buffer, offset, 3);
        break;
      case 15:
        node->nodes[i] = deserialize_4DxF_l0(buffer, offset, 4);
        break;
      default:
        break;
      }
    }
  }

  // verify end of file marker
  if (!verifyEndOfFileMarker(buffer, offset)) {
    return nullptr;
  }
  return node;
}


std::unique_ptr<Node4D_4x8_l0>
deserialize_4DxF_l0(const std::vector<char> &buffer, size_t &offset, int level,
                    bool recursively) {
  auto node = std::make_unique<Node4D_4x8_l0>();
  uint64_t mask[(BINS_256 + 63) / 64];
  if (!readPopulatedMask(buffer, offset, mask, BINS_256)
      || !deserializeCounts(buffer, offset, mask, BINS_256, node->counts)) {
    return nullptr;
  }
  setPopulated(node->populated, mask);

  if (level == 1) {
    return node;
  }

  if (!recursively) {
    return node;
  }

  // Recursively deserialize child nodes
  for (size_t i = 0; i < BINS_256; i++) {
    if (node->populated[i]) {
      node->nodes[i] = deserialize_4DxF_l1(buffer, offset, level);
    }
  }

  return node;
}

std::unique_ptr<Node4D_4x8_l1>
deserialize_4DxF_l1(const std::vector<char> &buffer, size_t &offset, int level,
                    bool recursively) {
  auto node = std::make_unique<Node4D_4x8_l1>();
  uint64_t mask[(BINS_256 + 63) / 64];
  if (!readPopulatedMask(buffer, offset, mask, BINS_256)
      || !deserializeCounts(buffer, offset, mask, BINS_256, node->counts)) {
    return nullptr;
  }
  setPopulated(node->populated, mask);

  if (level == 2) {
    return node;
  }

  if (!recursively) {
    return node;
  }

  // Recursively deserialize child nodes
  for (size_t i = 0; i < BINS_256; i++) {
    if (node->populated[i]) {
      node->nodes[i] = deserialize_4DxF_l2(buffer, offset, level);
    }
  }

  return node;
}

std::unique_ptr<TrieNode_16>
deserialize_4DxF_l2(const std::vector<char> &buffer, size_t &offset, int level,
                    bool recursively) {
  auto node = std::make_unique<TrieNode_16>();
  uint64_t mask[(BINS_256 + 63) / 64];
  if (!readPopulatedMask(buffer, offset, mask, BINS_256)
      || !deserializeCounts(buffer, offset, mask, BINS_256, node->counts)) {
    return nullptr;
  }
  setPopulated(node->populated, mask);

  if (level == 3) {
    return node;
  }

  if (!recursively) {
    return node;
  }

  // Recursively deserialize child nodes
  for (size_t i = 0; i < BINS_256; i++) {
    if (node->populated[i]) {
      node->nodes[i] = deserialize_4DxF_l3(buffer, offset, level);
    }
  }

  return node;
}

std::unique_ptr<TrieNode_16_Level1>
deserialize_4DxF_l3(const std::vector<char> &buffer, size_t &offset,
                    int level [[maybe_unused]]) {
  auto node = std::make_unique<TrieNode_16_Level1>();
  uint64_t mask[(BINS_256 + 63) / 64];
  if (!readPopulatedMask(buffer, offset, mask, BINS_256)
      || !deserializeCounts(buffer, offset, mask, BINS_256, node->counts)) {
    return nullptr;
  }

  return node;
}