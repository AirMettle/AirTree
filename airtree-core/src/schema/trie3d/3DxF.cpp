// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include "airtree/core/api/AirTreeGenerator.hpp"
#include <airtree/core/schema/trie3d/3DxF.hpp>
#include <airtree/core/Logger.hpp>
#include <airtree/util/FeatureFlags.h>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/common/BitCodec.hpp>
#include <airtree/core/serdes/trie3d/3DxF.hpp>
#include <airtree/util/UUID.hpp>

using namespace airtree::core;
using namespace airtree::core::common;
using namespace airtree::core::api;
using namespace airtree::core::schema::trie3d;
using namespace airtree::util::uuid;


std::vector<char>
Generator3DxF::generate(const std::vector<const FPHArray *> &arrays,
                        bool default_mode) const {
  if (arrays.size() != 3) {
    throw std::invalid_argument("Expected exactly 3 arrays for 3D generation");
  }
  return generate_3DxF(*arrays[0], *arrays[1], *arrays[2], default_mode);
}

void insertintoTrie_3D_888(TLE_3D_888 *root, unsigned int combined,
                           unsigned int combinedTLE, int ndims,
                           uint64_t &curr_trie_size) {

  if (!root) {
    SPDLOG_LOGGER_ERROR(logger(), "Root is null.");
    return;
  }

  // update TLE level
  if (!root->populated.test(combinedTLE)) {
    root->populated.set(combinedTLE);
  }
  root->counts[combinedTLE]++;

  if (ndims == 0) {
    return;
  }

  // if ndims is one of 1, 2 or 4 then we are processing a 8 bit value (max).
  if (ndims == 1 || ndims == 2 || ndims == 4) {
    // combined is a 8 bit number
    unsigned int l0_8 = combined;

    if (!root->nodes[combinedTLE]) {
      root->nodes[combinedTLE] = std::make_unique<Node3D_888_l0>();
      curr_trie_size += sizeof(Node3D_888_l0);
    }

    // if populated of l0_8 is not set
    if (!root->nodes[combinedTLE]->populated.test(l0_8)) {
      root->nodes[combinedTLE]->populated.set(l0_8);
    }

    root->nodes[combinedTLE]->counts[l0_8]++;
  }

  // if ndims is one of 3, 5 or 6 then we are processing a 16 bit value (max).
  if (ndims == 3 || ndims == 5 || ndims == 6) {
    unsigned int first8 = combined >> 8 & 0xFF;
    unsigned int last8 = combined & 0xFF;

    if (!root->nodes[combinedTLE]) {
      root->nodes[combinedTLE] = std::make_unique<Node3D_888_l0>();
      curr_trie_size += sizeof(Node3D_888_l0);
    }

    if (!root->nodes[combinedTLE]->populated.test(first8)) {
      root->nodes[combinedTLE]->populated.set(first8);
      root->nodes[combinedTLE]->nodes[first8] = std::make_unique<TrieNode_16>();
      curr_trie_size += sizeof(TrieNode_16);
    }

    root->nodes[combinedTLE]->counts[first8]++;

    if (!root->nodes[combinedTLE]->nodes[first8]->populated.test(last8)) {
      root->nodes[combinedTLE]->nodes[first8]->populated.set(last8);
    }

    root->nodes[combinedTLE]
        ->nodes[first8]
        ->counts[last8]++; // Trienode_16 count update
  }

  // if ndims is 7 then we are processing a 24 bit value (max).
  if (ndims == 7) {

    unsigned int first8 = combined >> 16 & 0xFF;
    unsigned int middle8 = combined >> 8 & 0xFF;
    unsigned int last8 = combined & 0xFF;

    if (!root->nodes[combinedTLE]) {
      root->nodes[combinedTLE] = std::make_unique<Node3D_888_l0>();
      curr_trie_size += sizeof(Node3D_888_l0);
    }

    if (!root->nodes[combinedTLE]->populated.test(first8)) {
      root->nodes[combinedTLE]->populated.set(first8);
      root->nodes[combinedTLE]->nodes[first8] = std::make_unique<TrieNode_16>();
      curr_trie_size += sizeof(TrieNode_16);
    }

    root->nodes[combinedTLE]->counts[first8]++;

    if (!root->nodes[combinedTLE]->nodes[first8]->populated.test(middle8)) {
      root->nodes[combinedTLE]->nodes[first8]->populated.set(middle8);
      root->nodes[combinedTLE]->nodes[first8]->nodes[middle8] =
          std::make_unique<TrieNode_16_Level1>();
      curr_trie_size += sizeof(TrieNode_16_Level1);
    }

    root->nodes[combinedTLE]
        ->nodes[first8]
        ->counts[middle8]++; // Trienode_16 count update
    root->nodes[combinedTLE]
        ->nodes[first8]
        ->nodes[middle8]
        ->counts[last8]++; // Trienode_16_l1 count update
  }
}

