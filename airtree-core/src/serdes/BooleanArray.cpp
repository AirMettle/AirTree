// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <cstring>
#include <airtree/core/serdes/BooleanArray.hpp>

#include <airtree/core/Logger.hpp>

using namespace airtree::core;

std::vector<char> serializeCompactBooleanArray(const BooleanArray &array) {
  std::vector<char> buffer;
  for (size_t i = 0; i < array.get_array().size(); i++) {
    auto compact_value = array.get_array()[i];
    auto compact_value_bytes = reinterpret_cast<const char *>(&compact_value);
    buffer.insert(buffer.end(), compact_value_bytes,
                  compact_value_bytes + sizeof(uint64_t));
  }
  return buffer;
}

std::vector<uint64_t>
deserializeCompactBooleanArray(std::span<const char> buffer, size_t &offset,
                               std::size_t len) {
  std::vector<uint64_t> compact_arr_values = std::vector<uint64_t>();

  for (std::size_t i = 0; i < len; i++) {
    if (offset + sizeof(uint64_t) <= buffer.size()) {
      uint64_t compact_value;
      std::memcpy(&compact_value, &buffer[offset], sizeof(uint64_t));
      offset += sizeof(uint64_t);
      compact_arr_values.push_back(compact_value);
    } else {
      SPDLOG_LOGGER_ERROR(
          logger(), "Buffer underflow when attempting to deserialize compact "
                    "BooleanArray.");
      return std::vector<uint64_t>();
    }
  }
  if (compact_arr_values.size() != len) {
    throw std::runtime_error("Error: Compact array values size mismatch");
    SPDLOG_LOGGER_ERROR(logger(), "Compact array values size mismatch");
  }
  return compact_arr_values;
}
bool readPopulatedMask(std::span<const char> buffer, size_t &offset,
                       uint64_t *words, std::size_t bins) {
  const std::size_t n_words = (bins + 63) / 64;
  if (offset + n_words * sizeof(uint64_t) > buffer.size()) {
    SPDLOG_LOGGER_ERROR(logger(), "Buffer underflow while reading populated mask.");
    return false;
  }
  std::memcpy(words, &buffer[offset], n_words * sizeof(uint64_t));
  offset += n_words * sizeof(uint64_t);
  return true;
}

void writePopulatedMask(const uint64_t *words, std::size_t bins,
                        std::vector<char> &out) {
  const auto *bytes = reinterpret_cast<const char *>(words);
  out.insert(out.end(), bytes, bytes + ((bins + 63) / 64) * sizeof(uint64_t));
}
