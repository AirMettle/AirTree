// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include "airtree/core/api/AirTreeGenerator.hpp"
#include <airtree/core/schema/trie3d/3DxP.hpp>
#include <airtree/core/common/NodeOps.hpp>
#include <airtree/core/Logger.hpp>
#include <airtree/util/FeatureFlags.h>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/common/BitCodec.hpp>
#include <airtree/core/serdes/Node.hpp>
#include <airtree/core/serdes/trie3d/3DxP.hpp>
#include <airtree/util/UUID.hpp>

using namespace airtree::core;
using namespace airtree::core::common;
using namespace airtree::core::api;
using namespace airtree::core::schema::trie3d;
using namespace airtree::util::uuid;

std::vector<char>
Generator3DxP::generate(const std::vector<const FPHArray *> &arrays) const {
  if (arrays.size() != 3) {
    throw std::invalid_argument("Expected exactly 3 arrays for 3D generation");
  }
  return generate_3DxP(*arrays[0], *arrays[1], *arrays[2]);
}

void insertintoTrie_3D_3x10(TLE_3D_3x10 *root, unsigned int combined,
                            unsigned int combinedTLE, int ndims,
                            uint64_t &curr_trie_size) {
  static constexpr uint8_t kDepth[8] = {0, 1, 1, 2, 1, 2, 2, 3};
  const int depth = kDepth[ndims & 0x7];
  if (depth == 0) {
    bumpCount(root->populated, root->counts, combinedTLE);
    return;
  }
  Node3D_3x10_l0 *l0 = descend(root->populated, root->nodes, combinedTLE, curr_trie_size);
  const unsigned int c0 = (combined >> (10 * (depth - 1))) & 0x3FF;
  if (depth == 1) {
    bumpCount(l0->populated, l0->counts, c0);
    return;
  }
  Node3D_3x10_l1 *l1 = descend(l0->populated, l0->nodes, c0, curr_trie_size);
  const unsigned int c1 = (combined >> (10 * (depth - 2))) & 0x3FF;
  if (depth == 2) {
    bumpCount(l1->populated, l1->counts, c1);
    return;
  }
  Node3D_3x10_l2 *l2 = descend(l1->populated, l1->nodes, c1, curr_trie_size);
  bumpCount(l2->populated, l2->counts, combined & 0x3FF);
}

void rollUpCounts(TLE_3D_3x10 *root) { rollUpNode(root); }

std::vector<char> generate_3DxP(const FPHArray &array1, const FPHArray &array2,
                                const FPHArray &array3) {
  std::string uuid = AirTreeUUID::generateUUID();
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[generate] [traceID: {}] Generating 3DxP Trie for {} values in dim1, "
      "{} values in dim2 and {} values in dim3.",
      uuid, array1.length, array2.length, array3.length);
  uint64_t curr_trie_size = 0;
  std::unique_ptr<SpecialCounts> specialCounts =
      std::make_unique<SpecialCounts>();
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[insert] [traceID: {}] Filing and inserting {} values for dim1, {} "
      "values for dim2 and {} values for dim3 "
      "into 3DxP Trie.",
      uuid, array1.length, array2.length, array3.length);
  std::unique_ptr<TLE_3D_3x10> root = execCreateAndInsert_3D_3x10(
      array1, array2, array3, curr_trie_size, specialCounts);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[insert] [traceID: {}] Completed filing and inserting into 3DxP Trie. "
      "Final trie size: {}.",
      uuid, curr_trie_size);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[serialization] [traceID: {}] Serializing 3DxP trie of size {}.", uuid,
      curr_trie_size);
  std::vector<char> buffer = execSerialize_3D_3x10(
      root.get(), specialCounts);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[serialization] [traceID: {}] Completed serializing 3DxP Trie.", uuid);
  SPDLOG_LOGGER_DEBUG(logger(),
                     "[generate] [traceID: {}] Completed generating 3DxP Trie.",
                     uuid);
  return buffer;
}

