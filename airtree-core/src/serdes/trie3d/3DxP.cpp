// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/serdes/trie3d/3DxP.hpp>
#include <airtree/core/serdes/BooleanArray.hpp>
#include <airtree/core/serdes/Count.hpp>
#include <airtree/core/serdes/ND.hpp>
#include <airtree/core/serdes/EOF.hpp>
#include <airtree/core/common/NDims.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/Logger.hpp>

using namespace airtree::core;
using namespace airtree::core::common;

std::pair<std::unique_ptr<TLE_3D_3x10>, airtree::core::common::AirTreeHeader>
processBuffer_3DxP(const std::vector<char> &buffer) {

  auto header = airtree::core::common::deserializeHeader(buffer);
  size_t offset = header.header_length;

  std::unique_ptr<TLE_3D_3x10> node = nullptr;

  if (offset < buffer.size()) {
    node = deserialize_3DxP(buffer, offset);
  } else {
    SPDLOG_LOGGER_ERROR(
        logger(), "Deserialization failed. Offset is out of bounds.");
  }

  return std::make_pair(std::move(node), header);
}

void serialize_3DxP_l2(const Node3D_3x10_l2 *node, std::vector<char> &buffer) {
  BooleanArray compact_array = BooleanArray(BINS_1024 / 64);
  for (size_t i = 0; i < BINS_1024; i++) {
    if (node->populated[i]) {
      compact_array.set(i, true);
    }
  }

  // Store compact array to buffer
  auto compact_array_buffer = serializeCompactBooleanArray(compact_array);
  buffer.insert(
      buffer.end(), compact_array_buffer.begin(), compact_array_buffer.end());

  // Store count for buckets with populated bit set using minBits
  auto counts_buffer = serializeCounts(node->counts, BINS_1024);
  buffer.insert(buffer.end(), counts_buffer.begin(), counts_buffer.end());
}

void serialize_3DxP_l1(const Node3D_3x10_l1 *node, std::vector<char> &buffer,
                       bool recursive) {
  BooleanArray compact_array = BooleanArray(BINS_1024 / 64);
  for (size_t i = 0; i < BINS_1024; i++) {
    if (node->populated[i]) {
      compact_array.set(i, true);
    }
  }

  // Store compact array to buffer
  auto compact_array_buffer = serializeCompactBooleanArray(compact_array);
  buffer.insert(
      buffer.end(), compact_array_buffer.begin(), compact_array_buffer.end());

  // Store count for buckets with populated bit set using minBits
  auto counts_buffer = serializeCounts(node->counts, BINS_1024);
  buffer.insert(buffer.end(), counts_buffer.begin(), counts_buffer.end());

  if (!recursive)
    return;

  for (size_t i = 0; i < BINS_1024; i++) {
    if (node->populated[i] && node->nodes[i]) {
      serialize_3DxP_l2(node->nodes[i].get(), buffer);
    }
  }
}

void serialize_3DxP_l0(const Node3D_3x10_l0 *node, std::vector<char> &buffer,
                       bool recursive) {
  BooleanArray compact_array = BooleanArray(BINS_1024 / 64);
  for (size_t i = 0; i < BINS_1024; i++) {
    if (node->populated[i]) {
      compact_array.set(i, true);
    }
  }

  // Store compact array to buffer
  auto compact_array_buffer = serializeCompactBooleanArray(compact_array);
  buffer.insert(
      buffer.end(), compact_array_buffer.begin(), compact_array_buffer.end());

  // Store count for buckets with populated bit set using minBits
  auto counts_buffer = serializeCounts(node->counts, BINS_1024);
  buffer.insert(buffer.end(), counts_buffer.begin(), counts_buffer.end());

  if (!recursive)
    return;

  for (size_t i = 0; i < BINS_1024; i++) {
    if (node->populated[i] && node->nodes[i]) {
      serialize_3DxP_l1(node->nodes[i].get(), buffer);
    }
  }
}


