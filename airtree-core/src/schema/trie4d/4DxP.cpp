// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/schema/trie4d/4DxP.hpp>
#include <bit>
#include <airtree/core/Logger.hpp>
#include <airtree/util/FeatureFlags.h>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/common/BitCodec.hpp>
#include <airtree/core/serdes/Node.hpp>
#include <airtree/core/serdes/trie4d/4DxP.hpp>
#include <airtree/util/UUID.hpp>

using namespace airtree::core;
using namespace airtree::core::common;
using namespace airtree::core::api;
using namespace airtree::core::schema::trie4d;
using namespace airtree::util::uuid;

std::vector<char>
Generator4DxP::generate(const std::vector<const FPHArray *> &arrays) const {
  if (arrays.size() != 4) {
    throw std::invalid_argument("Expected exactly 4 arrays for 4D generation");
  }
  return generate_4DxP(
      *arrays[0], *arrays[1], *arrays[2], *arrays[3]);
}

std::unique_ptr<TLE_4D_4x10> CreateParentNode_TLE4D_4x10() {
  auto parentNode = std::make_unique<TLE_4D_4x10>();

  return parentNode;
}

void insertintoTrie_4D_4x10(TLE_4D_4x10 *root, uint64_t combined,
                            unsigned int combinedTLE, int ndims,
                            uint64_t &curr_trie_size) {
  root->populated.set(combinedTLE);
  root->counts[combinedTLE]++;
  static constexpr uint8_t kDepth[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
  const int depth = kDepth[ndims & 0xF];
  if (depth == 0)
    return;
  std::unique_ptr<Node4D_4x10_l0> &l0p = root->nodes[combinedTLE];
  if (!l0p) {
    l0p = std::make_unique<Node4D_4x10_l0>();
    curr_trie_size += sizeof(Node4D_4x10_l0);
  }
  const unsigned int c0 = (combined >> (10 * (depth - 1))) & 0x3FF;
  if (depth == 1) {
    sparse_node::bumpSlot(*l0p, c0, curr_trie_size);
    return;
  }
  Node4D_4x10_l1 *l1 = sparse_node::descendInto<Node4D_4x10_l1>(*l0p, c0, curr_trie_size);
  const unsigned int c1 = (combined >> (10 * (depth - 2))) & 0x3FF;
  if (depth == 2) {
    sparse_node::bumpSlot(*l1, c1, curr_trie_size);
    return;
  }
  Node4D_4x10_l2 *l2 = sparse_node::descendInto<Node4D_4x10_l2>(*l1, c1, curr_trie_size);
  const unsigned int c2 = (combined >> (10 * (depth - 3))) & 0x3FF;
  if (depth == 3) {
    sparse_node::bumpSlot(*l2, c2, curr_trie_size);
    return;
  }
  Node4D_4x10_l3 *l3 = sparse_node::descendInto<Node4D_4x10_l3>(*l2, c2, curr_trie_size);
  sparse_node::bumpSlot(*l3, combined & 0x3FF, curr_trie_size);
}


std::vector<char> generate_4DxP(const FPHArray &array1, const FPHArray &array2,
                                const FPHArray &array3, const FPHArray &array4) {
  std::string uuid = AirTreeUUID::generateUUID();
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[generate] [traceID: {}] Generating 4DxP Trie for {} values in dim1, "
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
      "into 4DxP Trie.",
      uuid, array1.length, array2.length, array3.length, array4.length);
  std::unique_ptr<TLE_4D_4x10> root =
      execCreateAndInsert_4D_4x10(array1, array2, array3, array4,
                                  curr_trie_size, specialCounts);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[insert] [traceID: {}] Completed filing and inserting into 4DxP Trie. "
      "Final trie size: {}.",
      uuid, curr_trie_size);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[serialization] [traceID: {}] Serializing 4DxP trie of size {}.", uuid,
      curr_trie_size);
  std::vector<char> buffer = execSerialize_4D_4x10(
      root.get(), specialCounts);
  SPDLOG_LOGGER_DEBUG(
      logger(),
      "[serialization] [traceID: {}] Completed serializing 4DxP Trie.", uuid);
  SPDLOG_LOGGER_DEBUG(logger(),
                     "[generate] [traceID: {}] Completed generating 4DxP Trie.",
                     uuid);
  return buffer;
}

