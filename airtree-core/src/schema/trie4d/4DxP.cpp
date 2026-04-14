#include <airtree/core/schema/trie4d/4DxP.hpp>
#include <airtree/core/Logger.hpp>
#include <airtree/util/FeatureFlags.h>
#include <airtree/core/serdes/Header.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/common/BitCodec.hpp>
#include <airtree/core/serdes/trie4d/4DxP.hpp>
#include <airtree/util/UUID.hpp>

using namespace airtree::core;
using namespace airtree::core::schema::trie4d;
using namespace airtree::util::uuid;

std::vector<char>
Generator4DxP::generate(const std::vector<const FPHArray *> &arrays,
                        bool default_mode) const {
  if (arrays.size() != 4) {
    throw std::invalid_argument("Expected exactly 4 arrays for 4D generation");
  }
  return generate_4DxP(
      *arrays[0], *arrays[1], *arrays[2], *arrays[3], default_mode);
}

std::unique_ptr<TLE_4D_4x10> CreateParentNode_TLE4D_4x10() {
  auto parentNode = std::make_unique<TLE_4D_4x10>();

  return parentNode;
}

void insertintoTrie_4D_4x10(TLE_4D_4x10 *root, uint64_t combined,
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

  // We are going to handle 5 specific cases. 40, 30, 20, 10 bit values
  // and special cases with 0 dimensions and special values.

  // if ndims == 0, then that mean we only have to update the TLE level
  // since all the 3 dimensions have special values
  if (ndims == 0) { // combined == 0
    return;
  }

  // if ndims is one of 1, 2, 4 or 8then we are processing a 10 bit value (max).
  if (ndims == 1 || ndims == 2 || ndims == 4 || ndims == 8) {
    // combined is a 10 bit number
    unsigned int l0_10 = combined & 0x3FF; // Ensuring just 10 LSBs are picked

    if (!root->nodes[combinedTLE]) {
      root->nodes[combinedTLE] = std::make_unique<Node4D_4x10_l0>();
      curr_trie_size += sizeof(Node4D_4x10_l0);
    }

    // if populated of l0_10 is not set
    if (!root->nodes[combinedTLE]->populated.test(l0_10)) {
      root->nodes[combinedTLE]->populated.set(l0_10);
      root->nodes[combinedTLE]->counts[l0_10]++;
      return;
    }

    // populated of l0_10 is set - increment count
    root->nodes[combinedTLE]->counts[l0_10]++;
    return;
  }

  // if ndims is one of 3, 5, 6, 9, 10, 12 then we are processing a 20 bit value
  // (max).
  if (ndims == 3 || ndims == 5 || ndims == 6 || ndims == 9 || ndims == 10
      || ndims == 12) {
    // combined is a 20 bit number
    unsigned int l0_10 = (combined >> 10) & 0x3FF; // 10 bits
    unsigned int l1_10 = combined & 0x3FF;         // 10 bits

    if (!root->nodes[combinedTLE]) {
      root->nodes[combinedTLE] = std::make_unique<Node4D_4x10_l0>();
      curr_trie_size += sizeof(Node4D_4x10_l0);
    }

    // if populated of l0_10 is not set
    if (!root->nodes[combinedTLE]->populated.test(l0_10)) {
      root->nodes[combinedTLE]->populated.set(l0_10);
      root->nodes[combinedTLE]->counts[l0_10]++;
      root->nodes[combinedTLE]->nodes[l0_10] =
          std::make_unique<Node4D_4x10_l1>();
      curr_trie_size += sizeof(Node4D_4x10_l1);
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
    return;
  }

  // if ndims is 7, 11, 13, 14 then we are processing a 30 bit value (max).
  if (ndims == 7 || ndims == 11 || ndims == 13 || ndims == 14) {
    // combined is a 30 bit number
    unsigned int l0_10 = (combined >> 20) & 0x3FF; // 10 bits
    unsigned int l1_10 = (combined >> 10) & 0x3FF; // 10 bits
    unsigned int l2_10 = combined & 0x3FF;         // 10 bits

    if (!root->nodes[combinedTLE]) {
      root->nodes[combinedTLE] = std::make_unique<Node4D_4x10_l0>();
      curr_trie_size += sizeof(Node4D_4x10_l0);
    }

    // if populated of l0_10 is not set
    if (!root->nodes[combinedTLE]->populated.test(l0_10)) {
      root->nodes[combinedTLE]->populated.set(l0_10);
      root->nodes[combinedTLE]->counts[l0_10]++;
      root->nodes[combinedTLE]->nodes[l0_10] =
          std::make_unique<Node4D_4x10_l1>();
      curr_trie_size += sizeof(Node4D_4x10_l1);
      root->nodes[combinedTLE]->nodes[l0_10]->populated.set(l1_10);
      root->nodes[combinedTLE]->nodes[l0_10]->counts[l1_10]++;
      root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10] =
          std::make_unique<Node4D_4x10_l2>();
      curr_trie_size += sizeof(Node4D_4x10_l2);
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
          std::make_unique<Node4D_4x10_l2>();
      curr_trie_size += sizeof(Node4D_4x10_l2);
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
    return;
  }

  // if ndims is 15 then we are processing a 40 bit value (max).
  if (ndims == 15) {
    // combined is a 40 bit number
    unsigned int l0_10 = (combined >> 30) & 0x3FF; // 10 bits
    unsigned int l1_10 = (combined >> 20) & 0x3FF; // 10 bits
    unsigned int l2_10 = (combined >> 10) & 0x3FF; // 10 bits
    unsigned int l3_10 = combined & 0x3FF;         // 10 bits

    if (!root->nodes[combinedTLE]) {
      root->nodes[combinedTLE] = std::make_unique<Node4D_4x10_l0>();
      curr_trie_size += sizeof(Node4D_4x10_l0);
    }

    // if populated of l0_10 is not set
    if (!root->nodes[combinedTLE]->populated.test(l0_10)) {
      root->nodes[combinedTLE]->populated.set(l0_10);
      root->nodes[combinedTLE]->counts[l0_10]++;
      root->nodes[combinedTLE]->nodes[l0_10] =
          std::make_unique<Node4D_4x10_l1>();
      curr_trie_size += sizeof(Node4D_4x10_l1);
      root->nodes[combinedTLE]->nodes[l0_10]->populated.set(l1_10);
      root->nodes[combinedTLE]->nodes[l0_10]->counts[l1_10]++;
      root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10] =
          std::make_unique<Node4D_4x10_l2>();
      curr_trie_size += sizeof(Node4D_4x10_l2);
      root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]->populated.set(
          l2_10);
      root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]->counts[l2_10]++;
      root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]->nodes[l2_10] =
          std::make_unique<Node4D_4x10_l3>();
      curr_trie_size += sizeof(Node4D_4x10_l3);
      root->nodes[combinedTLE]
          ->nodes[l0_10]
          ->nodes[l1_10]
          ->nodes[l2_10]
          ->populated.set(l3_10);
      root->nodes[combinedTLE]
          ->nodes[l0_10]
          ->nodes[l1_10]
          ->nodes[l2_10]
          ->counts[l3_10]++;
      return;
    }

    // populated of l0_10 is set - increment count
    root->nodes[combinedTLE]->counts[l0_10]++;

    // if populated of l0_10 is set and l1_10 is not set
    if (!root->nodes[combinedTLE]->nodes[l0_10]->populated.test(l1_10)) {
      root->nodes[combinedTLE]->nodes[l0_10]->populated.set(l1_10);
      root->nodes[combinedTLE]->nodes[l0_10]->counts[l1_10]++;
      root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10] =
          std::make_unique<Node4D_4x10_l2>();
      curr_trie_size += sizeof(Node4D_4x10_l2);
      root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]->populated.set(
          l2_10);
      root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]->counts[l2_10]++;
      root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]->nodes[l2_10] =
          std::make_unique<Node4D_4x10_l3>();
      curr_trie_size += sizeof(Node4D_4x10_l3);
      root->nodes[combinedTLE]
          ->nodes[l0_10]
          ->nodes[l1_10]
          ->nodes[l2_10]
          ->populated.set(l3_10);
      root->nodes[combinedTLE]
          ->nodes[l0_10]
          ->nodes[l1_10]
          ->nodes[l2_10]
          ->counts[l3_10]++;
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
      root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]->nodes[l2_10] =
          std::make_unique<Node4D_4x10_l3>();
      curr_trie_size += sizeof(Node4D_4x10_l3);
      root->nodes[combinedTLE]
          ->nodes[l0_10]
          ->nodes[l1_10]
          ->nodes[l2_10]
          ->populated.set(l3_10);
      root->nodes[combinedTLE]
          ->nodes[l0_10]
          ->nodes[l1_10]
          ->nodes[l2_10]
          ->counts[l3_10]++;
      return;
    }

    // l0_10 is set, l1_10 is set and l2_10 is set so just increment count
    root->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]->counts[l2_10]++;

    // if populated of l0_10 is set, l1_10 is set, l2_10 is set and l3_10 is not
    // set
    if (!root->nodes[combinedTLE]
             ->nodes[l0_10]
             ->nodes[l1_10]
             ->nodes[l2_10]
             ->populated.test(l3_10)) {
      root->nodes[combinedTLE]
          ->nodes[l0_10]
          ->nodes[l1_10]
          ->nodes[l2_10]
          ->populated.set(l3_10);
      root->nodes[combinedTLE]
          ->nodes[l0_10]
          ->nodes[l1_10]
          ->nodes[l2_10]
          ->counts[l3_10]++;
      return;
    }

    // l0_10 is set, l1_10 is set, l2_10 is set and l3_10 is set so just
    // increment count
    root->nodes[combinedTLE]
        ->nodes[l0_10]
        ->nodes[l1_10]
        ->nodes[l2_10]
        ->counts[l3_10]++;
  }
}


