// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include "airtree/core/api/AirTreeGenerator.hpp"
#include <airtree/core/schema/trie2d/2DxF.hpp>
#include <airtree/core/common/NodeOps.hpp>
#include <airtree/core/Logger.hpp>
#include <airtree/util/FeatureFlags.h>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/common/BitCodec.hpp>
#include <airtree/core/serdes/Node.hpp>
#include <airtree/core/serdes/trie2d/2DxF.hpp>
#include <airtree/util/UUID.hpp>
#include <string>

using namespace airtree::core;
using namespace airtree::core::common;
using namespace airtree::core::api;
using namespace airtree::core::schema::trie2d;
using namespace airtree::util::uuid;

std::vector<char>
Generator2DxF::generate(const std::vector<const FPHArray *> &arrays) const {
  if (arrays.size() != 2) {
    throw std::invalid_argument("Expected exactly 2 arrays for 2D generation");
  }
  return generate_2DxF(*arrays[0], *arrays[1]);
}

std::unique_ptr<TLETrieNode_2D> CreateParentNode_TLE2D88() {
  auto parentNode = std::make_unique<TLETrieNode_2D>();
  parentNode->populated.reset();

  return parentNode;
}


void insertintoTLETrie_2D_88(TLETrieNode_2D *root, unsigned int combined,
                             unsigned int combinedTLE, int ndims,
                             uint64_t &curr_trie_size) {
  if (!root) {
    SPDLOG_LOGGER_ERROR(logger(), "Root is null in insertintoTLETrie_2D_88.");
    return;
  }
  if (ndims == 0) {
    bumpCount(root->populated, root->TLEcounts, combinedTLE);
    return;
  }
  TrieNode_16 *level1 = descend(root->populated, root->nodes, combinedTLE, curr_trie_size);
  if (ndims != 3) {
    bumpCount(level1->populated, level1->counts, combined);
    return;
  }
  TrieNode_16_Level1 *level2 =
      descend(level1->populated, level1->nodes, (combined >> 8) & 0xFF, curr_trie_size);
  level2->counts[combined & 0xFF]++;
}

void rollUpCounts(TLETrieNode_2D *root) {
  forEachSetBit(root->populated.words, BINS_64, [&](std::size_t t) {
    if (root->nodes[t])
      root->TLEcounts[t] = rollUpNode(root->nodes[t].get());
  });
}

std::vector<char> generate_2DxF(const FPHArray &array1, const FPHArray &array2) {
  std::string uuid = AirTreeUUID::generateUUID();
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[generate] [traceID: {}] Generating 2DxF Trie for {} "
      "values in dim1 and {} values in dim2.",
      uuid, array1.length, array2.length);
  uint64_t curr_trie_size = 0;
  std::unique_ptr<SpecialCounts> specialCounts =
      std::make_unique<SpecialCounts>();
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[insert] [traceID: {}] Filing and inserting {} values for dim1 and {} "
      "values for dim2 "
      "into 2DxF Trie.",
      uuid, array1.length, array2.length);
  std::unique_ptr<TLETrieNode_2D> root = execCreateAndInsert_2D(
      array1, array2, curr_trie_size, specialCounts);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[insert] [traceID: {}] Completed filing and inserting into 2DxF Trie. "
      "Final trie size: {}.",
      uuid, curr_trie_size);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[serialization] [traceID: {}] Serializing 2DxF trie of size {}.", uuid,
      curr_trie_size);
  std::vector<char> buffer =
      execSerialize_2D(root.get(), specialCounts);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[serialization] [traceID: {}] Completed serializing 2DxF Trie.", uuid);
  SPDLOG_LOGGER_DEBUG(logger(),
                     "[generate] [traceID: {}] Completed generating 2DxF Trie.",
                     uuid);
  return buffer;
}

std::unique_ptr<TLETrieNode_2D> execCreateAndInsert_2D(
    const FPHArray &array1, const FPHArray &array2, uint64_t &curr_trie_size,
    std::unique_ptr<SpecialCounts> &specialCounts) {
  if (array1.length != array2.length) {
    SPDLOG_LOGGER_ERROR(
        logger(), "Length of Dimension-1 and Dimension-2 is not equal");
  }

  std::unique_ptr<TLETrieNode_2D> root = CreateParentNode_TLE2D88();
  curr_trie_size += sizeof(TLETrieNode_2D);

  dispatchFPHArray(array1, [&](const auto *vals1) {
    dispatchFPHArray(array2, [&](const auto *vals2) {
      for (int i = 0; i < array1.length; ++i) {

        createAndInsert_2DxF(root.get(), vals1[i], vals2[i], curr_trie_size,
                             specialCounts);
        if (enable_threshold_2D && curr_trie_size > threshold_2D) {
          SPDLOG_LOGGER_ERROR(logger(),
                              "Trie size exceeded threshold limit of {}.",
                              threshold_2D);
          SPDLOG_LOGGER_ERROR(
              logger(),
              "Insertion process stopped at index {} of the input dataset.", i);
          SPDLOG_LOGGER_ERROR(logger(), "Last filed values -> @ 0: {} @ 1: {}",
                              vals1[i], vals2[i]);
          break;
        }
      }
    });
  });
  rollUpCounts(root.get());
  return root;
}

std::vector<char>
execSerialize_2D(TLETrieNode_2D *root, std::unique_ptr<SpecialCounts> &specialCounts) {
  auto header = airtree::core::common::makeHeader(
      ConfigWire::Config_2D_Fast, {}, countObservations(root->TLEcounts),
      specialCounts->posInfCount, specialCounts->negInfCount,
      specialCounts->posZeroCount, specialCounts->negZeroCount,
      specialCounts->nanCount);

  std::vector<char> buffer;
  serializeHeader(header, buffer);
  size_t header_end = buffer.size();
  serialize_2DxF(root, buffer);
  finalizeHeader(buffer, buffer.size() - header_end);

  return buffer;
}