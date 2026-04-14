#include <airtree/core/schema/trie1d/1DxT.hpp>
#include <airtree/core/common/Conversion.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/serdes/Header.hpp>
#include <airtree/core/serdes/trie1d/1DxT.hpp>
#include <airtree/core/Logger.hpp>

#include <airtree/util/FeatureFlags.h>
#include <airtree/util/UUID.hpp>


using namespace airtree::core;
using namespace airtree::core::schema::trie1d;
using namespace airtree::util::uuid;

std::vector<char>
Generator1DxT::generate(const std::vector<const FPHArray *> &arrays,
                       bool default_mode) const {
  if (arrays.size() != 1) {
    throw std::invalid_argument("Expected exactly 1 array for 1D generation");
  }
  return generate_1DxT(*arrays[0], default_mode);
}

std::unique_ptr<TrieNode_13> CreateParentNode() {
  auto parentNode = std::make_unique<TrieNode_13>();
  parentNode->populated.reset();

  return parentNode;
}

void createAndInsertFP_32(TrieNode_13 *node, uint32_t fpNumber,
                          uint64_t &curr_trie_size, bool default_mode) {
  // Convert IEEE-754 to 13-bit internal representation
  unsigned int internal13 = createInternal13Bit_32(fpNumber, default_mode);

  // Extract the two indices
  unsigned int index8 = (internal13 >> 5) & 0xFF; // Upper 8 bits (level 0)
  unsigned int index5 = internal13 & 0x1F;        // Lower 5 bits (level 1)

  // Trie Insertion Logic
  if (!node->populated.test(index8)) {
    node->populated.set(index8);
    node->nodes[index8] = std::make_unique<TrieNode_13_Level1>();
    curr_trie_size += sizeof(TrieNode_13_Level1);
  }

  node->counts[index8]++;
  node->nodes[index8]->counts[index5]++;
}

void createAndInsertFP(TrieNode_13 *node, uint64_t fpNumber,
                       uint64_t &curr_trie_size, bool default_mode) {
  // Convert IEEE-754 to 13-bit internal representation
  unsigned int internal13 = createInternal13Bit(fpNumber, default_mode);

  // Extract the two indices
  unsigned int index8 = (internal13 >> 5) & 0xFF; // Upper 8 bits (level 0)
  unsigned int index5 = internal13 & 0x1F;        // Lower 5 bits (level 1)

  // Trie Insertion Logic
  if (!node->populated.test(index8)) {
    node->populated.set(index8);
    node->nodes[index8] = std::make_unique<TrieNode_13_Level1>();
    curr_trie_size += sizeof(TrieNode_13_Level1);
  }

  node->counts[index8]++;
  node->nodes[index8]->counts[index5]++;
}

std::vector<char> generate_1DxT(const FPHArray &array, bool default_mode) {
  std::string uuid = AirTreeUUID::generateUUID();
  SPDLOG_LOGGER_INFO(logger(),
                     "[generate] [traceID: {}] Generating 1DxT Trie for {} "
                     "values using default_mode {}.",
                     uuid, array.length, default_mode);
  SpecialCounts specialCounts;
  uint64_t curr_trie_size = 0;
  SPDLOG_LOGGER_INFO(
      logger(),
      "[insert] [traceID: {}] Filing and inserting {} values using "
      "default_mode {} into 1DxT Trie.",
      uuid, array.length, default_mode);
  std::unique_ptr<TrieNode_13> root = execCreateAndInsert_TrieNode13(
      specialCounts, curr_trie_size, array, default_mode);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[insert] [traceID: {}] Completed filing and inserting values into 1DxT "
      "Trie. Final trie size: {}.",
      uuid, curr_trie_size);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[serialization] [traceID: {}] Serializing 1DxT trie of size {}.", uuid,
      curr_trie_size);
  std::vector<char> buffer = execSerialization_TrieNode13(
      root, specialCounts, curr_trie_size, default_mode);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[serialization] [traceID: {}] Completed serializing 1DxT Trie.", uuid);
  SPDLOG_LOGGER_INFO(logger(),
                     "[generate] [traceID: {}] Completed generating 1DxT Trie.",
                     uuid);
  return buffer;
}

std::unique_ptr<TrieNode_13>
execCreateAndInsert_TrieNode13(SpecialCounts &specialCounts,
                               uint64_t &curr_trie_size, const FPHArray &array,
                               bool default_mode) {
  std::unique_ptr<TrieNode_13> root = CreateParentNode();
  curr_trie_size = sizeof(TrieNode_13);

  switch (array.type) {
  case FPH_dtype::Double: {
    const double *values = static_cast<const double *>(array.values);
    for (int i = 0; i < array.length; ++i) {
      uint64_t fpNumber;
      std::memcpy(&fpNumber, &values[i], sizeof(values[i]));
      if (!isSpecialCase(fpNumber, specialCounts)) {
        createAndInsertFP(root.get(), fpNumber, curr_trie_size, default_mode);
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
        createAndInsertFP(root.get(), fpNumber, curr_trie_size, default_mode);
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
        createAndInsertFP(root.get(), fpNumber, curr_trie_size, default_mode);
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
        createAndInsertFP(root.get(), fpNumber, curr_trie_size, default_mode);
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
execSerialization_TrieNode13(const std::unique_ptr<TrieNode_13> &root,
                             const SpecialCounts &specialCounts,
                             uint64_t trieSize, bool default_mode) {
  std::vector<char> buffer;
  buffer.reserve(sizeof(trie_header) + trieSize);
  trie_header t_header;
  strcpy(t_header.type_code, "HierFPHG");
  t_header.version = 0;
  t_header.m_width = 4;
  t_header.precision_bits = 7;
  t_header.node_width = 8;
  strcpy(t_header.type, "1-D");
  strcpy(t_header.config, "113");
  t_header.mode = default_mode ? 1 : 0;
  t_header.pos_inf_count = specialCounts.posInfCount;
  t_header.neg_inf_count = specialCounts.negInfCount;
  t_header.pos_zero_count = specialCounts.posZeroCount;
  t_header.neg_zero_count = specialCounts.negZeroCount;
  t_header.nan_count = specialCounts.nanCount;
  t_header.trie_root_ref = sizeof(trie_header);
  serializeTrieHeader(t_header, buffer);
  serialize_1DxT(root.get(), buffer);

  return buffer;
}