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
using namespace airtree::core::schema::trie2d;
using namespace airtree::util::uuid;

std::vector<char>
Generator2DxP::generate(const std::vector<const FPHArray *> &arrays,
                        bool default_mode) const {
  if (arrays.size() != 2) {
    throw std::invalid_argument("Expected exactly 2 arrays for 2D generation");
  }
  return generate_2DxP(*arrays[0], *arrays[1], default_mode);
}

std::unique_ptr<TLEoption3_2D> CreateParent_TLE2D_option3() {
  auto parentNode = std::make_unique<TLEoption3_2D>();
  parentNode->populated.reset();

  return parentNode;
}

std::vector<char> generate_2DxP(const FPHArray &array1, const FPHArray &array2,
                                bool default_mode) {
  std::string uuid = AirTreeUUID::generateUUID();
  SPDLOG_LOGGER_INFO(
      logger(),
      "[generate] [traceID: {}] Generating 2DxP Trie for {} values in dim1 "
      "and {} values in dim2 using default_mode {}.",
      uuid, array1.length, array2.length, default_mode);
  uint64_t curr_trie_size = 0;

  std::unique_ptr<SpecialCounts> specialCounts =
      std::make_unique<SpecialCounts>();
  SPDLOG_LOGGER_INFO(
      logger(),
      "[insert] [traceID: {}] Filing and inserting {} values for dim1 and {} "
      "values for dim2 using "
      "default_mode {} into 2DxP Trie.",
      uuid, array1.length, array2.length, default_mode);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[insert] [traceID: {}] Completed filing and inserting into 2DxP Trie. "
      "Final trie size: {}.",
      uuid, curr_trie_size);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[serialization] [traceID: {}] Serializing 2DxP trie of size {}.", uuid,
      curr_trie_size);
  std::unique_ptr<TLEoption3_2D> root = execCreateAndInsert_2D_2x10(
      array1, array2, curr_trie_size, specialCounts, default_mode);

  std::vector<char> buffer = execSerialize_2D_2x10(
      root.get(), curr_trie_size, specialCounts, default_mode);
  SPDLOG_LOGGER_INFO(
      logger(),
      "[serialization] [traceID: {}] Completed serializing 2DxP Trie.", uuid);
  SPDLOG_LOGGER_INFO(logger(),
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
    std::unique_ptr<SpecialCounts> &specialCounts, bool default_mode) {
  if (array1.length != array2.length) {
    throw std::invalid_argument("Dimensions must be of equal length");
    SPDLOG_LOGGER_ERROR(
        logger(), "Length of Dimension-1 and Dimension-2 is not equal");
  }

  std::unique_ptr<TLEoption3_2D> root = CreateParent_TLE2D_option3(); // 10x10
  curr_trie_size += sizeof(TLEoption3_2D);

  unsigned int internalFPHNumber1 = 0;
  unsigned int internalFPHNumber2 = 0;

  for (int i = 0; i < array1.length; ++i) {
    TLE tle1, tle2;

    std::pair<TLE, unsigned int> input_1 =
        internal_10bit(array1, i, default_mode);
    std::pair<TLE, unsigned int> input_2 =
        internal_10bit(array2, i, default_mode);

    tle1 = input_1.first;
    tle2 = input_2.first;

    internalFPHNumber1 = input_1.second;
    internalFPHNumber2 = input_2.second;

    unsigned int combinedTLE = (tle1.TLE << 3) | tle2.TLE;
    // If input is a special case, determine the special case and increment
    // the appropriate special count
    unsigned int isTle1Special = update_special_counts(tle1, specialCounts);
    unsigned int isTle2Special = update_special_counts(tle2, specialCounts);

    // Check special conditions and set ndims accordingly
    // unsigned int isTle1Special =
    // (tle1.TLE == 0 || tle1.TLE == 1 || tle1.TLE == 4 || tle1.TLE == 7);
    // unsigned int isTle2Special =
    // (tle2.TLE == 0 || tle2.TLE == 1 || tle2.TLE == 4 || tle2.TLE == 7);
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
      combined = combine_chunks_10b(internalFPHNumber1, internalFPHNumber2);
      break;
    default:
      SPDLOG_LOGGER_ERROR(logger(), "Invalid value for ndims");
      break;
    }
    insertintoTLETrie_2D_option3(
        root.get(), combined, combinedTLE, ndims, curr_trie_size);
    if (enable_threshold_2D && curr_trie_size > threshold_2D) {
      SPDLOG_LOGGER_ERROR(
          logger(), "Trie size exceeded threshold limit of {}.", threshold_2D);
      SPDLOG_LOGGER_ERROR(
          logger(),
          "Insertion process stopped at index {} of the input dataset.", i);
      SPDLOG_LOGGER_ERROR(logger(), "Last filed values -> @ 0: {} @ 1: {}",
                          static_cast<const double *>(array1.values)[i],
                          static_cast<const double *>(array2.values)[i]);
      break;
    }
  }

  return root;
}

std::vector<char>
execSerialize_2D_2x10(TLEoption3_2D *root, uint64_t &curr_trie_size,
                      std::unique_ptr<SpecialCounts> &specialCounts,
                      bool default_mode) {
  auto header = airtree::core::common::makeHeader(
      ConfigWire::Config_2D_Precise, {}, 0,
      specialCounts->posInfCount, specialCounts->negInfCount,
      specialCounts->posZeroCount, specialCounts->negZeroCount,
      specialCounts->nanCount);

  std::vector<char> buffer;
  buffer.reserve(kHeaderLength + curr_trie_size);
  serializeHeader(header, buffer);
  size_t header_end = buffer.size();
  serialize_2DxP(root, buffer);
  finalizeHeader(buffer, buffer.size() - header_end);

  return buffer;
}