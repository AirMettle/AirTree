// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/serdes/trie3d/3DxF.hpp>
#include <airtree/core/serdes/BooleanArray.hpp>
#include <airtree/core/serdes/Count.hpp>
#include <airtree/core/serdes/Node.hpp>
#include <airtree/core/serdes/ND.hpp>
#include <airtree/core/serdes/EOF.hpp>
#include <airtree/core/common/NDims.hpp>

#include <airtree/core/Logger.hpp>

using namespace airtree::core;
using namespace airtree::core::common;

std::pair<std::unique_ptr<TLE_3D_888>, airtree::core::common::AirTreeHeader>
processBuffer_3DxF(std::span<const char> buffer) {

  auto header = airtree::core::common::deserializeHeader(buffer);
  size_t offset = header.header_length;

  std::unique_ptr<TLE_3D_888> node = nullptr;

  if (offset < buffer.size()) {
    node = deserialize_3DxF(buffer, offset);
  } else {
    SPDLOG_LOGGER_ERROR(
        logger(), "Deserialization failed. Offset is out of bounds.");
  }

  return std::make_pair(std::move(node), header);
}

void serialize_3DxF_l0(const Node3D_888_l0 *node, std::vector<char> &buffer,
                       bool recursive) {
  writeNode(node->populated.words, BINS_256, node->counts, buffer);

  if (!recursive)
    return;

  forEachSetBit(node->populated.words, BINS_256, [&](size_t i) {
    if (node->nodes[i])
      serializeTrieNode_16_ND(node->nodes[i].get(), buffer);
  });
}

void serialize_3DxF(const TLE_3D_888 *node, std::vector<char> &buffer,
                    bool recursive) {
  writeNode(node->populated.words, BINS_512, node->counts, buffer);

  if (!recursive)
    return;

  forEachSetBit(node->populated.words, BINS_512, [&](size_t i) {
    if (node->nodes[i])
      serialize_3DxF_l0(node->nodes[i].get(), buffer);
  });

  // add end of file marker
  writeEndOfFileMarker(buffer);
}


std::unique_ptr<TrieNode_16_Level1>
deserialize_3DxF_l2(std::span<const char> buffer, size_t &offset,
                    int level [[maybe_unused]]) {
  auto node = std::make_unique<TrieNode_16_Level1>();
  uint64_t mask[(BINS_256 + 63) / 64];
  if (!readPopulatedMask(buffer, offset, mask, BINS_256)
      || !deserializeCounts(buffer, offset, mask, BINS_256, node->counts)) {
    return nullptr;
  }

  return node;
}

std::unique_ptr<TrieNode_16>
deserialize_3DxF_l1(std::span<const char> buffer, size_t &offset, int level,
                    bool recursive) {
  auto node = std::make_unique<TrieNode_16>();
  uint64_t mask[(BINS_256 + 63) / 64];
  if (!readPopulatedMask(buffer, offset, mask, BINS_256)
      || !deserializeCounts(buffer, offset, mask, BINS_256, node->counts)) {
    return nullptr;
  }
  setPopulated(node->populated, mask);

  if (level == 2) {
    return node;
  }

  if (!recursive)
    return node;

  // Recursively deserialize child nodes
  for (size_t w = 0; w < PopulatedBins<BINS_256>::kWords; ++w)
    for (uint64_t m = mask[w]; m != 0; m &= m - 1) {
      const size_t i = w * 64 + std::countr_zero(m);
    node->nodes[i] = deserialize_3DxF_l2(buffer, offset, level);
    }

  return node;
}

std::unique_ptr<Node3D_888_l0>
deserialize_3DxF_l0(std::span<const char> buffer, size_t &offset, int level,
                    bool recursive) {
  auto node = std::make_unique<Node3D_888_l0>();
  uint64_t mask[(BINS_256 + 63) / 64];
  if (!readPopulatedMask(buffer, offset, mask, BINS_256)
      || !deserializeCounts(buffer, offset, mask, BINS_256, node->counts)) {
    return nullptr;
  }
  setPopulated(node->populated, mask);

  if (level == 1) {
    return node;
  }

  if (!recursive)
    return node;

  // Recursively deserialize child nodes
  for (size_t w = 0; w < PopulatedBins<BINS_256>::kWords; ++w)
    for (uint64_t m = mask[w]; m != 0; m &= m - 1) {
      const size_t i = w * 64 + std::countr_zero(m);
    node->nodes[i] = deserialize_3DxF_l1(buffer, offset, level);
    }

  return node;
}

std::unique_ptr<TLE_3D_888> deserialize_3DxF(std::span<const char> buffer,
                                             size_t &offset) {
  auto node = std::make_unique<TLE_3D_888>();
  uint64_t mask[(BINS_512 + 63) / 64];
  if (!readPopulatedMask(buffer, offset, mask, BINS_512)
      || !deserializeCounts(buffer, offset, mask, BINS_512, node->counts)) {
    return nullptr;
  }
  setPopulated(node->populated, mask);

  // Recursively deserialize child nodes
  for (size_t w = 0; w < PopulatedBins<BINS_512>::kWords; ++w)
    for (uint64_t m = mask[w]; m != 0; m &= m - 1) {
      const size_t i = w * 64 + std::countr_zero(m);
    size_t nDims = getNumDims3D(i);
    switch (nDims) {
    case 0:
      continue;
    case 1:
    case 2:
    case 4:
      node->nodes[i] = deserialize_3DxF_l0(buffer, offset, 1);
      break;
    case 3:
    case 5:
    case 6:
      node->nodes[i] = deserialize_3DxF_l0(buffer, offset, 2);
      break;
    case 7:
      node->nodes[i] = deserialize_3DxF_l0(buffer, offset, 3);
      break;
    default:
      break;
    }
    }

  // verify end of file marker
  if (!verifyEndOfFileMarker(buffer, offset)) {
    return nullptr;
  }

  return node;
}
