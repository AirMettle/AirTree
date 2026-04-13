#include <airtree/core/schema/trie1d/1DxF.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/common/Conversion.hpp>
#include <airtree/core/Logger.hpp>
#include <airtree/core/serdes/trie1d/1DxF.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/util/FeatureFlags.h>
#include <airtree/util/UUID.hpp>

using namespace airtree::core;
using namespace airtree::core::common;
using namespace airtree::core::schema::trie1d;
using namespace airtree::util::uuid;

std::vector<char>
Generator1DxF::generate(const std::vector<const FPHArray *> &arrays,
                       bool default_mode) const {
  if (arrays.size() != 1) {
    throw std::invalid_argument("Expected exactly 1 array for 1D generation");
  }
  return generate_1DxF(*arrays[0], default_mode);
}

std::unique_ptr<TrieNode_16> CreateParentNode_16() {
  auto parentNode = std::make_unique<TrieNode_16>();
  parentNode->populated.reset();

  return parentNode;
}

void createAndInsertFP16(TrieNode_16 *node, uint64_t fpNumber,
                         uint64_t &curr_trie_size, bool default_mode) {
  // Convert IEEE-754 to 16-bit internal representation
  unsigned int internal16 = createInternal16Bit(fpNumber, default_mode);

  // Extract the two 8-bit indices
  unsigned int index8 = (internal16 >> 8) & 0xFF; // Upper 8 bits (level 0)
  unsigned int index8_level2 = internal16 & 0xFF; // Lower 8 bits (level 1)

  // Trie Insertion Logic
  if (!node->populated.test(index8)) {
    node->populated.set(index8);
    node->nodes[index8] = std::make_unique<TrieNode_16_Level1>();
    curr_trie_size += sizeof(TrieNode_16_Level1);
  }

  if (node->nodes[index8]) {
    node->counts[index8]++;
    node->nodes[index8]->counts[index8_level2]++;
  }
}

void createAndInsertFP16_32(TrieNode_16 *node, uint32_t fpNumber,
                            uint64_t &curr_trie_size, bool default_mode) {
  // Convert IEEE-754 to 16-bit internal representation
  unsigned int internal16 = createInternal16Bit_32(fpNumber, default_mode);

  // Extract the two 8-bit indices
  unsigned int index8 = (internal16 >> 8) & 0xFF; // Upper 8 bits (level 0)
  unsigned int index8_level2 = internal16 & 0xFF; // Lower 8 bits (level 1)

  // Trie Insertion Logic
  if (!node->populated.test(index8)) {
    node->populated.set(index8);
    node->nodes[index8] = std::make_unique<TrieNode_16_Level1>();
    curr_trie_size += sizeof(TrieNode_16_Level1);
  }

  if (node->nodes[index8]) {
    node->counts[index8]++;
    node->nodes[index8]->counts[index8_level2]++;
  }
}


std::vector<char> generate_1DxF(const FPHArray &array, bool default_mode) {
  // generate a UUID for this trie
  std::string uuid = AirTreeUUID::generateUUID();
  SPDLOG_LOGGER_INFO(logger(),
                     "[generate] [traceID: {}] Generating 1DxF Trie for {} "
                     "values using default_mode {}.",
                     uuid, array.length, default_mode);
  SpecialCounts specialCounts;

  uint64_t curr_trie_size = 0;
  SPDLOG_LOGGER_INFO(
      logger(),
      "[insert] [traceID: {}] Filing and inserting {} values using "
      "default_mode {} into 1DxF Trie.",
      uuid, array.length, default_mode);
  std::unique_ptr<TrieNode_16> root = execCreateAndInsert_TrieNode16(
      specialCounts, curr_trie_size, array, default_mode);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[insert] [traceID: {}] Completed filing and inserting values into 1DxF "
      "Trie. Final trie size: {}.",
      uuid, curr_trie_size);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[serialization] [traceID: {}] Serializing 1DxF trie of size {}.", uuid,
      curr_trie_size);
  std::vector<char> buffer = execSerialization_TrieNode16(
      root, specialCounts, curr_trie_size, default_mode);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[serialization] [traceID: {}] Completed serializing 1DxF Trie.", uuid);
  SPDLOG_LOGGER_INFO(logger(),
                     "[generate] [traceID: {}] Completed generating 1DxF Trie.",
                     uuid);
  return buffer;
}

