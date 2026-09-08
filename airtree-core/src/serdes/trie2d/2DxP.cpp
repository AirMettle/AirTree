// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/serdes/trie2d/2DxP.hpp>
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


void serialize_2DxP_l1(const TrieNode_2D_10_Level1 *node,
                       std::vector<char> &buffer) {
  uint64_t mask[(BINS_1024 + 63) / 64];
  maskFromCounts(node->counts, BINS_1024, mask);
  writeNode(mask, BINS_1024, node->counts, buffer);
}

void serialize_2DxP_l0(const TrieNode_2D_10 *node, std::vector<char> &buffer,
                       bool recursive) {
  writeNode(node->populated.words, BINS_1024, node->counts, buffer);
  if (!recursive)
    return;
  forEachSetBit(node->populated.words, BINS_1024, [&](size_t i) {
    if (node->nodes[i])
      serialize_2DxP_l1(node->nodes[i].get(), buffer);
  });
}

void serialize_2DxP(const TLEoption3_2D *node, std::vector<char> &buffer,
                    bool recursive) {

  writeNode(node->populated.words, BINS_64, node->counts, buffer);

  if (!recursive)
    return;

  forEachSetBit(node->populated.words, BINS_64, [&](size_t i) {
    if (node->nodes[i])
      serialize_2DxP_l0(node->nodes[i].get(), buffer);
  });

  // add end of file marker
  writeEndOfFileMarker(buffer);
}

std::unique_ptr<TrieNode_2D_10_Level1>
deserialize_2DxP_l1(std::span<const char> buffer, size_t &offset) {
  uint64_t mask[(BINS_1024 + 63) / 64];
  auto node = std::make_unique<TrieNode_2D_10_Level1>();
  if (!readPopulatedMask(buffer, offset, mask, BINS_1024)
      || !deserializeCounts(buffer, offset, mask, BINS_1024, node->counts)) {
    return nullptr;
  }
  return node;
}

std::unique_ptr<TrieNode_2D_10>
deserialize_2DxP_l0(std::span<const char> buffer, size_t &offset, int level,
                    bool recursive) {
  if (level == 0) {
    return nullptr;
  }
  uint64_t mask[(BINS_1024 + 63) / 64];
  auto node = std::make_unique<TrieNode_2D_10>();
  if (!readPopulatedMask(buffer, offset, mask, BINS_1024)
      || !deserializeCounts(buffer, offset, mask, BINS_1024, node->counts)) {
    return nullptr;
  }
  setPopulated(node->populated, mask);
  if (level == 1 || level == 2 || !recursive)
    return node;
  forEachSetBit(mask, BINS_1024, [&](size_t i) { node->nodes[i] = deserialize_2DxP_l1(buffer, offset); });
  return node;
}

std::unique_ptr<TLEoption3_2D> deserialize_2DxP(std::span<const char> buffer,
                                                size_t &offset) {
  uint64_t mask[(BINS_64 + 63) / 64];
  auto node = std::make_unique<TLEoption3_2D>();
  if (!readPopulatedMask(buffer, offset, mask, BINS_64)
      || !deserializeCounts(buffer, offset, mask, BINS_64, node->counts)) {
    return nullptr;
  }
  setPopulated(node->populated, mask);
  for (size_t w = 0; w < PopulatedBins<BINS_64>::kWords; ++w)
    for (uint64_t m = mask[w]; m != 0; m &= m - 1) {
      const size_t i = w * 64 + std::countr_zero(m);
    int nDims = getNumDims2D(i);
    node->nodes[i] = deserialize_2DxP_l0(buffer, offset, nDims);
    }
  if (!verifyEndOfFileMarker(buffer, offset)) {
    SPDLOG_LOGGER_ERROR(logger(), "End of file marker not found");
    return node;
  }
  return node;
}

std::pair<std::unique_ptr<TLEoption3_2D>, airtree::core::common::AirTreeHeader>
processBuffer_2DxP(std::span<const char> buffer) {

  auto header = airtree::core::common::deserializeHeader(buffer);
  size_t offset = header.header_length;

  std::unique_ptr<TLEoption3_2D> node = nullptr;

  if (offset < buffer.size()) {
    node = deserialize_2DxP(buffer, offset);
  } else {
    SPDLOG_LOGGER_ERROR(
        logger(), "Deserialization failed. Offset is out of bounds.");
  }

  return std::make_pair(std::move(node), header);
}