std::unique_ptr<TLE_3D_3x10>
execCreateAndInsert_3D_3x10(const FPHArray &array1, const FPHArray &array2,
                            const FPHArray &array3, uint64_t &curr_trie_size,
                            std::unique_ptr<SpecialCounts> &specialCounts) {
  if (array1.length != array2.length and array1.length != array3.length) {
    throw std::invalid_argument("Dimensions must be of equal length");
    SPDLOG_LOGGER_ERROR(
        logger(),
        "Dimension size mismatch between the three dimensions.Must be of "
        "equal Length");
  }

  std::unique_ptr<TLE_3D_3x10> root = std::make_unique<TLE_3D_3x10>();
  curr_trie_size += sizeof(TLE_3D_3x10);

  dispatchFPHArray(array1, [&](const auto *vals1) {
    dispatchFPHArray(array2, [&](const auto *vals2) {
      dispatchFPHArray(array3, [&](const auto *vals3) {
        for (int i = 0; i < array1.length; ++i) {

          std::pair<TLE, unsigned int> input_1 =
              internal_10bit(vals1[i]);
          std::pair<TLE, unsigned int> input_2 =
              internal_10bit(vals2[i]);
          std::pair<TLE, unsigned int> input_3 =
              internal_10bit(vals3[i]);

          TLE tle1 = input_1.first;
          TLE tle2 = input_2.first;
          TLE tle3 = input_3.first;

          unsigned int internalFPHNumber1 = input_1.second;
          unsigned int internalFPHNumber2 = input_2.second;
          unsigned int internalFPHNumber3 = input_3.second;

          // Combine three 3-bit TLE values into a 9-bit number
          unsigned int combinedTLE =
              (tle1.encoding << 6) | (tle2.encoding << 3) | tle3.encoding;

          // Check special conditions and set ndims accordingly
          unsigned int isTle1Special =
              update_special_counts(tle1, specialCounts);
          unsigned int isTle2Special =
              update_special_counts(tle2, specialCounts);
          unsigned int isTle3Special =
              update_special_counts(tle3, specialCounts);
          // bool isTle1Special =
          //     (tle1.TLE == 0 || tle1.TLE == 1 || tle1.TLE == 4 || tle1.TLE ==
          //     7);
          // bool isTle2Special =
          //     (tle2.TLE == 0 || tle2.TLE == 1 || tle2.TLE == 4 || tle2.TLE ==
          //     7);
          // bool isTle3Special =
          //     (tle3.TLE == 0 || tle3.TLE == 1 || tle3.TLE == 4 || tle3.TLE ==
          //     7);

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
                combine_chunks_10b(internalFPHNumber2, internalFPHNumber3);
            break;
          case 4:
            combined = internalFPHNumber1;
            break;
          case 5:
            combined =
                combine_chunks_10b(internalFPHNumber1, internalFPHNumber3);
            break;
          case 6:
            combined =
                combine_chunks_10b(internalFPHNumber1, internalFPHNumber2);
            break;
          case 7:
            combined = combine_chunks_10b(
                internalFPHNumber1, internalFPHNumber2, internalFPHNumber3);
            break;
          default:
            SPDLOG_LOGGER_ERROR(logger(), "Invalid number of dimensions");
            break;
          }

          insertintoTrie_3D_3x10(
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

  rollUpCounts(root.get());
  return root;
}

std::vector<char>
execSerialize_3D_3x10(TLE_3D_3x10 *root, std::unique_ptr<SpecialCounts> &specialCounts) {
  auto header = airtree::core::common::makeHeader(
      ConfigWire::Config_3D_Precise, {}, countObservations(root->counts),
      specialCounts->posInfCount, specialCounts->negInfCount,
      specialCounts->posZeroCount, specialCounts->negZeroCount,
      specialCounts->nanCount);

  std::vector<char> buffer;
  serializeHeader(header, buffer);
  size_t header_end = buffer.size();
  serialize_3DxP(root, buffer);
  finalizeHeader(buffer, buffer.size() - header_end);

  return buffer;
}