void serialize_3DxP(const TLE_3D_3x10 *node, std::vector<char> &buffer,
                    bool recursive) {
  BooleanArray compact_array = BooleanArray(BINS_512 / 64);
  for (size_t i = 0; i < BINS_512; i++) {
    if (node->populated[i]) {
      compact_array.set(i, true);
    }
  }

  // Store compact array to buffer
  auto compact_array_buffer = serializeCompactBooleanArray(compact_array);
  buffer.insert(
      buffer.end(), compact_array_buffer.begin(), compact_array_buffer.end());

  // Store count for buckets with populated bit set using minBits
  auto counts_buffer = serializeCounts(node->counts, BINS_512);
  buffer.insert(buffer.end(), counts_buffer.begin(), counts_buffer.end());

  if (!recursive)
    return;

  for (size_t i = 0; i < BINS_512; i++) {
    if (node->populated[i] && node->nodes[i]) {
      serialize_3DxP_l0(node->nodes[i].get(), buffer);
    }
  }

  // add end of file marker
  int32_t endOfFileMarker = -1;
  auto marker_bytes = reinterpret_cast<const char *>(&endOfFileMarker);
  buffer.insert(
      buffer.end(), marker_bytes, marker_bytes + sizeof(endOfFileMarker));
}

std::unique_ptr<Node3D_3x10_l2>
deserialize_3DxP_l2(const std::vector<char> &buffer, size_t &offset) {
  auto node = std::make_unique<Node3D_3x10_l2>();
  uint64_t mask[(BINS_1024 + 63) / 64];
  if (!readPopulatedMask(buffer, offset, mask, BINS_1024)
      || !deserializeCounts(buffer, offset, mask, BINS_1024, node->counts)) {
    return nullptr;
  }
  setPopulated(node->populated, mask);

  return node;
}


std::unique_ptr<Node3D_3x10_l1>
deserialize_3DxP_l1(const std::vector<char> &buffer, size_t &offset, int level,
                    bool recursive) {
  auto node = std::make_unique<Node3D_3x10_l1>();
  uint64_t mask[(BINS_1024 + 63) / 64];
  if (!readPopulatedMask(buffer, offset, mask, BINS_1024)
      || !deserializeCounts(buffer, offset, mask, BINS_1024, node->counts)) {
    return nullptr;
  }
  setPopulated(node->populated, mask);

  if (level == 2) {
    return node;
  }

  if (!recursive)
    return node;

  // Recursively deserialize child nodes
  for (size_t i = 0; i < BINS_1024; i++) {
    if (node->populated[i]) {
      node->nodes[i] = deserialize_3DxP_l2(buffer, offset);
    }
  }

  return node;
}

std::unique_ptr<Node3D_3x10_l0>
deserialize_3DxP_l0(const std::vector<char> &buffer, size_t &offset, int level,
                    bool recursive) {
  auto node = std::make_unique<Node3D_3x10_l0>();
  uint64_t mask[(BINS_1024 + 63) / 64];
  if (!readPopulatedMask(buffer, offset, mask, BINS_1024)
      || !deserializeCounts(buffer, offset, mask, BINS_1024, node->counts)) {
    return nullptr;
  }
  setPopulated(node->populated, mask);

  if (level == 1) {
    return node;
  }

  if (!recursive)
    return node;

  // Recursively deserialize child nodes
  for (size_t i = 0; i < BINS_1024; i++) {
    if (node->populated[i]) {
      node->nodes[i] = deserialize_3DxP_l1(buffer, offset, level);
    }
  }

  return node;
}


std::unique_ptr<TLE_3D_3x10> deserialize_3DxP(const std::vector<char> &buffer,
                                              size_t &offset) {
  auto node = std::make_unique<TLE_3D_3x10>();
  uint64_t mask[(BINS_512 + 63) / 64];
  if (!readPopulatedMask(buffer, offset, mask, BINS_512)
      || !deserializeCounts(buffer, offset, mask, BINS_512, node->counts)) {
    return nullptr;
  }
  setPopulated(node->populated, mask);

  // Recursively deserialize child nodes
  for (size_t i = 0; i < BINS_512; i++) {
    if (node->populated[i]) {
      size_t nDims = getNumDims3D(i);
      switch (nDims) {
      case 0:
        continue;
      case 1:
      case 2:
      case 4:
        node->nodes[i] = deserialize_3DxP_l0(buffer, offset, 1);
        break;
      case 3:
      case 5:
      case 6:
        node->nodes[i] = deserialize_3DxP_l0(buffer, offset, 2);
        break;
      case 7:
        node->nodes[i] = deserialize_3DxP_l0(buffer, offset, 3);
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
