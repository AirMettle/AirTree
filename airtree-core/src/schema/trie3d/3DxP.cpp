#include <airtree/core/schema/trie3d/3DxP.hpp>
#include <airtree/core/Logger.hpp>
#include <airtree/util/FeatureFlags.h>
#include <airtree/core/serdes/Header.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/common/BitCodec.hpp>
#include <airtree/core/serdes/trie3d/3DxP.hpp>
#include <airtree/util/UUID.hpp>

using namespace airtree::core;
using namespace airtree::util::uuid;

void insertintoTrie_3D_3x10(TLE_3D_3x10 *root, unsigned int combined,
                            unsigned int combinedTLE, int ndims,
                            uint64_t &curr_trie_size) {
  if (!root) {
    SPDLOG_LOGGER_ERROR(logger(), "Root is null.");
    return;
  }

  // update TLE level and setup child node
  if (!root->populated.test(combinedTLE)) {
    root->populated.set(combinedTLE);
  }
  root->counts[combinedTLE]++;

  if (ndims == 0) {
    return;
  }

  // if ndims is one of 1, 2 or 4 then we are processing a 10 bit value (max).
  if (ndims == 1 || ndims == 2 || ndims == 4) {
    // combined is a 10 bit number

    if (!root->nodes[combinedTLE]) {
      root->nodes[combinedTLE] = std::make_unique<Node3D_3x10_l0>();
      curr_trie_size += sizeof(Node3D_3x10_l0);
    }

    // if populated of l0_10 is not set
    if (!root->nodes[combinedTLE]->populated.test(combined)) {
      root->nodes[combinedTLE]->populated.set(combined);
      root->nodes[combinedTLE]->counts[combined]++;
      return;
    }

    // populated of l0_10 is set - increment count
    root->nodes[combinedTLE]->counts[combined]++;
  }

  // if ndims is one of 3, 5 or 6 then we are processing a 20 bit value (max).
  if (ndims == 3 || ndims == 5 || ndims == 6) {
    // combined is a 20 bit number
    unsigned int l0_10 = (combined >> 10) & 0x3FF; // 10 bits
    unsigned int l1_10 = combined & 0x3FF;         // 10 bits

    if (!root->nodes[combinedTLE]) {
      root->nodes[combinedTLE] = std::make_unique<Node3D_3x10_l0>();
      curr_trie_size += sizeof(Node3D_3x10_l0);
    }

    // if populated of l0_10 is not set
    if (!root->nodes[combinedTLE]->populated.test(l0_10)) {
      root->nodes[combinedTLE]->populated.set(l0_10);
      root->nodes[combinedTLE]->counts[l0_10]++;
      root->nodes[combinedTLE]->nodes[l0_10] =
          std::make_unique<Node3D_3x10_l1>();
      curr_trie_size += sizeof(Node3D_3x10_l1);
      root->nodes[combinedTLE]->nodes[l0_10]->populated.set(l1_10);
      root->nodes[combinedTLE]->nodes[l0_10]->counts[l1_10]++;
      return;
    }

    // populated of l0_10 is set - increment count
    root->nodes[combinedTLE]->counts[l0_10]++;

    // if populated of l0_10 is set and l1_10 is not set
    if (!root->nodes[combinedTLE]->nodes[l0_10]->populated.test(l1_10)) {
      root->nodes[combinedTLE]->nodes[l0_10]->populated.set(l1_10);
      root->nodes[combinedTLE]->nodes[l0_10]->counts[l1_10]++;
      return;
    }

    // l0_10 is set and l1_10 is set so just increment count
    root->nodes[combinedTLE]->nodes[l0_10]->counts[l1_10]++;
  }

  // if ndims is 7 then we are processing a 30 bit value (max).
  if (ndims == 7) {

    if (!root->nodes[combinedTLE]) {
      root->nodes[combinedTLE] = std::make_unique<Node3D_3x10_l0>();
      curr_trie_size += sizeof(Node3D_3x10_l0);
    }

    // combined is a 30 bit number
    unsigned int l0_10 = (combined >> 20) & 0x3FF; // 10 bits
    unsigned int l1_10 = (combined >> 10) & 0x3FF; // 10 bits
    unsigned int l2_10 = combined & 0x3FF;         // 10 bits

    // if populated of l0_10 is not set
    if (!root->nodes[combinedTLE]->populated.test(l0_10)) {
      root->nodes[combinedTLE]->populated.set(l0_10);
      root->nodes[combinedTLE]->counts[l0_10]++;
      root->nodes[combinedTLE]->nodes[l0_10] =
          std::make_unique<Node3D_3x10_l1>();
      curr_trie_size += sizeof(Node3D_3x10_l1);
      root->nodes[combinedTLE]->nodes[l0_10]->populated.set(l1_10);
      root->nodes[combinedTLE]->nodes[l0_10]->counts[l1_10]++;
      root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10] =
          std::make_unique<Node3D_3x10_l2>();
      curr_trie_size += sizeof(Node3D_3x10_l2);
      root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]->populated.set(
          l2_10);
      root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]->counts[l2_10]++;
      return;
    }

    // populated of l0_10 is set - increment count
    root->nodes[combinedTLE]->counts[l0_10]++;

    // if populated of l0_10 is set and l1_10 is not set
    if (!root->nodes[combinedTLE]->nodes[l0_10]->populated.test(l1_10)) {
      root->nodes[combinedTLE]->nodes[l0_10]->populated.set(l1_10);
      root->nodes[combinedTLE]->nodes[l0_10]->counts[l1_10]++;
      root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10] =
          std::make_unique<Node3D_3x10_l2>();
      curr_trie_size += sizeof(Node3D_3x10_l2);
      root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]->populated.set(
          l2_10);
      root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]->counts[l2_10]++;
      return;
    }

    // l0_10 is set and l1_10 is set so just increment count
    root->nodes[combinedTLE]->nodes[l0_10]->counts[l1_10]++;

    // if populated of l0_10 is set, l1_10 is set and l2_10 is not set
    if (!root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]->populated.test(
            l2_10)) {
      root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]->populated.set(
          l2_10);
      root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]->counts[l2_10]++;
      return;
    }

    // l0_10 is set, l1_10 is set and l2_10 is set so just increment count
    root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]->counts[l2_10]++;
  }
}

