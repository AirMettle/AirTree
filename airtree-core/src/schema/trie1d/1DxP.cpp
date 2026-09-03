// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/schema/trie1d/1DxP.hpp>
#include <airtree/core/common/Conversion.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/serdes/Node.hpp>
#include <airtree/core/serdes/trie1d/1DxP.hpp>
#include <airtree/core/Logger.hpp>

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
TrieNode_20_Level2 *newLevel2(TrieNode_20_Level1 *level1, unsigned int index) {
  level1->nodes[index] = std::make_unique<TrieNode_20_Level2>();
  return level1->nodes[index].get();
}

std::unique_ptr<TrieNode_20> CreateParentNode_20() {
  auto parentNode = std::make_unique<TrieNode_20>();
  for (auto &child : parentNode->nodes)
    child = std::make_unique<TrieNode_20_Level1>();
  return parentNode;
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
      root, specialCounts);
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
  auto process_array = [&](const auto *typed_values) {
    for (int i = 0; i < array.length; ++i) {
      double value = static_cast<double>(typed_values[i]);
      uint64_t fpNumber;
      std::memcpy(&fpNumber, &value, sizeof(value));
      if (!isSpecialCase(fpNumber, specialCounts))
        createAndInsertFP20(root.get(), fpNumber);
    }
  };

  dispatchFPHArray(array, process_array);

  curr_trie_size = sizeof(TrieNode_20) + BINS_256 * sizeof(TrieNode_20_Level1);
  for (const auto &level1 : root->nodes)
    curr_trie_size += level1->populated.count() * sizeof(TrieNode_20_Level2);
  return root;
}

std::vector<char>
execSerialization_TrieNode20(const std::unique_ptr<TrieNode_20> &root,
                             const SpecialCounts &specialCounts) {
  auto header = airtree::core::common::makeHeader(
      ConfigWire::Config_1D_Precise, {}, countObservations(root->counts),
      specialCounts.posInfCount, specialCounts.negInfCount,
      specialCounts.posZeroCount, specialCounts.negZeroCount,
      specialCounts.nanCount);

  std::vector<char> buffer;
  serializeHeader(header, buffer);
  size_t header_end = buffer.size();
  serialize_1DxP(root.get(), buffer);
  finalizeHeader(buffer, buffer.size() - header_end);

  return buffer;
}