std::unique_ptr<TLE_4D_4x10> execCreateAndInsert_4D_4x10(
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

  std::unique_ptr<TLE_4D_4x10> root = CreateParentNode_TLE4D_4x10();
  curr_trie_size += sizeof(TLE_4D_4x10);

  dispatchFPHArray(array1, [&](const auto *vals1) {
    dispatchFPHArray(array2, [&](const auto *vals2) {
      dispatchFPHArray(array3, [&](const auto *vals3) {
        dispatchFPHArray(array4, [&](const auto *vals4) {
          for (int i = 0; i < array1.length; ++i) {

            std::pair<TLE, unsigned int> input1 =
                internal_10bit(vals1[i]);
            std::pair<TLE, unsigned int> input2 =
                internal_10bit(vals2[i]);
            std::pair<TLE, unsigned int> input3 =
                internal_10bit(vals3[i]);
            std::pair<TLE, unsigned int> input4 =
                internal_10bit(vals4[i]);

            TLE tle1 = input1.first;
            TLE tle2 = input2.first;
            TLE tle3 = input3.first;
            TLE tle4 = input4.first;

            unsigned int internalFPHNumber1 = input1.second;
            unsigned int internalFPHNumber2 = input2.second;
            unsigned int internalFPHNumber3 = input3.second;
            unsigned int internalFPHNumber4 = input4.second;

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
            uint64_t combined = 0;

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
                  combine_chunks_10b(internalFPHNumber3, internalFPHNumber4);
              break;
            case 4: // 0100
              combined = internalFPHNumber2;
              break;
            case 5: // 0101
              combined =
                  combine_chunks_10b(internalFPHNumber2, internalFPHNumber4);
              break;
            case 6: // 0110
              combined =
                  combine_chunks_10b(internalFPHNumber2, internalFPHNumber3);
              break;
            case 7: // 0111
              combined = combine_chunks_10b(
                  internalFPHNumber2, internalFPHNumber3, internalFPHNumber4);
              break;
            case 8: // 1000
              combined = internalFPHNumber1;
              break;
            case 9: // 1001
              combined =
                  combine_chunks_10b(internalFPHNumber1, internalFPHNumber4);
              break;
            case 10: // 1010
              combined =
                  combine_chunks_10b(internalFPHNumber1, internalFPHNumber3);
              break;
            case 11: // 1011
              combined = combine_chunks_10b(
                  internalFPHNumber1, internalFPHNumber3, internalFPHNumber4);
              break;
            case 12: // 1100
              combined =
                  combine_chunks_10b(internalFPHNumber1, internalFPHNumber2);
              break;
            case 13: // 1101
              combined = combine_chunks_10b(
                  internalFPHNumber1, internalFPHNumber2, internalFPHNumber4);
              break;
            case 14: // 1110
              combined = combine_chunks_10b(
                  internalFPHNumber1, internalFPHNumber2, internalFPHNumber3);
              break;
            case 15: // 1111
              combined =
                  combine_chunks_10b(internalFPHNumber1, internalFPHNumber2,
                                     internalFPHNumber3, internalFPHNumber4);
              break;
            default:
              SPDLOG_LOGGER_ERROR(logger(), "Invalid ndims value");
              break;
            }

            insertintoTrie_4D_4x10(
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

  return root;
}

std::vector<char>
execSerialize_4D_4x10(TLE_4D_4x10 *root, std::unique_ptr<SpecialCounts> &specialCounts) {
  auto header = airtree::core::common::makeHeader(
      ConfigWire::Config_4D_Precise, {}, countObservations(root->counts),
      specialCounts->posInfCount, specialCounts->negInfCount,
      specialCounts->posZeroCount, specialCounts->negZeroCount,
      specialCounts->nanCount);

  std::vector<char> buffer;
  serializeHeader(header, buffer);
  size_t header_end = buffer.size();
  serialize_4DxP(root, buffer);
  finalizeHeader(buffer, buffer.size() - header_end);

  return buffer;
}