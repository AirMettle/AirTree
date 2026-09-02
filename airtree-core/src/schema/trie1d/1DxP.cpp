// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/schema/trie1d/1DxP.hpp>
#include <airtree/core/common/Conversion.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/serdes/trie1d/1DxP.hpp>
#include <airtree/core/Logger.hpp>

#include <airtree/util/FeatureFlags.h>
#include <airtree/util/UUID.hpp>


using namespace airtree::core;
using namespace airtree::core::common;
using namespace airtree::core::api;
using namespace airtree::core::schema::trie1d;
using namespace airtree::util::uuid;

std::vector<char>
Generator1DxP::generate(const std::vector<const FPHArray *> &arrays) const {
  if (arrays.size() != 1) {
    throw std::invalid_argument("Expected exactly 1 array for 1D generation");
  }
  return generate_1DxP(*arrays[0]);
}
std::unique_ptr<TrieNode_20> CreateParentNode_20() {
  auto parentNode = std::make_unique<TrieNode_20>();
  parentNode->populated.reset();

  return parentNode;
}

void createAndInsertFP20(TrieNode_20 *node, uint64_t fpNumber,
                         uint64_t &curr_trie_size) {
  // Convert IEEE-754 to 20-bit internal representation
  unsigned int internal20 = createInternal20Bit(fpNumber);

  // Extract the three indices
  unsigned int index8 = (internal20 >> 12) & 0xFF; // Upper 8 bits (level 0)
  unsigned int index6_level1 =
      (internal20 >> 6) & 0x3F;                   // Middle 6 bits (level 1)
  unsigned int index6_level2 = internal20 & 0x3F; // Lower 6 bits (level 2)

  // Trie Insertion Logic
  if (!node->populated.test(index8)) {
    node->populated.set(index8);
    node->nodes[index8] = std::make_unique<TrieNode_20_Level1>();
    curr_trie_size += sizeof(TrieNode_20_Level1);
  }

  auto &level1Node = node->nodes[index8];
  node->counts[index8]++;

  if (!level1Node->populated.test(index6_level1)) {
    level1Node->populated.set(index6_level1);
    level1Node->nodes[index6_level1] = std::make_unique<TrieNode_20_Level2>();
    curr_trie_size += sizeof(TrieNode_20_Level2);
  }
  auto &level2Node = level1Node->nodes[index6_level1];
  // Increment the counts at level 1 and 2 in a guaranteed sequence
  level1Node->counts[index6_level1]++;
  level2Node->counts[index6_level2]++;
}

void createAndInsertFP20_32(TrieNode_20 *node, uint32_t fpNumber,
                            uint64_t &curr_trie_size) {
  // Convert IEEE-754 to 20-bit internal representation
  unsigned int internal20 = createInternal20Bit_32(fpNumber);

  // Extract the three indices
  unsigned int index8 = (internal20 >> 12) & 0xFF; // Upper 8 bits (level 0)
  unsigned int index6_level1 =
      (internal20 >> 6) & 0x3F;                   // Middle 6 bits (level 1)
  unsigned int index6_level2 = internal20 & 0x3F; // Lower 6 bits (level 2)

  // Trie Insertion Logic
  if (!node->populated.test(index8)) {
    node->populated.set(index8);
    node->nodes[index8] = std::make_unique<TrieNode_20_Level1>();
    curr_trie_size += sizeof(TrieNode_20_Level1);
  }

  auto &level1Node = node->nodes[index8];
  node->counts[index8]++;

  if (!level1Node->populated.test(index6_level1)) {
    level1Node->populated.set(index6_level1);
    level1Node->nodes[index6_level1] = std::make_unique<TrieNode_20_Level2>();
    curr_trie_size += sizeof(TrieNode_20_Level2);
  }
  auto &level2Node = level1Node->nodes[index6_level1];
  // Increment the counts at level 1 and 2 in a guaranteed sequence
  level1Node->counts[index6_level1]++;
  level2Node->counts[index6_level2]++;
}

std::vector<char> generate_1DxP(const FPHArray &array) {
  std::string uuid = AirTreeUUID::generateUUID();
  SPDLOG_LOGGER_DEBUG(logger(),
                     "[generate] [traceID: {}] Generating 1DxP Trie for {} "
                     "values.",
                     uuid, array.length);
  SpecialCounts specialCounts;
  uint64_t curr_trie_size = 0;
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[insert] [traceID: {}] Filing and inserting {} values "
      "into 1DxP Trie.",
      uuid, array.length);
  std::unique_ptr<TrieNode_20> root = execCreateAndInsert_TrieNode20(
      specialCounts, curr_trie_size, array);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[insert] [traceID: {}] Completed filing and inserting values into 1DxP "
      "Trie. Final trie size: {}.",
      uuid, curr_trie_size);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[serialization] [traceID: {}] Serializing 1DxP trie of size {}.", uuid,
      curr_trie_size);
  std::vector<char> buffer = execSerialization_TrieNode20(
      root, specialCounts, curr_trie_size);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[serialization] [traceID: {}] Completed serializing 1DxP Trie.", uuid);
  SPDLOG_LOGGER_DEBUG(logger(),
                     "[generate] [traceID: {}] Completed generating 1DxP Trie.",
                     uuid);
  return buffer;
}

std::unique_ptr<TrieNode_20>
execCreateAndInsert_TrieNode20(SpecialCounts &specialCounts,
                               uint64_t &curr_trie_size, const FPHArray &array) {
  std::unique_ptr<TrieNode_20> root = CreateParentNode_20();
  curr_trie_size = sizeof(TrieNode_20);

  auto process_array = [&](const auto *typed_values) {
    for (int i = 0; i < array.length; ++i) {
      double value = static_cast<double>(typed_values[i]);
      uint64_t fpNumber;
      std::memcpy(&fpNumber, &value, sizeof(value));
      if (!isSpecialCase(fpNumber, specialCounts)) {
        createAndInsertFP20(root.get(), fpNumber, curr_trie_size);
        if (enable_threshold_1D && curr_trie_size > threshold_1D) {
          SPDLOG_LOGGER_ERROR(logger(),
                              "Trie size exceeded threshold limit of {}.",
                              threshold_1D);
          SPDLOG_LOGGER_ERROR(
              logger(),
              "Insertion process stopped at index {} of the input dataset.", i);
          SPDLOG_LOGGER_ERROR(
              logger(), "Last failed value: {}", typed_values[i]);
          break;
        }
      }
    }
  };

  dispatchFPHArray(array, process_array);

  return root;
}

std::vector<char>
execSerialization_TrieNode20(const std::unique_ptr<TrieNode_20> &root,
                             const SpecialCounts &specialCounts,
                             uint64_t trieSize) {
  auto header = airtree::core::common::makeHeader(
      ConfigWire::Config_1D_Precise, {}, countObservations(root->counts),
      specialCounts.posInfCount, specialCounts.negInfCount,
      specialCounts.posZeroCount, specialCounts.negZeroCount,
      specialCounts.nanCount);

  std::vector<char> buffer;
  buffer.reserve(kHeaderLength + trieSize);
  serializeHeader(header, buffer);
  size_t header_end = buffer.size();
  serialize_1DxP(root.get(), buffer);
  finalizeHeader(buffer, buffer.size() - header_end);

  return buffer;
}