std::unique_ptr<TrieNode_16>
execCreateAndInsert_TrieNode16(SpecialCounts &specialCounts,
                               uint64_t &curr_trie_size, const FPHArray &array,
                               bool default_mode) {
  std::unique_ptr<TrieNode_16> root = CreateParentNode_16();
  curr_trie_size = sizeof(TrieNode_16);

  switch (array.type) {
  case FPH_dtype::Double: {
    const double *values = static_cast<const double *>(array.values);
    for (int i = 0; i < array.length; ++i) {
      uint64_t fpNumber;
      std::memcpy(&fpNumber, &values[i], sizeof(values[i]));
      if (!isSpecialCase(fpNumber, specialCounts)) {
        createAndInsertFP16(root.get(), fpNumber, curr_trie_size, default_mode);
        if (enable_threshold_1D && curr_trie_size > threshold_1D) {
          SPDLOG_LOGGER_ERROR(logger(),
                              "Trie size exceeded threshold limit of {}.",
                              threshold_1D);
          SPDLOG_LOGGER_ERROR(
              logger(),
              "Insertion process stopped at index {} of the input dataset.", i);
          SPDLOG_LOGGER_ERROR(logger(), "Last failed value: {}", values[i]);
          break;
        }
      }
    }
  } break;
  case FPH_dtype::Float: {
    const float *values = static_cast<const float *>(array.values);
    for (int i = 0; i < array.length; ++i) {
      double doubleValue =
          static_cast<double>(values[i]); // Convert float to double
      uint64_t fpNumber;
      std::memcpy(&fpNumber, &doubleValue, sizeof(doubleValue));
      if (!isSpecialCase(fpNumber, specialCounts)) {
        createAndInsertFP16(root.get(), fpNumber, curr_trie_size, default_mode);
        if (enable_threshold_1D && curr_trie_size > threshold_1D) {
          SPDLOG_LOGGER_ERROR(logger(),
                              "Trie size exceeded threshold limit of {}.",
                              threshold_1D);
          SPDLOG_LOGGER_ERROR(
              logger(),
              "Insertion process stopped at index {} of the input dataset.", i);
          SPDLOG_LOGGER_ERROR(logger(), "Last failed value: {}", values[i]);
          break;
        }
      }
    }
  } break;
  case FPH_dtype::Int32: {
    const int32_t *values = static_cast<const int32_t *>(array.values);
    for (int i = 0; i < array.length; ++i) {
      double floatValue = int32_to_double(values[i]);
      uint64_t fpNumber;
      std::memcpy(&fpNumber, &floatValue, sizeof(floatValue));
      if (!isSpecialCase(fpNumber, specialCounts)) {
        createAndInsertFP16(root.get(), fpNumber, curr_trie_size, default_mode);
        if (enable_threshold_1D && curr_trie_size > threshold_1D) {
          SPDLOG_LOGGER_ERROR(logger(),
                              "Trie size exceeded threshold limit of {}.",
                              threshold_1D);
          SPDLOG_LOGGER_ERROR(
              logger(),
              "Insertion process stopped at index {} of the input dataset.", i);
          SPDLOG_LOGGER_ERROR(logger(), "Last failed value: {}", values[i]);
          break;
        }
      }
    }
  } break;
  case FPH_dtype::Int64: {
    const int64_t *values = static_cast<const int64_t *>(array.values);
    for (int i = 0; i < array.length; ++i) {
      double floatValue = int64_to_double(values[i]);
      uint64_t fpNumber;
      std::memcpy(&fpNumber, &floatValue, sizeof(floatValue));
      if (!isSpecialCase(fpNumber, specialCounts)) {
        createAndInsertFP16(root.get(), fpNumber, curr_trie_size, default_mode);
        if (enable_threshold_1D && curr_trie_size > threshold_1D) {
          SPDLOG_LOGGER_ERROR(logger(),
                              "Trie size exceeded threshold limit of {}.",
                              threshold_1D);
          SPDLOG_LOGGER_ERROR(
              logger(),
              "Insertion process stopped at index {} of the input dataset.", i);
          SPDLOG_LOGGER_ERROR(logger(), "Last failed value: {}", values[i]);
          break;
        }
      }
    }
  } break;
  default:
    SPDLOG_LOGGER_ERROR(logger(), "Unknown data type");
    break;
  }
  return root;
}

std::vector<char>
execSerialization_TrieNode16(const std::unique_ptr<TrieNode_16> &root,
                             const SpecialCounts &specialCounts,
                             uint64_t trieSize, bool default_mode) {
  auto header = airtree::core::common::makeHeader(
      ConfigWire::Config_1D_Fast, {}, 0,
      specialCounts.posInfCount, specialCounts.negInfCount,
      specialCounts.posZeroCount, specialCounts.negZeroCount,
      specialCounts.nanCount);

  std::vector<char> buffer;
  buffer.reserve(kHeaderLength + trieSize);
  serializeHeader(header, buffer);
  size_t header_end = buffer.size();
  serialize_1DxF(root.get(), buffer);
  finalizeHeader(buffer, buffer.size() - header_end);

  return buffer;
}