// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/serdes/trie1d/1DxF.hpp>
#include <airtree/core/common/BooleanArray.hpp>
#include <airtree/core/serdes/BooleanArray.hpp>
#include <airtree/core/serdes/Count.hpp>
#include <airtree/core/serdes/Node.hpp>
#include <airtree/core/serdes/EOF.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/Logger.hpp>

using namespace airtree::core;
using namespace airtree::core::common;

void serialize_1DxF_l1(const TrieNode_16_Level1 *node,
                                         std::vector<char> &buffer) {
  // Build compact boolean array to represent populated bitset
  uint64_t mask[(BINS_256 + 63) / 64];
  maskFromCounts(node->counts, BINS_256, mask);
  writeNode(mask, BINS_256, node->counts, buffer);
}


void serialize_1DxF(const TrieNode_16 *node,
                                  std::vector<char> &buffer, bool recursive) {
  writeNode(node->populated.words, BINS_256, node->counts, buffer);

  if (!recursive) {
    return;
  }

  forEachSetBit(node->populated.words, BINS_256, [&](size_t i) {
    if (node->nodes[i])
      serialize_1DxF_l1(node->nodes[i].get(), buffer);
  });

  // Add end of file marker
  writeEndOfFileMarker(buffer);
}


std::unique_ptr<TrieNode_16_Level1>
deserialize_1DxF_l1(std::span<const char> buffer, size_t &offset) {
  uint64_t mask[(BINS_256 + 63) / 64];
  auto node = std::make_unique<TrieNode_16_Level1>();
  if (!readPopulatedMask(buffer, offset, mask, BINS_256)
      || !deserializeCounts(buffer, offset, mask, BINS_256, node->counts)) {
    return nullptr;
  }
  return node;
}

std::unique_ptr<TrieNode_16>
deserialize_1DxF(std::span<const char> buffer, size_t &offset) {
  uint64_t mask[(BINS_256 + 63) / 64];
  auto node = std::make_unique<TrieNode_16>();
  if (!readPopulatedMask(buffer, offset, mask, BINS_256)
      || !deserializeCounts(buffer, offset, mask, BINS_256, node->counts)) {
    return nullptr;
  }
  setPopulated(node->populated, mask);
  for (size_t w = 0; w < PopulatedBins<BINS_256>::kWords; ++w)
    for (uint64_t m = mask[w]; m != 0; m &= m - 1) {
      const size_t i = w * 64 + std::countr_zero(m);
      node->nodes[i] = deserialize_1DxF_l1(buffer, offset);
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

std::pair<std::unique_ptr<TrieNode_16>, airtree::core::common::AirTreeHeader>
processBuffer_1DxF(std::span<const char> buffer) {
  auto header = airtree::core::common::deserializeHeader(buffer);
  size_t offset = header.header_length;

  std::unique_ptr<TrieNode_16> root = nullptr;

  if (offset < buffer.size()) {
    root = deserialize_1DxF(buffer, offset);

    if (root.get() == nullptr) {
      SPDLOG_LOGGER_ERROR(
          logger(), "Deserialization resulted in a null root node.");
    }
  } else {
    SPDLOG_LOGGER_ERROR(logger(), "Insufficient buffer size for trie data.");
  }

  return std::make_pair(std::move(root), header);
}