#ifndef AIRTREE_CORE_AIRTREE_HEADER_HPP
#define AIRTREE_CORE_AIRTREE_HEADER_HPP

#include <airtree/core/common/ConfigWire.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace airtree::core::common {

// Magic bytes: "AIRT" (0x41 0x49 0x52 0x54)
inline constexpr uint8_t kMagic[4] = {0x41, 0x49, 0x52, 0x54};

inline constexpr uint16_t kHeaderLength = 49;

struct AirTreeHeader {
  uint16_t version = 1;
  uint16_t header_length = kHeaderLength;
  ConfigWire config{};
  std::array<uint8_t, 4> data_types{};
  uint64_t payload_length = 0;
  uint32_t trie_count = 0;
  uint32_t pos_inf_count = 0;
  uint32_t neg_inf_count = 0;
  uint32_t pos_zero_count = 0;
  uint32_t neg_zero_count = 0;
  uint32_t nan_count = 0;
};

AirTreeHeader makeHeader(ConfigWire config,
                         std::array<uint8_t, 4> data_types,
                         uint32_t trie_count, uint32_t pos_inf_count,
                         uint32_t neg_inf_count, uint32_t pos_zero_count,
                         uint32_t neg_zero_count, uint32_t nan_count);


void serializeHeader(const AirTreeHeader &header,
                     std::vector<char> &buffer);


AirTreeHeader deserializeHeader(const std::vector<char> &buffer);


void finalizeHeader(std::vector<char> &buffer,
                                   uint64_t payload_length);

ConfigParams configParams(const AirTreeHeader &header);

} // namespace airtree::core::common

#endif // AIRTREE_CORE_AIRTREE_HEADER_HPP