std::vector<char> generate_3DxP(const FPHArray &array1, const FPHArray &array2,
                                const FPHArray &array3, bool default_mode) {
  std::string uuid = AirTreeUUID::generateUUID();
  SPDLOG_LOGGER_INFO(
      logger(),
      "[generate] [traceID: {}] Generating 3DxP Trie for {} values in dim1, "
      "{} values in dim2 and {} values in dim3 using default_mode {}.",
      uuid, array1.length, array2.length, array3.length, default_mode);
  uint64_t curr_trie_size = 0;
  std::unique_ptr<SpecialCounts> specialCounts =
      std::make_unique<SpecialCounts>();
  SPDLOG_LOGGER_INFO(
      logger(),
      "[insert] [traceID: {}] Filing and inserting {} values for dim1, {} "
      "values for dim2 and {} values for dim3 using "
      "default_mode {} into 3DxP Trie.",
      uuid, array1.length, array2.length, array3.length, default_mode);
  std::unique_ptr<TLE_3D_3x10> root = execCreateAndInsert_3D_3x10(
      array1, array2, array3, curr_trie_size, specialCounts, default_mode);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[insert] [traceID: {}] Completed filing and inserting into 3DxP Trie. "
      "Final trie size: {}.",
      uuid, curr_trie_size);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[serialization] [traceID: {}] Serializing 3DxP trie of size {}.", uuid,
      curr_trie_size);
  std::vector<char> buffer = execSerialize_3D_3x10(
      root.get(), curr_trie_size, specialCounts, default_mode);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[serialization] [traceID: {}] Completed serializing 3DxP Trie.", uuid);
  SPDLOG_LOGGER_INFO(logger(),
                     "[generate] [traceID: {}] Completed generating 3DxP Trie.",
                     uuid);
  return buffer;
}