std::vector<char> generate_4DxP(const FPHArray &array1, const FPHArray &array2,
                                const FPHArray &array3, const FPHArray &array4,
                                bool default_mode) {
  std::string uuid = AirTreeUUID::generateUUID();
  SPDLOG_LOGGER_INFO(
      logger(),
      "[generate] [traceID: {}] Generating 4DxP Trie for {} values in dim1, "
      "{} values in dim2, {} values in dim3 and {} values in "
      "dim4 using default_mode {}.",
      uuid, array1.length, array2.length, array3.length, array4.length,
      default_mode);
  uint64_t curr_trie_size = 0;
  std::unique_ptr<SpecialCounts> specialCounts =
      std::make_unique<SpecialCounts>();
  SPDLOG_LOGGER_INFO(
      logger(),
      "[insert] [traceID: {}] Filing and inserting {} values for dim1, {} "
      "values for dim2, {} values for dim3 and {} values for dim4 using "
      "default_mode {} into 4DxP Trie.",
      uuid, array1.length, array2.length, array3.length, array4.length,
      default_mode);
  std::unique_ptr<TLE_4D_4x10> root =
      execCreateAndInsert_4D_4x10(array1, array2, array3, array4,
                                  curr_trie_size, specialCounts, default_mode);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[insert] [traceID: {}] Completed filing and inserting into 4DxP Trie. "
      "Final trie size: {}.",
      uuid, curr_trie_size);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[serialization] [traceID: {}] Serializing 4DxP trie of size {}.", uuid,
      curr_trie_size);
  std::vector<char> buffer = execSerialize_4D_4x10(
      root.get(), curr_trie_size, specialCounts, default_mode);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[serialization] [traceID: {}] Completed serializing 4DxP Trie.", uuid);
  SPDLOG_LOGGER_INFO(logger(),
                     "[generate] [traceID: {}] Completed generating 4DxP Trie.",
                     uuid);
  return buffer;
}

