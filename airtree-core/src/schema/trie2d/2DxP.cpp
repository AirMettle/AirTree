// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include "airtree/core/api/AirTreeGenerator.hpp"
#include <airtree/core/schema/trie2d/2DxP.hpp>
#include <airtree/core/Logger.hpp>
#include <airtree/util/FeatureFlags.h>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/common/BitCodec.hpp>
#include <airtree/core/serdes/trie2d/2DxP.hpp>
#include <airtree/util/UUID.hpp>

using namespace airtree::core;
using namespace airtree::core::common;
using namespace airtree::core::api;
using namespace airtree::core::schema::trie2d;
using namespace airtree::util::uuid;

std::vector<char>
Generator2DxP::generate(const std::vector<const FPHArray *> &arrays) const {
  if (arrays.size() != 2) {
    throw std::invalid_argument("Expected exactly 2 arrays for 2D generation");
  }
  return generate_2DxP(*arrays[0], *arrays[1]);
}

std::unique_ptr<TLEoption3_2D> CreateParent_TLE2D_option3() {
  auto parentNode = std::make_unique<TLEoption3_2D>();
  parentNode->populated.reset();

  return parentNode;
}

std::vector<char> generate_2DxP(const FPHArray &array1, const FPHArray &array2) {
  std::string uuid = AirTreeUUID::generateUUID();
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[generate] [traceID: {}] Generating 2DxP Trie for {} values in dim1 "
      "and {} values in dim2.",
      uuid, array1.length, array2.length);
  uint64_t curr_trie_size = 0;

  std::unique_ptr<SpecialCounts> specialCounts =
      std::make_unique<SpecialCounts>();
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[insert] [traceID: {}] Filing and inserting {} values for dim1 and {} "
      "values for dim2 "
      "into 2DxP Trie.",
      uuid, array1.length, array2.length);
  std::unique_ptr<TLEoption3_2D> root = execCreateAndInsert_2D_2x10(
      array1, array2, curr_trie_size, specialCounts);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[insert] [traceID: {}] Completed filing and inserting into 2DxP Trie. "
      "Final trie size: {}.",
      uuid, curr_trie_size);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[serialization] [traceID: {}] Serializing 2DxP trie of size {}.", uuid,
      curr_trie_size);
  std::vector<char> buffer = execSerialize_2D_2x10(
      root.get(), curr_trie_size, specialCounts);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[serialization] [traceID: {}] Completed serializing 2DxP Trie.", uuid);
  SPDLOG_LOGGER_DEBUG(logger(),
                     "[generate] [traceID: {}] Completed generating 2DxP Trie.",
                     uuid);
  return buffer;
}

void insertintoTLETrie_2D_option3(TLEoption3_2D *root, unsigned int combined,
                                  unsigned int combinedTLE, int ndims,
                                  uint64_t &curr_trie_size) {
  if (!root) {
    SPDLOG_LOGGER_ERROR(
        logger(), "Root is null in insertintoTLETrie_2D_option3.");
    return;
  }

  if (!root->populated.test(combinedTLE)) {
    root->populated.set(combinedTLE);
  }

  root->counts[combinedTLE]++;

  if (ndims == 0) {
    return;
  }

  if (ndims == 3) {
    unsigned int first10 = (combined >> 10) & 0x3FF;
    unsigned int last10 = combined & 0x3FF;

    if (!root->nodes[combinedTLE]) {
      root->nodes[combinedTLE] = std::make_unique<TrieNode_2D_10>();
      curr_trie_size += sizeof(TrieNode_2D_10);
    }

    if (!root->nodes[combinedTLE]->populated.test(first10)) {
      root->nodes[combinedTLE]->populated.set(first10);
      root->nodes[combinedTLE]->nodes[first10] =
          std::make_unique<TrieNode_2D_10_Level1>();
      curr_trie_size += sizeof(TrieNode_2D_10_Level1);
    }

    root->nodes[combinedTLE]->counts[first10]++;
    root->nodes[combinedTLE]->nodes[first10]->counts[last10]++;
  }

  if (ndims == 1 || ndims == 2) {

    if (!root->nodes[combinedTLE]) {
      root->nodes[combinedTLE] = std::make_unique<TrieNode_2D_10>();
      curr_trie_size += sizeof(TrieNode_2D_10);
    }

    if (!root->nodes[combinedTLE]->populated.test(combined)) {
      root->nodes[combinedTLE]->populated.set(combined);
    }
    root->nodes[combinedTLE]->counts[combined]++;
  }
}


std::unique_ptr<TLEoption3_2D> execCreateAndInsert_2D_2x10(
    const FPHArray &array1, const FPHArray &array2, uint64_t &curr_trie_size,
    std::unique_ptr<SpecialCounts> &specialCounts) {
  if (array1.length != array2.length) {
    throw std::invalid_argument("Dimensions must be of equal length");
    SPDLOG_LOGGER_ERROR(
        logger(), "Length of Dimension-1 and Dimension-2 is not equal");
  }

  std::unique_ptr<TLEoption3_2D> root = CreateParent_TLE2D_option3(); // 10x10
  curr_trie_size += sizeof(TLEoption3_2D);

  dispatchFPHArray(array1, [&](const auto *vals1) {
    dispatchFPHArray(array2, [&](const auto *vals2) {
      for (int i = 0; i < array1.length; ++i) {

        createAndInsert_2DxP(root.get(), vals1[i], vals2[i], curr_trie_size,
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

  return root;
}

std::vector<char>
execSerialize_2D_2x10(TLEoption3_2D *root, uint64_t &curr_trie_size,
                      std::unique_ptr<SpecialCounts> &specialCounts) {
  auto header = airtree::core::common::makeHeader(
      ConfigWire::Config_2D_Precise, {}, countObservations(root->counts), specialCounts->posInfCount,
      specialCounts->negInfCount, specialCounts->posZeroCount,
      specialCounts->negZeroCount, specialCounts->nanCount);

  std::vector<char> buffer;
  buffer.reserve(kHeaderLength + curr_trie_size);
  serializeHeader(header, buffer);
  size_t header_end = buffer.size();
  serialize_2DxP(root, buffer);
  finalizeHeader(buffer, buffer.size() - header_end);

  return buffer;
}