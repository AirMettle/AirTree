// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/schema/trie1d/1DxF.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/common/Conversion.hpp>
#include <airtree/core/Logger.hpp>
#include <airtree/core/serdes/Node.hpp>
#include <airtree/core/serdes/trie1d/1DxF.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/util/UUID.hpp>

using namespace airtree::core;
using namespace airtree::core::common;
using namespace airtree::core::api;
using namespace airtree::core::schema::trie1d;
using namespace airtree::util::uuid;

std::vector<char> Generator1DxF::generate(
    const std::vector<const FPHArray *> &arrays) const {
  if (arrays.size() != 1) {
    throw std::invalid_argument("Expected exactly 1 array for 1D generation");
  }
  return generate_1DxF(*arrays[0]);
}

std::unique_ptr<TrieNode_16> CreateParentNode_16() {
  auto parentNode = std::make_unique<TrieNode_16>();
  for (auto &child : parentNode->nodes)
    child = std::make_unique<TrieNode_16_Level1>();
  return parentNode;
}

std::vector<char> generate_1DxF(const FPHArray &array) {
  // generate a UUID for this trie
  std::string uuid = AirTreeUUID::generateUUID();
  SPDLOG_LOGGER_DEBUG(logger(),
                     "[generate] [traceID: {}] Generating 1DxF Trie for {} "
                     "values.",
                     uuid, array.length);
  SpecialCounts specialCounts;

  uint64_t curr_trie_size = 0;
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[insert] [traceID: {}] Filing and inserting {} values "
      "into 1DxF Trie.",
      uuid, array.length);
  std::unique_ptr<TrieNode_16> root = execCreateAndInsert_TrieNode16(
      specialCounts, curr_trie_size, array);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[insert] [traceID: {}] Completed filing and inserting values into 1DxF "
      "Trie. Final trie size: {}.",
      uuid, curr_trie_size);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[serialization] [traceID: {}] Serializing 1DxF trie of size {}.", uuid,
      curr_trie_size);
  std::vector<char> buffer = execSerialization_TrieNode16(
      root, specialCounts);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[serialization] [traceID: {}] Completed serializing 1DxF Trie.", uuid);
  SPDLOG_LOGGER_DEBUG(logger(),
                     "[generate] [traceID: {}] Completed generating 1DxF Trie.",
                     uuid);
  return buffer;
}

std::unique_ptr<TrieNode_16>
execCreateAndInsert_TrieNode16(SpecialCounts &specialCounts,
                               uint64_t &curr_trie_size, const FPHArray &array) {
  std::unique_ptr<TrieNode_16> root = CreateParentNode_16();
  curr_trie_size = sizeof(TrieNode_16) + BINS_256 * sizeof(TrieNode_16_Level1);

  auto process_array = [&](const auto *typed_values) {
    for (int i = 0; i < array.length; ++i) {
      double value = static_cast<double>(typed_values[i]);
      uint64_t fpNumber;
      std::memcpy(&fpNumber, &value, sizeof(value));
      if (!isSpecialCase(fpNumber, specialCounts))
        createAndInsertFP16(root.get(), fpNumber);
    }
  };

  dispatchFPHArray(array, process_array);
  return root;
}

std::vector<char>
execSerialization_TrieNode16(const std::unique_ptr<TrieNode_16> &root,
                             const SpecialCounts &specialCounts) {
  auto header = airtree::core::common::makeHeader(
      ConfigWire::Config_1D_Fast, {}, countObservations(root->counts),
      specialCounts.posInfCount, specialCounts.negInfCount,
      specialCounts.posZeroCount, specialCounts.negZeroCount,
      specialCounts.nanCount);

  std::vector<char> buffer;
  serializeHeader(header, buffer);
  size_t header_end = buffer.size();
  serialize_1DxF(root.get(), buffer);
  finalizeHeader(buffer, buffer.size() - header_end);

  return buffer;
}