std::unique_ptr<TLE_4D_4x10> execCreateAndInsert_4D_4x10(
    const FPHArray &array1, const FPHArray &array2, const FPHArray &array3,
    const FPHArray &array4, uint64_t &curr_trie_size,
    std::unique_ptr<SpecialCounts> &specialCounts, bool default_mode) {
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

  unsigned int internalFPHNumber1;
  unsigned int internalFPHNumber2;
  unsigned int internalFPHNumber3;
  unsigned int internalFPHNumber4;

  for (int i = 0; i < array1.length; ++i) {
    TLE tle1, tle2, tle3, tle4;

    std::pair<TLE, unsigned int> input1 =
        internal_10bit(array1, i, default_mode);
    std::pair<TLE, unsigned int> input2 =
        internal_10bit(array2, i, default_mode);
    std::pair<TLE, unsigned int> input3 =
        internal_10bit(array3, i, default_mode);
    std::pair<TLE, unsigned int> input4 =
        internal_10bit(array4, i, default_mode);

    tle1 = input1.first;
    tle2 = input2.first;
    tle3 = input3.first;
    tle4 = input4.first;

    internalFPHNumber1 = input1.second;
    internalFPHNumber2 = input2.second;
    internalFPHNumber3 = input3.second;
    internalFPHNumber4 = input4.second;

    // Combine four 3-bit TLE values into a 12-bit number
    unsigned int combinedTLE =
        (tle1.TLE << 9) | (tle2.TLE << 6) | (tle3.TLE << 3) | tle4.TLE;

    // Check special conditions and set ndims accordingly
    bool isTle1Special = update_special_counts(tle1, specialCounts);
    bool isTle2Special = update_special_counts(tle2, specialCounts);
    bool isTle3Special = update_special_counts(tle3, specialCounts);
    bool isTle4Special = update_special_counts(tle4, specialCounts);
    // bool isTle1Special =
    //     (tle1.TLE == 0 || tle1.TLE == 1 || tle1.TLE == 4 || tle1.TLE == 7);
    // bool isTle2Special =
    //     (tle2.TLE == 0 || tle2.TLE == 1 || tle2.TLE == 4 || tle2.TLE == 7);
    // bool isTle3Special =
    //     (tle3.TLE == 0 || tle3.TLE == 1 || tle3.TLE == 4 || tle3.TLE == 7);
    // bool isTle4Special =
    //     (tle4.TLE == 0 || tle4.TLE == 1 || tle4.TLE == 4 || tle4.TLE == 7);

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
      combined = combine_chunks_10b(internalFPHNumber3, internalFPHNumber4);
      break;
    case 4: // 0100
      combined = internalFPHNumber2;
      break;
    case 5: // 0101
      combined = combine_chunks_10b(internalFPHNumber2, internalFPHNumber4);
      break;
    case 6: // 0110
      combined = combine_chunks_10b(internalFPHNumber2, internalFPHNumber3);
      break;
    case 7: // 0111
      combined = combine_chunks_10b(
          internalFPHNumber2, internalFPHNumber3, internalFPHNumber4);
      break;
    case 8: // 1000
      combined = internalFPHNumber1;
      break;
    case 9: // 1001
      combined = combine_chunks_10b(internalFPHNumber1, internalFPHNumber4);
      break;
    case 10: // 1010
      combined = combine_chunks_10b(internalFPHNumber1, internalFPHNumber3);
      break;
    case 11: // 1011
      combined = combine_chunks_10b(
          internalFPHNumber1, internalFPHNumber3, internalFPHNumber4);
      break;
    case 12: // 1100
      combined = combine_chunks_10b(internalFPHNumber1, internalFPHNumber2);
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
      combined = combine_chunks_10b(internalFPHNumber1, internalFPHNumber2,
                                    internalFPHNumber3, internalFPHNumber4);
      break;
    default:
      SPDLOG_LOGGER_ERROR(logger(), "Invalid ndims value");
      break;
    }

    insertintoTrie_4D_4x10(
        root.get(), combined, combinedTLE, ndims, curr_trie_size);

    if (enable_threshold_4D && curr_trie_size > threshold_4D) {
      SPDLOG_LOGGER_ERROR(
          logger(), "Trie size exceeded threshold limit of {}.", threshold_4D);
      SPDLOG_LOGGER_ERROR(
          logger(),
          "Insertion process stopped at index {} of the input dataset.", i);
      SPDLOG_LOGGER_ERROR(
          logger(), "Last filed values -> @ 0: {} @ 1: {} @ 2: {} @ 3: {}",
          static_cast<const double *>(array1.values)[i],
          static_cast<const double *>(array2.values)[i],
          static_cast<const double *>(array3.values)[i],
          static_cast<const double *>(array4.values)[i]);
      break;
    }
  }

  return root;
}

std::vector<char>
execSerialize_4D_4x10(TLE_4D_4x10 *root, uint64_t &curr_trie_size,
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
  strcpy(t_header.type, "4-D");
  strcpy(t_header.config, "410");
  t_header.mode = default_mode ? 1 : 0;
  // Add the logic to handle the special counts from the TLE
  t_header.trie_root_ref = sizeof(trie_header);
  serializeTrieHeader(t_header, buffer);

  serialize_4DxP(root, buffer);

  return buffer;
}