std::vector<char> generate_3DxF(const FPHArray &array1, const FPHArray &array2,
                                const FPHArray &array3, bool default_mode) {
  std::string uuid = AirTreeUUID::generateUUID();
  SPDLOG_LOGGER_INFO(
      logger(),
      "[generate] [traceID: {}] Generating 3DxF Trie for {} values in dim1, "
      "{} values in dim2 and {} values in dim3 using default_mode {}.",
      uuid, array1.length, array2.length, array3.length, default_mode);
  uint64_t curr_trie_size = 0;
  std::unique_ptr<SpecialCounts> specialCounts =
      std::make_unique<SpecialCounts>();
  SPDLOG_LOGGER_INFO(
      logger(),
      "[insert] [traceID: {}] Filing and inserting {} values for dim1, {} "
      "values for dim2 and {} values for dim3 using "
      "default_mode {} into 3DxF Trie.",
      uuid, array1.length, array2.length, array3.length, default_mode);
  std::unique_ptr<TLE_3D_888> root = execCreateAndInsert_3D_888(
      array1, array2, array3, curr_trie_size, specialCounts, default_mode);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[insert] [traceID: {}] Completed filing and inserting into 3DxF Trie. "
      "Final trie size: {}.",
      uuid, curr_trie_size);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[serialization] [traceID: {}] Serializing 3DxF trie of size {}.", uuid,
      curr_trie_size);
  std::vector<char> buffer = execSerialize_3D_888(
      root.get(), curr_trie_size, specialCounts, default_mode);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[serialization] [traceID: {}] Completed serializing 3DxF Trie.", uuid);
  SPDLOG_LOGGER_INFO(logger(),
                     "[generate] [traceID: {}] Completed generating 3DxF Trie.",
                     uuid);
  return buffer;
}

std::unique_ptr<TLE_3D_888>
execCreateAndInsert_3D_888(const FPHArray &array1, const FPHArray &array2,
                           const FPHArray &array3, uint64_t &curr_trie_size,
                           std::unique_ptr<SpecialCounts> &specialCounts,
                           bool default_mode) {
  if (array1.length != array2.length and array1.length != array3.length) {
    throw std::invalid_argument("Dimensions must be of equal length");
    SPDLOG_LOGGER_ERROR(
        logger(),
        "Dimension size mismatch between the three dimensions.Must be of "
        "equal Length");
  }

  std::unique_ptr<TLE_3D_888> root = std::make_unique<TLE_3D_888>();
  curr_trie_size += sizeof(TLE_3D_888);

  dispatchFPHArray(array1, [&](const auto *vals1) {
    dispatchFPHArray(array2, [&](const auto *vals2) {
      dispatchFPHArray(array3, [&](const auto *vals3) {
        for (int i = 0; i < array1.length; ++i) {

          std::pair<TLE, unsigned int> input_1 =
              internal_8bit(vals1[i], default_mode);
          std::pair<TLE, unsigned int> input_2 =
              internal_8bit(vals2[i], default_mode);
          std::pair<TLE, unsigned int> input_3 =
              internal_8bit(vals3[i], default_mode);

          TLE tle1 = input_1.first;
          TLE tle2 = input_2.first;
          TLE tle3 = input_3.first;

          unsigned int internalFPHNumber1 = input_1.second;
          unsigned int internalFPHNumber2 = input_2.second;
          unsigned int internalFPHNumber3 = input_3.second;

          unsigned int combinedTLE =
              (tle1.encoding << 6) | (tle2.encoding << 3) | tle3.encoding;

          // Check special conditions and set ndims accordingly
          unsigned int isTle1Special =
              update_special_counts(tle1, specialCounts);
          unsigned int isTle2Special =
              update_special_counts(tle2, specialCounts);
          unsigned int isTle3Special =
              update_special_counts(tle3, specialCounts);

          int ndims = (~((isTle1Special << 2) | (isTle2Special << 1)
                         | isTle3Special << 0))
                      & 0x7;
          unsigned int combined = 0;

          switch (ndims) {
          case 0:
            combined = 0;
            break;
          case 1:
            combined = internalFPHNumber3;
            break;
          case 2:
            combined = internalFPHNumber2;
            break;
          case 3:
            combined =
                combine_chunks_8b(internalFPHNumber2, internalFPHNumber3);
            break;
          case 4:
            combined = internalFPHNumber1;
            break;
          case 5:
            combined =
                combine_chunks_8b(internalFPHNumber1, internalFPHNumber3);
            break;
          case 6:
            combined =
                combine_chunks_8b(internalFPHNumber1, internalFPHNumber2);
            break;
          case 7:
            combined = combine_chunks_8b(
                internalFPHNumber1, internalFPHNumber2, internalFPHNumber3);
            break;
          default:
            SPDLOG_LOGGER_ERROR(logger(), "Invalid number of dimensions");
            break;
          }
          insertintoTrie_3D_888(
              root.get(), combined, combinedTLE, ndims, curr_trie_size);
          if (enable_threshold_3D && curr_trie_size > threshold_3D) {
            SPDLOG_LOGGER_ERROR(logger(),
                                "Trie size exceeded threshold limit of {}.",
                                threshold_3D);
            SPDLOG_LOGGER_ERROR(
                logger(),
                "Insertion process stopped at index {} of the input dataset.",
                i);
            SPDLOG_LOGGER_ERROR(logger(),
                                "Last filed values -> @ 0: {} @ 1: {} @ 2: {}",
                                vals1[i], vals2[i], vals3[i]);
            break;
          }
        }
      });
    });
  });


  return root;
}

std::vector<char>
execSerialize_3D_888(TLE_3D_888 *root, uint64_t &curr_trie_size,
                     std::unique_ptr<SpecialCounts> &specialCounts,
                     [[maybe_unused]] bool default_mode) {
  auto header = airtree::core::common::makeHeader(
      ConfigWire::Config_3D_Fast, {}, 0,
      specialCounts->posInfCount, specialCounts->negInfCount,
      specialCounts->posZeroCount, specialCounts->negZeroCount,
      specialCounts->nanCount);

  std::vector<char> buffer;
  buffer.reserve(kHeaderLength + curr_trie_size);
  serializeHeader(header, buffer);
  size_t header_end = buffer.size();
  serialize_3DxF(root, buffer);
  finalizeHeader(buffer, buffer.size() - header_end);

  return buffer;
}