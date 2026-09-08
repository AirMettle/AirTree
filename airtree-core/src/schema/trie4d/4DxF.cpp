// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/schema/trie4d/4DxF.hpp>
#include <airtree/core/common/NodeOps.hpp>
#include <airtree/core/Logger.hpp>
#include <airtree/util/FeatureFlags.h>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/common/BitCodec.hpp>
#include <airtree/core/serdes/Node.hpp>
#include <airtree/core/serdes/trie4d/4DxF.hpp>
#include <airtree/util/UUID.hpp>
#include <string>

using namespace airtree::core;
using namespace airtree::core::common;
using namespace airtree::core::api;
using namespace airtree::core::schema::trie4d;
using namespace airtree::util::uuid;

std::vector<char>
Generator4DxF::generate(const std::vector<const FPHArray *> &arrays) const {
  if (arrays.size() != 4) {
    throw std::invalid_argument("Expected exactly 4 arrays for 4D generation");
  }
  return generate_4DxF(
      *arrays[0], *arrays[1], *arrays[2], *arrays[3]);
}

std::unique_ptr<TLE_4D_4x8> CreateParentNode_TLE4D_4x8() {
  auto parentNode = std::make_unique<TLE_4D_4x8>();

  return parentNode;
}

void insertintoTrie_4D_4x8(TLE_4D_4x8 *root, unsigned int combined,
                           unsigned int combinedTLE, int ndims,
                           uint64_t &curr_trie_size) {
  if (!root) {
    SPDLOG_LOGGER_ERROR(logger(), "Root is null.");
    return;
  }
  static constexpr uint8_t kDepth[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
  const int depth = kDepth[ndims & 0xF];
  if (depth == 0) {
    bumpCount(root->populated, root->counts, combinedTLE);
    return;
  }
  Node4D_4x8_l0 *l0 = descend(root->populated, root->nodes, combinedTLE, curr_trie_size);
  const unsigned int c0 = (combined >> (8 * (depth - 1))) & 0xFF;
  if (depth == 1) {
    bumpCount(l0->populated, l0->counts, c0);
    return;
  }
  Node4D_4x8_l1 *l1 = descend(l0->populated, l0->nodes, c0, curr_trie_size);
  const unsigned int c1 = (combined >> (8 * (depth - 2))) & 0xFF;
  if (depth == 2) {
    bumpCount(l1->populated, l1->counts, c1);
    return;
  }
  TrieNode_16 *l2 = descend(l1->populated, l1->nodes, c1, curr_trie_size);
  const unsigned int c2 = (combined >> (8 * (depth - 3))) & 0xFF;
  if (depth == 3) {
    bumpCount(l2->populated, l2->counts, c2);
    return;
  }
  TrieNode_16_Level1 *l3 = descend(l2->populated, l2->nodes, c2, curr_trie_size);
  l3->counts[combined & 0xFF]++;
}

void rollUpCounts(TLE_4D_4x8 *root) { rollUpNode(root); }

std::vector<char> generate_4DxF(const FPHArray &array1, const FPHArray &array2,
                                const FPHArray &array3, const FPHArray &array4) {
  std::string uuid = AirTreeUUID::generateUUID();
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[generate] [traceID: {}] Generating 4DxF Trie for {} values in dim1, "
      "{} values in dim2, {} values in dim3 and {} values in "
      "dim4.",
      uuid, array1.length, array2.length, array3.length, array4.length);
  uint64_t curr_trie_size = 0;
  std::unique_ptr<SpecialCounts> specialCounts =
      std::make_unique<SpecialCounts>();
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[insert] [traceID: {}] Filing and inserting {} values for dim1, {} "
      "values for dim2, {} values for dim3 and {} values for dim4 "
      "into 4DxF Trie.",
      uuid, array1.length, array2.length, array3.length, array4.length);
  std::unique_ptr<TLE_4D_4x8> root =
      execCreateAndInsert_4D_4x8(array1, array2, array3, array4, curr_trie_size,
                                 specialCounts);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[insert] [traceID: {}] Completed filing and inserting into 4DxF Trie. "
      "Final trie size: {}.",
      uuid, curr_trie_size);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[serialization] [traceID: {}] Serializing 4DxF trie of size {}.", uuid,
      curr_trie_size);
  std::vector<char> buffer = execSerialize_4D_4x8(
      root.get(), specialCounts);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[serialization] [traceID: {}] Completed serializing 4DxF Trie.", uuid);
  SPDLOG_LOGGER_DEBUG(logger(),
                     "[generate] [traceID: {}] Completed generating 4DxF Trie.",
                     uuid);

  return buffer;
}