std::unique_ptr<TLE_3D_3x10>
execCreateAndInsert_3D_3x10(const FPHArray &array1, const FPHArray &array2,
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

  std::unique_ptr<TLE_3D_3x10> root = std::make_unique<TLE_3D_3x10>();
  curr_trie_size += sizeof(TLE_3D_3x10);

  unsigned int internalFPHNumber1;
  unsigned int internalFPHNumber2;
  unsigned int internalFPHNumber3;

  for (int i = 0; i < array1.length; ++i) {
    TLE tle1, tle2, tle3;

    std::pair<TLE, unsigned int> input_1 =
        internal_10bit(array1, i, default_mode);
    std::pair<TLE, unsigned int> input_2 =
        internal_10bit(array2, i, default_mode);
    std::pair<TLE, unsigned int> input_3 =
        internal_10bit(array3, i, default_mode);

    tle1 = input_1.first;
    tle2 = input_2.first;
    tle3 = input_3.first;

    internalFPHNumber1 = input_1.second;
    internalFPHNumber2 = input_2.second;
    internalFPHNumber3 = input_3.second;

    // Combine three 3-bit TLE values into a 9-bit number
    unsigned int combinedTLE = (tle1.TLE << 6) | (tle2.TLE << 3) | tle3.TLE;

    // Check special conditions and set ndims accordingly
    unsigned int isTle1Special = update_special_counts(tle1, specialCounts);
    unsigned int isTle2Special = update_special_counts(tle2, specialCounts);
    unsigned int isTle3Special = update_special_counts(tle3, specialCounts);
    // bool isTle1Special =
    //     (tle1.TLE == 0 || tle1.TLE == 1 || tle1.TLE == 4 || tle1.TLE == 7);
    // bool isTle2Special =
    //     (tle2.TLE == 0 || tle2.TLE == 1 || tle2.TLE == 4 || tle2.TLE == 7);
    // bool isTle3Special =
    //     (tle3.TLE == 0 || tle3.TLE == 1 || tle3.TLE == 4 || tle3.TLE == 7);

    int ndims =
        (~((isTle1Special << 2) | (isTle2Special << 1) | isTle3Special << 0))
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
      combined = combine_chunks_10b(internalFPHNumber2, internalFPHNumber3);
      break;
    case 4:
      combined = internalFPHNumber1;
      break;
    case 5:
      combined = combine_chunks_10b(internalFPHNumber1, internalFPHNumber3);
      break;
    case 6:
      combined = combine_chunks_10b(internalFPHNumber1, internalFPHNumber2);
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
      SPDLOG_LOGGER_ERROR(
          logger(), "Trie size exceeded threshold limit of {}.", threshold_3D);
      SPDLOG_LOGGER_ERROR(
          logger(),
          "Insertion process stopped at index {} of the input dataset.", i);
      SPDLOG_LOGGER_ERROR(logger(),
                          "Last filed values -> @ 0: {} @ 1: {} @ 2: {}",
                          static_cast<const double *>(array1.values)[i],
                          static_cast<const double *>(array2.values)[i],
                          static_cast<const double *>(array3.values)[i]);
      break;
    }
  }

  return root;
}

std::vector<char>
execSerialize_3D_3x10(TLE_3D_3x10 *root, uint64_t &curr_trie_size,
                      std::unique_ptr<SpecialCounts> &specialCounts,
                      bool default_mode) {
  std::vector<char> buffer;

  buffer.reserve(sizeof(trie_header) + curr_trie_size);
  trie_header t_header;
  strcpy(t_header.type_code, "HierFPHG");
  t_header.version = 0;
  t_header.m_width = 4;
  t_header.precision_bits = 6;
  t_header.node_width = 10;
  t_header.nan_count = specialCounts->nanCount;
  t_header.neg_inf_count = specialCounts->negInfCount;
  t_header.pos_inf_count = specialCounts->posInfCount;
  t_header.pos_zero_count = specialCounts->posZeroCount;
  strcpy(t_header.type, "3-D");
  strcpy(t_header.config, "310");
  t_header.mode = default_mode ? 1 : 0;
  // Add the logic to handle the special counts from the TLE
  t_header.trie_root_ref = sizeof(trie_header);
  serializeTrieHeader(t_header, buffer);

  serialize_3DxP(root, buffer);

  return buffer;
}
