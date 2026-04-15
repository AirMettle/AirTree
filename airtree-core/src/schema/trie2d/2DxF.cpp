#include "airtree/core/api/AirTreeGenerator.hpp"
#include <airtree/core/schema/trie2d/2DxF.hpp>
#include <airtree/core/Logger.hpp>
#include <airtree/util/FeatureFlags.h>
#include <airtree/core/serdes/Header.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/common/BitCodec.hpp>
#include <airtree/core/serdes/trie2d/2DxF.hpp>
#include <airtree/util/UUID.hpp>
#include <string>

using namespace airtree::core;
using namespace airtree::core::api;
using namespace airtree::core::schema::trie2d;
using namespace airtree::util::uuid;

std::vector<char>
Generator2DxF::generate(const std::vector<const FPHArray *> &arrays,
                        bool default_mode) const {
  if (arrays.size() != 2) {
    throw std::invalid_argument("Expected exactly 2 arrays for 2D generation");
  }
  return generate_2DxF(*arrays[0], *arrays[1], default_mode);
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

  if (!root->populated.test(combinedTLE)) {
    root->populated.set(combinedTLE);
  }

  root->TLEcounts[combinedTLE]++;

  if (ndims == 0) {
    return;
  }

  if (ndims == 1 || ndims == 2) {
    if (!root->nodes[combinedTLE]) {
      root->nodes[combinedTLE] = std::make_unique<TrieNode_16>();
      curr_trie_size += sizeof(TrieNode_16);
    }
    if (!root->nodes[combinedTLE]->populated.test(combined)) {
      root->nodes[combinedTLE]->populated.set(combined);
    }
    root->nodes[combinedTLE]->counts[combined]++;
  }

  if (ndims == 3) {
    unsigned int first8 = combined >> 8 & 0xFF;
    unsigned int last8 = combined & 0xFF;

    if (!root->nodes[combinedTLE]) {
      root->nodes[combinedTLE] = std::make_unique<TrieNode_16>();
      curr_trie_size += sizeof(TrieNode_16);
    }

    if (!root->nodes[combinedTLE]->populated.test(first8)) {
      root->nodes[combinedTLE]->populated.set(first8);
      root->nodes[combinedTLE]->nodes[first8] =
          std::make_unique<TrieNode_16_Level1>();
      curr_trie_size += sizeof(TrieNode_16_Level1);
    }

    root->nodes[combinedTLE]->counts[first8]++;
    root->nodes[combinedTLE]->nodes[first8]->counts[last8]++;
  }
}

std::vector<char> generate_2DxF(const FPHArray &array1, const FPHArray &array2,
                                bool default_mode) {
  std::string uuid = AirTreeUUID::generateUUID();
  SPDLOG_LOGGER_INFO(
      logger(),
      "[generate] [traceID: {}] Generating 2DxF Trie for {} "
      "values in dim1 and {} values in dim2 using default_mode {}.",
      uuid, array1.length, array2.length, default_mode);
  uint64_t curr_trie_size = 0;
  std::unique_ptr<SpecialCounts> specialCounts =
      std::make_unique<SpecialCounts>();
  SPDLOG_LOGGER_INFO(
      logger(),
      "[insert] [traceID: {}] Filing and inserting {} values for dim1 and {} "
      "values for dim2 using "
      "default_mode {} into 2DxF Trie.",
      uuid, array1.length, array2.length, default_mode);
  std::unique_ptr<TLETrieNode_2D> root = execCreateAndInsert_2D(
      array1, array2, curr_trie_size, specialCounts, default_mode);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[insert] [traceID: {}] Completed filing and inserting into 2DxF Trie. "
      "Final trie size: {}.",
      uuid, curr_trie_size);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[serialization] [traceID: {}] Serializing 2DxF trie of size {}.", uuid,
      curr_trie_size);
  std::vector<char> buffer =
      execSerialize_2D(root.get(), curr_trie_size, specialCounts, default_mode);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[serialization] [traceID: {}] Completed serializing 2DxF Trie.", uuid);
  SPDLOG_LOGGER_INFO(logger(),
                     "[generate] [traceID: {}] Completed generating 2DxF Trie.",
                     uuid);
  return buffer;
}

std::unique_ptr<TLETrieNode_2D> execCreateAndInsert_2D(
    const FPHArray &array1, const FPHArray &array2, uint64_t &curr_trie_size,
    std::unique_ptr<SpecialCounts> &specialCounts, bool default_mode) {
  if (array1.length != array2.length) {
    SPDLOG_LOGGER_ERROR(
        logger(), "Length of Dimension-1 and Dimension-2 is not equal");
  }

  std::unique_ptr<TLETrieNode_2D> root = CreateParentNode_TLE2D88();
  curr_trie_size += sizeof(TLETrieNode_2D);

  dispatchFPHArray(array1, [&](const auto *vals1) {
    dispatchFPHArray(array2, [&](const auto *vals2) {
      for (int i = 0; i < array1.length; ++i) {

        std::pair<TLE, unsigned int> input_1 =
            internal_8bit(vals1[i], default_mode);
        std::pair<TLE, unsigned int> input_2 =
            internal_8bit(vals2[i], default_mode);

        TLE tle1 = input_1.first;
        TLE tle2 = input_2.first;

        unsigned int internalFPHNumber1 = input_1.second;
        unsigned int internalFPHNumber2 = input_2.second;

        unsigned int combinedTLE = (tle1.TLE << 3) | tle2.TLE;

        // Check special conditions and set ndims accordingly
        unsigned int isTle1Special = update_special_counts(tle1, specialCounts);
        unsigned int isTle2Special = update_special_counts(tle2, specialCounts);

        unsigned int ndims = (~((isTle1Special << 1) | isTle2Special)) & 0x3;

        unsigned int combined = 0;

        switch (ndims) {
        case 0:
          combined = 0; // No need to compute internal numbers
          break;
        case 1:
          combined = internalFPHNumber2;
          break;
        case 2:
          combined = internalFPHNumber1;
          break;
        case 3:
          combined =
              combine_chunks_8b_temp(internalFPHNumber1, internalFPHNumber2);
          break;
        default:
          SPDLOG_LOGGER_ERROR(logger(), "Invalid value for ndims");
          break;
        }
        insertintoTLETrie_2D_88(
            root.get(), combined, combinedTLE, ndims, curr_trie_size);
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
execSerialize_2D(TLETrieNode_2D *root, uint64_t &curr_trie_size,
                 std::unique_ptr<SpecialCounts> &specialCounts,
                 bool default_mode) {
  std::vector<char> buffer;

  buffer.reserve(sizeof(trie_header) + curr_trie_size);
  trie_header t_header;
  strcpy(t_header.type_code, "HierFPHG");
  t_header.version = 0;
  t_header.m_width = 3;
  t_header.precision_bits = 5;
  t_header.node_width = 8;
  t_header.nan_count = specialCounts->nanCount;
  t_header.neg_inf_count = specialCounts->negInfCount;
  t_header.pos_inf_count = specialCounts->posInfCount;
  t_header.pos_zero_count = specialCounts->posZeroCount;
  strcpy(t_header.type, "2-D");
  strcpy(t_header.config, "288");
  t_header.mode = default_mode ? 1 : 0;
  // Add the logic to handle the special counts from the TLE
  t_header.trie_root_ref = sizeof(trie_header);
  serializeTrieHeader(t_header, buffer);
  // serialization logic goes here
  serialize_2DxF(root, buffer);

  return buffer;
}