std::unique_ptr<TLE_4D_4x8> execCreateAndInsert_4D_4x8(
    const FPHArray &array1, const FPHArray &array2, const FPHArray &array3,
    const FPHArray &array4, uint64_t &curr_trie_size,
    std::unique_ptr<SpecialCounts> &specialCounts) {
  if (array1.length != array2.length && array1.length != array3.length
      && array1.length != array4.length) {
    throw std::invalid_argument("Dimensions must be of equal length");
    SPDLOG_LOGGER_ERROR(
        logger(),
        "Dimension size mismatch between the four dimensions.Must be of "
        "equal Length");
  }

  std::unique_ptr<TLE_4D_4x8> root = CreateParentNode_TLE4D_4x8();
  curr_trie_size += sizeof(TLE_4D_4x8);

  dispatchFPHArray(array1, [&](const auto *vals1) {
    dispatchFPHArray(array2, [&](const auto *vals2) {
      dispatchFPHArray(array3, [&](const auto *vals3) {
        dispatchFPHArray(array4, [&](const auto *vals4) {
          for (int i = 0; i < array1.length; ++i) {

            std::pair<TLE, unsigned int> input_1 =
                internal_8bit(vals1[i]);
            std::pair<TLE, unsigned int> input_2 =
                internal_8bit(vals2[i]);
            std::pair<TLE, unsigned int> input_3 =
                internal_8bit(vals3[i]);
            std::pair<TLE, unsigned int> input_4 =
                internal_8bit(vals4[i]);

            TLE tle1 = input_1.first;
            TLE tle2 = input_2.first;
            TLE tle3 = input_3.first;
            TLE tle4 = input_4.first;

            unsigned int internalFPHNumber1 = input_1.second;
            unsigned int internalFPHNumber2 = input_2.second;
            unsigned int internalFPHNumber3 = input_3.second;
            unsigned int internalFPHNumber4 = input_4.second;

            // Combine four 3-bit TLE values into a 12-bit number
            unsigned int combinedTLE =
                (tle1.encoding << 9) | (tle2.encoding << 6) | (tle3.encoding << 3) | tle4.encoding;

            // Check special conditions and set ndims accordingly
            bool isTle1Special = update_special_counts(tle1, specialCounts);
            bool isTle2Special = update_special_counts(tle2, specialCounts);
            bool isTle3Special = update_special_counts(tle3, specialCounts);
            bool isTle4Special = update_special_counts(tle4, specialCounts);

            int ndims = (~((isTle1Special << 3) | (isTle2Special << 2)
                           | (isTle3Special << 1) | isTle4Special << 0))
                        & 0xF;
            unsigned int combined = 0;

            switch (ndims) {
            case 0: // 0000
              combined = 0;
              break;
            case 1: // 0001
              combined = internalFPHNumber4;
              break;
            case 2: // 0010
              combined = internalFPHNumber3;
              break;
            case 3: // 0011
              combined =
                  combine_chunks_8b(internalFPHNumber3, internalFPHNumber4);
              break;
            case 4: // 0100
              combined = internalFPHNumber2;
              break;
            case 5: // 0101
              combined =
                  combine_chunks_8b(internalFPHNumber2, internalFPHNumber4);
              break;
            case 6: // 0110
              combined =
                  combine_chunks_8b(internalFPHNumber2, internalFPHNumber3);
              break;
            case 7: // 0111
              combined = combine_chunks_8b(
                  internalFPHNumber2, internalFPHNumber3, internalFPHNumber4);
              break;
            case 8: // 1000
              combined = internalFPHNumber1;
              break;
            case 9: // 1001
              combined =
                  combine_chunks_8b(internalFPHNumber1, internalFPHNumber4);
              break;
            case 10: // 1010
              combined =
                  combine_chunks_8b(internalFPHNumber1, internalFPHNumber3);
              break;
            case 11: // 1011
              combined = combine_chunks_8b(
                  internalFPHNumber1, internalFPHNumber3, internalFPHNumber4);
              break;
            case 12: // 1100
              combined =
                  combine_chunks_8b(internalFPHNumber1, internalFPHNumber2);
              break;
            case 13: // 1101
              combined = combine_chunks_8b(
                  internalFPHNumber1, internalFPHNumber2, internalFPHNumber4);
              break;
            case 14: // 1110
              combined = combine_chunks_8b(
                  internalFPHNumber1, internalFPHNumber2, internalFPHNumber3);
              break;
            case 15: // 1111
              combined =
                  combine_chunks_8b(internalFPHNumber1, internalFPHNumber2,
                                    internalFPHNumber3, internalFPHNumber4);
              break;
            default:
              SPDLOG_LOGGER_ERROR(logger(), "Invalid ndims value");
              break;
            }

            insertintoTrie_4D_4x8(
                root.get(), combined, combinedTLE, ndims, curr_trie_size);
            if (enable_threshold_4D && curr_trie_size > threshold_4D) {
              SPDLOG_LOGGER_ERROR(logger(),
                                  "Trie size exceeded threshold limit of {}.",
                                  threshold_4D);
              SPDLOG_LOGGER_ERROR(
                  logger(),
                  "Insertion process stopped at index {} of the input dataset.",
                  i);
              SPDLOG_LOGGER_ERROR(
                  logger(),
                  "Last filed values -> @ 0: {} @ 1: {} @ 2: {} @ 3: {}",
                  vals1[i], vals2[i], vals3[i], vals4[i]);
              break;
            }
          }
        });
      });
    });
  });

  rollUpCounts(root.get());
  return root;
}

std::vector<char>
execSerialize_4D_4x8(TLE_4D_4x8 *root, std::unique_ptr<SpecialCounts> &specialCounts) {
  auto header = airtree::core::common::makeHeader(
      ConfigWire::Config_4D_Fast, {}, countObservations(root->counts),
      specialCounts->posInfCount, specialCounts->negInfCount,
      specialCounts->posZeroCount, specialCounts->negZeroCount,
      specialCounts->nanCount);

  std::vector<char> buffer;
  serializeHeader(header, buffer);
  size_t header_end = buffer.size();
  serialize_4DxF(root, buffer);
  finalizeHeader(buffer, buffer.size() - header_end);

  return buffer;
}