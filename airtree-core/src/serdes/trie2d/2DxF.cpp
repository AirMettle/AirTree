// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/serdes/trie2d/2DxF.hpp>
#include <airtree/core/serdes/BooleanArray.hpp>
#include <airtree/core/serdes/Count.hpp>
#include <airtree/core/serdes/ND.hpp>
#include <airtree/core/serdes/EOF.hpp>
#include <airtree/core/common/NDims.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/Logger.hpp>

using namespace airtree::core;
using namespace airtree::core::common;

std::pair<std::unique_ptr<TLETrieNode_2D>, airtree::core::common::AirTreeHeader>
processBuffer_2DxF(const std::vector<char> &buffer) {

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
  BooleanArray compact_array = BooleanArray(BINS_64 / 64);
  for (size_t i = 0; i < BINS_64; i++) {
    if (node->populated[i]) {
      compact_array.set(i, true);
    }
  }

  // Store compact array to buffer
  auto compact_array_buffer = serializeCompactBooleanArray(compact_array);
  buffer.insert(
      buffer.end(), compact_array_buffer.begin(), compact_array_buffer.end());

  // Store count for buckets with populated bit set using minBits
  auto counts_buffer = serializeCounts(node->TLEcounts, BINS_64);
  buffer.insert(buffer.end(), counts_buffer.begin(), counts_buffer.end());

  if (!recursive)
    return;

  for (size_t i = 0; i < BINS_64; i++) {
    if (node->populated[i] && node->nodes[i]) {
      serializeTrieNode_16_ND(node->nodes[i].get(), buffer);
    }
  }

  // add end of file marker
  int32_t endOfFileMarker = -1;
  auto marker_bytes = reinterpret_cast<const char *>(&endOfFileMarker);
  buffer.insert(
      buffer.end(), marker_bytes, marker_bytes + sizeof(endOfFileMarker));
}

std::unique_ptr<TrieNode_16_Level1>
deserialize_2DxF_l1(const std::vector<char> &buffer, size_t &offset,
                    int level [[maybe_unused]]) {
  auto node = std::make_unique<TrieNode_16_Level1>();
  // deserialize compact BooleanArray
  std::vector<uint64_t> compact_arr_values =
      deserializeCompactBooleanArray(buffer, offset, BINS_256 / 64);
  BooleanArray compact_array = BooleanArray(compact_arr_values);

  // Deserialize count for buckets with populated bit set
  auto counts = deserializeCounts(buffer, offset, compact_array.count());
  auto count_idx = 0;
  for (size_t i = 0; i < BINS_256; i++) {
    if (compact_array.get(i)) {
      node->counts[i] = counts[count_idx];
      count_idx++;
    }
  }

  return node;
}

std::unique_ptr<TrieNode_16>
deserialize_2DxF_l0(const std::vector<char> &buffer, size_t &offset, int level,
                    bool recursive) {
  auto node = std::make_unique<TrieNode_16>();
  // deserialize compact BooleanArray
  std::vector<uint64_t> compact_arr_values =
      deserializeCompactBooleanArray(buffer, offset, BINS_256 / 64);
  BooleanArray compact_array = BooleanArray(compact_arr_values);
  // Convert compact BooleanArray to populated bitset
  for (size_t i = 0; i < BINS_256; i++) {
    node->populated[i] = compact_array.get(i);
  }

  // Deserialize count for buckets with populated bit set
  auto counts = deserializeCounts(buffer, offset, node->populated.count());
  auto count_idx = 0;
  for (size_t i = 0; i < BINS_256; i++) {
    if (node->populated[i]) {
      node->counts[i] = counts[count_idx];
      count_idx++;
    }
  }

  if (level == 1) {
    return node;
  }

  if (!recursive)
    return node;

  // Recursively deserialize child nodes
  for (size_t i = 0; i < BINS_256; i++) {
    if (node->populated[i]) {
      node->nodes[i] = deserialize_2DxF_l1(buffer, offset, level);
    }
  }

  return node;
}

std::unique_ptr<TLETrieNode_2D> deserialize_2DxF(std::vector<char> buffer,
                                                 size_t &offset) {
  auto node = std::make_unique<TLETrieNode_2D>();

  std::vector<uint64_t> compact_arr_values =
      deserializeCompactBooleanArray(buffer, offset, BINS_64 / 64);
  BooleanArray compact_array = BooleanArray(compact_arr_values);

  for (int i = 0; i < BINS_64; i++) {
    node->populated[i] = compact_array.get(i);
  }

  auto counts = deserializeCounts(buffer, offset, node->populated.count());
  auto count_idx = 0;
  for (int i = 0; i < BINS_64; i++) {
    if (node->populated[i]) {
      node->TLEcounts[i] = counts[count_idx];
      count_idx++;
    }
  }

  for (int i = 0; i < BINS_64; i++) {
    if (node->populated[i]) {
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
  }

  if (!verifyEndOfFileMarker(buffer, offset)) {
    SPDLOG_LOGGER_ERROR(logger(), "End of file marker not found");
    return node;
  }

  return node;
}
