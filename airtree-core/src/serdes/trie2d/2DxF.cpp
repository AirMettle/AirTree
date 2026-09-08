// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/serdes/trie2d/2DxF.hpp>
#include <airtree/core/serdes/BooleanArray.hpp>
#include <airtree/core/serdes/Count.hpp>
#include <airtree/core/serdes/Node.hpp>
#include <airtree/core/serdes/ND.hpp>
#include <airtree/core/serdes/EOF.hpp>
#include <airtree/core/common/NDims.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/Logger.hpp>

using namespace airtree::core;
using namespace airtree::core::common;

std::pair<std::unique_ptr<TLETrieNode_2D>, airtree::core::common::AirTreeHeader>
processBuffer_2DxF(std::span<const char> buffer) {

  auto header = airtree::core::common::deserializeHeader(buffer);
  size_t offset = header.header_length;

  std::unique_ptr<TLETrieNode_2D> node = nullptr;

  if (offset < buffer.size()) {
    node = deserialize_2DxF(buffer, offset);
  } else {
    SPDLOG_LOGGER_ERROR(
        logger(), "Deserialization failed. Offset is out of bounds.");
  }

  return std::make_pair(std::move(node), header);
}


void serialize_2DxF(const TLETrieNode_2D *node, std::vector<char> &buffer,
                    bool recursive) {
  writeNode(node->populated.words, BINS_64, node->TLEcounts, buffer);

  if (!recursive)
    return;

  forEachSetBit(node->populated.words, BINS_64, [&](size_t i) {
    if (node->nodes[i])
      serializeTrieNode_16_ND(node->nodes[i].get(), buffer);
  });

  // add end of file marker
  writeEndOfFileMarker(buffer);
}

std::unique_ptr<TrieNode_16_Level1>
deserialize_2DxF_l1(std::span<const char> buffer, size_t &offset,
                    int level [[maybe_unused]]) {
  uint64_t mask[(BINS_256 + 63) / 64];
  auto node = std::make_unique<TrieNode_16_Level1>();
  if (!readPopulatedMask(buffer, offset, mask, BINS_256)
      || !deserializeCounts(buffer, offset, mask, BINS_256, node->counts)) {
    return nullptr;
  }
  return node;
}

std::unique_ptr<TrieNode_16>
deserialize_2DxF_l0(std::span<const char> buffer, size_t &offset, int level,
                    bool recursive) {
  uint64_t mask[(BINS_256 + 63) / 64];
  auto node = std::make_unique<TrieNode_16>();
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
  for (size_t w = 0; w < PopulatedBins<BINS_256>::kWords; ++w)
    for (uint64_t m = mask[w]; m != 0; m &= m - 1) {
      const size_t i = w * 64 + std::countr_zero(m);
    node->nodes[i] = deserialize_2DxF_l1(buffer, offset, level);
    }
  return node;
}

std::unique_ptr<TLETrieNode_2D> deserialize_2DxF(std::span<const char> buffer,
                                                 size_t &offset) {
  uint64_t mask[(BINS_64 + 63) / 64];
  auto node = std::make_unique<TLETrieNode_2D>();
  if (!readPopulatedMask(buffer, offset, mask, BINS_64)
      || !deserializeCounts(buffer, offset, mask, BINS_64, node->TLEcounts)) {
    return nullptr;
  }
  setPopulated(node->populated, mask);
  for (size_t w = 0; w < PopulatedBins<BINS_64>::kWords; ++w)
    for (uint64_t m = mask[w]; m != 0; m &= m - 1) {
      const size_t i = w * 64 + std::countr_zero(m);
    int nDims = getNumDims2D(i);
    switch (nDims) {
    case 0:
      continue;
    case 1:
    case 2:
      node->nodes[i] = deserialize_2DxF_l0(buffer, offset, 1);
      break;
    case 3:
      node->nodes[i] = deserialize_2DxF_l0(buffer, offset, 2);
      break;
    default:
      break;
    }
    }
  if (!verifyEndOfFileMarker(buffer, offset)) {
    SPDLOG_LOGGER_ERROR(logger(), "End of file marker not found");
    return node;
  }
  return node;
}
