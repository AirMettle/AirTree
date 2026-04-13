#include <airtree/core/serdes/trie2d/2DxP.hpp>
#include <airtree/core/serdes/BooleanArray.hpp>
#include <airtree/core/serdes/Count.hpp>
#include <airtree/core/serdes/ND.hpp>
#include <airtree/core/serdes/EOF.hpp>
#include <airtree/core/common/NDims.hpp>
#include <airtree/core/serdes/Header.hpp>
#include <airtree/core/Logger.hpp>

using namespace airtree::core;


void serialize_2DxP_l1(const TrieNode_2D_10_Level1 *node,
                       std::vector<char> &buffer) {

  BooleanArray compact_array = BooleanArray(BINS_1024 / 64);
  for (size_t i = 0; i < BINS_1024; i++) {
    if (node->counts[i] > 0) {
      compact_array.set(i, true);
    }
  }
  // Store compact array to buffer
  auto compact_array_buffer = serializeCompactBooleanArray(compact_array);
  buffer.insert(
      buffer.end(), compact_array_buffer.begin(), compact_array_buffer.end());

  // Store counts for buckets with count > 0 using minBits
  auto counts_buffer = serializeCounts(node->counts, BINS_1024);
  buffer.insert(buffer.end(), counts_buffer.begin(), counts_buffer.end());
}

void serialize_2DxP_l0(const TrieNode_2D_10 *node, std::vector<char> &buffer,
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
    if (node->nodes[i].get()) {
      serialize_2DxP_l1(node->nodes[i].get(), buffer);
    }
  }
}

void serialize_2DxP(const TLEoption3_2D *node, std::vector<char> &buffer,
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
  auto counts_buffer = serializeCounts(node->counts, BINS_64);
  buffer.insert(buffer.end(), counts_buffer.begin(), counts_buffer.end());

  if (!recursive)
    return;

  for (size_t i = 0; i < BINS_64; i++) {
    if (node->populated[i] && node->nodes[i]) {
      serialize_2DxP_l0(node->nodes[i].get(), buffer);
    }
  }

  // add end of file marker
  int32_t endOfFileMarker = -1;
  auto marker_bytes = reinterpret_cast<const char *>(&endOfFileMarker);
  buffer.insert(
      buffer.end(), marker_bytes, marker_bytes + sizeof(endOfFileMarker));
}

std::unique_ptr<TrieNode_2D_10_Level1>
deserialize_2DxP_l1(const std::vector<char> &buffer, size_t &offset) {
  auto node = std::make_unique<TrieNode_2D_10_Level1>();
  // deserialize compact BooleanArray
  std::vector<uint64_t> compact_arr_values =
      deserializeCompactBooleanArray(buffer, offset, BINS_1024 / 64);
  BooleanArray compact_array = BooleanArray(compact_arr_values);

  // Deserialize count for buckets with populated bit set
  auto counts = deserializeCounts(buffer, offset, compact_array.count());
  auto count_idx = 0;
  for (size_t i = 0; i < BINS_1024; i++) {
    if (compact_array.get(i)) {
      node->counts[i] = counts[count_idx];
      count_idx++;
    }
  }

  return node;
}

std::unique_ptr<TrieNode_2D_10>
deserialize_2DxP_l0(const std::vector<char> &buffer, size_t &offset, int level,
                    bool recursive) {

  if (level == 0) {
    return nullptr;
  }

  auto node = std::make_unique<TrieNode_2D_10>();
  // deserialize compact BooleanArray
  std::vector<uint64_t> compact_arr_values =
      deserializeCompactBooleanArray(buffer, offset, BINS_1024 / 64);
  BooleanArray compact_array = BooleanArray(compact_arr_values);
  // Convert compact BooleanArray to populated bitset
  for (size_t i = 0; i < BINS_1024; i++) {
    node->populated[i] = compact_array.get(i);
  }

  // Deserialize count for buckets with populated bit set
  auto counts = deserializeCounts(buffer, offset, node->populated.count());
  auto count_idx = 0;
  for (size_t i = 0; i < BINS_1024; i++) {
    if (node->populated[i]) {
      node->counts[i] = counts[count_idx];
      count_idx++;
    }
  }

  if (level == 1 or level == 2) {
    return node;
  }

  if (!recursive)
    return node;

  // Recursively deserialize child nodes
  for (size_t i = 0; i < BINS_1024; i++) {
    if (node->populated[i]) {
      node->nodes[i] = deserialize_2DxP_l1(buffer, offset);
    }
  }

  return node;
}


std::unique_ptr<TLEoption3_2D> deserialize_2DxP(std::vector<char> buffer,
                                                size_t &offset) {
  auto node = std::make_unique<TLEoption3_2D>();

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
      node->counts[i] = counts[count_idx];
      count_idx++;
    }
  }

  for (int i = 0; i < BINS_64; i++) {
    if (node->populated[i]) {
      int nDims = getNumDims2D(i);
      node->nodes[i] = deserialize_2DxP_l0(buffer, offset, nDims);
    }
  }

  if (!verifyEndOfFileMarker(buffer, offset)) {
    SPDLOG_LOGGER_ERROR(logger(), "End of file marker not found");
    return node;
  }

  return node;
}

std::pair<std::unique_ptr<TLEoption3_2D>, trie_header>
processBuffer_2DxP(const std::vector<char> &buffer) {

  size_t offset = 0;
  trie_header t_header_deserialized = deserializeTrieHeader(buffer, offset);

  std::unique_ptr<TLEoption3_2D> node = nullptr;

  if (offset < buffer.size()) {
    node = deserialize_2DxP(buffer, offset);
  } else {
    SPDLOG_LOGGER_ERROR(
        logger(), "Deserialization failed. Offset is out of bounds.");
  }

  return std::make_pair(std::move(node), t_header_deserialized);
}
