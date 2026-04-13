#include <airtree/core/serdes/trie3d/3DxF.hpp>
#include <airtree/core/serdes/BooleanArray.hpp>
#include <airtree/core/serdes/Count.hpp>
#include <airtree/core/serdes/ND.hpp>
#include <airtree/core/serdes/EOF.hpp>
#include <airtree/core/common/NDims.hpp>

#include <airtree/core/Logger.hpp>

using namespace airtree::core;

std::pair<std::unique_ptr<TLE_3D_888>, trie_header>
processBuffer_3DxF(const std::vector<char> &buffer) {

  size_t offset = 0;
  trie_header t_header_deserialized = deserializeTrieHeader(buffer, offset);

  std::unique_ptr<TLE_3D_888> node = nullptr;

  if (offset < buffer.size()) {
    node = deserialize_3DxF(buffer, offset);
  } else {
    SPDLOG_LOGGER_ERROR(
        logger(), "Deserialization failed. Offset is out of bounds.");
  }

  return std::make_pair(std::move(node), t_header_deserialized);
}

void serialize_3DxF_l0(const Node3D_888_l0 *node, std::vector<char> &buffer,
                       bool recursive) {
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

  // Store count for buckets with populated bit set using minBits
  auto counts_buffer = serializeCounts(node->counts, BINS_256);
  buffer.insert(buffer.end(), counts_buffer.begin(), counts_buffer.end());

  if (!recursive)
    return;

  for (size_t i = 0; i < BINS_256; i++) {
    if (node->populated[i] && node->nodes[i]) {
      serializeTrieNode_16_ND(node->nodes[i].get(), buffer);
    }
  }
}

void serialize_3DxF(const TLE_3D_888 *node, std::vector<char> &buffer,
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
      serialize_3DxF_l0(node->nodes[i].get(), buffer);
    }
  }

  // add end of file marker
  int32_t endOfFileMarker = -1;
  auto marker_bytes = reinterpret_cast<const char *>(&endOfFileMarker);
  buffer.insert(
      buffer.end(), marker_bytes, marker_bytes + sizeof(endOfFileMarker));
}


std::unique_ptr<TrieNode_16_Level1>
deserialize_3DxF_l2(const std::vector<char> &buffer, size_t &offset,
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
deserialize_3DxF_l1(const std::vector<char> &buffer, size_t &offset, int level,
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

  if (level == 2) {
    return node;
  }

  if (!recursive)
    return node;

  // Recursively deserialize child nodes
  for (size_t i = 0; i < BINS_256; i++) {
    if (node->populated[i]) {
      node->nodes[i] = deserialize_3DxF_l2(buffer, offset, level);
    }
  }

  return node;
}

std::unique_ptr<Node3D_888_l0>
deserialize_3DxF_l0(const std::vector<char> &buffer, size_t &offset, int level,
                    bool recursive) {
  auto node = std::make_unique<Node3D_888_l0>();
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
      node->nodes[i] = deserialize_3DxF_l1(buffer, offset, level);
    }
  }

  return node;
}

std::unique_ptr<TLE_3D_888> deserialize_3DxF(const std::vector<char> &buffer,
                                             size_t &offset) {
  auto node = std::make_unique<TLE_3D_888>();
  // deserialize compact BooleanArray
  std::vector<uint64_t> compact_arr_values =
      deserializeCompactBooleanArray(buffer, offset, BINS_512 / 64);
  BooleanArray compact_array = BooleanArray(compact_arr_values);
  // Convert compact BooleanArray to populated bitset
  for (size_t i = 0; i < BINS_512; i++) {
    node->populated[i] = compact_array.get(i);
  }

  // Deserialize count for buckets with populated bit set
  auto counts = deserializeCounts(buffer, offset, node->populated.count());
  auto count_idx = 0;
  for (size_t i = 0; i < BINS_512; i++) {
    if (node->populated[i]) {
      node->counts[i] = counts[count_idx];
      count_idx++;
    }
  }

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
  }

  // verify end of file marker
  if (!verifyEndOfFileMarker(buffer, offset)) {
    return nullptr;
  }

  return node;
}
